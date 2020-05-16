//
//  BMFFT.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 9/14/19.
//  Anyone may use this file without restrictions
//

#ifndef BMFFT_h
#define BMFFT_h

#include <stdio.h>
#import <Accelerate/Accelerate.h>

enum BMFFTWindowType {BMFFT_NONE,BMFFT_BLACKMANHARRIS,BMFFT_HAMMING};

typedef struct BMFFT {
    size_t maxInputLength;
    
    FFTSetup setup;
    DSPSplitComplex fft_input;
    DSPSplitComplex fft_output;
    DSPSplitComplex fft_buffer;
    
    void* fft_input_buffer_i;
    void* fft_input_buffer_r;
    void* fft_output_buffer_i;
    void* fft_output_buffer_r;
    void* fft_buffer_buffer_i;
    void* fft_buffer_buffer_r;
    
    size_t recursionLevels;
    
    float* window;
    size_t windowCurrentLength;
	
	enum BMFFTWindowType windowType;
} BMFFT;


/*!
 *BMFFT_init
 */
void BMFFT_init(BMFFT *This, size_t maxInputLength);

/*!
 *BMFFT_free
 */
void BMFFT_free(BMFFT *This);


/*!
 *BMFFT_complexFFT
 *
 * calculates complex output from real-valued input. Only the positive frequency side of the output is returned (from DC to nyquist). Because the DC and Nyquist terms are both real-valued, the Nyquist term is stored in output[0].imag so that the output length can be inputLength/2 rather than inputLength/2 + 1.
 *
 * @param This pointer to an uninitialised struct
 * @param input real valued input array of length inputLength
 * @param output a complex-valued array of length inputLength / 2
 * @param inputLength a power of 2 such that 0 < inputLength <= This->maxInputLength
 */
void BMFFT_complexFFT(BMFFT *This,
                      const float* input,
                      DSPSplitComplex *output,
                      size_t inputLength);



/*!
 *BMFFT_absFFTCombinedDCNQ
 *
 * @abstract This function computes the absolute value of the FFT with the DC and Nyquist terms grouped together in the first element of the output and divided by sqrt(2). This is not the standard way to compute abs(fft) so do not use this function unless this unusual method is what you actually need. The reason for combining DC and Nyquist is that the combined term will have the same statistical distribution as the others. In a normal FFT output where DC and Nyquist are separate, those two terms follow a different distribution because they are real valued while the others are complex.
 *
 * @param This    pointer to an initialized BMFFT struct
 * @param input   an array of real valued inputs with length = inputLength
 * @param output  an array of real valued output with length = inputLength/2
 * @param inputLength MUST BE <= THE maxInputLength GIVEN WHEN *This was initialised
 */
void BMFFT_absFFTCombinedDCNQ(BMFFT *This,
                              const float* input,
                              float* output,
                              size_t inputLength);




/*!
 *BMFFT_absFFT
 *
 * @abstract This function computes the absolute value of the FFT
 *
 * @param This    pointer to an initialized BMFFT struct
 * @param input   an array of real valued inputs with length = inputLength
 * @param output  an array of real valued output with length = 1 + inputLength/2
 * @param inputLength MUST BE EQUAL TO THE LENGTH USED WHEN *This was initialised
 */
void BMFFT_absFFT(BMFFT *This,
                  const float* input,
                  float* output,
                  size_t inputLength);



/*!
 *BMFFT_absFFTReturnNyquist
 *
 * @abstract This function computes the absolute value of the FFT
 *
 * @param This    pointer to an initialized BMFFT struct
 * @param input   an array of real valued inputs with length >= inputLength
 * @param output  an array of real valued output with length >= inputLength/2
 * @param inputLength MUST BE EQUAL TO THE LENGTH USED WHEN *This was initialised
 *
 * @returns the absolute value of the nyquist term
 */
float BMFFT_absFFTReturnNyquist(BMFFT *This,
                                const float* input,
                                float* output,
                                size_t inputLength);






/*!
 *BMFFT_hammingWindow
 *
 * applies a hamming window to the input and stores the result in output
 *
 * @param This    pointer to an initialized BMFFT struct
 * @param input   an array of real valued inputs with length = numSamples
 * @param output  an array of real valued output with length = numSamples
 * @param numSamples must be <= This->maxLength
 */
void BMFFT_hammingWindow(BMFFT *This,
                         const float* input,
                         float* output,
                         size_t numSamples);



/*!
 *BMFFT_blackmanHarrisWindow
 *
 * applies a Blackman Harris to the input and stores the result in output
 *
 * @param This    pointer to an initialized BMFFT struct
 * @param input   an array of real valued inputs with length = numSamples
 * @param output  an array of real valued output with length = numSamples
 * @param numSamples must be <= This->maxLength
 */
void BMFFT_blackmanHarrisWindow(BMFFT *This,
								const float* input,
								float* output,
								size_t numSamples);


/*!
 *BMFFT_generateBlackmanHarris
 *
 * This function generates the blackman-harris window coefficients with the
 * window centred at length/2.
 * If you need to apply this window in realtime you should use
 * BMFFT_blackmanHarrisWindow instead because it automatically caches previous
 * calculations and reuses them the next time you call the function for a
 * significant improvement in performance.
 */
void BMFFT_generateBlackmanHarrisCoefficients(float* window, size_t length);



#endif /* BMFFT_h */
