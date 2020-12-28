// wav format: http://soundfile.sapp.org/doc/WaveFormat/
// understanding of typename template: static_cast <char> (value & 0xFF)
// static_cast vs dynamic_cast vs c style cast: https://stackoverflow.com/questions/28002/regular-cast-vs-static-cast-vs-dynamic-cast

/* 
wav_write.cpp: default values: seconds=10, freq=1000, sampling_rate=48000, signal_name=""
how to call: 
1  ./svg_to_wav canvas_dim_in_pixel<int> filename.txt<string>
2  ./svg_to_wav canvas_dim_in_pixel<int> filename.txt<string> seconds<int> freq<int> signal_name("sine", "rectangle") <string> sampling_rate<int>
3. all parameters in 2 are optional and sequential
4. signal_name parameter should be passed only if rectangle or sine wave is wanted e.g. input text file will be overridden

// example:
./svg_to_wav 400 triangle.txt
*/

#include <cmath>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <vector>
//#include <string>
enum wave_type {rectangle = 0, sine = 1, input = 3};
int canvas_dim = 400;    // input from user, or parse from svg file
const unsigned int amp_multiplyer = 60000;  // multiplier for 16 bit signal, the range of points [-0.5, +0.5] 
int lut_size = 48000;  // lookup table initial size
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

class Point{
    public:
        Point();
        Point(double mx, double my);
        void set_x(double mx);
        void set_y(double my);
        void print_point(void);
        void rescale_point(Point* point_element);    // obj pass by ref
        double get_x();
        double get_y();
        ~Point();
    private:
        double x, y;
};

Point::Point(){
    x = 0.0;
    y = 0.0;
}

Point::Point(double mx, double my){
    x = mx;
    y = my;
}

void Point::set_x(double mx){
    x = mx;
}

void Point::set_y(double my){
    y = my;
}

void Point::print_point(void){
    std::cout.precision(17);    // to print after decimals
    std:: cout << "x: " << x << ", y: " << y << std::endl;
}

double Point::get_x(){
    return this->x;
}

double Point::get_y(){
    return this->y;
}

void Point::rescale_point(Point* point_element){
    point_element->x = (point_element->x - 0.0) / ((double)canvas_dim - 0.0);   // normalization between 0 and 1
    point_element->y = (point_element->y - 0.0) / ((double)canvas_dim - 0.0);
    point_element->x -= 0.5;   // scale in [-0.5, 0.5] range
    point_element->y -= 0.5;
    point_element->x *= (double)amp_multiplyer;   // multiply to reflect in signal
    point_element->y *= (double)amp_multiplyer;
}

Point::~Point()
{
}

int set_input_params(int argc, char* argv[], int* canvas_dim, int* seconds, int* freq, std::string & signal_name, int* sampling_rate, std::string & points_file){
    if(argc < 2)  // argv[0] always file name
        return -1;  // error
    else{
        char *endptr = NULL;;
        *canvas_dim = std::strtol (argv[1], &endptr, 10);  // array to int conversion
        points_file = argc > 2 ? argv[2] : points_file;
        *seconds = argc > 3 ? std::strtol (argv[3], &endptr, 10) : *seconds; // checking if arg exists
        *freq = argc > 4 ? std::strtol (argv[4], &endptr, 10) : *freq;
        signal_name = argc > 5 ? argv[5] : signal_name;
        *sampling_rate = argc > 6 ? std::strtol (argv[6], &endptr, 10): *sampling_rate;

        if (!*canvas_dim || !*seconds || !*freq || !*sampling_rate || !points_file.length())
            return -1;  // none of these values should be zero
        
    } 
    return 0;
}

/*
void set_wav_header(int* num_samples, int* sampling_rate){
    const int BITS_PER_SAMPLE = 16;
    const int NUM_CHANNELS = 2;

    int block_align = (int)(NUM_CHANNELS * BITS_PER_SAMPLE/8);
    int subchunk2_size = *num_samples * block_align;
    int byte_rate = *sampling_rate * block_align;

    // check machine endian
    // if (endian::isLittleEndian() == 1)

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
}
*/

// Creates sound buffer and returns 1 on success
void create_sample_buffer(int16_t x_buff[], int16_t y_buff[], 
                          int freq, int Fs, int num_samples, int wave_typ,
                        std::vector<Point> &scaled_points, int interpolation_factor)
{
    int16_t lut[lut_size];      // lookup table used if wave is sine/rectangle
    int16_t lut_x[lut_size];      // lookup table for x values for input svg points
    int16_t lut_y[lut_size];      // lookup table for y values for input svg points
    const float phase_increment = (float)(freq * lut_size) / (float) Fs;
    float phase_x = 0.0f, phase_y = 0.0f;   // phase accumulator for sine and rectangular wave
    float phase = 0.0f;    // phase for custom input svg points

    switch (wave_typ)
    {
    case wave_type::rectangle:
        // fill table with rectangle wave, first half zero, rest 1
        for (int i = 0; i < lut_size; ++i)
        {
            if (i < (int)(lut_size/2))
                lut[i] = 0;  // first half fill with zero
            else
                lut[i] = 30000;
        }
        phase_y = phase_x + (lut_size/2) ;   // phase accumulator initial for rectangle wave right channel
        break;

    case wave_type::sine:

        // fill table with sin vals
        for (int i = 0; i < lut_size; ++i)
        {
            // convert sin vals between (-1,1) to int_16 by multiplying 0x FF FF (SHRT_MAX)
            lut[i] = (int16_t)roundf(SHRT_MAX * sinf(2.0f * M_PI * (float)i / (float)lut_size));       // sinf takes float arg
        }
        phase_y = phase_x + (lut_size/4) ;   // phase accumulator for cosine wave
        break;
    
    case wave_type::input:
        if (interpolation_factor == 0)  // no interpolation
        {
            int i, reverse_counter;
            for (i = 0; i < scaled_points.size(); i++){ 
                lut_x[i] = (int16_t)scaled_points.at(i).get_x();    //forward points i.e. start->end
                lut_y[i] = (int16_t)scaled_points.at(i).get_y();
            }
            reverse_counter = i - 1;   // counter for reversing half filled lut -> points to the last element of half filled array
            while (i < lut_size){
                lut_x[i] = lut_x[reverse_counter];    //backward points i.e. end->start
                lut_y[i] = lut_y[reverse_counter];
                i++;
                reverse_counter--;
            }
            // print lut
            for (i = 0; i < lut_size; i++){ 
                std::cout <<"i=" << i << ",  x: " << lut_x[i] << ", y: " << lut_y[i] << std::endl;
            }
        }
        else    // interpolation necessary
        {
            enum interpolation_type {increment = 1, decreament = -1, same = 0};
            interpolation_type interpolation_type_x, interpolation_type_y;
            int i, interpolation_counter, lut_counter = 0;
            double current_x, current_y, next_x, next_y, inc_x, inc_y, interpolated_x, interpolated_y;

            // Fill lut with forward points
            for (i = 0; i < scaled_points.size() -1; i++){  // last point excluded to avoid overrun
                current_x = scaled_points.at(i).get_x();    // save current point
                current_y = scaled_points.at(i).get_y();

                next_x = scaled_points.at(i+1).get_x();
                next_y = scaled_points.at(i+1).get_y();

                inc_x = std::abs(current_x - next_x) / (interpolation_factor + 1);  // distance / factor+1
                inc_y = std::abs(current_y - next_y) / (interpolation_factor + 1);

                if( (next_x - current_x) > 0){
                    interpolation_type_x = interpolation_type::increment;
                }
                else if( (next_x - current_x) < 0)
                {
                    interpolation_type_x = interpolation_type::decreament;
                }
                else if( (next_x - current_x) == 0 ) 
                {
                    interpolation_type_x = interpolation_type::same;
                }

                if( (next_y - current_y) > 0){
                    interpolation_type_y = interpolation_type::increment;
                }
                else if( (next_y - current_y) < 0)
                {
                    interpolation_type_y = interpolation_type::decreament;
                }
                else if( (next_y - current_y) == 0 )
                {
                    interpolation_type_y = interpolation_type::same;
                }
 
                interpolation_counter = 0;
                while (interpolation_counter < interpolation_factor + 1)    // +1 for the original point
                {   // fill lut with original point(first one) and interpolated points

                    if ( (lut_counter % (interpolation_factor + 1)) == 0 )  // first point fill with original
                    {
                        interpolated_x = current_x;
                        interpolated_y = current_y;
                    }
                    else    // these are interpolated points
                    {
                        switch (interpolation_type_x)
                        {
                        case interpolation_type::increment:
                            interpolated_x += inc_x;
                            break;

                        case interpolation_type::decreament:
                            interpolated_x -= inc_x;
                            break;

                        case interpolation_type::same:
                            interpolated_x = current_x;
                            break;
                        
                        default:
                            break;
                        }

                        switch (interpolation_type_y)
                        {
                        case interpolation_type::increment:
                            interpolated_y += inc_y;
                            break;

                        case interpolation_type::decreament:
                            interpolated_y -= inc_y;
                            break;

                        case interpolation_type::same:
                            interpolated_y = current_y;
                            break;
                        
                        default:
                            break;
                        }
                    }
                    
                    // Fill lut with interpolated points
                    lut_x[lut_counter] = (int16_t)interpolated_x;
                    lut_y[lut_counter] = (int16_t)interpolated_y;

                    lut_counter++;
                    interpolation_counter++;
                }
            }

            // fill out lut with last Point, no interpolation
            lut_x[lut_counter] = scaled_points.at(i).get_x();
            lut_y[lut_counter] = scaled_points.at(i).get_y();

            /*-----------------  forward lut insertion done  ----------------------*/

            // fill out rest half of lut with reverse values i.e. end -> start
            int reverse_counter = lut_counter; // point to the last index of the half filled lut
            lut_counter = lut_counter + 1;  // point to the next index
            while (lut_counter < lut_size){
                lut_x[lut_counter] = lut_x[reverse_counter];    //backward points i.e. end->start
                lut_y[lut_counter] = lut_y[reverse_counter];
                lut_counter++;
                reverse_counter--;
            }

            // print lut
            for (i = 0; i < lut_size; i++){ 
                std::cout <<"i=" << i << ",  x: " << lut_x[i] << ", y: " << lut_y[i] << std::endl;
            }
        }        

    default:
        break;
    }

    // generate buffer for left and right channel
    if(wave_typ == wave_type::rectangle || wave_typ == wave_type::sine){
        for (int i = 0; i < num_samples; ++i)
        {
            int phase_i = (int)phase_x;        // get integer part of our phase
            x_buff[i] = lut[phase_i];          // get sample value from LUT
            phase_x += phase_increment;        // increment phase
            if (phase_x >= (float)lut_size)    // handle wraparound
            {
                phase_x -= (float)lut_size;
            }

            // write to right buffer, same as before
            int phase_j = (int)phase_y;
            y_buff[i] = lut[phase_j];
            phase_y += phase_increment;
            if (phase_y >= (float)lut_size)    // handle wraparound
            {
                phase_y -= (float)lut_size;
            }
        }
    }
    else if (wave_typ == wave_type::input){ // fill the entire buffer with lut values
        for (int i = 0; i < num_samples; ++i){
            int int_phase = (int)phase;
            x_buff[i] = lut_x[int_phase];
            y_buff[i] = lut_y[int_phase];
            phase += phase_increment;
            if (phase >= (float)lut_size)    // handle wraparound
            {
                phase -= (float)lut_size;
            }
        }
    }
}

void load_points_from_file(std::string file, std::vector<Point> &points)
{
    std::ifstream file_points(file);
    std::string line, x, y;
    int pos;
    Point current_point;
    std::size_t offset = 0; //offset will be set to the length of characters of the "value" - 1.

    if (file_points.is_open())
    {
        while (std::getline(file_points, line))
        {
            pos = line.find(",");
            if (pos != std::string::npos)    // npos means not found
            {
                x = line.substr(0, pos);   // x coordinate
                y = line.substr(pos + 1, line.length());   // y cordinate
                current_point.set_x(std::stod (x, &offset));
                current_point.set_y(std::stod (y, &offset));
                points.push_back(current_point); // convert string to double and push
            }
            else if (line.compare("#") == 0)    // end of file
            {
                break; //exit from while loop
            }
        }
        file_points.close();
    }
}

int main(int argc, char* argv[])
{
    // init default values for the signal
    int num_samples, freq = 500, sampling_rate = 48000, seconds = 10;
    int signal = -1;
    std::string signal_name = "", points_file = "";

    if (set_input_params(argc, argv, &canvas_dim, &seconds, &freq, signal_name, &sampling_rate, points_file) == -1){
        std::cout << "Please provide canvas dimension<int> and points file name<string>" << std::endl;
        return 0;   // exit
    }

    signal = (signal_name.compare("sine") == 0) ? wave_type::sine : (signal_name.compare("rect") == 0) ? wave_type::rectangle : signal;
    num_samples = seconds * sampling_rate;
    if (signal == -1)
    {
        signal_name = points_file.substr(0, points_file.length()-4);  // input picture name from file name, remove the last 4 chars (.txt)
    }

    // code for setting wav file header
    const int BITS_PER_SAMPLE = 16;
    const int NUM_CHANNELS = 2;
    int block_align = (int)(NUM_CHANNELS * BITS_PER_SAMPLE/8);
    int subchunk2_size = num_samples * block_align;
    int byte_rate = sampling_rate * block_align;
    
    // ios is base class for streams
    // ofstream = stream class to write on files, ios::binary is static constant, ios is under std namespace
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

/*
    {   // print input params
        std::cout << "canvas_dim: " << canvas_dim << std::endl;
        std::cout << "freq: " << freq << std::endl;
        std::cout << "sampling_rate: " << sampling_rate << std::endl;
        std::cout << "num_samples: " << num_samples << std::endl;
        std::cout << "pionts file name: " << points_file << std::endl;        
    }
 */
    std::vector<Point> points;
    load_points_from_file(points_file, points);   // points is passed by ref
    std::cout << "vector size: " << points.size() << std::endl;
    for (Point &element: points)    // rescale points, pass each element by ref
        element.rescale_point(&element);

/*
    for (Point element: points) // print to see the scaled points vector vals
        element.print_point();
*/
    /*
        -- Set lookup table size according to need. LUT must contain at least 2x points for drawing signal as
        start point -> (...optional interpolated points in middle...) -> end point + 
        end point -> (...optional interpolated points in middle...) -> start point)
    */
    // lut_size is always even number because it contains forward and reverse points e.g. start->end + end->start
    int interpolation_factor = 0, input_points_count = points.size();
    if (lut_size <= input_points_count * 2)   // number of points more than default lut_size, resize lut
    {
        lut_size = input_points_count * 2;  // no interpolated points
    }
    else
    {   // lut_size is bigger ie. can hold more points than available -> interpolation needed
        // interpolation_factor is num of points between 2 adjacent points
        interpolation_factor = floor( ( (lut_size/2) - input_points_count ) / (input_points_count-1) );

        // redefine lut_size so that it only contains signal forward + backward 
        lut_size = 2 * ( input_points_count + interpolation_factor * (input_points_count - 1) );
    }
    
    if (lut_size > num_samples){
        std::cout << "too many points!" << std::endl;
        return 0;
    }

    std::cout << "interpolation points: " << interpolation_factor << ", lut_size: " << lut_size << std::endl; 


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
        wave = wave_type::input;  // input comes from custom svg
    }
        
    int16_t x_buff[num_samples];
    int16_t y_buff[num_samples];
    create_sample_buffer(x_buff, y_buff, freq, sampling_rate, num_samples, wave, points, interpolation_factor);

    // write samples to wav file
    for(int i = 0; i < num_samples; ++i)
    {
        little_endian_io::write_word(file_wav, x_buff[i], 2); // write sample to left channel
        little_endian_io::write_word(file_wav, y_buff[i], 2); // write sample to right channel
/*
        std::cout << "i: " << i << std::endl; // left buff vals
        std::cout << "left buff: " << x_buff[i] << std::endl; // left buff vals
        std::cout << "right: " << y_buff[i] << std::endl; // left buff vals
*/
    }
    file_wav.close();

    return 0;
}