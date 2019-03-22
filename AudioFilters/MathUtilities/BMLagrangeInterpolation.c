//
//  BMLagrangeInterpolation.c
//  BMAudioFilters
//
//  Created by Nguyen Minh Tien on 12/26/18.
//  Copyright © 2018 Hans. All rights reserved.
//

#include "BMLagrangeInterpolation.h"
#include <stdlib.h>
#include <Accelerate/Accelerate.h>

float calculateH(float fractionalDelay, float n,float order);
void BMLI_getStartIdx(BMLagrangeInterpolation* This,float strideIdx, int* startIdx,size_t inputLength);

void BMLagrangeInterpolation_init(BMLagrangeInterpolation* This, int order){
    assert(order%2==0);
    
    This->order = order;
    This->totalStepPerSample = order;
    This->deltaStep = 1.0/This->totalStepPerSample;
    This->deltaRange = This->totalStepPerSample * This->order * 0.5;
    This->table = malloc(sizeof(float*)*This->deltaRange);
    This->sampleRange = order+1;
    for(int i=0;i<This->deltaRange;i++){
        This->table[i] = malloc(sizeof(float)*This->sampleRange);
    }
    This->temp = malloc(sizeof(float)* This->sampleRange);
    
    //
    for(int i=0;i<This->deltaRange;i++){
        for(int j=0;j<This->sampleRange;j++){
            This->table[i][j] = calculateH(i * This->deltaStep, j,This->order);
//            printf("%f\n",This->table[i][j]);
        }
    }
    
}

void BMLagrangeInterpolation_processUpSample(BMLagrangeInterpolation* This, const float* input, const float* strideInput, float* output, int outputStride,size_t inputLength,size_t outputLength){
    int startIdx = 0;
    for(int i=0;i<outputLength;i++){
        //Get start index
        BMLI_getStartIdx(This, strideInput[i], &startIdx, inputLength);
        //Calculate the delta
        float delta = strideInput[i] - startIdx;
        //Get delta index in the table by the delta
        int deltaIdx = roundf(delta/This->deltaStep);
        //Multiple input to lagrange interpolation table
        vDSP_vmul(input+startIdx, 1, This->table[deltaIdx], 1, This->temp, 1, This->sampleRange);
        //Sum it
        vDSP_sve(This->temp, 1, output+i, This->sampleRange);
//        output[i] = 0;
//        for(int j=0;j<This->sampleRange;j++){
//            //Vdsp_vsmsa
//
//            output[i] += input[j+startIdx] * This->table[deltaIdx][j];
////            printf("%f %d\n",This->table[j][deltaIdx],startIdx);
//        }
    }
}

float BMLagrangeInterpolation_processOneSample(BMLagrangeInterpolation* This, const float* input, const float strideInput,size_t inputLength){
    float output;
    int startIdx = 0;
    //Get start index
    BMLI_getStartIdx(This, strideInput, &startIdx, inputLength);
    //Calculate the delta
    float delta = strideInput - startIdx;
    //Get delta index in the table by the delta
    int deltaIdx = floorf(delta/This->deltaStep);
    //Multiple input to lagrange interpolation table
    vDSP_vmul(input+startIdx, 1, This->table[deltaIdx], 1, This->temp, 1, This->sampleRange);
    //Sum it
    vDSP_sve(This->temp, 1, &output, This->sampleRange);
    return output;
}

void BMLagrangeInterpolation_processDownSample(BMLagrangeInterpolation* This, const float* input,int inputStride, float* output, int outputStride,size_t inputLength,size_t outputLength){
    //Skip sample = This->order
    for(int i=0;i<outputLength&&i*This->order<inputLength;i++){
        output[i] = input[i*This->order];
    }
}

void BMLI_getStartIdx(BMLagrangeInterpolation* This,float strideIdx, int* startIdx,size_t inputLength){
    //The input not contain enough data
    assert(strideIdx>=This->order*0.5);
    assert(strideIdx<=inputLength - This->order * 0.5);

    //Normal inputIdx
    *startIdx = floorf(strideIdx - (This->order* 0.5 - 1));

}
                  
/*
                N
 H(delta)(n) = Tich( (delta-k)/(n-k), n : 0 -> N )
                k=0
                k!=n
 */
float calculateH(float fractionalDelay, float n,float order){
    float result = 1;
    for(int k=0;k<=order;k++){
        if(k!=n){
            result *= (fractionalDelay-k)/(n-k);
        }
    }
    return result;
}
