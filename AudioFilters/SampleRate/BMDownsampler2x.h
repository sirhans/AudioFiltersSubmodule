//
//  BMDownsampler2x.h
//  AudioFiltersXcodeProject
//
//  Based on Downsampler2xFpu.h from Laurent de Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMDownsampler2x_h
#define BMDownsampler2x_h

#include <stdio.h>
#include "BMMultilevelBiquad.h"

typedef struct BMDownsampler2x {
    BMMultiLevelBiquad* even;
    BMMultiLevelBiquad* odd;
    size_t numCoefficients,numBiquadStages;
} BMDownsampler2x;


/*!
 *BMDownsampler2x_init
 *
 * @param stopbandAttenuationDb maximum allowable leakage of signal into the stopband
 * @param transitionBandwidth fraction of the frequency range from 0 to Pi/2 for which the AA filters are in transition
 * @return the stopband attenuation, in decibels, for the specified filter.
 */
float BMDownsampler2x_init (BMDownsampler2x* This, float stopbandAttenuationDb, float transitionBandwidth);

void BMDownsampler2x_free (BMDownsampler2x* This);

void BMDownsampler2x_setCoefs (BMDownsampler2x* This, const double* coef_arr);

void BMDownsampler2x_processBufferMono (BMDownsampler2x* This, float* input, float* output, size_t numSamplesIn);

void BMDownsampler2x_clearBuffers (BMDownsampler2x* This);

#endif /* BMDownsampler2x_h */
