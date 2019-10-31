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

void BMSFM_init(BMSFM *This, size_t inputLength){
    BMFFT_init(&This->fft, inputLength);
    
    This->buffer = malloc(sizeof(float)*inputLength);
}


void BMSFM_free(BMSFM *This){
    BMFFT_free(&This->fft);
    
    free(This->buffer);
    This->buffer = NULL;
}



float BMGeometricMean(float* input, size_t inputLength){
    // find the sum of the logs of all elements
    float logSum = 0.0f;
    for(size_t i=0; i<inputLength; i++){
        logSum += log2f(input[i]);
        if(input[i]==0)
            printf("zero");
    }
    
    return powf(2.0f,logSum/(float)inputLength);
}



/*
 * geometric mean of a and b is sqrt(a*b), which is equivalent to
 * 2^(log2(a)/2 + log2(b)/2). We use the log formula because it will not
 * overflow.
 */
float BMGeometricMean2(float a, float b){
	return powf(2.0f,0.5*(log2(a)+log2(b)));
}





float BMSFM_process(BMSFM *This, float* input, size_t inputLength){
    
    // take the abs fft, with nyquist and DC combined into a single term
    BMFFT_absFFTCombinedDCNQ(&This->fft, input, This->buffer, inputLength);
    
    size_t outputLength = inputLength / 2;
    
    // square the result
    vDSP_vsq(This->buffer, 1, This->buffer, 1, outputLength);
    
    float geometricMean = BMGeometricMean(This->buffer, outputLength);
    
    float arithmeticMean;
    vDSP_meanv(This->buffer, 1, &arithmeticMean, outputLength);
    
    return geometricMean / arithmeticMean;
}
