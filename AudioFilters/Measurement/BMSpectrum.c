//
//  BMSpectrum.c
//  Saturator
//
//  Created by Nguyen Minh Tien on 1/31/18.
//  This file is public domain. No restrictions.
//

#include "BMSpectrum.h"
#include <assert.h>
#include "Constants.h"

#define BM_SPECTRUM_DEFAULT_BUFFER_LENGTH 1024

void BMSpectrum_setupFFT(BMSpectrum* This,size_t n);


bool IsPowerOfTwo(size_t x)
{
    return (x & (x - 1)) == 0;
}



void BMSpectrum_init(BMSpectrum* This){
    BMSpectrum_initWithLength(This, BM_SPECTRUM_DEFAULT_BUFFER_LENGTH);
}



void BMSpectrum_free(BMSpectrum* This){
    BMFFT_free(&This->fft);
    free(This->buffer);
    This->buffer = NULL;
}



void BMSpectrum_initWithLength(BMSpectrum* This, size_t maxInputLength){
    This->maxInputLength = maxInputLength;
    BMFFT_init(&This->fft, maxInputLength);
    This->buffer = malloc(sizeof(float)*maxInputLength);
}





bool BMSpectrum_processData(BMSpectrum* This,
                            const float* input,
                            float* output,
                            int inputLength,
                            int *outputLength,
                            float* nyquist){
    if(inputLength > 0){
        *outputLength = inputLength / 2;
        
        // calculate the FFT
        *nyquist = BMFFT_absFFTReturnNyquist(&This->fft, input, output, inputLength);
        
        // Limit the range of output values
        float minValue = BM_DB_TO_GAIN(-60.0f);
        float maxValue = BM_DB_TO_GAIN(60.0f);
        vDSP_vclip(output, 1, &minValue, &maxValue, output, 1, *outputLength);
        float b = 0.01;
        vDSP_vdbcon(input, 1, &b, output, 1, *outputLength, 1);
        
        
        return true;
    }
    return false;
}




float BMSpectrum_processDataBasic(BMSpectrum* This,
                                  const float* input,
                                  float* output,
                                  bool applyWindow,
                                  size_t inputLength){
    assert(inputLength > 0);
    assert(IsPowerOfTwo(inputLength));
    assert(inputLength <= This->maxInputLength);
    
    // apply a hamming window to the input
    const float *windowedInput = input;
    if(applyWindow){
        BMFFT_hammingWindow(&This->fft, input, This->buffer, inputLength);
        windowedInput = This->buffer;
    }
    
    // compute abs(fft(input)) and drop the nyquist term
    float absNyquist = BMFFT_absFFTReturnNyquist(&This->fft, windowedInput, output, inputLength);
    
    return absNyquist;
}

