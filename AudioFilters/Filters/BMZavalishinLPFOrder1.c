//
//  BMZavalishinLPFOrder1.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/2/19.
//  Anyone may use this file without restrictions
//

#include "BMZavalishinLPFOrder1.h"
#include <simd/simd.h>


void BMZavalishinLPFOrder1_init(BMZavalishinLPFOrder1* This, float fc, float sampleRate){
    This->sampleRate = sampleRate;
    This->z1L = This->z1R = 0.0f;
    BMZavalishinLPFOrder1_setCutoff(This, fc);
}



void BMZavalishinLPFOrder1_setCutoff(BMZavalishinLPFOrder1* This, float fc){
    float g = fc / (2.0f * This->sampleRate); // zavalishin 3.11
    This->G = g / (1.0f + g);
}



float BMZavalishinLPFOrder1_processSample(BMZavalishinLPFOrder1* This, float x){
    float v = (x - This->z1L) * This->G;
    float y = v + This->z1L;
    This->z1L = y + v;
    
    return y;
}



void BMZavalishinLPFOrder1_processBufferMono(BMZavalishinLPFOrder1* This, const float* input, float* output, size_t numSamples){
    
    for(size_t i=0; i<numSamples; i++){
        float v = (input[i] - This->z1L) * This->G;
        float y = v + This->z1L;
        This->z1L = y + v;
        output[i] = y;
    }
}



void BMZavalishinLPFOrder1_processBufferStereo(BMZavalishinLPFOrder1* This,
                                               const float* inputL, const float* inputR,
                                               float* outputL, float* outputR,
                                               size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
        float vL = (inputL[i] - This->z1L) * This->G;
        float vR = (inputR[i] - This->z1R) * This->G;
        float yL = vL + This->z1L;
        float yR = vR + This->z1R;
        This->z1L = yL + vL;
        This->z1R = yR + vR;
        outputL[i] = yL;
        outputR[i] = yR;
    }
}
