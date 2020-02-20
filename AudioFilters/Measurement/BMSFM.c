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
#include <Accelerate/Accelerate.h>

void BMSFM_init(BMSFM *This, size_t inputLength){
    BMFFT_init(&This->fft, inputLength);
    
    size_t bufferLength = sizeof(float)*(size_t)ceilf((float)inputLength / 2.0f);
    This->b1 = malloc(bufferLength);
    This->b2 = malloc(bufferLength);
}


void BMSFM_free(BMSFM *This){
    BMFFT_free(&This->fft);
    
    free(This->b1);
    This->b1 = NULL;
    free(This->b2);
    This->b2 = NULL;
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
    BMFFT_absFFTCombinedDCNQ(&This->fft, input, This->b1, inputLength);
    
    size_t spectrumLength = inputLength / 2;
    
    // square the result
    vDSP_vsq(This->b1, 1, This->b1, 1, spectrumLength);
    
    // get the geometric mean using logarithmic transformation
    int spectrumLength_i = (int)spectrumLength;
    vvlog2f(This->b1, This->b2, &spectrumLength_i);
    float meanExp;
    vDSP_meanv(This->b2,1,&meanExp,spectrumLength);
    float geometricMean = powf(2.0f, meanExp);
    
    // find the arithmetic mean
    float arithmeticMean;
    vDSP_meanv(This->b1, 1, &arithmeticMean, spectrumLength);
    
    return geometricMean / arithmeticMean;
}
