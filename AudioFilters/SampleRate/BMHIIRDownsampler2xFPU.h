//
//  BMHIIRDownsampler2xFPU.h
//  AudioFiltersXcodeProject
//
//  Based on Downsampler2xFpu.hpp from Laurent de Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMHIIRDownsampler2xFPU_h
#define BMHIIRDownsampler2xFPU_h

#include <stdio.h>

typedef struct BMHIIRDownsampler2xFPU {
    float *x, *y, *coef;
    size_t numCoefficients;
} BMHIIRDownsampler2xFPU;


/*!
 *BMHIIRDownsampler2xFPU_init
 *
 * @param stopbandAttenuationDb maximum allowable leakage of signal into the stopband
 * @param transitionBandwidth fraction of the frequency range from 0 to Pi/2 for which the AA filters are in transition
 * @return the stopband attenuation, in decibels, for the specified filter.
 */
float BMHIIRDownsampler2xFPU_init (BMHIIRDownsampler2xFPU* This, float stopbandAttenuationDb, float transitionBandwidth);

void BMHIIRDownsampler2xFPU_free (BMHIIRDownsampler2xFPU* This);

void BMHIIRDownsampler2xFPU_setCoefs (BMHIIRDownsampler2xFPU* This, const double* coef_arr);

/*
 * Doesn't work in place
 */
inline void BMHIIRDownsampler2xFPU_processSample(BMHIIRDownsampler2xFPU* This, float* input2, float* output);

/*
 * Doesn't work in place
 */
void BMHIIRDownsampler2xFPU_processBufferMono (BMHIIRDownsampler2xFPU* This, float* input, float* output, size_t numSamplesIn);

void BMHIIRDownsampler2xFPU_clearBuffers (BMHIIRDownsampler2xFPU* This);

#endif /* BMHIIRDownsampler2xFPU_h */
