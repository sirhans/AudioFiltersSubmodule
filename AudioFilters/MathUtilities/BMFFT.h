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



typedef struct BMFFT {
    size_t inputLength, outputLength;
    
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
    
    float* hammingWindow;
} BMFFT;



void BMFFT_init(BMFFT* this, size_t inputLength);

void BMFFT_free(BMFFT* this);




/*!
 *BMFFT_absFFTCombinedDCNQ
 *
 * @abstract This function computes the absolute value of the FFT with the DC and Nyquist terms grouped together in the first element of the output and divided by sqrt(2). This is not the standard way to compute abs(fft) so do not use this function unless this unusual method is what you actually need. The reason for combining DC and Nyquist is that the combined term will have the same statistical distribution as the others. In a normal FFT output where DC and Nyquist are separate, those two terms follow a different distribution because they are real valued while the others are complex.
 *
 * @param This    pointer to an initialized BMFFT struct
 * @param input   an array of real valued inputs with length = inputLength
 * @param output  an array of real valued output with length = inputLength/2
 * @param inputLength MUST BE EQUAL TO THE LENGTH USED WHEN *This was initialised
 */
void BMFFT_absFFTCombinedDCNQ(BMFFT* This, float* input, float* output, size_t inputLength);




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
void BMFFT_absFFT(BMFFT* This, float* input, float* output, size_t inputLength);






/*!
 *BMFFT_hammingWindow
 *
 * @abstract This function computes the absolute value of the FFT
 *
 * @param This    pointer to an initialized BMFFT struct
 * @param input   an array of real valued inputs with length = numSamples
 * @param output  an array of real valued output with length = numSamples
 * @param numSamples MUST BE EQUAL TO THE LENGTH USED WHEN *This was initialised
 */
void BMFFT_hammingWindow(BMFFT* This, float* input, float* output, size_t numSamples);



#endif /* BMFFT_h */
