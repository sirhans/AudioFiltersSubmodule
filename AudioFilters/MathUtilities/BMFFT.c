//
//  BMFFT.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 9/14/19.
//  Anyone may use this file
//

#include "BMFFT.h"
#include "BMIntegerMath.h"




void BMFFT_init(BMFFT *This, size_t inputLength){

    assert(isPowerOfTwo(inputLength));
    
    This->inputLength = inputLength;
    This->outputLength = inputLength / 2;
    
    // find out how many levels of fft recursion the FFT setup will require
    This->recursionLevels = log2i((int)inputLength);
    
    // allocate memory for buffers
    //input
    This->fft_input_buffer_r = malloc(This->outputLength*sizeof(float)+15);
    This->fft_input.realp = (float*)(((uintptr_t)This->fft_input_buffer_r+15) & ~ (uintptr_t)0x0F);
    This->fft_input_buffer_i = malloc(This->outputLength*sizeof(float)+15);
    This->fft_input.imagp = (float*)(((uintptr_t)This->fft_input_buffer_i+15) & ~ (uintptr_t)0x0F);
    //output
    This->fft_output_buffer_r = malloc(This->outputLength*sizeof(float)+15);
    This->fft_output.realp = (float*)(((uintptr_t)This->fft_output_buffer_r+15) & ~ (uintptr_t)0x0F);
    This->fft_output_buffer_i = malloc(This->outputLength*sizeof(float)+15);
    This->fft_output.imagp = (float*)(((uintptr_t)This->fft_output_buffer_i+15) & ~ (uintptr_t)0x0F);
    //Buffer
    This->fft_buffer_buffer_r = malloc(This->outputLength*sizeof(float)+15);
    This->fft_buffer.realp = (float*)(((uintptr_t)This->fft_buffer_buffer_r+15) & ~ (uintptr_t)0x0F);
    This->fft_buffer_buffer_i = malloc(This->outputLength*sizeof(float)+15);
    This->fft_buffer.imagp = (float*)(((uintptr_t)This->fft_buffer_buffer_i+15) & ~ (uintptr_t)0x0F);
    
    // prepare the fft coefficeints
    This->setup = vDSP_create_fftsetup(This->recursionLevels, kFFTRadix2);
    
    // set up the hamming window
    This->hammingWindow = malloc(sizeof(float)*inputLength);
    vDSP_hamm_window(This->hammingWindow, inputLength, 0);
}






void BMFFT_free(BMFFT *This){
    vDSP_destroy_fftsetup(This->setup);
    
    free(This->fft_input_buffer_i);
    free(This->fft_input_buffer_r);
    free(This->fft_output_buffer_i);
    free(This->fft_output_buffer_r);
    free(This->fft_buffer_buffer_i);
    free(This->fft_buffer_buffer_r);
    free(This->hammingWindow);
    
    This->fft_input_buffer_i = NULL;
    This->fft_input_buffer_r = NULL;
    This->fft_output_buffer_i = NULL;
    This->fft_output_buffer_r = NULL;
    This->fft_buffer_buffer_i = NULL;
    This->fft_buffer_buffer_r = NULL;
    This->hammingWindow = NULL;
}







void BMFFT_absFFTCombinedDCNQ(BMFFT *This, float* input, float* output, size_t inputLength){
    // require the input length to match the size of the FFT setup
    assert(inputLength == This->inputLength);
    
    // copy the input to the packed complex array that the fft algo uses
    vDSP_ctoz((DSPComplex *) input, 2, &This->fft_input, 1, This->outputLength);
    
    // calculate the fft
    vDSP_fft_zropt(This->setup, &This->fft_input, 1, &This->fft_output, 1, &This->fft_buffer, This->recursionLevels, FFT_FORWARD);
    
    // scale the combined DC / Nyquist term so that it has the same statistical
    // distribution as the complext terms
    This->fft_output.imagp[0] *= 1.0f / sqrtf(2.0f);
    This->fft_output.realp[0] *= 1.0f / sqrtf(2.0f);
    
    // take the absolute value of the complex output to get real output
    vDSP_zvabs(&This->fft_output, 1, output, 1, This->outputLength);
}





void BMFFT_absFFT(BMFFT *This, float* input, float* output, size_t inputLength){
    // require the input length to match the size of the FFT setup
    assert(inputLength == This->inputLength);
    
    // copy the input to the packed complex array that the fft algo uses
    vDSP_ctoz((DSPComplex *) input, 2, &This->fft_input, 1, This->outputLength);
    
    // calculate the fft
    vDSP_fft_zropt(This->setup, &This->fft_input, 1, &This->fft_output, 1, &This->fft_buffer, This->recursionLevels, FFT_FORWARD);
    
    // extract the nyquist term from its place and store it
    float nyquist = This->fft_output.imagp[0];
    This->fft_output.imagp[0] = 0.0f;
    
    // take the absolute value of the complex output to get real output
    vDSP_zvabs(&This->fft_output, 1, output, 1, This->outputLength);
    
    // take the absolute value fo the nyquist term and store it at the end of the output array
    output[This->outputLength] = nyquist;
}





void BMFFT_hammingWindow(BMFFT *This, float* input, float* output, size_t numSamples){
    assert(numSamples == This->inputLength);
    vDSP_vmul(input,1,This->hammingWindow,1,output,1,numSamples);
}
