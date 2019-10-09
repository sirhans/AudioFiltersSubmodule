//
//  BMVelvetNoiseDecorrelator.c
//  Saturator
//
//  Created by TienNM on 12/4/17.
//  Copyright © 2017 TienNM. All rights reserved.
//

#include "BMVelvetNoiseDecorrelator.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <Accelerate/Accelerate.h>
#include "Constants.h"
#include "BMVelvetNoise.h"
#include "BMReverb.h"

void BMVelvetNoiseDecorrelator_updateDelayTimeNumTap(BMVelvetNoiseDecorrelator* this);

void BMVelvetNoiseDecorrelator_setupImpulse(BMVelvetNoiseDecorrelator* this,float numImpulse,float samplesEachDelay,bool isStereo){
    for(int i=0;i<numImpulse;i++){
        this->delayTimesL[i] = i * samplesEachDelay;
        //Add randomtime between each impulse
        this->delayTimesL[i] += fmodf(arc4random(),samplesEachDelay);
        if(isStereo){
            this->delayTimesR[i] = i * samplesEachDelay;
            //Add randomtime between each impulse
            this->delayTimesR[i] += fmodf(arc4random(),samplesEachDelay);
        }
    }
    
    //Balance the number of positive & negative value
    BMVelvetNoise_setTapSigns(this->gainsL, numImpulse);
    if(isStereo)
        BMVelvetNoise_setTapSigns(this->gainsR, numImpulse);
    
//    //Set the 1st tap to zero delay time & positive value
//    this->delayTimesL[0] = 0;
//    this->delayTimesR[0] = 0;
//    for(int i=0;i<numImpulse;i++){
//        //Find the first positive value & swap the 1st position of gain
//        if(this->gainsL[0]<0){
//            if(this->gainsL[i]>0){
//                float swap = this->gainsL[i];
//                this->gainsL[i] = this->gainsL[0];
//                this->gainsL[0] = swap;
//            }
//        }
//
//        if(this->gainsR[0]<0){
//            if(this->gainsR[i]>0){
//                float swap = this->gainsR[i];
//                this->gainsR[i] = this->gainsR[0];
//                this->gainsR[0] = swap;
//            }
//        }
//
//        if(this->gainsL[0]>0&&this->gainsR[0]>0)
//            break;
//    }
}

void BMVelvetNoiseDecorrelator_nomallize(float* gains,int numImpulse){
    float rmsTotal = 0;
    vDSP_svesq(gains, 1, &rmsTotal, numImpulse);
    rmsTotal = sqrtf(rmsTotal);
    vDSP_vsdiv(gains, 1, &rmsTotal, gains, 1, numImpulse);
}

void BMVelvetNoiseDecorrelator_decayEnvelop(size_t* delayTimes,float* gains,int numImpulse,float sampleRate,float decayTimeMS){
    //Get maxDelayTimes
    float maxDTInS = decayTimeMS/1000.;
    for(int i=0;i<numImpulse;i++){
        float delayTimesInS = delayTimes[i]/sampleRate;
        float decayEnvelop = BMReverbDelayGainFromRT30(maxDTInS, delayTimesInS);
        //Apply to gain
        gains[i] = gains[i]*decayEnvelop;
//        printf("%f %f %f %d\n",decayEnvelop,delayTimesInS,maxDTInS,i);
    }
}

void BMVelvetNoiseDecorrelator_init(BMVelvetNoiseDecorrelator* this,
                                    bool isStereo,
                                    float msDelayTime, float msMaxDelayTime,
                                    int numImpulse, int maxNumTaps,
                                    float sampleRate, float decayTime){
    this->delayTimesL = malloc(sizeof(size_t)*maxNumTaps);
    this->delayTimesR = malloc(sizeof(size_t)*maxNumTaps);
    this->gainsL = malloc(sizeof(float)*maxNumTaps);
    this->gainsR = malloc(sizeof(float)*maxNumTaps);
    this->numChannels = isStereo ? 2:1;
    this->updateDLNT = false;
    
    float lengthInSamples = ((msDelayTime/1000.)*sampleRate);
    float samplesDelay = lengthInSamples/numImpulse;

    //Generate evenly space between impulse with gain
    BMVelvetNoiseDecorrelator_setupImpulse(this,numImpulse,samplesDelay,isStereo);

    //Exponenlly decaying with rt20
    BMVelvetNoiseDecorrelator_decayEnvelop(this->delayTimesL, this->gainsL, numImpulse, sampleRate,decayTime);
    if(isStereo)
        BMVelvetNoiseDecorrelator_decayEnvelop(this->delayTimesR, this->gainsR, numImpulse, sampleRate,decayTime);

    //Normallise gains
    BMVelvetNoiseDecorrelator_nomallize(this->gainsL, numImpulse);
    if(isStereo)
        BMVelvetNoiseDecorrelator_nomallize(this->gainsR, numImpulse);

    //Init multi-tap delay
    size_t maxDelayInSample = (msMaxDelayTime/1000.) * sampleRate;
    BMMultiTapDelay_Init(&this->multiTapDelay, isStereo, this->delayTimesL, this->delayTimesR,maxDelayInSample, this->gainsL, this->gainsR, numImpulse,maxNumTaps);
    
//    BMVelvetNoiseDecorrelator_setDelayTimeNumTap(this, msDelayTime, numImpulse, sampleRate);
//    BMVelvetNoiseDecorrelator_updateDelayTimeNumTap(this);
    
//    BMMultiTapDelay_impulseResponse(&this->multiTapDelay);
}

void BMVelvetNoiseDecorrelator_free(BMVelvetNoiseDecorrelator* this){
    free(this->delayTimesL);
    this->delayTimesL = NULL;
    
    free(this->delayTimesR);
    this->delayTimesR = NULL;
    
    free(this->gainsL);
    this->gainsL = NULL;
    free(this->gainsR);
    this->gainsR = NULL;
    
    BMMultiTapDelay_free(&this->multiTapDelay);
}

void BMVelvetNoiseDecorrelator_setDelayTimeNumTap(BMVelvetNoiseDecorrelator* this,float msDelayTime,int numImpulse,float sampleRate,float decayTimeMS){
    assert(msDelayTime<= this->multiTapDelay.maxDelayTime);
//    printf("numtap %d dt %f\n",numImpulse,msDelayTime);
    this->msDelayTime = msDelayTime;
    this->numImpulse = numImpulse;
    this->sampleRate = sampleRate;
    this->decayTimeMS = decayTimeMS;
    this->updateDLNT = true;
}

void BMVelvetNoiseDecorrelator_updateDelayTimeNumTap(BMVelvetNoiseDecorrelator* this){
    if(this->updateDLNT){
        this->updateDLNT = false;
        
        float lengthInSamples = ((this->msDelayTime/1000.)*this->sampleRate);
        float samplesDelay = lengthInSamples/this->numImpulse;
        
        //Generate evenly space between impulse with gain
        BMVelvetNoiseDecorrelator_setupImpulse(this,this->numImpulse,samplesDelay,this->numChannels==2);
        
        //Exponenlly decaying with rt20
        BMVelvetNoiseDecorrelator_decayEnvelop(this->delayTimesL, this->gainsL, this->numImpulse, this->sampleRate,this->decayTimeMS);
        if(this->numChannels==2)
            BMVelvetNoiseDecorrelator_decayEnvelop(this->delayTimesR, this->gainsR, this->numImpulse, this->sampleRate,this->decayTimeMS);
        
        //Normallise gains
        BMVelvetNoiseDecorrelator_nomallize(this->gainsL, this->numImpulse);
        if(this->numChannels==2)
            BMVelvetNoiseDecorrelator_nomallize(this->gainsR, this->numImpulse);
        
        BMMultiTapDelay_setDelayTimeNumTap(&this->multiTapDelay, this->delayTimesL, this->delayTimesR, this->numImpulse);
        BMMultiTapDelay_setGains(&this->multiTapDelay, this->gainsL, this->gainsR);
    }
}

void BMVelvetNoiseDecorrelator_processBufferMono(BMVelvetNoiseDecorrelator* this,float* input,float* output,size_t length){
    assert(this->numChannels==1);
    //Update delaytime & numtap if neccessary
    BMVelvetNoiseDecorrelator_updateDelayTimeNumTap(this);
    //Process
    BMMultiTapDelay_ProcessBufferMono(&this->multiTapDelay, input, output, length);
}

void BMVelvetNoiseDecorrelator_processBufferStereo(BMVelvetNoiseDecorrelator* this,float* inputL,float* inputR,float* outputL,float* outputR,size_t length){
    assert(this->numChannels==2);
    //Update delaytime & numtap if neccessary
    BMVelvetNoiseDecorrelator_updateDelayTimeNumTap(this);
    //Process
    BMMultiTapDelay_ProcessBufferStereo(&this->multiTapDelay, inputL, inputR, outputL, outputR, length);
}



