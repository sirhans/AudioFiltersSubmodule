//
//  BMZavalishinLPFOrder1.h
//  AudioFiltersXcodeProject
//
//  First order lowpass filter from 3.18 of
//  The Art of Virtual Analog Filter Design by V. Zavalishin
//
//  Created by hans anderson on 7/2/19.
//  Anyone may use this file without restrictions
//

#ifndef BMZavalishinLPFOrder1_h
#define BMZavalishinLPFOrder1_h

#include <stdio.h>
#include <simd/simd.h>

typedef struct BMZavalishinLPFOrder1 {
    float G,z1,sampleRate;
    simd_float2 G_2, z1_2;
    simd_float4 G_4, z1_4;
} BMZavalishinLPFOrder1;

void BMZavalishinLPFOrder1_init(BMZavalishinLPFOrder1* This, float fc, float sampleRate);


void BMZavalishinLPFOrder1_setCutoff(BMZavalishinLPFOrder1* This, float fc);


static inline float BMZavalishinLPFOrder1_processSampleMono(BMZavalishinLPFOrder1* This, float x);


void BMZavalishinLPFOrder1_processBufferMono(BMZavalishinLPFOrder1* This,
                                         const float* input,
                                         float* output,
                                         size_t numSamples);


void BMZavalishinLPFOrder1_processBufferStereo(BMZavalishinLPFOrder1* This,
                                             const float* inputL, const float* inputR,
                                             float* outputL, float* outputR,
                                             size_t numSamples);

void BMZavalishinLPFOrder1_processBuffer4(BMZavalishinLPFOrder1* This,
                                          const float* input1, const float* input2, const float* input3, const float* input4,
                                          float* output1, float* output2, float* output3, float* output4,
                                          size_t numSamples);

#endif /* BMZavalishinLPFOrder1_h */
