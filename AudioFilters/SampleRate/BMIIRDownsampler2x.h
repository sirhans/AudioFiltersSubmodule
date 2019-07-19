//
//  BMIIRDownsampler2x.h
//  AudioFiltersXcodeProject
//
//  Based on Downsampler2xFpu.h from Laurent de Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMIIRDownsampler2x_h
#define BMIIRDownsampler2x_h

#include <stdio.h>
#include "BMMultilevelBiquad.h"

typedef struct BMIIRDownsampler2x {
    BMMultiLevelBiquad even;
    BMMultiLevelBiquad odd;
    float *b1L, *b2L, *b1R, *b2R;
    size_t numCoefficients,numBiquadStages;
    bool stereo;
} BMIIRDownsampler2x;


/*!
 *BMIIRDownsampler2x_init
 *
 * @param This                         pointer to an uninitialised BMIIRDownsampler2x struct
 * @param minStopbandAttenuationDb     the AA filters will acheive at least this much stopband attenuation. specified in dB as a positive number.
 * @param maxTransitionBandwidth       the AA filters will not let the transition bandwidth exceed this value. In (0,0.5).
 * @param maximiseStopbandAttenuation  When this is set true, the transition bandwidth will be equal to the specified maximum value but the stopband attenuation will exceed the specified minimum value. When this is set false, the stopband attenuation will be equal to the specified minimum value but the transition bandwidth will be less than the specified maximum value.
 * @param stereo                       Set this to true to process audio in stereo; false for mono
 */
size_t BMIIRDownsampler2x_init (BMIIRDownsampler2x* This,
                              float minStopbandAttenuationDb,
                              float maxTransitionBandwidth,
                              bool maximiseStopbandAttenuation,
                              bool stereo);

void BMIIRDownsampler2x_free (BMIIRDownsampler2x* This);

void BMIIRDownsampler2x_setCoefs (BMIIRDownsampler2x* This, const double* coef_arr);

void BMIIRDownsampler2x_processBufferMono (BMIIRDownsampler2x* This, float* input, float* output, size_t numSamplesIn);


#endif /* BMIIRDownsampler2x_h */
