//
//  BMQuadraticRectifier.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/2/19.
//  Anyone may use this file without restrictions
//

#include "BMQuadraticRectifier.h"



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
