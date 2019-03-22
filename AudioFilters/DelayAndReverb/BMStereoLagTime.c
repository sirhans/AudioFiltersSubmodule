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
#include "Constants.h"

static inline void BMStereoLagTime_processOneChannel(BMStereoLagTime* This,TPCircularBuffer* circularBuffer,float* delaySamples, size_t desiredDS, float* tempBuffer,float* strideIdx,float* inBuffer, float* outBuffer,size_t numSamples);
void BMStereoLagTime_prepareLGIBuffer(BMStereoLagTime* This,size_t bufferSize);

void BMStereoLagTime_init(BMStereoLagTime* This,size_t maxDelayTimeInMilSecond,size_t sampleRate){
    //init buffer
    This->sampleRate = sampleRate;
    This->maxDelaySamples = (uint32_t)ceilf((maxDelayTimeInMilSecond/1000. * sampleRate)/BM_BUFFER_CHUNK_SIZE) * BM_BUFFER_CHUNK_SIZE;
    TPCircularBufferInit(&This->bufferL, (This->maxDelaySamples + BM_BUFFER_CHUNK_SIZE)*sizeof(float));
    TPCircularBufferClear(&This->bufferL);
//    TPCircularBufferProduce(&This->bufferL, sizeof(float)*5);
    
    TPCircularBufferInit(&This->bufferR, (This->maxDelaySamples + BM_BUFFER_CHUNK_SIZE)*sizeof(float));
    TPCircularBufferClear(&This->bufferR);
//    TPCircularBufferProduce(&This->bufferR, sizeof(float)*5);
    
    This->delaySamplesL = 0;
    This->delaySamplesR = 0;
    This->desiredDSL = 0;
    This->desiredDSR = 0;
    This->targetDSL = 0;
    This->targetDSR = 0;
    This->shouldUpdateDS = false;
    This->strideIdxL = 0;
    This->strideIdxR = 0;
    
    BMStereoLagTime_prepareLGIBuffer(This, AudioBufferLength);
}

void BMStereoLagTime_prepareLGIBuffer(BMStereoLagTime* This,size_t bufferSize){
    BMLagrangeInterpolation_init(&This->lgInterpolation, LGI_Order);
    //Stride
    size_t strideSize = bufferSize * LGI_Order;
    This->lgiStrideIdx = malloc(sizeof(float) * strideSize);
    float steps = 1./LGI_Order;
    float baseIdx = LGI_Order* 0.5 - 1;
    for(int i=0;i<strideSize;i++){
        This->lgiStrideIdx[i] = i* steps + baseIdx;
    }
    
    //Temp buffer
    This->lgiBufferL = malloc(sizeof(float) * (bufferSize + LGI_Order));
    This->lgiBufferR = malloc(sizeof(float) * (bufferSize + LGI_Order));
    //memset(&lgiBuffer, 0, sizeof(lgiBuffer));
    
    This->lgiUpBuffer = malloc(sizeof(float) * bufferSize * LGI_Order);
}

void BMStereoLagTime_destroy(BMStereoLagTime* This){
    TPCircularBufferCleanup(&This->bufferL);
    TPCircularBufferCleanup(&This->bufferR);
}

void BMStereoLagTime_setDelayLeft(BMStereoLagTime* This,size_t delayInMilSecond){
    This->targetDSL = delayInMilSecond/ 1000. * This->sampleRate;
    This->targetDSR = 0;
    This->shouldUpdateDS = true;
}

void BMStereoLagTime_setDelayRight(BMStereoLagTime* This,size_t delayInMilSecond){
    This->targetDSR = delayInMilSecond/ 1000. * This->sampleRate;
    This->targetDSL = 0;
    This->shouldUpdateDS = true;
}

void BMStereoLagTime_updateTargetDelaySamples(BMStereoLagTime* This){
    if(This->shouldUpdateDS){
        This->shouldUpdateDS = false;
        This->desiredDSL = This->targetDSL;
        This->desiredDSR = This->targetDSR;
//        printf("%d %d %d %d\n",This->delaySamplesL,This->desiredDSL,This->delaySamplesR,This->desiredDSR);
        if(This->targetDSL==0){
            This->isDelayLeftChannel = false;
        }else{
            This->isDelayLeftChannel = true;
        }
    }
}

void BMStereoLagTime_process(BMStereoLagTime* This,float* inL, float* inR, float* outL, float* outR,size_t numSamples){
    //Update delay sample target
    BMStereoLagTime_updateTargetDelaySamples(This);
    //Process
    float delaySamples = 0;
    if(This->isDelayLeftChannel){
        //Delay left channel -> check right channel still have delay samples first
        if(This->delaySamplesR>0){
            //Cpy left channel
            BMStereoLagTime_processOneChannel(This, &This->bufferL, &delaySamples, delaySamples, This->lgiBufferL,&This->strideIdxL, inL, outL, numSamples);
            //Process right channel
            BMStereoLagTime_processOneChannel(This, &This->bufferR, &This->delaySamplesR, This->desiredDSR,This->lgiBufferR,&This->strideIdxR, inR, outR, numSamples);
        }else{
            //Cpy right channel
            BMStereoLagTime_processOneChannel(This, &This->bufferR, &delaySamples, delaySamples,This->lgiBufferR,&This->strideIdxR, inR, outR, numSamples);
            //Process left channel
            BMStereoLagTime_processOneChannel(This, &This->bufferL, &This->delaySamplesL, This->desiredDSL,This->lgiBufferL,&This->strideIdxL, inL, outL, numSamples);
        }
    }else{
        //Delay right channel -> check left channel still have delay samples first
        if(This->delaySamplesL>0){
            //Cpy right channel
            BMStereoLagTime_processOneChannel(This, &This->bufferR, &delaySamples, delaySamples,This->lgiBufferR,&This->strideIdxR, inR, outR, numSamples);
            //Process left channel
            BMStereoLagTime_processOneChannel(This, &This->bufferL, &This->delaySamplesL, This->desiredDSL,This->lgiBufferL,&This->strideIdxL, inL, outL, numSamples);
        }else{
            //Cpy left channel
            BMStereoLagTime_processOneChannel(This, &This->bufferL, &delaySamples, delaySamples,This->lgiBufferL,&This->strideIdxL, inL, outL, numSamples);
            
            //Process right channel
            BMStereoLagTime_processOneChannel(This, &This->bufferR, &This->delaySamplesR, This->desiredDSR,This->lgiBufferR,&This->strideIdxR, inR, outR, numSamples);
        }
        
    }
}

static inline void BMStereoLagTime_processOneChannel(BMStereoLagTime* This,TPCircularBuffer* circularBuffer,float* delaySamples, size_t desiredDS, float* tempBuffer,float* strideIdx,float* inBuffer, float* outBuffer,size_t numSamples)
{
    size_t samplesProcessing;
    size_t samplesProcessed = 0;
    while (samplesProcessed<numSamples) {
        samplesProcessing = BM_MIN(numSamples- samplesProcessed,BM_BUFFER_CHUNK_SIZE);
        
        //Process here
        uint32_t bytesAvailableForWrite;
        void* head = TPCircularBufferHead(circularBuffer, &bytesAvailableForWrite);
        
        // copy from input to the buffer head
        uint32_t byteProcessing = (uint32_t)(samplesProcessing * sizeof(float));
        memcpy(head, inBuffer + samplesProcessed, byteProcessing);
        
        // mark the written region of the buffer as written
        TPCircularBufferProduce(circularBuffer, byteProcessing);
        
        //Tail
        uint32_t bytesAvailableForRead;
        float* tail = TPCircularBufferTail(circularBuffer, &bytesAvailableForRead);
        
        float sampleToConsume = 0;
        
        float speed = 1;
        
        float speedFactor = 1.1;
        float delaySamplesGap;
        if(*delaySamples>desiredDS){
            //Decrease delay samples -> move faster
            speed = speedFactor;
            delaySamplesGap = fabsf(floorf(*delaySamples) - desiredDS);
        }else{
            //increase delay samples -> move slower
            speed = 1.0/speedFactor;
            delaySamplesGap = fabsf(ceilf(*delaySamples) - desiredDS);
        }

        float advanceDSIdx = 1 - speed;
        float samplesToReachTarget = BM_MIN(samplesProcessing, floorf(delaySamplesGap/fabsf(advanceDSIdx)));
        
        
        if(samplesToReachTarget>0){//
            //Copy input to lgi buffer - skip 10 first order because we will reuse the last buffer data
            size_t inputLength = BM_MIN(bytesAvailableForRead/sizeof(float), ceilf(samplesToReachTarget * speed + *strideIdx + samplesProcessing - samplesToReachTarget));
            memcpy(tempBuffer + LGI_Order, tail, sizeof(float)*inputLength);
            
            int retainInputIdx = 0;
            float baseIdx = LGI_Order * 0.5;
            bool reachTarget = false;
            *strideIdx += baseIdx;
            for(int i=0;i<samplesProcessing;i++){
                if(i<=samplesToReachTarget){
                    //Speed mode -> go faster or slower to reach Desired Delay Samples
                    outBuffer[i+samplesProcessed] = BMLagrangeInterpolation_processOneSample(&This->lgInterpolation, tempBuffer, *strideIdx, inputLength + LGI_Order);
                    
                    if(i==samplesToReachTarget&&samplesToReachTarget!=samplesProcessing){
                        //Reach the desiredDS
                        retainInputIdx = roundf(*strideIdx);
                        sampleToConsume = retainInputIdx + 1 - baseIdx;
                        *delaySamples += i - sampleToConsume + 1;
                        printf("reached target %f %d %f %zu %f %zu\n",*strideIdx,retainInputIdx,speed,inputLength,*delaySamples,desiredDS);
                        
                        *strideIdx = 0;
                        reachTarget = true;
                    }
                    
                    //possibility : when i==samplesProcess - 1 & reach target = true -> error
                    if(!reachTarget){
                        if(i==samplesProcessing-1){
                            //End of loop
                            sampleToConsume = floorf(*strideIdx) + 1;
                            //Not reach target -> calculate stride idx
                            *strideIdx -= floorf(*strideIdx);
                            sampleToConsume -= baseIdx;
                            *delaySamples += i - sampleToConsume + 1;
//                            printf("stride %f\n",*strideIdx);
                        }else{
                            *strideIdx += speed;
                        }
                    }
                }else{
                    //DelayTime reached target -> switch to Normal mode
                    retainInputIdx++;
                    sampleToConsume++;
                    outBuffer[i+samplesProcessed] = tempBuffer[retainInputIdx];
                    if(i==samplesProcessing-1){
                        //End
                        printf("idx %f %f %f\n",retainInputIdx-baseIdx,tempBuffer[retainInputIdx],sampleToConsume);
                    }
                }
                
                
                if(i>0&&fabsf(outBuffer[i+samplesProcessed] - outBuffer[i+samplesProcessed-1])>0.2){
                    printf("error %d %f\n",i,*strideIdx);
                    int j =0;
                    j++;
                }
            }
        }else{
            // copy from buffer tail to output
            //copy 5 samples from buffer
            size_t samplesFromLastFrame = 5;
            memcpy(outBuffer+samplesProcessed, tempBuffer+samplesFromLastFrame, sizeof(float)*samplesFromLastFrame);
            //Copy the rest
            memcpy(outBuffer+samplesProcessed + samplesFromLastFrame, tail, byteProcessing-sizeof(float)*samplesFromLastFrame);
            
            sampleToConsume = samplesProcessing;
        }
        
//        printf("%f\n",sampleToConsume);
        //Save 10 last samples into lgiUpBuffer 10 first samples
        int samplesToStoreFromTail = sampleToConsume - LGI_Order;
        memcpy(tempBuffer, tail  + samplesToStoreFromTail, sizeof(float)*LGI_Order);
        
        // mark the read bytes as used
        TPCircularBufferConsume(circularBuffer, BM_MIN(sampleToConsume*sizeof(float),bytesAvailableForRead));
        
//        float* nexttail = TPCircularBufferTail(circularBuffer, &bytesAvailableForRead);
//        printf("next %f %lu\n",*delaySamples,bytesAvailableForRead/sizeof(float));
        
        samplesProcessed += samplesProcessing;
    }
}

