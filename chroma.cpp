 





//chroma.cpp

#include <emscripten.h>
#include <cmath>
#include "kiss_fftr.h"
#include <cstdlib> // For malloc and free
#include <algorithm> //for std::max


#define PI 3.14159265358979323846

extern "C" {

  // FrequencyMagnitude struct

  struct FrequencyMagnitude {
      float frequency;
      float magnitude;
  };


  // Core FFT function, brought over by KissFFT
  void fft(float* inputArray, float* outputArray, int length) {
      // Create KissFFT config
      kiss_fftr_cfg cfg = kiss_fftr_alloc(length, 0, nullptr, nullptr);

      // Allocate memory for the intermediate output
      kiss_fft_cpx* fft_out = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * (length / 2 + 1));

      // Perform FFT
      kiss_fftr(cfg, inputArray, fft_out);

      // Copy the KissFFT output to the output array
      for (int i = 0; i < (length / 2 + 1); i++) {
          outputArray[2 * i] = fft_out[i].r;     // Real part
          outputArray[2 * i + 1] = fft_out[i].i; // Imaginary part
      }

      // Clean up
      free(fft_out);
      kiss_fftr_free(cfg);
  }


  // Hamming window function

  void Hamming(float* input, float* output, int length) {
    for (int i = 0; i < length; i++) {
        float window = 0.54 - 0.46 * cos((2 * PI * i) / (length - 1));
        output[i] = input[i] * window; //modifies output directly: void function - so don't need to reallocate
    }
  }

  // band-pass filter: attenuate frequencies outside range of interest 

  void bandPassFilter(float* input, float* output, int length, float lowCutoff, float highCutoff, float sampleRate) {
    // Implement a simple IIR band-pass filter
    // This is a basic example and might need tuning
    float dt = 1.0f / sampleRate;
    float RC_low = 1.0f / (2 * M_PI * lowCutoff);
    float RC_high = 1.0f / (2 * M_PI * highCutoff);
    float alpha_low = dt / (RC_low + dt);
    float alpha_high = RC_high / (RC_high + dt);
    
    float lastLow = input[0];
    float lastHigh = input[0];
    
    for (int i = 0; i < length; ++i) {
        lastLow = lastLow + alpha_low * (input[i] - lastLow);
        lastHigh = alpha_high * (lastHigh + input[i] - input[std::max(0, i-1)]);
        output[i] = lastHigh - lastLow;
    }
  }

  // downsample function - going from high sample rate 

  void downsample(float* input, float* output, int inputLength, int factor) {
    for (int i = 0; i < inputLength / factor; ++i) {
        output[i] = input[i * factor];
    }
  }

  // zero-padding

  void zeroPad(float* input, float* output, int inputLength, int outputLength) {
    memcpy(output, input, inputLength * sizeof(float));
    memset(output + inputLength, 0, (outputLength - inputLength) * sizeof(float));
  }




  // Convert raw FFT output to frequency bands
  void fft_to_frequency(float* array, FrequencyMagnitude* result, int length, int sampleRate) {
    int binCount = length / 2 + 1;
    float frequencyResolution = (float)sampleRate / length;

    for (int i = 0; i < binCount; i++) {
        float frequency = i * frequencyResolution;
        float realPart = array[i*2];
        float imagPart = array[i*2 + 1];
        float magnitude = sqrt(realPart * realPart + imagPart * imagPart);

        result[i].frequency = frequency;
        result[i].magnitude = magnitude;
    }
  }

  // Get frequency of particular notes, as denoted by n, where n = 0 --> C1

  float frequency(int n) {
    const float frequency_c1 = 32.703;  // Hz
    float f = frequency_c1 * pow(2.0, n / 12.0);  // Use pow for exponentiation
    return f;
  }

  // Base function kf_nh --> essentially, finding the hth harmonic of a particular base frequency

  float kf(int n, int h){
    return frequency(n) * h;
  }


  // for some note n, given some harmonic h of that note, find the magnitude maximand w.r.t. frequency around a certain window, given by k_0(n,h) and k_1(n,h), which is constructed out of kf above.

  
  float fft_windowmaxxer(int n, int h, int r, FrequencyMagnitude* freq_mag_array, int length, float frequency_resolution) {
    // r is chosen as a hyperparameter: the number of bins on either side to search. adam stark: r=2 --> TEST LATER
    // I think this will be a costly operation because you're doing it over and over again
      float Kf = kf(n, h); //rounding?
      float K_zero = Kf - r * h;
      float K_one = Kf + r * h;

      float max_magnitude = 0.0f;

      // Convert frequency bounds to array indices
      int start_index = static_cast<int>(K_zero / frequency_resolution);
      int end_index = static_cast<int>(K_one / frequency_resolution);

      // Ensure indices are within array bounds
      start_index = std::max(0, start_index);
      end_index = std::min(static_cast<int>(length) - 1, end_index);

      // Find maximum magnitude within the frequency window
      for (int i = start_index; i <= end_index; i++) {
          if (freq_mag_array[i].magnitude > max_magnitude) {
              max_magnitude = freq_mag_array[i].magnitude;
          }
      }

      return max_magnitude;
  }

  
  float energy(int n, FrequencyMagnitude* freq_mag_array, int length, float frequency_resolution){
    const int r = 2; // can change later
    // we are hardcoding it to just consider up to second harmonic since it works the best anyway
    float Energy = fft_windowmaxxer(n, 1, r, freq_mag_array, length, frequency_resolution) * 1/1 + fft_windowmaxxer(n, 2, r, freq_mag_array, length, frequency_resolution) * 1/2;

    return Energy;



  }

  
  float* chroma_vector(int octaves, FrequencyMagnitude* freq_mag_array, int length, float frequency_resolution) {
      float* Chroma_vector = new float[12]();  // Initialize with zeros

      for (int j = 0; j < 12; j++) {
          for (int i = 0; i < octaves; i++) {
              int n = j + i * 12;  // Calculate the note number
              Chroma_vector[j] += energy(n, freq_mag_array, length, frequency_resolution);
          }
      }

      float max_value = 0.0f;
      for (int j = 0; j < 12; j++) {
          if (Chroma_vector[j] > max_value) {
              max_value = Chroma_vector[j];
          }
      }

      if (max_value > 0) {
          for (int j = 0; j < 12; j++) {
              Chroma_vector[j] /= max_value;
          }
      }

      return Chroma_vector;
  }
  
  // Final chroma function

  EMSCRIPTEN_KEEPALIVE
  float* chroma(float* inputArray, int length, int sampleRate) {
    float lowCutoff = 20.0f;  // 20 Hz
    float highCutoff = 4000.0f;  // 4000 Hz
    int downsampleFactor = 6;  // Assuming original sample rate is 48kHz
    int newSampleRate = sampleRate / downsampleFactor;
    int newLength = length / downsampleFactor;
    int paddedLength = newLength * 2;  // Double the length for zero-padding

    // Allocate memory for intermediate steps
    float* filtered = (float*)malloc(sizeof(float) * length);
    float* downsampled = (float*)malloc(sizeof(float) * newLength);
    float* padded = (float*)malloc(sizeof(float) * paddedLength);
    
    // Apply band-pass filter
    bandPassFilter(inputArray, filtered, length, lowCutoff, highCutoff, sampleRate);
    
    // Downsample
    downsample(filtered, downsampled, length, downsampleFactor);
    
    // Zero-pad
    zeroPad(downsampled, padded, newLength, paddedLength);
    
    // Apply Hamming window
    float* windowed = (float*)malloc(sizeof(float) * paddedLength);
    Hamming(padded, windowed, paddedLength);
    
    // Perform FFT
    float* ffted = (float*)malloc(sizeof(float) * paddedLength * 2);
    fft(windowed, ffted, paddedLength);

    // Convert to frequency-magnitude pairs
    int binCount = paddedLength / 2 + 1;
    FrequencyMagnitude* frequencyBands = (FrequencyMagnitude*)malloc(sizeof(FrequencyMagnitude) * binCount);
    fft_to_frequency(ffted, frequencyBands, paddedLength, newSampleRate);

    // Clean up
    free(filtered);
    free(downsampled);
    free(padded);
    free(windowed);
    free(ffted);

    // do we need to do windowing?
    float frequency_resolution = newSampleRate / paddedLength;
    float* chroma = chroma_vector(7, frequencyBands, paddedLength, frequency_resolution);
    
    return chroma;
    delete[] chroma; 

    // also, think about trash collection a bit more in detail!

    
  }




}


/*
THOUGHT EXERCISE FOR REQUIRED RESOLUTION

C4 frequency: 261.626Hz
C#4 frequency: 277.18Hz -->

ideally you would need a resolution of around 10Hz (around the middle)

but could need as far down as C2, reasonably.
C2: 65Hz
C#2: 73Hz

*/


/*
Final valid output

output will be like 
20Hz 2Hz 3Hz .... 20000Hz, 0.00, 0.00, 0.00

how to find the index at which this happens?

(assuming these params are still true)
- sample rate: 48 kHz
- signal length: 4096 samples
- downsample factor: 6

//
new sample rate = 48,000/6 = 8000
new length = 4096/6 = 682 
zeropadding: paddedlength = newlength * 2 = 1364 samples --> FFT will be performed on 1364 samples

frequency resolution = new sample rate / padded length = 8000/1364 = 5.87 Hz
# of frequency bins = paddedlength / 2 + 1  = 683 --> this is # of valid frequency bins: beyond this, it's all zeroes


*/