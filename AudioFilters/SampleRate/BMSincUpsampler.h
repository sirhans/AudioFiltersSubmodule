//
//  BMSincUpsampler.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/16/20.
//  This file is released to the public domain.
//

#ifndef BMSincUpsampler_h
#define BMSincUpsampler_h

#include <stdio.h>

#define BMSINC_MAX_KERNEL_LENGTH

typedef struct BMSincUpsampler {
	size_t upsampleFactor, kernelLength, inputPadding, numKernels;
	float **filterKernels;
} BMSincUpsampler;



/*!
 *BMSincUpsampler_init
 *
 * @param This pointer to an uninitialised struct
 * @param interpolationPoints number of points read for each new point generated. This MUST be an even number.
 * @param upsampleFactor if upsampleFactor = N then you get N points out from every 1 point in, minus the padding at the left and right ends of the input
 */
void BMSincUpsampler_init(BMSincUpsampler *This,
						  size_t interpolationPoints,
						  size_t upsampleFactor);




/*!
 *BMSincUpsampler_free
 */
void BMSincUpsampler_free(BMSincUpsampler *This);




/*!
 *BMSincUpsampler_process
 *
 * @param This pointer to an initialised struct
 * @param input input array with length inputLength
 * @param output output array with length (inputLength - interpolationPoints/2) * upsampleFactor
 *
 * @returns the number of output points
 */
size_t BMSincUpsampler_process(BMSincUpsampler *This,
							 const float* input,
							 float* output,
							 size_t inputLength);


#endif /* BMSincUpsampler_h */
