//
//  BMSimpleFDN.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 9/14/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifndef BMSimpleFDN_h
#define BMSimpleFDN_h

#include <stdio.h>

enum delayTimeMethod {DTM_VELVETNOISE, DTM_RANDOM, DTM_RELATIVEPRIME};

typedef struct BMSimpleFDN {
    float** delays;
    size_t *delayLengths, *rwIndices;
    float *attenuationCoefficients, *buffer1, *buffer2, *buffer3, *outputTapSigns;
    size_t numDelays;
    float sampleRate, RT60DecayTime, maxDelayS, minDelayS;
} BMSimpleFDN;


void BMSimpleFDN_init(BMSimpleFDN* This,
                      float sampleRate,
                      size_t numDelays,
                      enum delayTimeMethod method,
                      float minDelayTimeSeconds,
                      float maxDelayTimeSeconds,
                      float RT60DecayTimeSeconds);


void BMSimpleFDN_processBuffer(BMSimpleFDN* This,
                               const float* input,
                               float* output,
                               size_t numSamples);


void BMSimpleFDN_impulseResponse(BMSimpleFDN* This, float* IR, size_t numSamples);


void BMSimpleFDN_free(BMSimpleFDN* This);

#endif /* BMSimpleFDN_h */
