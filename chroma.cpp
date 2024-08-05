 
//chroma.cpp

// base -without lead cancellation

#include <emscripten.h>
#include <cmath>
#include "kiss_fftr.h"
#include <cstdlib> // For malloc and free
#include <algorithm> // for std::max
#include <cstring> // for memset
#include "chordProfiles.h" // create a separate library here for cleanliness


#define PI 3.14159265358979323846

extern "C" {

  struct FrequencyMagnitude {
      float frequency;
      float magnitude;
  };

  void fft(float* inputArray, float* outputArray, int length) {
      kiss_fftr_cfg cfg = kiss_fftr_alloc(length, 0, nullptr, nullptr);
      kiss_fft_cpx* fft_out = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * (length / 2 + 1));
      kiss_fftr(cfg, inputArray, fft_out);
      for (int i = 0; i < (length / 2 + 1); i++) {
          outputArray[2 * i] = fft_out[i].r;
          outputArray[2 * i + 1] = fft_out[i].i;
      }
      free(fft_out);
      kiss_fftr_free(cfg);
  }

  void Hamming(float* input, float* output, int length) {
    for (int i = 0; i < length; i++) {
        float window = 0.54 - 0.46 * cos((2 * PI * i) / (length - 1));
        output[i] = input[i] * window;
    }
  }

  void bandPassFilter(float* input, float* output, int length, float lowCutoff, float highCutoff, float sampleRate) {
    float dt = 1.0f / sampleRate;
    float RC_low = 1.0f / (2 * PI * lowCutoff);
    float RC_high = 1.0f / (2 * PI * highCutoff);
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

  void downsample(float* input, float* output, int inputLength, int factor) {
    for (int i = 0; i < inputLength / factor; ++i) {
        output[i] = input[i * factor];
    }
  }

  void zeroPad(float* input, float* output, int inputLength, int outputLength) {
    memcpy(output, input, inputLength * sizeof(float));
    memset(output + inputLength, 0, (outputLength - inputLength) * sizeof(float));
  }


  void fft_to_K(float* array, float* K_array, int length, int sampleRate) {
    int binCount = length / 2 + 1;
    float maxMagnitude = 0.0f;
    float exponent = 2.0f; // Adjust this exponent as needed

    // Calculate magnitudes and find the maximum magnitude
    for (int i = 0; i < binCount; i++) {
        float realPart = array[i * 2];
        float imagPart = array[i * 2 + 1];
        float magnitude = sqrt(realPart * realPart + imagPart * imagPart);
        maxMagnitude = std::max(maxMagnitude, magnitude);
    }

    // Calculate threshold as 5% of the maximum magnitude
    float threshold = 0.15 * maxMagnitude;

    // Apply thresholding and exponential scaling
    for (int i = 0; i < binCount; i++) {
        float realPart = array[i * 2];
        float imagPart = array[i * 2 + 1];
        float magnitude = sqrt(realPart * realPart + imagPart * imagPart);

        // Apply thresholding
        if (magnitude < threshold) {
            magnitude = 0;
        }

        // Apply exponential scaling
        /*
        magnitude = pow(magnitude, exponent);
        */
        

        K_array[i] = magnitude;
    }
}
  

  float frequency(int n) { // checked: this is correct
    const float frequency_c1 = 32.703;
    return frequency_c1 * pow(2.0, n / 12.0);
  }

  float knh(int n, int h, int length, int sampleRate){ // yeah, ought to be correct
    return frequency(n) * h / (sampleRate / length);
  }

  float fft_windowmaxxer(int n, int h, int r, float* K_array, int length, float sampleRate) {
    //float Knh = knh(n, h, length, sampleRate);
    float Knh = std::round(knh(n, h, length, sampleRate));
    float K_zero = Knh - r * h;
    float K_one = Knh + r * h;

    float max_magnitude = 0.0f;

    int start_index = static_cast<int>(K_zero);
    int end_index = static_cast<int>(K_one);

    start_index = std::max(0, start_index);
    end_index = std::min(static_cast<int>(length) - 1, end_index);

    for (int i = start_index; i <= end_index; i++) {
        if (K_array[i] > max_magnitude) {
            max_magnitude = K_array[i];
        }
    }

    return max_magnitude;
  }

  float energy(int n, float* K_array, int length, float sampleRate) {
    const int r = 3;
    float Energy = fft_windowmaxxer(n, 1, r, K_array, length, sampleRate) + 
                   fft_windowmaxxer(n, 2, r, K_array, length, sampleRate) / 2.0f+
                   fft_windowmaxxer(n, 3, r, K_array, length, sampleRate) / 3.0f+
                   fft_windowmaxxer(n, 4, r, K_array, length, sampleRate) / 4.0f+
                   fft_windowmaxxer(n, 5, r, K_array, length, sampleRate) / 5.0f+
                   fft_windowmaxxer(n, 6, r, K_array, length, sampleRate) / 6.0f+
                   fft_windowmaxxer(n, 7, r, K_array, length, sampleRate) / 7.0f+
                   fft_windowmaxxer(n, 8, r, K_array, length, sampleRate) / 8.0f+
                   fft_windowmaxxer(n, 9, r, K_array, length, sampleRate) / 9.0f+
                   fft_windowmaxxer(n, 10, r, K_array, length, sampleRate) / 10.0f+
                   fft_windowmaxxer(n, 11, r, K_array, length, sampleRate) / 11.0f;

    return Energy;
  }

  float* chroma_vector(int octaves, float* K_array, int length, float sampleRate) {
      float* Chroma_vector = new float[12]();

      for (int n = 0; n < 12; n++) {
          for (int d = 0; d < octaves; d++) {
              
              Chroma_vector[n] += energy(n + 12 * d, K_array, length, sampleRate);
          }
      }
      return Chroma_vector;
  }

  void rotateRight(float* array, int length, int positions) {
    if (positions < 0 || positions >= length) {
        // Invalid rotation count, return without doing anything
        return;
    }

    float* temp = (float*)malloc(sizeof(float) * positions);

    // Copy the last 'positions' elements to a temporary array
    memcpy(temp, array + length - positions, sizeof(float) * positions);

    // Shift the remaining elements to the right
    memmove(array + positions, array, sizeof(float) * (length - positions));

    // Copy the elements from the temporary array to the beginning
    memcpy(array, temp, sizeof(float) * positions);

    // Free the temporary array
    free(temp);
  }

  

  EMSCRIPTEN_KEEPALIVE
  float* chroma(float* inputArray, int length, int sampleRate) {
    float lowCutoff = 20.0f;
    float highCutoff = 4000.0f;
    int downsampleFactor = 1;
    int newSampleRate = sampleRate / downsampleFactor;
    int newLength = length / downsampleFactor;
    int paddedLength = newLength * 10;

    float* filtered = (float*)malloc(sizeof(float) * length);
    float* downsampled = (float*)malloc(sizeof(float) * newLength);
    float* padded = (float*)malloc(sizeof(float) * paddedLength);

    //bandPassFilter(inputArray, filtered, length, lowCutoff, highCutoff, sampleRate);
    //downsample(filtered, downsampled, length, downsampleFactor);
    //zeroPad(downsampled, padded, newLength, paddedLength);
    zeroPad(inputArray, padded, newLength, paddedLength);

    float* windowed = (float*)malloc(sizeof(float) * paddedLength);
    Hamming(padded, windowed, paddedLength);

    float* ffted = (float*)malloc(sizeof(float) * paddedLength * 2);
    fft(windowed, ffted, paddedLength);

    int binCount = paddedLength / 2 + 1;
    float* K_array = (float*)malloc(sizeof(float) *  binCount);
    fft_to_K(ffted, K_array, paddedLength, newSampleRate);


    float* chroma = (float*)malloc(sizeof(float) * 12);
    chroma = chroma_vector(7, K_array, paddedLength, newSampleRate); //HWUC: 3 for just wanting to capture the lower notes (used to be 5)
    rotateRight(chroma, 12, 3);

    // Free allocated memory
    free(filtered);
    free(downsampled);
    free(padded);
    free(windowed);
    free(ffted);
    free(K_array);
    return chroma;




  }


  EMSCRIPTEN_KEEPALIVE
  int classifyChord(float* chroma){
    int classified;
    // Create chord profiles only if not already initialized
    if (!chordProfilesInitialized) {
        createChordProfiles();
    }

    classified = classifyChromagram(chroma, chordProfiles);

    return classified;

    
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