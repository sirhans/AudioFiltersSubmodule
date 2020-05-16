//
//  BMFFT.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 9/14/19.
//  Anyone may use this file
//

#include "BMFFT.h"
#include "BMIntegerMath.h"




void BMFFT_init(BMFFT *This, size_t maxInputLength){

    assert(isPowerOfTwo(maxInputLength));
    
    This->maxInputLength = maxInputLength;
    size_t maxOutputLength = maxInputLength / 2;
    
    // find out how many levels of fft recursion the FFT setup will require
    This->recursionLevels = log2i((int)maxInputLength);
    
    // allocate memory for buffers
    //input
    This->fft_input_buffer_r = malloc(maxOutputLength*sizeof(float)+15);
    This->fft_input.realp = (float*)(((uintptr_t)This->fft_input_buffer_r+15) & ~ (uintptr_t)0x0F);
    This->fft_input_buffer_i = malloc(maxOutputLength*sizeof(float)+15);
    This->fft_input.imagp = (float*)(((uintptr_t)This->fft_input_buffer_i+15) & ~ (uintptr_t)0x0F);
    //output
    This->fft_output_buffer_r = malloc(maxOutputLength*sizeof(float)+15);
    This->fft_output.realp = (float*)(((uintptr_t)This->fft_output_buffer_r+15) & ~ (uintptr_t)0x0F);
    This->fft_output_buffer_i = malloc(maxOutputLength*sizeof(float)+15);
    This->fft_output.imagp = (float*)(((uintptr_t)This->fft_output_buffer_i+15) & ~ (uintptr_t)0x0F);
    //Buffer
    This->fft_buffer_buffer_r = malloc(maxOutputLength*sizeof(float)+15);
    This->fft_buffer.realp = (float*)(((uintptr_t)This->fft_buffer_buffer_r+15) & ~ (uintptr_t)0x0F);
    This->fft_buffer_buffer_i = malloc(maxOutputLength*sizeof(float)+15);
    This->fft_buffer.imagp = (float*)(((uintptr_t)This->fft_buffer_buffer_i+15) & ~ (uintptr_t)0x0F);
    
    // prepare the fft coefficeints
    This->setup = vDSP_create_fftsetup(This->recursionLevels, kFFTRadix2);
    
    // set up the window
	This->windowType = BMFFT_HAMMING;
    This->window = malloc(sizeof(float)*maxInputLength);
    vDSP_hamm_window(This->window, maxInputLength, 0);
    This->windowCurrentLength = maxInputLength;
}






void BMFFT_free(BMFFT *This){
    vDSP_destroy_fftsetup(This->setup);
    
    free(This->fft_input_buffer_i);
    free(This->fft_input_buffer_r);
    free(This->fft_output_buffer_i);
    free(This->fft_output_buffer_r);
    free(This->fft_buffer_buffer_i);
    free(This->fft_buffer_buffer_r);
    free(This->window);
    
    This->fft_input_buffer_i = NULL;
    This->fft_input_buffer_r = NULL;
    This->fft_output_buffer_i = NULL;
    This->fft_output_buffer_r = NULL;
    This->fft_buffer_buffer_i = NULL;
    This->fft_buffer_buffer_r = NULL;
    This->window = NULL;
}




void BMFFT_complexFFT(BMFFT *This,
                      const float* input,
                      DSPSplitComplex *output,
                      size_t inputLength){
    // require the input length to meet the requirements of the FFT setup struct
    assert(inputLength <= This->maxInputLength);
    assert(isPowerOfTwo(inputLength));
    assert(inputLength > 0);
    
    // copy the input to the packed complex array that the fft algo uses
    size_t outputLength = inputLength / 2;
    vDSP_ctoz((DSPComplex *) input, 2, &This->fft_input, 1, outputLength);
    
    // calculate the complex fft
    size_t recursionLevels = log2i((uint32_t)inputLength);
    vDSP_fft_zropt(This->setup, &This->fft_input, 1, output, 1, &This->fft_buffer, recursionLevels, FFT_FORWARD);
}




void BMFFT_absFFTCombinedDCNQ(BMFFT *This,
                              const float* input,
                              float* output,
                              size_t inputLength){
    // take the compled-valued fft
    BMFFT_complexFFT(This, input, &This->fft_output, inputLength);
    
    // scale the combined DC / Nyquist term so that it has the same statistical
    // distribution as the complex terms
    This->fft_output.imagp[0] *= 1.0f / sqrtf(2.0f);
    This->fft_output.realp[0] *= 1.0f / sqrtf(2.0f);
    
    // take the absolute value of the complex output to get real output
    size_t outputLength = inputLength / 2;
    vDSP_zvabs(&This->fft_output, 1, output, 1, outputLength);
}





void BMFFT_absFFT(BMFFT *This,
                  const float* input,
                  float* output,
                  size_t inputLength){
    float absNyquist = BMFFT_absFFTReturnNyquist(This, input, output, inputLength);
    
    // take the absolute value of the nyquist term and store it at the end of the output array
    size_t outputLength = inputLength / 2;
    output[outputLength] = absNyquist;
}





float BMFFT_absFFTReturnNyquist(BMFFT *This,
                                const float* input,
                                float* output,
                                size_t inputLength){
    // calculate the FFT
    BMFFT_complexFFT(This, input, &This->fft_output, inputLength);
    
    // extract the nyquist term from its place and store it
    float nyquist = This->fft_output.imagp[0];
    This->fft_output.imagp[0] = 0.0f;
    
    // take the absolute value of the complex output to get real output
    size_t outputLength = inputLength / 2;
    vDSP_zvabs(&This->fft_output, 1, output, 1, outputLength);
    
    // take the absolute value of the nyquist term and return it
    return fabsf(nyquist);
}



void BMFFT_hammingWindow(BMFFT *This,
                         const float* input,
                         float* output,
                         size_t numSamples){
    assert(numSamples <= This->maxInputLength);
    
    // if the window cached in the buffer does not have the specified length or isn't a hamming window, recompute it.
    if(This->windowCurrentLength != numSamples || This->windowType != BMFFT_HAMMING){
        vDSP_hamm_window(This->window, numSamples, 0);
        This->windowCurrentLength = numSamples;
		This->windowType = BMFFT_HAMMING;
    }
    
    vDSP_vmul(input,1,This->window,1,output,1,numSamples);
}






void BMFFT_generateBlackmanHarrisCoefficients(float* window, size_t length){
	for(size_t i=0; i<length; i++){
		// the function is defined on [-1/2,1/2]
		double x = -0.5 + ( (double)i / (double)(length-1) );
		
		// this formula is copied out of the documentation for the blackmanHarris function in Mathematica
		double y = 48829.0 * cos(2.0 * M_PI * x) +
				   14128.0 * cos(4.0 * M_PI * x) +
		           1168.0 * cos(6.0 * M_PI * x) +
				   35875.0;
		y /= 100000.0;
		
		// copy the result to the output
		window[i] = y;
	}
}




void BMFFT_blackmanHarrisWindow(BMFFT *This,
								const float* input,
								float* output,
								size_t numSamples){
	assert(numSamples <= This->maxInputLength);
    
    // if the window cached in the buffer does not have the specified length, or isn't a blackman-harris window recompute it.
    if(This->windowCurrentLength != numSamples || This->windowType != BMFFT_BLACKMANHARRIS){
        BMFFT_generateBlackmanHarrisCoefficients(This->window, numSamples);
        This->windowCurrentLength = numSamples;
		This->windowType = BMFFT_BLACKMANHARRIS;
    }
    
    vDSP_vmul(input,1,This->window,1,output,1,numSamples);
}
