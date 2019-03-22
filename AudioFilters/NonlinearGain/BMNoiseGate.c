//
//  BMNoiseGate.c
//  AudioUnitTest
//
//  Created by TienNM on 9/25/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#include "BMNoiseGate.h"
#include <stdlib.h>
#include <assert.h>
#include <Accelerate/Accelerate.h>
#include "Constants.h"
#include <string.h>
#include "BMRMSPower.h"

#define NG_ProcessSamples 64.

void BMNoiseGate_init(BMNoiseGate* this,float threshold,float decayTime,float sampleRate){
    BMNoiseGate_setThreshold(this, threshold);
    BMNoiseGate_setDecayTime(this, decayTime,sampleRate);
    this->volume = 1;
    this->preVolume = 1;
    this->fadeCount_Max = roundf((sampleRate/10)/NG_ProcessSamples);
    this->fadeCount_Current = 0;
}

void BMNoiseGate_free(BMNoiseGate* This){
    free(This);
}

void BMNoiseGate_process(BMNoiseGate* this,
                         float* input,
                         float* output,
                         size_t numSamples){
    //calculate norm
    while(numSamples>0){
        int numSamplesProcessing = MIN((int)numSamples,NG_ProcessSamples);
        float currentVolume = BMRMSPower_process(input, numSamplesProcessing);
        
        if(currentVolume==0){
            //Dont need to do anything
            //Just write zeros to the output
            memset(output, 0, numSamplesProcessing*sizeof(float));
        }else{
            float volumeDB = BM_GAIN_TO_DB(currentVolume);
            if(volumeDB<this->threshold){
                //Check fadeCount reach max
                if(this->fadeCount_Current >= this->fadeCount_Max){
                    //Start fade out
                    //Multiply volume with k every 64 samples
                    this->preVolume = this->volume;
                    this->volume *= this->k;
                }else{
                    //Count the fadecount current until reach max
                    this->fadeCount_Current++;
                }
            } else if(volumeDB > this->threshold && this->volume < 1.){
                //Reset fadecount
                this->fadeCount_Current = 0;
                //Asymptotically increase the volume
                float k = (1. - this->volume) * 0.5;
                this->preVolume = this->volume;
                this->volume += k;
            }
        }
        
        //Apply volume to raw data
        float step = (this->volume - this->preVolume)/numSamplesProcessing;
        vDSP_vrampmul(input, 1, &this->preVolume, &step, output, 1, numSamplesProcessing);
        
        input += numSamplesProcessing;
        output += numSamplesProcessing;
        numSamples -= numSamplesProcessing;
    }
}

void BMNoiseGate_setDecayTime(BMNoiseGate* this,float decayTime,float sampleRate){
    this->decayTime = decayTime;
    float bps = sampleRate/NG_ProcessSamples;
    this->k = pow(0.001,1./(double)(bps*this->decayTime));
}

void BMNoiseGate_setThreshold(BMNoiseGate* this,float thres){
    this->threshold = thres;
}
    
#ifdef __cplusplus
}
#endif
