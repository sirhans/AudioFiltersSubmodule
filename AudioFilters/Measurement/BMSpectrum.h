//
//  BMSpectrum.h
//  Saturator
//
//  Created by Nguyen Minh Tien on 1/31/18.
//  Anyone may use this file without restrictions
//

#ifndef BMSpectrum_h
#define BMSpectrum_h

#include <stdio.h>
#import <Accelerate/Accelerate.h>

typedef struct BMSpectrum {
    //FFT
    FFTSetup setup;
    DSPSplitComplex fft_input;
    DSPSplitComplex fft_output;
    DSPSplitComplex fft_buffer;
    
    void* fft_input_buffer1;
    void* fft_input_buffer2;
    void* fft_output_buffer1;
    void* fft_output_buffer2;
    void* fft_buffer_buffer1;
    void* fft_buffer_buffer2;
    bool fft_initialized;
    
    float* windowData;
    int dataSize;
    
    size_t fftLength;
} BMSpectrum;

void BMSpectrum_init(BMSpectrum* this, size_t fftLength);
float* BMSpectrum_processData(BMSpectrum* this,float* inData,int inSize,int* outsize,float* nq);

#endif /* BMSpectrum_h */
