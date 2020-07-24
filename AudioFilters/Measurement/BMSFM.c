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
#include "Constants.h"
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




float BMGeometricMean(const float *input, float *temp, size_t length){
	// 2^(mean(log2(input)))
	int length_i = (int)length;
    vvlog2f(temp, input, &length_i);
    float meanExp;
    vDSP_meanv(input,1,&meanExp,length);
    return powf(2.0f, meanExp);
	
}




/*
 * geometric mean of a and b is sqrt(a*b), which is equivalent to
 * 2^(log2(a)/2 + log2(b)/2). We use the log formula because it will not
 * overflow.
 */
float BMGeometricMean2(float a, float b){
	return powf(2.0f,0.5*(log2(a)+log2(b)));
}




float BMGeometricArithmeticMean(const float *X, float *temp, size_t length){
	
    // remove values near zero
    float smallNumber = BM_DB_TO_GAIN(-140.0f);
	vDSP_vthr(X, 1, &smallNumber, temp, 1, length);
	
	// find the geometric mean
	float geometricMean = BMGeometricMean(X, temp, length);
    
    // find the arithmetic mean
    float arithmeticMean;
    vDSP_meanv(X, 1, &arithmeticMean, length);
	
	// return the result
	return geometricMean / arithmeticMean;
	
}




float BMSFM_process(BMSFM *This, float* input, size_t inputLength){
    
    // take the abs fft, with nyquist and DC combined into a single term
    BMFFT_absFFTCombinedDCNQ(&This->fft, input, This->b1, inputLength);
    
    size_t spectrumLength = inputLength / 2;
    
    // return geometric mean / arithmetic mean
	return BMGeometricArithmeticMean(This->b1, This->b2, spectrumLength);
}
