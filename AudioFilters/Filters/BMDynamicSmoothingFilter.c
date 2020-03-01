//
//  BMDynamicSmoothingFilter.c
//  AudioFiltersXCodeProject
//
//  Created by hans anderson on 3/19/19.
//
//  The author places no restrictions on the use of this file.
//
//  See https://cytomic.com/files/dsp/DynamicSmoothing.pdf for the source of
//  of the ideas implemented here.
//
//  In summary: when operated as a lowpass filter, the value in the first
//  integrator of a second order state variable filter indicates the gradient
//  of the incoming signal. Using the absolute value of this gradient to control
//  the cutoff frequency of the filter, we get a lowpass filter that operates
//  on audio frequency signals safely with cutoff frequencies below 1 Hz and
//  is capable moving its cutoff frequency up near the Nyquist frequency when
//  there is a large spike in the input value. This way we can smooth out small
//  changes and still react immediately to large changes.

#include "BMDynamicSmoothingFilter.h"
#include <Accelerate/Accelerate.h>
#include "Constants.h"

#define BM_DSF_SENSITIVITY 0.125f
#define BM_DSF_MIN_FC 1.0f

// this softens the way the filter changes its own cutoff frequency
static inline float DSControlCurve(float x){
    // Mathematica: DSControlCurve[x_]:=1-(x-1)^2
    float xMinus1 = x - 1.0f;
    return 1.0f - xMinus1 * xMinus1;
}


void BMDynamicSmoothingFilter_initDefault(BMDynamicSmoothingFilter *This,
                                          float sampleRate){
    BMDynamicSmoothingFilter_init(This,
                                  BM_DSF_SENSITIVITY,
                                  BM_DSF_MIN_FC,
                                  sampleRate);
}


void BMDynamicSmoothingFilter_init(BMDynamicSmoothingFilter *This,
                                    float sensitivity,
                                    float minFc,
                                    float sampleRate){
    This->sensitivity = sensitivity;
    This->g0 = tanf(M_PI * minFc / sampleRate);
	This->low1z = 0.0f;
	This->low2z = 0.0f;
}


void BMDynamicSmoothingFilter_processBuffer(BMDynamicSmoothingFilter *This,
                                           const float* input,
                                           float* output,
                                           size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
        float bandz = This->low1z - This->low2z;
        float g = This->g0 + This->sensitivity * fabs(bandz);
        g = MIN(g, 1.0f);
        This->low2z += g * (bandz);
        This->low1z += g * (input[i] - This->low1z);
        output[i] = This->low2z;
    }
}


float BMDSF_positiveOnly(float x){
	return 0.5 * (x + fabsf(x));
}

float BMDSF_negativeOnly(float x){
	return 0.5 * (x - fabsf(x));
}


void BMDynamicSmoothingFilter_processBufferWithFastDescent(BMDynamicSmoothingFilter *This,
                                           const float* input,
                                           float* output,
                                           size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
        float bandz = This->low2z - This->low1z;
        float g = This->g0 + This->sensitivity * BMDSF_positiveOnly(bandz) - 0.001 * This->low2z;
        g = MIN(g, 1.0f);
        This->low2z += g * (-bandz);
        This->low1z += g * (input[i] - This->low1z);
        output[i] = This->low2z;
    }
}
