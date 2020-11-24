//
//  BMSpectralCentroid.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/15/20.
//  This file is public domain without restrictions.
//

#include "BMSpectralCentroid.h"
#include <Accelerate/Accelerate.h>
#include "Constants.h"




void BMSpectralCentroid_init(BMSpectralCentroid *This, float sampleRate, size_t maxInputLength){
	BMSpectrum_initWithLength(&This->spectrum, maxInputLength);
	
	This->sampleRate = sampleRate;
	
	size_t maxOutputLength = 1 + (maxInputLength / 2);
	This->buffer = malloc(sizeof(float)*maxOutputLength);
	This->weights = malloc(sizeof(float)*maxOutputLength);
	
	This->previousOutputLength = 0;
}





void BMSpectralCentroid_free(BMSpectralCentroid *This){
	BMSpectrum_free(&This->spectrum);
	free(This->buffer);
	This->buffer = NULL;
	free(This->weights);
	This->weights = NULL;
}


float BMSpectralCentroid_processBufferAtPeak(BMSpectralCentroid *This, float* input, size_t inputLength){
    //Get the idx of the peak value
    vDSP_Length maxIdx;
    float maxV;
    vDSP_maxmgvi(input, 1, &maxV, &maxIdx, inputLength);
    
    size_t windowSize = 4096;
    size_t halfWS = windowSize/2;
    size_t startIdx = BM_MAX(0, maxIdx-halfWS);
    startIdx = BM_MIN(startIdx, inputLength-windowSize);
    
    return BMSpectralCentroid_process(This, input+startIdx, windowSize);
}


float BMSpectralCentroid_process(BMSpectralCentroid *This, float* input, size_t inputLength){
	size_t outputLength = 1 + (inputLength / 2);
	
	// get the magnitude spectrum of the input
	float nyquist = BMSpectrum_processDataBasic(&This->spectrum, input, This->buffer, true, inputLength);
	This->buffer[outputLength - 1] = nyquist;
	
	// calculate the sum of the values in the spectrum
	float sum;
	vDSP_sve(This->buffer, 1, &sum, outputLength);
	
	// if the length has changed or if this is the first time we do this,
	// calculate weights from 0 to 1 for a weighted sum
	if (This->previousOutputLength != outputLength){
		This->previousOutputLength = outputLength;
		
		float zero = 0.0f;
		float increment = 1.0 / (double)(outputLength - 1);
		vDSP_vramp(&zero, &increment, This->weights, 1, outputLength);
	}
	
	// calculate the weighted sum of the spectrum
	float weightedSum;
	vDSP_vmul(This->buffer, 1, This->weights, 1, This->buffer, 1, outputLength);
	vDSP_sve(This->buffer, 1, &weightedSum, outputLength);
	
	// if the sum is non-zero, calculate the spectral centroid as usual
	float centroidIn01;
	if (sum > 0.0f)
		centroidIn01 = weightedSum / sum;
	
	// if the volume is zero, we define the spectrum to be balanced in the middle
	else centroidIn01 = 0.5f;
	
	// convert the centroid to Hz and return
	return centroidIn01 * This->sampleRate * 0.5f;
}
