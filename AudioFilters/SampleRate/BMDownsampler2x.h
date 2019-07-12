//
//  BMDownsampler2x.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/10/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMDownsampler2x_h
#define BMDownsampler2x_h

#include <simd/simd.h>
#include <MacTypes.h>
#include "BMHIIRStage.h"

typedef struct BMDownsampler2x {
    BMHIIRStage* filterStages;
    size_t numCoefficients, numFilterStages;
} BMDownsampler2x;


/*!
 *BMDownsampler2x_init
 *
 * @param coefficientArray  generate these using BMPolyphaseIIR2Designer
 * @param numCoefficients   length of coefficientArray
 */
void BMDownsampler2x_init(BMDownsampler2x* This, const float* coefficientArray, size_t numCoefficients);



/*!
 *BMDownsampler2x_free
 *
 * @abstract free up memory used by the filter coefficients and delays
 */
void BMDownsampler2x_free(BMDownsampler2x* This);



/*!
 * BMDownsampler2x_processBuffer
 * @param input    buffer of input samples with length >= numSamplesIn
 * @param output   buffer of output samples with lenth >= numSamplesIn / 2
 * @abstract Downsample by 2x. Works in place.
 */
void BMDownsampler2x_processBuffer(BMDownsampler2x* This, float* input, float* output, size_t numSamplesIn);


#endif /* BMDownsampler2x_h */
