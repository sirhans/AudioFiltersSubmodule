//
//  BMHIIRUpsampler2xFPU.h
//  AudioFiltersXcodeProject
//
//  Based on Upsampler2xFpu.h from Laurent de Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMHIIRUpsampler2xFPU_h
#define BMHIIRUpsampler2xFPU_h

#include <stdio.h>

typedef struct BMHIIRUpsampler2xFPU {
    float *x, *y, *coef;
    size_t numCoefficients;
} BMHIIRUpsampler2xFPU;


float BMHIIRUpsampler2xFPU_init (BMHIIRUpsampler2xFPU* This, size_t numCoefficients, float transitionBandwidth);
void BMHIIRUpsampler2xFPU_free (BMHIIRUpsampler2xFPU* This);
void BMHIIRUpsampler2xFPU_setCoefs (BMHIIRUpsampler2xFPU* This, const double* coef_arr);
inline void BMHIIRUpsampler2xFPU_processSample(BMHIIRUpsampler2xFPU* This, float* out_0, float* out_1, float input);
void BMHIIRUpsampler2xFPU_processBufferMono(BMHIIRUpsampler2xFPU* This, const float* in_ptr, float* out_ptr, size_t nbr_spl);
void BMHIIRUpsampler2xFPU_clearBuffers (BMHIIRUpsampler2xFPU* This);

#endif /* BMHIIRUpsampler2xFPU_h */
