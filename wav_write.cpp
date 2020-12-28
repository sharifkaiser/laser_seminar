// wav format: http://soundfile.sapp.org/doc/WaveFormat/
// understanding of typename template: static_cast <char> (value & 0xFF)
// static_cast vs dynamic_cast vs c style cast: https://stackoverflow.com/questions/28002/regular-cast-vs-static-cast-vs-dynamic-cast


/* 
wav_write.cpp
how to call: 
1   ./wav_write   (this lodads with default vals: duration=1 sec, freq=1000, sampling_rate=48000, wave=rectangle)
2   ./wav_write seconds(int) freq(u_int) wave_type(str: sine/rect) sampling_rate(u_int)
3.  all parameters in 2 are optional and sequential 

// ideal test case for testing wav write with small values:
./wav_write 1 128 256 0 (and make LUT_SIZE 128 in the code) => 2 (256/128) seconds of rectangle wave

 
*/


#include <cmath>
#include <fstream>
#include <iostream>
#include <cstdlib>
//#include <string>
enum wave_type {rectangle = 0, sine = 1};
/*
namespace endian
{
  union _endian_test {  // all vars in union points to the same address e.g. all are same variable
      uint16_t word;
      uint8_t byte[2];
  };

  int isLittleEndian() {
      _endian_test test;
      test.word = 0xAA00;

      return test.byte[0] == 0x00;
  }

  int isBigEndian() {
      _endian_test test;
      test.word = 0xAA00;

      return test.byte[0] == 0xAA;
  }
}

uint16_t swap16(uint16_t in)    // swaps bytes of 2 byte uint, used for little to big endian conversion or vice versa
{
    return (in >> 8) | (in << 8);
}

uint32_t swap32(uint32_t in)  // swaps bytes of 4 byte uint, used for little/big endian conversion, ex. 0x (01 23 45 67) => 0x (67 45 23 01)
{
    return  ((in & 0xFF000000) >> 24) | ((in & 0x00FF0000) >>  8) |     // swap leftmost 2 bytes and bring them to rightmost bytes
            ((in & 0x0000FF00) <<  8) | ((in & 0x000000FF) << 24);      // swap rightmost 2 bytes and bring them to the left
}
*/

namespace little_endian_io
{
  template <typename Word>  //template used here to make function write_word operate with a generic type: Word
  std::ostream& write_word( std::ostream& outs, Word value, unsigned size = sizeof( Word ))    // ostrem is output stream object, used for output operation
  {
    for (; size; --size)
    {
        switch (size)
        {
        case 2:
            value =  static_cast <u_int16_t> (value);
            break;
        case 4:
            value =  static_cast <u_int32_t> (value);
            break;
        default:
            value =  static_cast <u_int32_t> (value);
            break;
        }
        
        outs.put( static_cast <char> (value & 0xFF) );    // convert to char to to eliminate leading zeros before writing
        value >>= 8;    // shift one byte
    }
    return outs;
  }
}

class wav_write
{
private:
  /* data */
public:
  wav_write(/* args */);
  ~wav_write();
};

wav_write::wav_write(/* args */)
{
}

wav_write::~wav_write()
{
}


// Creates sound buffer and returns 1 on success
void create_sample_buffer(int16_t left_buff[], int16_t right_buff[], 
                          int freq=1000, int Fs=48000, int buff_size=4096, int signal=wave_type::rectangle)
{
  // const uint16_t Fs = 48000;       // sample rate (Hz)
  // const uint16_t LUT_SIZE = 128;  // lookup table size
  const uint16_t LUT_SIZE = 4096;
  int16_t lut[LUT_SIZE];      // lookup table

  const float phase_increment = (float)(freq * LUT_SIZE) / (float) Fs;
  float phase_left = 0.0f;          // phase accumulator for left channel, initially always zero
  float phase_right = 0.0f;

  switch (signal)
  {
    case wave_type::rectangle:
      // fill table with rectangle wave, first half zero, rest 1
      for (int i = 0; i < LUT_SIZE; ++i)
      {
        if (i < (int)(LUT_SIZE/2))
          lut[i] = 0;  // first half fill with zero
        else
          // std::cout<< "writing1, ival: "<< i << std::endl;
          lut[i] = 30000;
      }
      phase_right = phase_left + (LUT_SIZE/2) ;   // phase accumulator initial for rectangle wave right channel
      break;
    
    case wave_type::sine:
      // std::cout << "sine wave" << std::endl;

      // fill table with sin vals
      for (int i = 0; i < LUT_SIZE; ++i)
      {
        // convert sin vals between (-1,1) to int_16 by multiplying 0x FF FF (SHRT_MAX)
        lut[i] = (int16_t)roundf(SHRT_MAX * sinf(2.0f * M_PI * (float)i / (float)LUT_SIZE));       // sinf takes float arg
      }
      phase_right = phase_left + (LUT_SIZE/4) ;   // phase accumulator for cosine wave
      break;

    default:
      // should never happen, fill with all zeros
      for (int i = 0; i < LUT_SIZE; ++i)
        lut[i] = 0;
      break;
  }

  // generate buffer for left and right channel
  for (int i = 0; i < buff_size; ++i)
  {
    int phase_i = (int)phase_left;        // get integer part of our phase
    left_buff[i] = lut[phase_i];          // get sample value from LUT
    phase_left += phase_increment;        // increment phase
    if (phase_left >= (float)LUT_SIZE)    // handle wraparound
    {
      phase_left -= (float)LUT_SIZE;
    }
    
    // write to right buffer, same as before
    int phase_j = (int)phase_right;
    right_buff[i] = lut[phase_j];
    phase_right += phase_increment;
    if (phase_right >= (float)LUT_SIZE)    // handle wraparound
    {
      phase_right -= (float)LUT_SIZE;
    }
  }
}

int main(int argc, char* argv[])
{
  int num_samples;    // set default values
  int freq = 1000;
  int sampling_rate = 48000;
  int signal = wave_type::rectangle; // default rectangle
  int seconds = 1;
  std::string signal_name = "";

  if(argc > 1)  // starting param always file name
  {
    seconds = atoi(argv[1]);  // array to int conversion
    freq = argc > 2 ? atoi(argv[2]) : freq; // checking if arg exists
    signal_name = argc > 3 ? argv[3] : signal_name;
    sampling_rate = argc > 4 ? atoi(argv[4]): sampling_rate;
  }

  signal = (signal_name.compare("sine") == 0) ? wave_type::sine : (signal_name.compare("rect") == 0) ? wave_type::rectangle : signal;
  num_samples = seconds * sampling_rate;

  const int BITS_PER_SAMPLE = 16;
  const int NUM_CHANNELS = 2;

  int block_align = (int)(NUM_CHANNELS * BITS_PER_SAMPLE/8);
  int subchunk2_size = num_samples * block_align;
  int byte_rate = sampling_rate * block_align;

  std::cout << "freq: " << freq << std::endl;
  std::cout << "sampling_rate: " << sampling_rate << std::endl;
  std::cout << "num_samples: " << num_samples << std::endl;
  std::cout << "signal type: " << signal << std::endl;

  // check machine endian
  // if (endian::isLittleEndian() == 1)


  // ofstream = stream class to write on files, ios::binary is static constant, ios is under std namespace
  // ios is base class for streams
  std::ofstream file_wav(std::to_string(seconds) + "sec," + std::to_string(freq) + "Hz," + signal_name + ".wav", std::ios::binary); 
  // Write the file headers
  file_wav << "RIFF";     // 4bytes, each char is 1 byte
  little_endian_io::write_word(file_wav, 36 + subchunk2_size, 4);
  file_wav << "WAVEfmt "; 
  little_endian_io::write_word(file_wav, 16, 4);  // no extension data, (16=size of rest subchunk for PCM)
  little_endian_io::write_word(file_wav, 1, 2);  // 1=PCM - integer samples (e.g. linear quantization means no compression)
  little_endian_io::write_word(file_wav, NUM_CHANNELS, 2 );  // two channels (stereo file)
  little_endian_io::write_word(file_wav, sampling_rate, 4);  // sampling rate (Hz)
  little_endian_io::write_word(file_wav, byte_rate, 4);  // 176400=ByteRate (Sample Rate=44100 * BitsPerSample=16 * Channels=2) / 8
  little_endian_io::write_word(file_wav, block_align, 2);  // data block size=number of bytes for one sample (NumChannels=2 * BitsPerSample=16/8)
  little_endian_io::write_word(file_wav, BITS_PER_SAMPLE, 2);  // bits per sample=16 (use a multiple of 8)

  // Write the data chunk header
  size_t data_chunk_pos = file_wav.tellp();
  file_wav << "data";
  little_endian_io::write_word(file_wav, subchunk2_size, 4);

  // Prepare sample data for left and right channels
  wave_type wave;
  if (signal == 0)
    wave = wave_type::rectangle;
  else if(signal == 1)
    wave = wave_type::sine;
  else
  {
    wave = wave_type::rectangle; // default
  }
  
  int16_t left_buff[num_samples];
  int16_t right_buff[num_samples];
  create_sample_buffer(left_buff, right_buff, freq, sampling_rate, num_samples, wave);

  // write samples to wav file
  for(int i = 0; i < num_samples; ++i)
  {
    little_endian_io::write_word(file_wav, left_buff[i], 2); // write sample to left channel
    little_endian_io::write_word(file_wav, right_buff[i], 2); // write sample to right channel

    std::cout << "i: " << i << std::endl; // left buff vals
    std::cout << "left buff: " << left_buff[i] << std::endl; // left buff vals
    std::cout << "right: " << right_buff[i] << std::endl; // left buff vals

  }

  file_wav.close();

  return 0;
}