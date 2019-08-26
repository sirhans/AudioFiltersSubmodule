//
//  BMQuadraticLimiter.h
//  SaturatorAU
//
//  Created by hans anderson on 8/26/19.
//  Anyone may use this file 
//

#ifndef BMQuadraticLimiter_h
#define BMQuadraticLimiter_h

#include <stdio.h>
#include "BMQuadraticThreshold.h"

typedef struct BMQuadraticLimiter {
    BMQuadraticThreshold upper, lower;
} BMQuadraticLimiter;

void BMQuadraticLimiter_init(BMQuadraticLimiter* This, float limit, float width);

void BMQuadraticLimiter_processBufferMono(BMQuadraticLimiter* This,
                                          const float* input,
                                          float* buffer,
                                          float* output,
                                          size_t numSamples);


void BMQuadraticLimiter_processBufferStereo(BMQuadraticLimiter* This,
                                            const float* inputL, const float* inputR,
                                            float* buffer,
                                            float* outputL, float* outputR,
                                            size_t numSamples);

#endif /* BMQuadraticLimiter_h */
