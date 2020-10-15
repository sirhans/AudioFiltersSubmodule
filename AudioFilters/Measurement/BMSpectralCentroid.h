//
//  BMSpectralCentroid.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/15/20.
//  This file is public domain without restrictions.
//

#ifndef BMSpectralCentroid_h
#define BMSpectralCentroid_h

#include <stdio.h>
#include "BMSpectrum.h"

typedef struct BMSpectralCentroid {
	BMSpectrum spectrum;
	float *buffer, *weights;
	size_t previousOutputLength;
} BMSpectralCentroid;

void BMSpectralCentroid_init(BMSpectralCentroid *This, size_t maxInputLength);

void BMSpectralCentroid_free(BMSpectralCentroid *This);

/*!
 *BMSpectralCentroid_process
 *
 * @param This pointer to an initialised struct
 * @param input pointer to a buffer of audio samples. Do not apply an FFT analysis window function to the buffer. That is done automatically by this function.
 * @param inputLength length of input buffer
 *
 * @returns the spectral centroid in [0,1] where 0 is DC and 1 is nyquist
 */
float BMSpectralCentroid_process(BMSpectralCentroid *This, float* input, size_t inputLength);

#endif /* BMSpectralCentroid_h */
