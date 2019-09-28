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
    This->z1 = 0.0f;
    
    // set vector versions for stereo and four channel processing
    This->z1_2 = This->z1;
    This->z1_4 = This->z1;
    
    BMZavalishinLPFOrder1_setCutoff(This, fc);
}



void BMZavalishinLPFOrder1_setCutoff(BMZavalishinLPFOrder1* This, float fc){
    float g = fc / (2.0f * This->sampleRate); // zavalishin 3.11
    This->G = g / (1.0f + g);
    
    // set vector versions for stereo and four channel processing
    This->G_2 = This->G;
    This->G_4 = This->G;
}



float BMZavalishinLPFOrder1_processSample(BMZavalishinLPFOrder1* This, float x){
    float v = (x - This->z1) * This->G;
    float y = v + This->z1;
    This->z1 = y + v;
    
    return y;
}



void BMZavalishinLPFOrder1_processBufferMono(BMZavalishinLPFOrder1* This, const float* input, float* output, size_t numSamples){
    
    for(size_t i=0; i<numSamples; i++){
        float v = (input[i] - This->z1) * This->G;
        float y = v + This->z1;
        This->z1 = y + v;
        output[i] = y;
    }
}



void BMZavalishinLPFOrder1_processBufferStereo(BMZavalishinLPFOrder1* This,
                                               const float* inputL, const float* inputR,
                                               float* outputL, float* outputR,
                                               size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
        simd_float2 in = {inputL[i], inputR[i]};
        
        simd_float2 v = (in - This->z1_2) * This->G_2;
        simd_float2 y = v + This->z1_2;
        This->z1_2 = y + v;
        
        outputL[i] = y[0];
        outputR[i] = y[1];
    }
}


void BMZavalishinLPFOrder1_processBuffer4(BMZavalishinLPFOrder1* This,
                                          const float* input1, const float* input2, const float* input3, const float* input4,
                                          float* output1, float* output2, float* output3, float* output4,
                                          size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
        // group inputs into a vector
        simd_float4 in = {input1[i], input2[i], input3[i], input4[i]};
        
        simd_float4 v = (in - This->z1_4) * This->G_4;
        simd_float4 y = v + This->z1_4;
        This->z1_4 = y + v;
        
        // split outputs to four channels
        output1[i] = y.x;
        output2[i] = y.y;
        output3[i] = y.z;
        output4[i] = y.w;
    }
}
