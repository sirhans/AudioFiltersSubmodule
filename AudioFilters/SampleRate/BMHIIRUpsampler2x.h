//
//  BMHIIRUpsampler2x.h
//  AudioFiltersXcodeProject
//
//  Based on Upsampler2x4Neon.h from Laurent de Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMHIIRUpsampler2x_h
#define BMHIIRUpsampler2x_h

#include <simd/simd.h>
#include <mactypes.h>
#include "BMHIIRStage.h"

typedef struct BMHIIRUpsampler2x {
    BMHIIRStage* filterStages;
    size_t numCoefficients, numFilterStages;
} BMHIIRUpsampler2x;

/*!
 *BMHIIRUpsampler2x_init
 * @param This pointer to an initialised BMHIIRUpsampler2x struct
 * @param numCoefficients     more coefficients results in more stopband attenuation
 * @param transitionBandwidth in (0, 0.5)
 * @return the stopband attenuation in decibels
 */
float BMHIIRUpsampler2x_init(BMHIIRUpsampler2x* This, size_t numCoefficients, float transitionBandwidth);


/*!
 *BMHIIRUpsampler2x_free
 * @param This  pointer to an initialised BMHIIRUpsampler2x struct
 * @abstract frees memory allocated to the antialiasing filters
 */
void BMHIIRUpsampler2x_free(BMHIIRUpsampler2x* This);



/*!
 *BMHIIRUpsampler2x_processBufferMono
 * @param This pointer to an initialised BMHIIRUpsampler2x struct
 * @param input pointer to an array of floats with length >= numSamplesIn
 * @param output pointer to an array of floats with length >= 2*numSamplesIn
 * @param numSamplesIn the length of the input or half the length of the output
 */
void BMHIIRUpsampler2x_processBufferMono(BMHIIRUpsampler2x* This, const float* input, float* output, size_t numSamplesIn);



/*!
 * BMHIIRUpsampler2x_impulseResponse
 * @param IR  pointer to an array of length numSamples for storing the output
 * @param numSamples  length of the impulse response
 */
void BMHIIRUpsampler2x_impulseResponse(BMHIIRUpsampler2x* This, float* IR, size_t numSamples);


#endif /* BMHIIRUpsampler2x_h */
