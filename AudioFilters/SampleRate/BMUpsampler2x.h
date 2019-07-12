//
//  BMUpsampler2x.h
//  AudioFiltersXcodeProject
//
//  Based on Upsampler2x4Neon.h from Laurent de Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMUpsampler2x_h
#define BMUpsampler2x_h

#include <simd/simd.h>
#include <mactypes.h>
#include "BMHIIRStage.h"

typedef struct BMUpsampler2x {
    size_t numCoefficients, numStages;
    BMHIIRStage* filterStages;
} BMUpsampler2x;

void BMUpsampler2x_init(BMUpsampler2x* This, size_t numCoefficients);
void BMUpsampler2x_setCoefficients(BMUpsampler2x* This, const double* coefficientArray);
void BMUpsampler2x_processBuffer(BMUpsampler2x* This, const float* input, float* output, size_t numSamples);
void BMUpsampler2x_clearBuffers(BMUpsampler2x* This);


#endif /* BMUpsampler2x_h */
