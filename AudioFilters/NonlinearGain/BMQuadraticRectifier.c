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


void BMQuadraticRectifier_processBufferStereoVDSP(BMQuadraticRectifier* This,
                                              const float* inputL, const float* inputR,
                                              float* outputPosL, float* outputNegL,
                                              float* outputPosR, float* outputNegR,
                                              size_t numSamples){

//    for(size_t i=0; i<numSamples; i++){
//        simd_float2 in = {inputL[i],inputR[i]};
//
//        // clamp the input to the curved region for the upper and lower thresholds
//        //
//        // for the rectifier, both sides clamp at the same boundaries so we can
//        // get away with a single clamp function
//        simd_float2 inClamped = simd_clamp(in, This->qtNeg.lMinusW, This->qtNeg.lPlusW);
    float *clippedL = outputNegL;
    float *clippedR = outputNegR;
    vDSP_vclip(inputL, 1, &This->qtNeg.lMinusW, &This->qtNeg.lPlusW, clippedL, 1, numSamples);
    vDSP_vclip(inputR, 1, &This->qtNeg.lMinusW, &This->qtNeg.lPlusW, clippedR, 1, numSamples);
    
//
//        // evaluate the second order polynomials for the curves
//        simd_float2 inPos = inClamped * (inClamped * This->qtPos.coefficients[0] + This->qtPos.coefficients[1]) + This->qtPos.coefficients[2];
    float *curvedLPos = outputPosL;
    float *curvedRPos = outputPosR;
    vDSP_vsmsa(clippedL, 1, &This->qtPos.coefficients[0], &This->qtPos.coefficients[1], curvedLPos, 1, numSamples);
    vDSP_vsmsa(clippedR, 1, &This->qtPos.coefficients[0], &This->qtPos.coefficients[1], curvedRPos, 1, numSamples);
    vDSP_vmsa(clippedL, 1, curvedLPos, 1, &This->qtPos.coefficients[2], curvedLPos, 1, numSamples);
    vDSP_vmsa(clippedR, 1, curvedRPos, 1, &This->qtPos.coefficients[2], curvedRPos, 1, numSamples);
    
//        // we are lucky that this next line happens to work
//        simd_float2 inNeg = inClamped - inPos;
    float* curvedLNeg = outputNegL;
    float* curvedRNeg = outputNegR;
    vDSP_vsub(curvedLPos, 1, clippedL, 1, curvedLNeg, 1, numSamples);
    vDSP_vsub(curvedRPos, 1, clippedR, 1, curvedRNeg, 1, numSamples);
    
//
//        // for the upper threshold,
//        // where the input is less than the polynomial output, return the input
//        // *** This works because the input was clamped before curving ***
//        simd_float2 outputNeg = simd_min(in, inNeg);
//        outputNegL[i] = outputNeg.x;
//        outputNegR[i] = outputNeg.y;
    vDSP_vmin(inputL,1,curvedLNeg,1,outputNegL,1,numSamples);
    vDSP_vmin(inputR,1,curvedRNeg,1,outputNegR,1,numSamples);
//
//        // for the lower threshold,
//        // where the input is greater than the polynomical output, return the input
//        // *** This works because the input was clamped before curving ***
//        simd_float2 outputPos = simd_max(in, inPos);
//        outputPosL[i] = outputPos.x;
//        outputPosR[i] = outputPos.y;
    vDSP_vmax(inputL,1,curvedLPos,1,outputPosL,1,numSamples);
    vDSP_vmax(inputR,1,curvedRPos,1,outputPosR,1,numSamples);
//    }
}
