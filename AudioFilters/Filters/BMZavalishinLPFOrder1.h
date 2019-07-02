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

typedef struct BMZavalishinLPFOrder1 {
    float G,z1L,z1R,sampleRate;
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

#endif /* BMZavalishinLPFOrder1_h */
