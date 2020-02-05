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
#include "Constants.h"

float calculateH(float fractionalDelay, float n,float order);
static inline int BMLI_getStartIdx(float orderF,float strideIdx);

void BMLagrangeInterpolation_init(BMLagrangeInterpolation* This, int order){
    assert(order%2==0);
    
    This->order = order;
    This->totalStepPerSample = order*20;
    This->startIdxFactor = (order* 0.5f - 1.0f);
    This->deltaStep = 1.0/This->totalStepPerSample;
    This->deltaRange = This->totalStepPerSample * This->order * 0.5;
    
    This->sampleRange = order+1;
    This->startIdxBuffer = malloc(sizeof(int)*BM_BUFFER_CHUNK_SIZE);
    This->deltaX = malloc(sizeof(int)*BM_BUFFER_CHUNK_SIZE);
    This->floatBuffer1 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->floatBuffer2 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->sizeTBuffer1 = malloc(sizeof(size_t)*BM_BUFFER_CHUNK_SIZE);
    This->sizeTBuffer2 = malloc(sizeof(size_t)*BM_BUFFER_CHUNK_SIZE);
    
    This->table = malloc(sizeof(float*)*This->sampleRange);
    for(int i=0;i<This->sampleRange;i++){
        This->table[i] = malloc(sizeof(float)*This->deltaRange);
    }
    This->temp = malloc(sizeof(float)* This->sampleRange);
    
    //
    for(int i=0;i<This->deltaRange;i++){
        for(int j=0;j<This->sampleRange;j++){
            This->table[j][i] = calculateH(i * This->deltaStep, j,This->order);
//            printf("%f\n",This->table[i][j]);
        }
    }
    
}

void BMLagrangeInterpolation_destroy(BMLagrangeInterpolation* This){
    free(This->startIdxBuffer);
    free(This->deltaX);
    free(This->floatBuffer1);
    free(This->floatBuffer2);
    free(This->sizeTBuffer1);
    free(This->sizeTBuffer2);
    
    for(int i=0;i<This->sampleRange;i++){
        free(This->table[i]);
    }
    
    free(This->table);
    free(This->temp);
}

typedef unsigned vUint64_2 __attribute__((ext_vector_type(2),aligned(4)));
typedef signed vSint32_2 __attribute__((ext_vector_type(2),aligned(4)));

void vectorConvertIntToSize_t(const int* input, size_t* output, size_t length){
    for(size_t i=0; i<length; i++)
        output[i] = (size_t)input[i];
    
//    size_t n=length;
//    while(n>2){
//        *(vSint32_2*)output = __builtin_convertvector(*(vUint64_2*)input,vSint32_2);
//        output -= 2;
//        input -= 2;
//        n -= 2;
//    }
//    if(n>0)
//        *output = *input;
}


void BMLagrangeInterpolation_processUpSample(BMLagrangeInterpolation* This, const float* input, const float* strideInput, float* output, int outputStride,size_t inputLength,size_t outputLength){
    
    // get the start index for the input data used to calculate each sample of output
    float negStartIdxFactor = - This->startIdxFactor;
    float* startIdxBufferF = (float*) This->startIdxBuffer;
    vDSP_vsadd(strideInput, 1, &negStartIdxFactor, startIdxBufferF, 1, outputLength);
    int frameCount = (int)outputLength;
    vvfloorf(startIdxBufferF, startIdxBufferF,&frameCount);

    //Calculate the delta
    float* deltaXF = (float*) This->deltaX;
    vDSP_vsub(startIdxBufferF, 1, strideInput, 1, deltaXF, 1, outputLength);
    vDSP_vsmul(deltaXF, 1, &This->totalStepPerSample, deltaXF, 1, outputLength);
    
    vDSP_vfix32(deltaXF, 1, This->deltaX, 1, outputLength);
    vectorConvertIntToSize_t(This->deltaX, This->sizeTBuffer2, outputLength);

    //Convert startIdxBuffer float to int
    vDSP_vfix32(startIdxBufferF, 1, This->startIdxBuffer, 1, outputLength);

    // calculate the locations from which we need to read
    int offset = 1;
    vDSP_vsaddi(This->startIdxBuffer, 1, &offset, This->startIdxBuffer, 1, outputLength);
    vectorConvertIntToSize_t(This->startIdxBuffer, This->sizeTBuffer1, outputLength);

    for(int i=0; i<This->sampleRange; i++){

        // copy from the specified locations in the input buffer to a temp buffer
        vDSP_vgathr(input+i, This->sizeTBuffer1, 1, This->floatBuffer1, 1, outputLength);

        // get the ith interpolation coefficients and store into a buffer
        vDSP_vgathr(This->table[i],This->sizeTBuffer2,1,This->floatBuffer2, 1, outputLength);

        // multiply the coefficients by the input buffer data
        if (i==0) // first row of coefficients: write straight to output
           vDSP_vmul(This->floatBuffer1,1,This->floatBuffer2,1,output,1,outputLength);

        else // subsequent rows, sum to output
            vDSP_vma(This->floatBuffer1, 1, This->floatBuffer2, 1, output, 1, output, 1, outputLength);
    }
}

//float BMLagrangeInterpolation_processOneSample(BMLagrangeInterpolation* This, const float* input, const float strideInput,size_t inputLength){
//    float output;
//    int startIdx = 0;
//    //Get start index
//    startIdx = BMLI_getStartIdx(This->startIdxFactor, strideInput);
//    //Calculate the delta
//    float delta = strideInput - startIdx;
//    //Get delta index in the table by the delta
//    int deltaIdx = floorf(delta*This->totalStepPerSample);
//    //Multiple input to lagrange interpolation table
//    vDSP_vmul(input+startIdx, 1, This->table[deltaIdx], 1, This->temp, 1, This->sampleRange);
//    //Sum it
//    vDSP_sve(This->temp, 1, &output, This->sampleRange);
//    return output;
//}

//inline int BMLI_getStartIdx(float startIdxFactor,float strideIdx){
//    //The input not contain enough data
//    //    assert(strideIdx>=This->order*0.5);
//    //    assert(strideIdx<=inputLength - This->order * 0.5);
//    
//    //Normal inputIdx
//    return floorf(strideIdx - startIdxFactor);
//    
//}

void BMLagrangeInterpolation_processDownSample(BMLagrangeInterpolation* This, const float* input,int inputStride, float* output, int outputStride,size_t inputLength,size_t outputLength){
    //Skip sample = This->order
    for(int i=0;i<outputLength&&i*This->order<inputLength;i++){
        output[i] = input[i*This->order];
    }
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
