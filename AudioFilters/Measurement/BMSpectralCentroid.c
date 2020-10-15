//
//  BMSpectralCentroid.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/15/20.
//  This file is public domain without restrictions.
//

#include "BMSpectralCentroid.h"
#include <Accelerate/Accelerate.h>





void BMSpectralCentroid_init(BMSpectralCentroid *This, size_t maxInputLength){
	BMSpectrum_initWithLength(&This->spectrum, maxInputLength);
	
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





/*!
 *BMSpectralCentroid_process
 *
 * @param This pointer to an initialised struct
 * @param input pointer to a buffer of audio samples. Do not apply an FFT analysis window function to the buffer. That is done automatically by this function.
 * @param inputLength length of input buffer
 *
 * @returns the spectral centroid in [0,1] where 0 is DC and 1 is nyquist
 */
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
	
	// if the sum is non-zero, return the spectral centroid
	if (sum > 0.0f)
		return (weightedSum / sum);
	
	// if the volume is zero, we define the spectrum to be balanced in the middle
	return 0.5;
}
