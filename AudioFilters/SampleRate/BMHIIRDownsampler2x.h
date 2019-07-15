//
//  BMHIIRDownsampler2x.h
//  AudioFiltersXcodeProject
//
//  Ported from Downsampler2x4.hpp from Laurent De Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by hans anderson on 7/10/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMHIIRDownsampler2x_h
#define BMHIIRDownsampler2x_h

#include <simd/simd.h>
#include <MacTypes.h>
#include "BMHIIRStage.h"

typedef struct BMHIIRDownsampler2x {
    BMHIIRStage* filterStages;
    size_t numCoefficients, numFilterStages;
} BMHIIRDownsampler2x;


/*!
 *BMHIIRDownsampler2x_init
 *
 * @param coefficientArray  generate these using BMPolyphaseIIR2Designer
 * @param numCoefficients   length of coefficientArray
 * @returns the stopband attenuation, in decibels, for the specified filter.
 */
float BMHIIRDownsampler2x_init(BMHIIRDownsampler2x* This, size_t numCoefficients, float transitionBandwidth);



/*!
 *BMHIIRDownsampler2x_free
 *
 * @abstract free up memory used by the filter coefficients and delays
 */
void BMHIIRDownsampler2x_free(BMHIIRDownsampler2x* This);



/*!
 * BMHIIRDownsampler2x_processBuffer
 * @param input    buffer of input samples with length >= numSamplesIn
 * @param output   buffer of output samples with lenth >= numSamplesIn / 2
 * @abstract Downsample by 2x. Works in place.
 */
void BMHIIRDownsampler2x_processBuffer(BMHIIRDownsampler2x* This, float* input, float* output, size_t numSamplesIn);




void BMHIIRDownsampler2x_impulseResponse(BMHIIRDownsampler2x* This, float* IR, size_t numSamples);



#endif /* BMHIIRDownsampler2x_h */
