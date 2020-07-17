//
//  BMStereoLagTime.c
//  BMAUStereoLagTime
//
//  Created by Nguyen Minh Tien on 3/5/19.
//  Copyright © 2019 Nguyen Minh Tien. All rights reserved.
//

#include "BMStereoLagTime.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <Accelerate/Accelerate.h>
#include "Constants.h"

void BMStereoLagTime_init(BMStereoLagTime* This,size_t maxDelaySamples,float duration,size_t sampleRate){
    //init buffer
    This->sampleRate = sampleRate;
    This->maxDelayRange = (uint32_t)maxDelaySamples;
    
    BMSmoothDelay_init(&This->delayLeft, 0, duration, maxDelaySamples, sampleRate);
    BMSmoothDelay_init(&This->delayRight, 0, duration, maxDelaySamples, sampleRate);
    
    //Smooth delay
    This->delayParamL.sampleToReachTarget = duration*sampleRate;
    This->delayParamR.sampleToReachTarget = duration*sampleRate;
    float speed = 1 - maxDelaySamples/This->delayParamL.sampleToReachTarget;
    BMSmoothDelay_init(&This->delayLeft, This->delayParamL.startSamples, speed, maxDelaySamples, sampleRate);
    BMSmoothDelay_init(&This->delayRight, This->delayParamR.startSamples, speed, maxDelaySamples, sampleRate);
    This->strideBufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->strideBufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->strideBufferUnion = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    float incre = 1;
    float start = 0;
    vDSP_vramp(&start, &incre, This->strideBufferUnion, 1, BM_BUFFER_CHUNK_SIZE);
    
}

void BMStereoLagTime_destroy(BMStereoLagTime* This){
    BMSmoothDelay_destroy(&This->delayLeft);
    BMSmoothDelay_destroy(&This->delayRight);

    free(This->strideBufferL);
    This->strideBufferL = nil;
    free(This->strideBufferR);
    This->strideBufferR = nil;
}

void BMStereoLagTime_setDelayLeft(BMStereoLagTime* This,size_t delaySample,bool now){
    This->delayParamL.desiredDS = delaySample;
    This->delayParamR.desiredDS = 0;
    if(now){
        BMSmoothDelay_resetToDelaySample(&This->delayLeft, delaySample);
        BMSmoothDelay_resetToDelaySample(&This->delayRight, 0);
        This->delayLeft.sampleToReachTarget = 0;
        This->delayLeft.shouldUpdateDS = false;
        This->delayRight.sampleToReachTarget = 0;
        This->delayRight.shouldUpdateDS = false;
    }
}

void BMStereoLagTime_setDelayRight(BMStereoLagTime* This,size_t delaySample,bool now){
    This->delayParamR.desiredDS = delaySample;
    This->delayParamL.desiredDS = 0;
    if(now){
        BMSmoothDelay_resetToDelaySample(&This->delayLeft, 0);
        BMSmoothDelay_resetToDelaySample(&This->delayRight, delaySample);
        This->delayLeft.sampleToReachTarget = 0;
        This->delayLeft.shouldUpdateDS = false;
        This->delayRight.sampleToReachTarget = 0;
        This->delayRight.shouldUpdateDS = false;
    }
}


void BMStereoLagTime_prepareStrideBufferL(BMStereoLagTime* This,size_t samplesProcessed,size_t samplesProcessing){
    assert(This->delayParamL.sampleToReachTarget!=0);
    
    float dr = (This->delayParamL.stopSamples-This->delayParamL.startSamples);
    Float64 speedL = 1 - dr/This->delayParamL.sampleToReachTarget;
    float startIdxL = (This->delayParamL.currentSample+1)*speedL - This->delayParamL.lastSTC;
    float rampSpeed = speedL;
    vDSP_vramp(&startIdxL, &rampSpeed, This->strideBufferL, 1, samplesProcessing);
    //Get the decimal of realIdx & force to equal the last idx
    double currentStrideIdxL = (This->delayParamL.currentSample+samplesProcessing)*speedL;
    This->strideBufferL[samplesProcessing-1] = currentStrideIdxL - This->delayParamL.lastSTC;
    
    float currentSTC = floorf(currentStrideIdxL);
    if(This->delayParamL.currentSample+samplesProcessing==This->delayParamL.sampleToReachTarget)
        currentSTC = roundf(currentStrideIdxL);
    This->delayParamL.sampleToConsume = currentSTC - This->delayParamL.lastSTC;
    This->delayParamL.lastSTC = currentSTC;
}

void BMStereoLagTime_prepareStrideBufferR(BMStereoLagTime* This,size_t samplesProcessed,size_t samplesProcessing){
    assert(This->delayParamR.sampleToReachTarget!=0);

    //Right
    float dr = (This->delayParamR.stopSamples-This->delayParamR.startSamples);
    Float64 speedR = 1 - dr/This->delayParamR.sampleToReachTarget;
    float startIdxR = (This->delayParamR.currentSample+1)*speedR - This->delayParamR.lastSTC;
    float rampSpeed = speedR;
    vDSP_vramp(&startIdxR, &rampSpeed, This->strideBufferR, 1, samplesProcessing);
    //Get the decimal of realIdx & force to equal the last idx
    double currentStrideIdxR = (This->delayParamR.currentSample+samplesProcessing)*speedR;
    //Correct decimal
    This->strideBufferR[samplesProcessing-1] = currentStrideIdxR - This->delayParamR.lastSTC;

    float currentSTC = floorf(currentStrideIdxR);
    if(This->delayParamR.currentSample+samplesProcessing==This->delayParamR.sampleToReachTarget)
        currentSTC = roundf(currentStrideIdxR);
    This->delayParamR.sampleToConsume = currentSTC - This->delayParamR.lastSTC;
    This->delayParamR.lastSTC = currentSTC;
//    printf("t %f %f %f %f\n",startIdxL,startIdxR,This->strideBufferL[samplesProcessing-1],This->strideBufferL[samplesProcessing-1]);
}

void BMStereoLagTime_process(BMStereoLagTime* This,float* inL, float* inR, float* outL, float* outR,size_t numSamples){
    size_t samplesProcessing;
    size_t samplesProcessed = 0;
    while (samplesProcessed<numSamples) {
        samplesProcessing = BM_MIN(numSamples- samplesProcessed,BM_BUFFER_CHUNK_SIZE);
        //left
        if(This->delayParamL.currentSample==This->delayParamL.sampleToReachTarget&&
           This->delayParamL.stopSamples==This->delayParamL.desiredDS){
            //Stop state - make copy
            float incre = 1;
            float start = 0;
            vDSP_vramp(&start, &incre, This->strideBufferUnion, 1, samplesProcessing);
            BMSmoothDelay_processBufferByStride(&This->delayLeft, inL+samplesProcessed, outL+samplesProcessed, This->strideBufferUnion, samplesProcessing, samplesProcessing);
        }else{
            //Pitch shift state
            if(This->delayParamL.currentSample==This->delayParamL.sampleToReachTarget){
                //Reach target
                //Need to reset sample to reach target
                This->delayParamL.currentSample = 0;
                This->delayParamL.startSamples = This->delayParamL.stopSamples;
                This->delayParamL.stopSamples = This->delayParamL.desiredDS;
                This->delayParamL.lastSTC = 0;
                printf("L %f %f %lu\n",This->delayParamL.startSamples,This->delayParamL.stopSamples,numSamples-samplesProcessed);
                
            }else{
                //Not reach target
                samplesProcessing = BM_MIN(samplesProcessing, This->delayParamL.sampleToReachTarget- This->delayParamL.currentSample);
            }
            
            //Process pitch shift
            BMStereoLagTime_prepareStrideBufferL(This,samplesProcessed,samplesProcessing);
            BMSmoothDelay_processBufferByStride(&This->delayLeft, inL+samplesProcessed, outL+samplesProcessed, This->strideBufferL, This->delayParamL.sampleToConsume, samplesProcessing);
            
            This->delayParamL.currentSample += samplesProcessing;
        }
        
        samplesProcessed += samplesProcessing;
    }
    
    samplesProcessed = 0;
    while (samplesProcessed<numSamples) {
        samplesProcessing = BM_MIN(numSamples- samplesProcessed,BM_BUFFER_CHUNK_SIZE);
        
        //Right
        if(This->delayParamR.currentSample==This->delayParamR.sampleToReachTarget&&
           This->delayParamR.stopSamples==This->delayParamR.desiredDS){
            //Stop state - make copy
            float incre = 1;
            float start = 0;
            vDSP_vramp(&start, &incre, This->strideBufferUnion, 1, samplesProcessing);
            BMSmoothDelay_processBufferByStride(&This->delayRight, inR+samplesProcessed, outR+samplesProcessed, This->strideBufferUnion, samplesProcessing, samplesProcessing);
        }else{
            //Pitch shift state
            if(This->delayParamR.currentSample==This->delayParamR.sampleToReachTarget){
                //Reach target
                //Need to reset sample to reach target
                This->delayParamR.currentSample = 0;
                This->delayParamR.startSamples = This->delayParamR.stopSamples;
                This->delayParamR.stopSamples = This->delayParamR.desiredDS;
                This->delayParamR.lastSTC = 0;
                printf("R %f %f %lu\n",This->delayParamR.startSamples,This->delayParamR.stopSamples,numSamples-samplesProcessed);
                
            }else{
                //Not reach target
                samplesProcessing = BM_MIN(samplesProcessing, This->delayParamR.sampleToReachTarget- This->delayParamR.currentSample);
            }
            
            //Process pitch shift
            BMStereoLagTime_prepareStrideBufferR(This,samplesProcessed,samplesProcessing);
            BMSmoothDelay_processBufferByStride(&This->delayRight, inR+samplesProcessed, outR+samplesProcessed, This->strideBufferR, This->delayParamR.sampleToConsume, samplesProcessing);
            
            This->delayParamR.currentSample += samplesProcessing;
        }
        samplesProcessed += samplesProcessing;
    }
}


