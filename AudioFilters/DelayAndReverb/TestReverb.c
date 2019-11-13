//
//  TestReverb.c
//  BMAudioFilters
//
//  Created by Nguyen Minh Tien on 4/26/18.
//  Anyone may use this file without restrictions of any kind
//

#include "TestReverb.h"
#include "BMFastHadamard.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "BMFastHadamard.h"
#include "BMVelvetNoise.h"

float TestReverb_processSample(TestReverb *This,float x);

void TestReverb_init(TestReverb *This,size_t numOfDelay,float sampleRate){
    assert(BMPowerOfTwoQ(numOfDelay));
    assert(numOfDelay>=16);
    
    //Init delay line
    This->delayLines = malloc(sizeof(float*)*numOfDelay);
    This->readIdx = malloc(sizeof(size_t)*numOfDelay);
    This->writeIdx = malloc(sizeof(size_t)*numOfDelay);
    This->dlOut = malloc(sizeof(float)*numOfDelay);
    This->temp = malloc(sizeof(float)*numOfDelay);
    This->temp2 = malloc(sizeof(float)*numOfDelay);
    This->delayTimes = malloc(sizeof(size_t)*numOfDelay);
    This->numOfDelay = numOfDelay;
    //Randomize delaytime between min & max
    
    BMVelvetNoise_setTapIndices(Rev_DT_Min, Rev_DT_Max, This->delayTimes, sampleRate, numOfDelay);
//    This->delayTimes[0] = 6;
    //Init delay line
    for(size_t i = 0;i<numOfDelay;i++){
        printf("%zu\n",This->delayTimes[i]);
        This->delayLines[i] = malloc(sizeof(float)*This->delayTimes[i]);
        memset(This->delayLines[i],0,sizeof(float)*This->delayTimes[i]);
        
        This->readIdx[i] = 0;
        This->writeIdx[i] = This->delayTimes[i];
    }
}

inline float TestReverb_processSample(TestReverb *This,float x){
    //Get dlOut
    float xOut = x;
    float gain = 1./sqrt(This->numOfDelay);
    for(size_t i=0;i<This->numOfDelay;i++){
        This->dlOut[i] = This->delayLines[i][This->readIdx[i]] * gain;
        xOut += This->dlOut[i];
    }
    //Apply BMFastHadamard
    BMFastHadamardTransform(This->dlOut, This->dlOut, This->temp, This->temp2, This->numOfDelay);
    
    //Save dlIn
    for(size_t i=0;i<This->numOfDelay;i++){
        This->delayLines[i][This->writeIdx[i]] = x + This->dlOut[i];
    }
    
    //Process read/write idx
    for (size_t i=0; i<This->numOfDelay; i++) {
        This->writeIdx[i] ++;
        if(This->writeIdx[i]>This->delayTimes[i])
            This->writeIdx[i] = 0;
        This->readIdx[i] ++;
        if(This->readIdx[i]>This->delayTimes[i])
            This->readIdx[i] = 0;
    }
    
    return xOut;
}

void TestReverb_process(TestReverb *This,float* input,float* output, size_t numSamples){
    for (size_t i = 0; i<numSamples; i++) {
        output[i] = TestReverb_processSample(This, input[i]);
    }
}

void TestReverb_impulseResponse(TestReverb *This,size_t frameCount){
    float* irBuffer = malloc(sizeof(float)*frameCount);
    float* outBuffer = malloc(sizeof(float)*frameCount);
    for(int i=0;i<frameCount;i++){
        if(i==0)
            irBuffer[i] = 1;
        else
            irBuffer[i] = 0;
    }
    
    TestReverb_process(This, irBuffer, outBuffer, frameCount);
    
    printf("\n\\impulse response: {\n");
    for(size_t i=0; i<(frameCount-1); i++)
        printf("%f\n", outBuffer[i]);
    printf("%f}\n\n",outBuffer[frameCount-1]);
    
}


