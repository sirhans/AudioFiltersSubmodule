//
//  BMQuadraticLimiter.c
//  SaturatorAU
//
//  Created by hans anderson on 8/26/19.
//  Anyone may use this file 
//

#include "BMQuadraticLimiter.h"
#include <simd/simd.h>

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
                                            float* outputL, float* outputR,
                                            size_t numSamples){
//    // left channel
//    BMQuadraticThreshold_lowerBuffer(&This->lower, inputL, buffer, numSamples);
//    BMQuadraticThreshold_upperBuffer(&This->upper, buffer, outputL, numSamples);
//
//    // right channel
//    BMQuadraticThreshold_lowerBuffer(&This->lower, inputR, buffer, numSamples);
//    BMQuadraticThreshold_upperBuffer(&This->upper, buffer, outputR, numSamples);
    
    for(size_t i=0; i<numSamples; i++){
        
        // copy the input into a vector2
        simd_float2 in = {inputL[i],inputR[i]};
        
        /***************************
         * PROCESS THE LOWER LIMIT *
         ***************************/
        
        // clamp the input to the curved region for the upper and lower thresholds
        simd_float2 lowerLimited = simd_clamp(in, This->lower.lMinusW, This->lower.lPlusW);
        
        // evaluate the second order polynomials for the curves
        lowerLimited = lowerLimited * (lowerLimited * This->lower.coefficients[2] + This->lower.coefficients[1]) + This->lower.coefficients[0];
        
        // for the upper threshold,
        // where the input is greater than the polynomial output, return the input
        // *** This works because the input was clamped ***
        lowerLimited = simd_min(in,lowerLimited);
        
        
        /***************************
         * PROCESS THE UPPER LIMIT *
         ***************************/
        
        // clamp the input to the curved region for the upper and lower thresholds
        simd_float2 upperLimited = simd_clamp(lowerLimited, This->upper.lMinusW, This->upper.lPlusW);
        
        // evaluate the second order polynomials for the curves
        upperLimited = upperLimited * (upperLimited * This->upper.coefficients[2] + This->upper.coefficients[1]) + This->upper.coefficients[0];
        
        // for the upper threshold,
        // where the lowerLimited signal is less than the polynomial output,
        // return the lowerLimited signal
        // *** This works because the signal was clamped ***
        upperLimited = simd_max(lowerLimited,upperLimited);
        
        /*
         * Copy output
         */
        outputL[i] = upperLimited.x;
        outputR[i] = upperLimited.y;
    }
}
