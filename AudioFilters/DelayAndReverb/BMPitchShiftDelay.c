//
//  BMPitchShiftDelay.c
//  BMAUDimensionChorus
//
//  Created by Nguyen Minh Tien on 1/2/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMPitchShiftDelay.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <Accelerate/Accelerate.h>
#include "Constants.h"

void BMPitchShiftDelay_init(BMPitchShiftDelay* This,float duration,size_t delayRange,size_t maxDelayRange,size_t sampleRate,bool startAtMaxRange,bool useFilter){
    //init buffer
    This->sampleRate = sampleRate;
    This->duration = duration;
    This->delayRange = delayRange;
    This->maxDelayRange = maxDelayRange;
    This->mixToOtherValue = 1.0f;
    This->useFilter = useFilter;
    if(!startAtMaxRange){
        This->delayParamL.startSamples = 0;
        This->delayParamL.stopSamples = delayRange;
        This->delayParamR.startSamples = delayRange;
        This->delayParamR.stopSamples = 0;
    }else{
        This->delayParamL.startSamples = delayRange;
        This->delayParamL.stopSamples = 0;
        This->delayParamR.startSamples = 0;
        This->delayParamR.stopSamples = delayRange;
    }
    This->delayParamL.lastSTC = 0;
    This->delayParamR.lastSTC = 0;
    This->delayParamL.sampleToConsume = 0;
    This->delayParamR.sampleToConsume = 0;
    This->currentSample = 0;
    This->sampleToReachTarget = duration * sampleRate;
    This->resetFilter = false;
    This->wetDirectGain = BM_DB_TO_GAIN(-10);
    
    This->initNo = ReadyNo;
    
    float speed = 1 - delayRange/(This->sampleToReachTarget);
    BMSmoothDelay_init(&This->delayLeft, This->delayParamL.startSamples, speed, maxDelayRange, sampleRate);
    BMSmoothDelay_init(&This->delayRight, This->delayParamR.startSamples, speed, maxDelayRange, sampleRate);
    
    BMSmoothGain_init(&This->wetGain, sampleRate);
    BMSmoothGain_init(&This->dryGain, sampleRate);
    
    if(This->useFilter){
        BMMultiLevelBiquad_init(&This->midBandFilter, 2, sampleRate, true, true, false);
        BMMultiLevelBiquad_setLowPass6db(&This->midBandFilter, 4000, 0);
        BMMultiLevelBiquad_setHighPass6db(&This->midBandFilter, 120, 1);
        
        BMMultiLevelBiquad_init(&This->midBandAfterDelayFilter, 2, sampleRate, true, true, false);
        BMMultiLevelBiquad_setLowPass6db(&This->midBandAfterDelayFilter, 4000, 0);
        BMMultiLevelBiquad_setHighPass6db(&This->midBandAfterDelayFilter, 120, 1);
    }
    
    This->wetBufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->wetBufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->tempBufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->tempBufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->strideBufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->strideBufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    BMPitchShiftDelay_setWetGain(This, 0.5f);
    
    BMSmoothSwitch_init(&This->offSwitchL, sampleRate);
    BMSmoothSwitch_init(&This->offSwitchR, sampleRate);
}

void BMPitchShiftDelay_destroy(BMPitchShiftDelay* This){
    This->initNo = 0;
    
    BMSmoothDelay_destroy(&This->delayLeft);
    BMSmoothDelay_destroy(&This->delayRight);
    
    free(This->wetBufferL);
    This->wetBufferL = nil;
    free(This->wetBufferR);
    This->wetBufferR = nil;
    free(This->tempBufferL);
    This->tempBufferL = nil;
    free(This->tempBufferR);
    This->tempBufferR = nil;
    free(This->strideBufferL);
    This->strideBufferL = nil;
    free(This->strideBufferR);
    This->strideBufferR = nil;
    
    if(This->useFilter){
        BMMultiLevelBiquad_free(&This->midBandFilter);
        BMMultiLevelBiquad_free(&This->midBandAfterDelayFilter);
    }
}

#pragma mark - Set
void BMPitchShiftDelay_setWetGain(BMPitchShiftDelay* This,float vol){
    BMSmoothGain_setGainDb(&This->wetGain, BM_GAIN_TO_DB(sqrtf(vol)));
    BMSmoothGain_setGainDb(&This->dryGain, BM_GAIN_TO_DB(sqrtf((1-vol))));
}

void BMPitchShiftDelay_setDelayRange(BMPitchShiftDelay* This,size_t newDelayRange){
    assert(newDelayRange<=This->maxDelayRange);
    This->delayRange = newDelayRange;
    //Reset filter
    BMSmoothSwitch_setState(&This->offSwitchL, false);
    BMSmoothSwitch_setState(&This->offSwitchR, false);
    This->resetFilter = true;
}

void BMPitchShiftDelay_setBandLowpass(BMPitchShiftDelay* This,float fc){
    BMMultiLevelBiquad_setLowPass6db(&This->midBandFilter, fc, 0);
    BMMultiLevelBiquad_setLowPass6db(&This->midBandAfterDelayFilter, fc, 0);
}

void BMPitchShiftDelay_setBandHighpass(BMPitchShiftDelay* This,float fc){
    BMMultiLevelBiquad_setHighPass6db(&This->midBandFilter, fc, 1);
    BMMultiLevelBiquad_setHighPass6db(&This->midBandAfterDelayFilter, fc, 1);
}

void BMPitchShiftDelay_setMixOtherChannel(BMPitchShiftDelay* This,float value){
    This->mixToOtherValue = value;
}

void BMPitchShiftDelay_setDelayDuration(BMPitchShiftDelay* This,float duration){
    This->duration = duration;
    BMSmoothSwitch_setState(&This->offSwitchL, false);
    BMSmoothSwitch_setState(&This->offSwitchR, false);
    This->resetFilter = true;
}

//void BMPitchShiftDelay_setSampleRate(BMPitchShiftDelay* This,size_t sr){
//    if(This->sampleRate!=sr){
//        This->sampleRate = sr;
//        BMSmoothSwitch_init(&This->offSwitchL, sr);
//        BMSmoothSwitch_init(&This->offSwitchR, sr);
//        
//        BMPitchShiftDelay_setDelayDuration(This, This->duration);
//    }
//}

void BMPitchShiftDelay_resetFilters(BMPitchShiftDelay* This){
    This->currentSample = 0;
    This->sampleToReachTarget = This->duration*This->sampleRate;
    This->delayParamL.startSamples = 0;
    This->delayParamL.stopSamples = This->delayRange;
    This->delayParamR.startSamples = This->delayRange;
    This->delayParamR.stopSamples = 0;
    This->delayParamL.lastSTC = 0;
    This->delayParamR.lastSTC = 0;
    This->delayParamL.sampleToConsume = 0;
    This->delayParamR.sampleToConsume = 0;
    BMSmoothDelay_resetToDelaySample(&This->delayLeft, This->delayParamL.startSamples);
    BMSmoothDelay_resetToDelaySample(&This->delayRight, This->delayParamR.startSamples);
}

#pragma mark - Process
void BMPitchShiftDelay_prepareStrideBuffer(BMPitchShiftDelay* This,size_t samplesProcessed,size_t samplesProcessing){
        
    float dr = (This->delayParamL.stopSamples-This->delayParamL.startSamples);
    Float64 speedL = 1 - dr/This->sampleToReachTarget;
    float startIdxL = (This->currentSample+1)*speedL - This->delayParamL.lastSTC;
    float rampSpeed = speedL;
    vDSP_vramp(&startIdxL, &rampSpeed, This->strideBufferL, 1, samplesProcessing);
    //Get the decimal of realIdx & force to equal the last idx
    double currentStrideIdxL = (This->currentSample+samplesProcessing)*speedL;
    This->strideBufferL[samplesProcessing-1] = currentStrideIdxL - This->delayParamL.lastSTC;
    
    float currentSTC = floorf(currentStrideIdxL);
    if(This->currentSample+samplesProcessing==This->sampleToReachTarget)
        currentSTC = roundf(currentStrideIdxL);
    This->delayParamL.sampleToConsume = currentSTC - This->delayParamL.lastSTC;
    This->delayParamL.lastSTC = currentSTC;

    //Right
    dr = (This->delayParamR.stopSamples-This->delayParamR.startSamples);
    Float64 speedR = 1 - dr/This->sampleToReachTarget;
    float startIdxR = (This->currentSample+1)*speedR - This->delayParamR.lastSTC;
    rampSpeed = speedR;
    vDSP_vramp(&startIdxR, &rampSpeed, This->strideBufferR, 1, samplesProcessing);
    //Get the decimal of realIdx & force to equal the last idx
    double currentStrideIdxR = (This->currentSample+samplesProcessing)*speedR;
    //Correct decimal
    This->strideBufferR[samplesProcessing-1] = currentStrideIdxR - This->delayParamR.lastSTC;

    currentSTC = floorf(currentStrideIdxR);
    if(This->currentSample+samplesProcessing==This->sampleToReachTarget)
        currentSTC = roundf(currentStrideIdxR);
    This->delayParamR.sampleToConsume = currentSTC - This->delayParamR.lastSTC;
    This->delayParamR.lastSTC = currentSTC;
//    printf("t %f %f %f %f\n",startIdxL,startIdxR,This->strideBufferL[samplesProcessing-1],This->strideBufferL[samplesProcessing-1]);
}


void BMPitchShiftDelay_processResetFilter(BMPitchShiftDelay* This){
    if(BMSmoothSwitch_getState(&This->offSwitchL)==BMSwitchOff){
        if(This->resetFilter){
            This->resetFilter = false;
            BMPitchShiftDelay_resetFilters(This);
            BMSmoothSwitch_setState(&This->offSwitchL, true);
            BMSmoothSwitch_setState(&This->offSwitchR, true);
        }
    }
}

void BMPitchShiftDelay_processStereoBuffer(BMPitchShiftDelay* This,float* inL, float* inR, float* outL, float* outR,size_t numSamples){
    assert(numSamples<=BM_BUFFER_CHUNK_SIZE);
    
    if(This->initNo==ReadyNo){
        
        //Reset filter if neccessary
        BMPitchShiftDelay_processResetFilter(This);
        
        if(This->useFilter){
        //Filter midrange between 120 - 4000 hz
            BMMultiLevelBiquad_processBufferStereo(&This->midBandFilter, inL, inR, This->wetBufferL, This->wetBufferR, numSamples);
        }else{
            memcpy(This->wetBufferL, inL, sizeof(float)*numSamples);
            memcpy(This->wetBufferR, inR, sizeof(float)*numSamples);
        }
        
        //Process left channel
        size_t samplesProcessing;
        size_t samplesProcessed = 0;
        while (samplesProcessed<numSamples) {
            
            samplesProcessing = BM_MIN(numSamples- samplesProcessed,BM_BUFFER_CHUNK_SIZE);
            samplesProcessing = BM_MIN(samplesProcessing, This->sampleToReachTarget- This->currentSample);
            
            BMPitchShiftDelay_prepareStrideBuffer(This,samplesProcessed,samplesProcessing);
            BMSmoothDelay_processBufferByStride(&This->delayLeft, This->wetBufferL+samplesProcessed, This->wetBufferL+samplesProcessed, This->strideBufferL, This->delayParamL.sampleToConsume, samplesProcessing);
            BMSmoothDelay_processBufferByStride(&This->delayRight, This->wetBufferR+samplesProcessed, This->wetBufferR+samplesProcessed, This->strideBufferR, This->delayParamR.sampleToConsume, samplesProcessing);
    //        printf("%f %f\n",This->delayLeft.delaySamples,This->delayRight.delaySamples);
            
            if(BMSmoothSwitch_getState(&This->offSwitchL)!=BMSwitchOn){
                BMSmoothSwitch_processBufferMono(&This->offSwitchL, This->wetBufferL+samplesProcessed, This->wetBufferL+samplesProcessed, samplesProcessing);
                BMSmoothSwitch_processBufferMono(&This->offSwitchR, This->wetBufferR+samplesProcessed, This->wetBufferR+samplesProcessed, samplesProcessing);
            }
            samplesProcessed += samplesProcessing;
            This->currentSample += samplesProcessing;
            
            if(This->currentSample==This->sampleToReachTarget){
                //Need to reset sample to reach target
                This->currentSample = 0;
                This->delayParamL.stopSamples = This->delayRange - This->delayParamL.stopSamples;
                This->delayParamL.startSamples = This->delayLeft.delaySamples;
                This->delayParamR.stopSamples = This->delayRange - This->delayParamR.stopSamples;
                This->delayParamR.startSamples = This->delayRight.delaySamples;
                This->delayParamL.lastSTC = 0;
                This->delayParamR.lastSTC = 0;
                printf("%f %f %lu\n",This->delayParamL.startSamples,This->delayParamR.startSamples,numSamples-samplesProcessed);
            }
        }
//        //Afterdelay - test
//        BMMultiLevelBiquad_processBufferStereo(&This->midBandAfterDelayFilter, This->wetBufferL, This->wetBufferR, This->wetBufferL, This->wetBufferR, numSamples);
        
        //Apply wetgain by -10db
        float wetGainReverse = - This->wetDirectGain;
        vDSP_vsmul(This->wetBufferL, 1, &This->wetDirectGain, This->tempBufferL, 1, numSamples);
        vDSP_vsmul(This->wetBufferR, 1, &wetGainReverse, This->tempBufferR, 1, numSamples);
        
        //Add wet signal from other channel
        //Add right signal to left channel
        vDSP_vsma(This->wetBufferR, 1, &This->mixToOtherValue, This->tempBufferL, 1, This->tempBufferL, 1, numSamples);
        //Add left signal to right channel
        vDSP_vsma(This->wetBufferL, 1, &This->mixToOtherValue, This->tempBufferR, 1, This->tempBufferR, 1, numSamples);
        
        //Re-calculate wetgain so it has unity gain
        float counterGain =  1.0f/(This->mixToOtherValue + This->wetDirectGain);
        vDSP_vsmul(This->tempBufferL, 1, &counterGain, This->wetBufferL, 1, numSamples);
        vDSP_vsmul(This->tempBufferR, 1, &counterGain, This->wetBufferR, 1, numSamples);
        
        //Process wet vol
        BMSmoothGain_processBuffer(&This->wetGain, This->wetBufferL, This->wetBufferR, This->wetBufferL, This->wetBufferR, numSamples);
        //Process dry vol
        BMSmoothGain_processBuffer(&This->dryGain, inL, inR, outL, outR, numSamples);
        
        //Mix wet & dry
        vDSP_vadd(This->wetBufferL, 1,outL , 1, outL, 1, numSamples);
        vDSP_vadd(This->wetBufferR, 1,outR , 1, outR, 1, numSamples);
        
        
    }
}

void BMPitchShiftDelay_processMonoBuffer(BMPitchShiftDelay* This,float* input, float* output, size_t numSamples){
    assert(numSamples<=BM_BUFFER_CHUNK_SIZE);
    
    BMPitchShiftDelay_processResetFilter(This);
    
    //if delaysampleL reach 0 -> set it to max delay
    if(This->delayLeft.delaySamples==This->delayLeft.desiredDS){
        
        //Reach target
        if(This->delayLeft.delaySamples<=0){
            BMSmoothDelay_setDelaySample(&This->delayLeft, This->delayRange);
        }else if(This->delayLeft.delaySamples>=This->delayRange){
            BMSmoothDelay_setDelaySample(&This->delayLeft, 0);
        }
    }
    
    //Filter midrange between 120 - 4000 hz
    BMMultiLevelBiquad_processBufferStereo(&This->midBandFilter, input, input, This->wetBufferL, This->wetBufferL, numSamples);
    
    //Process left channel
    size_t samplesProcessing;
    size_t samplesProcessed = 0;
    while (samplesProcessed<numSamples) {
        samplesProcessing = BM_MIN(numSamples- samplesProcessed,BM_BUFFER_CHUNK_SIZE);
        samplesProcessing = BM_MIN(samplesProcessing, This->sampleToReachTarget- This->currentSample);
        
        BMPitchShiftDelay_prepareStrideBuffer(This,samplesProcessed,samplesProcessing);
        BMSmoothDelay_processBufferByStride(&This->delayLeft, This->wetBufferL+samplesProcessed, This->wetBufferL+samplesProcessed, This->strideBufferL, This->delayParamL.sampleToConsume, samplesProcessing);
        
        if(BMSmoothSwitch_getState(&This->offSwitchL)!=BMSwitchOn){
            BMSmoothSwitch_processBufferMono(&This->offSwitchL, This->wetBufferL+samplesProcessed, This->wetBufferL+samplesProcessed, samplesProcessing);
        }
        
        samplesProcessed += samplesProcessing;
        This->currentSample += samplesProcessing;
        
        if(This->currentSample==This->sampleToReachTarget){
            //Need to reset sample to reach target
            This->currentSample = 0;
            This->delayParamL.stopSamples = This->delayRange - This->delayParamL.stopSamples;
            This->delayParamL.startSamples = This->delayLeft.delaySamples;
            This->delayParamL.lastSTC = 0;
            printf("%f %f\n",This->delayParamL.stopSamples,This->delayParamL.startSamples);
        }
    }
    
    //Process wet vol
    BMSmoothGain_processBufferMono(&This->wetGain, This->wetBufferL, This->wetBufferL, numSamples);
    //Process dry vol
    BMSmoothGain_processBufferMono(&This->dryGain, input, output, numSamples);
    
    //Mix
    float mul = 1.0f;
    vDSP_vsma(This->wetBufferL, 1, &mul, output, 1, output, 1, numSamples);
}


