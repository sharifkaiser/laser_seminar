// plays sine wave and cos wave to 2 channels
// library install from terminal: sudo apt-get install libasound2-dev
// side note: build and run stderr $ gcc stderr.c -o s && ./s
// link it with -lasound and -lm (math), i.e. compile like this: gcc -o alsa_sin alsa_sin.c -lasound -lm
// to run: sudo ./alsa_sin

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <alsa/asoundlib.h>

int main() {
    int err;    // error number
    int num_channels = 2;
    int sampling_rate = 48000, dir = 0, i, j;
    char* buffer;  //stores sine wave samples, character is 1 byte
    double x, sin_val, cos_val, freq = 480.0, seconds = 20;
    int sample_sin, sample_cos; // each sample of sine, cos wave
    int amp = 10000;

    // alsa specific structures
    snd_pcm_t* handle; // reference to the sound card
    snd_pcm_hw_params_t* params; // Information about hardware params

    snd_pcm_uframes_t frames = 4; // period size, period contains frames
    //snd_pcm_uframes_t is unsigned long which is  bytes, e.g. long frames = 4  

    // open reference to the sound card
    err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);	// default sound card
    if (err < 0) {
        fprintf(stderr, "unable to open defualt device: %s\n", snd_strerror(err));
        exit(1);
    }

    // Now we allocate memory for the parameters structure on the stack
    snd_pcm_hw_params_alloca(&params);

    // Next, we're going to set all the parameters for the sound card

    // This sets up the soundcard with some default parameters and we'll 
    // customize it a bit afterwards
    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0) {
        fprintf(stderr, "Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
        exit(1);
    }

    // Set the samples for each channel to be interleaved
    err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        fprintf(stderr, "Access type not available for playback: %s\n", snd_strerror(err));
        exit(1);
    }

    // This says our samples represented as signed, 16 bit integers in little endian format
    err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        fprintf(stderr, "sample_sin format not available for playback: %s\n", snd_strerror(err));
        exit(1);
    }

    // We use 2 channels (left audio and right audio)
    err = snd_pcm_hw_params_set_channels(handle, params, num_channels);  // we want to write to mono channel, so 1, another channel will be used to write cos
    if (err < 0) {
        fprintf(stderr, "Channels count (%d) not available for playbacks: %s\n", num_channels, snd_strerror(err));
        exit(1);
    }

    // Here we set our sampling rate. 
    err = snd_pcm_hw_params_set_rate_near(handle, params, &sampling_rate, &dir);
    if (err < 0) {
        fprintf(stderr, "Rate %dHz not available for playback: %s\n", sampling_rate, snd_strerror(err));
        exit(1);
    }

    // This sets the period size
    err = snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
    if (err < 0) {
        fprintf(stderr, "Set period error: %s\n", snd_strerror(err));
        exit(1);
    }

    // Finally, the parameters get written to the sound card
    err = snd_pcm_hw_params(handle, params);
    if (err < 0) {
        fprintf(stderr, "unable to set the hw params: %s\n", snd_strerror(err));
        exit(1);
    }

    // This allocates memory to hold our samples
    // buffer contains 4*4 = array of 16 chars, each chars is a byte
    buffer = (char*)malloc(frames * 4);   // frame = 4

    j = 0;
    for (i = 0; i < seconds * sampling_rate; i++) {
        // Create a sample_sin and convert it back to an integer
        x = (double)i / (double)sampling_rate;
        sin_val = sin(2.0 * 3.14159 * freq * x);  // sin_val between -1 and 1, double is 8 bytes
        // cos_val = sin((2.0 * 3.14159 * freq * x) + (90 * 3.14159 / 180));    // cos is sine with 90 degree positive phase shift
        cos_val = cos(2.0 * 3.14159 * freq * x);
        
        sample_sin = amp * sin_val;   // sample_sin is int, 4 bytes
        sample_cos = amp * cos_val;

        // Store the sample_sin in our buffer using Little Endian format i.e. filling buffer from LSB to MSB right to left
        // 0 and 1 are for white channel in hifiberry, probably channel 1
        buffer[0 + 4 * j] = sample_sin & 0xff;    // 0001, output LSB of sample_sin in LSB of buffer
        buffer[1 + 4 * j] = (sample_sin & 0xff00) >> 8;   // sample_sin & 1100 and right shift by 8 bits to get bits 8 to 15 (right to left) 

        // following works for red channel, does not work as expected e.g.shows same as sine
        buffer[2 + 4 * j] = sample_cos & 0xff;
        buffer[3 + 4 * j] = (sample_cos & 0xff00) >> 8;
        
        //test red channel with random instead, it works expected
        //buffer[2 + 4 * j] = random() & 0xff;
        //buffer[3 + 4 * j] = (random() & 0xff00) >> 8;

        // If we have a buffer full of samples, write 1 period of samples to the sound card
        if (++j == frames) {  // true when j == 3
            j = snd_pcm_writei(handle, buffer, frames);

            // Check for under runs
            if (j < 0) {
                snd_pcm_prepare(handle);
            }

            j = 0;
        }
    }

    // Play all remaining samples before exitting
    snd_pcm_drain(handle);

    // Close the sound card handle
    snd_pcm_close(handle);  return 0;
}
