//
//  BMSmoothDelay.c
//  BMAUDimensionChorus
//
//  Created by Nguyen Minh Tien on 1/8/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMSmoothDelay.h"
#include "Constants.h"
#include <Accelerate/Accelerate.h>

void BMSmoothDelay_prepareLGIBuffer(BMSmoothDelay* This,size_t bufferSize);
void BMSmoothDelay_updateDelaySpeed(BMSmoothDelay* This,float speed);

void BMSmoothDelay_init(BMSmoothDelay* This,size_t defaultDS,float speed,size_t delayRange,size_t sampleRate){
    //init buffer
    This->sampleRate = sampleRate;
    This->delaySpeed = speed;
    This->delaySamples = defaultDS;
    This->desiredDS = (int32_t)defaultDS;
    This->targetDS = (int32_t)defaultDS;
    This->baseIdx = LGI_Order * 0.5f;
    This->storeSamples = LGI_Order+2;
    This->delaySampleRange = (uint32_t)delayRange;
    
    //Left channel start at 0 delaytime
    TPCircularBufferInit(&This->buffer, (This->delaySampleRange + BM_BUFFER_CHUNK_SIZE)*sizeof(float));
    //Produce
    uint32_t availableByte;
    float* head = TPCircularBufferHead(&This->buffer, &availableByte);
    memset(head, 0, This->delaySamples*sizeof(float));
    TPCircularBufferProduce(&This->buffer, This->delaySamples*sizeof(float));
    
    void* tail = TPCircularBufferTail(&This->buffer, &availableByte);
    memset(tail, 0, availableByte);
    
    This->shouldUpdateDS = false;
    This->strideIdx = 0;
    
    This->speed = 1;
    This->strideBuffer = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    BMSmoothDelay_prepareLGIBuffer(This, AudioBufferLength);
//    BMSmoothDelay_updateDelaySpeed(This, speed);

}

void BMSmoothDelay_resetToDelaySample(BMSmoothDelay* This,size_t defaultDS){
    This->delaySamples = defaultDS;
    TPCircularBufferClear(&This->buffer);
    //Produce
    uint32_t availableByte;
    float* head = TPCircularBufferHead(&This->buffer, &availableByte);
    memset(head, 0, This->delaySamples*sizeof(float));
    TPCircularBufferProduce(&This->buffer, This->delaySamples*sizeof(float));
    
    uint32_t bytesAvailableForRead;
    float* tail = TPCircularBufferTail(&This->buffer, &bytesAvailableForRead);
    memset(tail, 0, bytesAvailableForRead);
//    size_t correctDS = bytesAvailableForRead/sizeof(float);
//    printf("reset %zu\n",correctDS);
}

void BMSmoothDelay_prepareLGIBuffer(BMSmoothDelay* This,size_t bufferSize){
    BMLagrangeInterpolation_init(&This->lgInterpolation, LGI_Order);
    
    //Temp buffer
    This->lgiBuffer = calloc(bufferSize + This->storeSamples, sizeof(float));
}

void BMSmoothDelay_destroy(BMSmoothDelay* This){
    TPCircularBufferCleanup(&This->buffer);
    
    free(This->strideBuffer);
    This->strideBuffer = nil;
    
    
    free(This->lgiBuffer);
    This->lgiBuffer = nil;
    
    BMLagrangeInterpolation_destroy(&This->lgInterpolation);
}

#pragma mark - Set
void BMSmoothDelay_setDelaySample(BMSmoothDelay* This,size_t delaySample){
    if(delaySample!=This->targetDS){
        This->targetDS = (int32_t)delaySample;
        This->shouldUpdateDS = true;
    }
}

void BMSmoothDelay_setDelaySpeed(BMSmoothDelay* This,float speed){
    This->delaySpeed = speed;
}

bool BMSmoothDelay_reachTarget(BMSmoothDelay* This){
    return This->sampleToReachTarget==0&&!This->shouldUpdateDS;
}

#pragma mark - Process buffer
void BMSmoothDelay_updateDelaySpeed(BMSmoothDelay* This,float speed){
//    assert(speed!=1);
    //calculate speed
    This->speed = speed;
    This->sampleToReachTarget = floorf((This->desiredDS - This->delaySamples)/(1-speed));
    float duration = This->sampleToReachTarget/This->sampleRate;
    printf("duration %f %zu\n",duration,This->sampleToReachTarget);
}

void BMSmoothDelay_updateDelaySpeedByDuration(BMSmoothDelay* This,float duration){
    //calculate speed
    This->sampleToReachTarget = duration * This->sampleRate;
    float dsIdxStep = (This->desiredDS - This->delaySamples)/This->sampleToReachTarget;
    This->speed = 1 - dsIdxStep;
}

void BMSmoothDelay_updateTargetDelaySamples(BMSmoothDelay* This){
    if(This->shouldUpdateDS){
        This->shouldUpdateDS = false;
        This->desiredDS = This->targetDS;
        
        BMSmoothDelay_updateDelaySpeed(This, This->delaySpeed);
    }
}

void BMSmoothDelay_processBuffer(BMSmoothDelay* This,float* inBuffer, float* outBuffer,size_t numSamples){
    BMSmoothDelay_updateTargetDelaySamples(This);
    
    size_t samplesProcessing;
    size_t samplesProcessed = 0;
    while (samplesProcessed<numSamples) {
        samplesProcessing = BM_MIN(numSamples- samplesProcessed,BM_BUFFER_CHUNK_SIZE);
        
        //Prepare stride buffer idx
        float sampleToConsume = 0;
        
        This->strideIdx += This->baseIdx + This->speed - 1.0f;
        if(This->sampleToReachTarget>0){
            size_t sampleToRamp = BM_MIN(This->sampleToReachTarget, samplesProcessing);
            vDSP_vramp(&This->strideIdx, &This->speed, This->strideBuffer, 1, sampleToRamp);
        
            //Update strideIdx
            This->strideIdx += This->speed * (sampleToRamp - 1);

            if(This->sampleToReachTarget<=samplesProcessing){
                //Reach target
                samplesProcessing = sampleToRamp;
                //Change speed back to 1
                This->speed = 1;
            }
            //not reach target yet
            sampleToConsume = floorf(This->strideIdx) + 1 - This->baseIdx;
            This->strideIdx -= floorf(This->strideIdx);
            This->delaySamples += samplesProcessing - sampleToConsume;
            This->sampleToReachTarget -= sampleToRamp;
        }else{
            assert(This->speed==1);
            //Normal speed
            vDSP_vramp(&This->strideIdx, &This->speed, This->strideBuffer, 1, samplesProcessing);
            sampleToConsume = samplesProcessing;
//            printf("%f %lu %f\n",This->strideBuffer[samplesProcessing-1] - baseIdx,samplesProcessing,This->strideIdx-baseIdx);
            This->strideIdx -= floorf(This->strideIdx);
        }
        
        //Produce buffer
        uint32_t bytesAvailableForWrite;
        void* head = TPCircularBufferHead(&This->buffer, &bytesAvailableForWrite);
        
        // copy from input to the buffer head
        uint32_t byteProcessing = (uint32_t)(samplesProcessing * sizeof(float));
        memcpy(head, inBuffer + samplesProcessed, byteProcessing);
        
        // mark the written region of the buffer as written
        TPCircularBufferProduce(&This->buffer, byteProcessing);
        
        //Tail
        uint32_t bytesAvailableForRead;
        float* tail = TPCircularBufferTail(&This->buffer, &bytesAvailableForRead);
        
        //Copy input to lgi buffer - skip 10 first order because we will reuse the last buffer data
        size_t inputLength = BM_MIN(bytesAvailableForRead/sizeof(float), ceilf(samplesProcessing * This->speed + This->strideIdx + samplesProcessing));
        memcpy(This->lgiBuffer + This->storeSamples, tail, sizeof(float)*inputLength);
        
        //Proccess
        BMLagrangeInterpolation_processUpSample(&This->lgInterpolation, This->lgiBuffer, This->strideBuffer, outBuffer + samplesProcessed, 1, inputLength + This->storeSamples, samplesProcessing);
        
        //        printf("%f\n",sampleToConsume);
        //Save 10 last samples into lgiUpBuffer 10 first samples
        int samplesToStoreFromTail = sampleToConsume - This->storeSamples;
        memcpy(This->lgiBuffer, tail  + samplesToStoreFromTail, sizeof(float)*This->storeSamples);
        
        // mark the read bytes as used
        TPCircularBufferConsume(&This->buffer, BM_MIN(sampleToConsume*sizeof(float),bytesAvailableForRead));
        
        //Check
        TPCircularBufferTail(&This->buffer, &bytesAvailableForRead);
        size_t correctDS = bytesAvailableForRead/sizeof(float);
        if(This->delaySamples!=correctDS){
            printf("next %f %lu\n",This->delaySamples,correctDS);
        }
        
        samplesProcessed += samplesProcessing;
    }
}

#pragma mark - Process by stride
void BMSmoothDelay_processBufferByStride(BMSmoothDelay* This,float* inBuffer, float* outBuffer,float* strideBuffer,float sampleToConsume,size_t numSamples){
    
    vDSP_vsadd(strideBuffer, 1, &This->baseIdx, strideBuffer, 1, numSamples);
    
    size_t samplesProcessing = numSamples;
    size_t samplesProcessed = 0;
    
    //Produce buffer
    uint32_t bytesAvailableForWrite;
    void* head = TPCircularBufferHead(&This->buffer, &bytesAvailableForWrite);
    
    // copy from input to the buffer head
    uint32_t byteProcessing = (uint32_t)(samplesProcessing * sizeof(float));
    memcpy(head, inBuffer + samplesProcessed, byteProcessing);
    
    // mark the written region of the buffer as written
    TPCircularBufferProduce(&This->buffer, byteProcessing);
    
    //Tail
    uint32_t bytesAvailableForRead;
    float* tail = TPCircularBufferTail(&This->buffer, &bytesAvailableForRead);
    
    //Copy input to lgi buffer - skip 10 first order because we will reuse the last buffer data
    size_t inputLength = BM_MIN(bytesAvailableForRead/sizeof(float), sampleToConsume);
    memcpy(This->lgiBuffer + This->storeSamples, tail, sizeof(float)*inputLength);
    
    //Proccess
    BMLagrangeInterpolation_processUpSample(&This->lgInterpolation, This->lgiBuffer, strideBuffer, outBuffer + samplesProcessed, 1, inputLength + This->storeSamples, samplesProcessing);
    
    //        printf("%f\n",sampleToConsume);
    //Save 10 last samples into lgiUpBuffer 10 first samples
    size_t samplesToStoreFromTail = sampleToConsume - This->storeSamples;
    memcpy(This->lgiBuffer, tail  + samplesToStoreFromTail, sizeof(float)*This->storeSamples);
    
    // mark the read bytes as used
    TPCircularBufferConsume(&This->buffer, (uint32_t)BM_MIN(sampleToConsume*sizeof(float),bytesAvailableForRead));
    
    This->delaySamples += samplesProcessing - sampleToConsume;
    //Check
    TPCircularBufferTail(&This->buffer, &bytesAvailableForRead);
    size_t correctDS = bytesAvailableForRead/sizeof(float);
    if(This->delaySamples!=correctDS){
        printf("next %f %lu\n",This->delaySamples,correctDS);
    }
}
