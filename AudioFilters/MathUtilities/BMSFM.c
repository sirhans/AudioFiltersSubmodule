//
//  BMSFM.c
//  AudioFiltersXcodeProject
//
//  Spectral Flatness Measure is the (geometricMean / arithmeticMean) of abs(fft(x))
//
//  Created by hans anderson on 9/14/19.
//  Anyone may use this file without restrictions
//

#include "BMSFM.h"

void BMSFM_init(BMSFM* This, size_t inputLength){
    BMFFT_init(&This->fft, inputLength);
    
    This->buffer = malloc(sizeof(float)*inputLength);
}


void BMSFM_free(BMSFM* This){
    BMFFT_free(&This->fft);
    
    free(This->buffer);
    This->buffer = NULL;
}



float BMGeometricMean(float* input, size_t inputLength){
    // find the sum of the logs of all elements
    float logSum = 0.0f;
    for(size_t i=0; i<inputLength; i++)
        logSum += log2f(input[i]);
    
    return powf(2.0f,logSum/(float)inputLength);
}



float BMSFM_process(BMSFM* This, float* input, size_t inputLength){
    
    // take the abs fft, with nyquist and DC combined into a single term
    BMFFT_absFFTCombinedDCNQ(&This->fft, input, This->buffer, inputLength);
    
    float geometricMean = BMGeometricMean(This->buffer, inputLength);
    
    float arithmeticMean;
    vDSP_meanv(This->buffer, 1, &arithmeticMean, inputLength/2);
    
    return geometricMean / arithmeticMean;
}
