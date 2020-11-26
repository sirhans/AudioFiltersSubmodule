//
//  BMDownsampler.h
//  BMAudioFilters
//
//  Created by Hans on 13/9/17.
//  Anyone may use this file without restrictions
//


#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMDownsampler_h
#define BMDownsampler_h

#include <MacTypes.h>
#include "BMIIRDownsampler2x.h"
#include "BMMultiLevelBiquad.h"
#include "BMUpsampler.h"

typedef struct BMDownsampler {
	BMIIRDownsampler2x* downsamplers2x;
	BMMultiLevelBiquad antiRingingFilter;
	float *bufferL1, *bufferR1, *bufferL2, *bufferR2;
	size_t numStages, downsampleFactor;
	bool stereo;
} BMDownsampler;


/*!
 *BMDownsampler_init
 * @This   pointer to a BMDownsampler struct
 * @stereo set true for stereo AA filters; false for mono
 * @param  downsampleFactor supported values: 2^n
 */
void BMDownsampler_init(BMDownsampler* This, bool stereo, size_t downsampleFactor, enum resamplerType type);


/*!
 *BMDownsampler_processBufferMono
 *
 * @absract upsample a buffer that contains a single channel of audio samples
 *
 * @param input    length = numSamplesIn
 * @param output   length = numSamplesIn / upsampleFactor
 * @param numSamplesIn  number of input samples to process
 */
void BMDownsampler_processBufferMono(BMDownsampler* This, float* input, float* output, size_t numSamplesIn);


/*!
 *BMDownsampler_processBufferStereo
 *
 * @absract upsample a buffer that contains a single channel of audio samples
 *
 * @param inputL    length = numSamplesIn
 * @param inputR    length = numSamplesIn
 * @param outputL   length = numSamplesIn / upsampleFactor
 * @param outputR   length = numSamplesIn / upsampleFactor
 * @param numSamplesIn  number of input samples to process
 */
void BMDownsampler_processBufferStereo(BMDownsampler* This, float* inputL, float* inputR, float* outputL, float* outputR, size_t numSamplesIn);


void BMDownsampler_free(BMDownsampler* This);


/*!
 *BMDownsampler_impulseResponse
 * @param IR empty array for storing the impulse response with length = IRLength
 * @param IRLength -
 */
void BMDownsampler_impulseResponse(BMDownsampler* This, float* IR, size_t IRLength);


/*!
 *BMDownsampler_getLatencyInSamples
 *
 * @returns the estimated latency of the downsampler in samples
 */
float BMDownsampler_getLatencyInSamples(BMDownsampler *This);


#endif /* BMDownsampler_h */

#ifdef __cplusplus
}
#endif
