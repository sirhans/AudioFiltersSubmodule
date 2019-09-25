//
//  BMQuadraticRectifier.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/2/19.
//  Anyone may use this file without restrictions
//

#include "BMQuadraticRectifier.h"
#include <simd/simd.h>



void BMQuadraticRectifier_init(BMQuadraticRectifier* This, float kneeWidth){
    BMQuadraticThreshold_initLower(&This->qtPos, 0, kneeWidth);
    BMQuadraticThreshold_initUpper(&This->qtNeg, 0, kneeWidth);
}



void BMQuadraticRectifier_processBuffer(BMQuadraticRectifier* This,
                                        const float* input,
                                        float* outputPos, float* outputNeg,
                                        size_t numSamples){
    BMQuadraticThreshold_lowerBuffer(&This->qtPos, input, outputPos, numSamples);
    BMQuadraticThreshold_upperBuffer(&This->qtNeg, input, outputNeg, numSamples);
}




void BMQuadraticRectifier_processBufferStereo(BMQuadraticRectifier* This,
                                              const float* inputL, const float* inputR,
                                              float* outputPosL, float* outputNegL,
                                              float* outputPosR, float* outputNegR,
                                              size_t numSamples){

    for(size_t i=0; i<numSamples; i++){
        simd_float2 in = {inputL[i],inputR[i]};

        // clamp the input to the curved region for the upper and lower thresholds
        //
        // for the rectifier, both sides clamp at the same boundaries so we can
        // get away with a single clamp function
        simd_float2 inClamped = simd_clamp(in, This->qtNeg.lMinusW, This->qtNeg.lPlusW);

        // evaluate the second order polynomials for the curves
        simd_float2 inPos = inClamped * (inClamped * This->qtPos.coefficients[0] + This->qtPos.coefficients[1]) + This->qtPos.coefficients[2];
        // we are lucky that this next line happens to work
        simd_float2 inNeg = inClamped - inPos;

        // for the upper threshold,
        // where the input is less than the polynomial output, return the input
        // *** This works because the input was clamped before curving ***
        simd_float2 outputNeg = simd_min(in, inNeg);
        outputNegL[i] = outputNeg.x;
        outputNegR[i] = outputNeg.y;

        // for the lower threshold,
        // where the input is greater than the polynomical output, return the input
        // *** This works because the input was clamped before curving ***
        simd_float2 outputPos = simd_max(in, inPos);
        outputPosL[i] = outputPos.x;
        outputPosR[i] = outputPos.y;
    }
}
