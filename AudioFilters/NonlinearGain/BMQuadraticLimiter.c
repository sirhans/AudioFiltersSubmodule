//
//  BMQuadraticLimiter.c
//  SaturatorAU
//
//  Created by hans anderson on 8/26/19.
//  Anyone may use this file 
//

#include "BMQuadraticLimiter.h"

void BMQuadraticLimiter_init(BMQuadraticLimiter* This, float limit, float width){
    BMQuadraticThreshold_initLower(&This->lower, -limit, width);
    BMQuadraticThreshold_initUpper(&This->upper, limit, width);
}


void BMQuadraticLimiter_processBufferMono(BMQuadraticLimiter* This, const float* input, float* buffer, float* output, size_t numSamples){
    BMQuadraticThreshold_lowerBuffer(&This->lower, input, buffer, numSamples);
    BMQuadraticThreshold_upperBuffer(&This->upper, buffer, output, numSamples);
}


void BMQuadraticLimiter_processBufferStereo(BMQuadraticLimiter* This,
                                            const float* inputL, const float* inputR,
                                            float* buffer,
                                            float* outputL, float* outputR,
                                            size_t numSamples){
    // left channel
    BMQuadraticThreshold_lowerBuffer(&This->lower, inputL, buffer, numSamples);
    BMQuadraticThreshold_upperBuffer(&This->upper, buffer, outputL, numSamples);
    
    // right channel
    BMQuadraticThreshold_lowerBuffer(&This->lower, inputR, buffer, numSamples);
    BMQuadraticThreshold_upperBuffer(&This->upper, buffer, outputR, numSamples);
}
