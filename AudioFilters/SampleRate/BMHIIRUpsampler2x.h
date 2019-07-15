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
 * @numCoefficients     more coefficients results in more stopband attenuation
 * @transitionBandwidth in (0, 0.5)
 * @returns the stopband attenuation in decibels
 */
float BMHIIRUpsampler2x_init(BMHIIRUpsampler2x* This, size_t numCoefficients, float transitionBandwidth);
void BMHIIRUpsampler2x_free(BMHIIRUpsampler2x* This);
void BMHIIRUpsampler2x_setCoefficients(BMHIIRUpsampler2x* This, const double* coefficientArray);
void BMHIIRUpsampler2x_processBuffer(BMHIIRUpsampler2x* This, const float* input, float* output, size_t numSamples);
void BMHIIRUpsampler2x_clearBuffers(BMHIIRUpsampler2x* This);
void BMHIIRUpsampler2x_impulseResponse(BMHIIRUpsampler2x* This, float* input, float* output, size_t numSamples);


#endif /* BMHIIRUpsampler2x_h */
