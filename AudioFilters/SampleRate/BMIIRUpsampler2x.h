//
//  BMIIRUpsampler2x.h
//  AudioFiltersXcodeProject
//
//  Based on Upsampler2xFpu.h from Laurent de Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMIIRUpsampler2x_h
#define BMIIRUpsampler2x_h

#include <stdio.h>
#include "BMMultiLevelBiquad.h"

typedef struct BMIIRUpsampler2x {
    BMMultiLevelBiquad even;
    BMMultiLevelBiquad odd;
    float *b1L, *b2L, *b1R, *b2R;
    size_t numCoefficients,numBiquadStages;
    bool stereo;
} BMIIRUpsampler2x;


/*!
 *BMIIRUpsampler2x_init
 *
 * @param This                      pointer to an uninitialised BMIIRUpsampler2x struct
 * @param stopbandAttenuationDb     the AA filters will acheive at least this much stopband attenuation. specified in dB as a positive number.
 * @param transitionBandwidth       the AA filter transition bandwidth, in (0,0.5).
 * @param stereo                    Set this to true to process audio in stereo; false for mono
 */
size_t BMIIRUpsampler2x_init (BMIIRUpsampler2x* This,
                              float stopbandAttenuationDb,
                              float transitionBandwidth,
                              bool stereo);

void BMIIRUpsampler2x_free (BMIIRUpsampler2x* This);

void BMIIRUpsampler2x_setCoefs (BMIIRUpsampler2x* This, const double* coef_arr);

void BMIIRUpsampler2x_processBufferMono (BMIIRUpsampler2x* This, const float* input, float* output, size_t numSamplesIn);

#endif /* BMIIRUpsampler2x_h */
