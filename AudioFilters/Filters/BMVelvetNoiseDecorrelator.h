//
//  BMVelvetNoiseDecorrelator.h
//  Saturator
//
//  Created by TienNM on 12/4/17.
//  Copyright © 2017 TienNM. All rights reserved.
//

#ifndef BMVelvetNoiseDecorrelator_h
#define BMVelvetNoiseDecorrelator_h

#include <stdio.h>
#include "BMMultiTapDelay.h"

typedef struct BMVelvetNoiseDecorrelator {
    int numChannels;
    size_t* delayTimesL; // time between each impulse in velvet noise decorrelator
    size_t* delayTimesR;
    float* gainsL; //Gain is either 1 or -1
    float* gainsR;
    BMMultiTapDelay multiTapDelay;
    bool updateDLNT;
    float msDelayTime;
    int numImpulse;
    float sampleRate;
    float decayTimeMS;
} BMVelvetNoiseDecorrelator;

void BMVelvetNoiseDecorrelator_init(BMVelvetNoiseDecorrelator* this,
                                    bool isStereo,
                                    float msDelayTime, float msMaxDelayTime,
                                    int numImpulse, int maxNumTaps,
                                    float sampleRate, float decayTime);
void BMVelvetNoiseDecorrelator_free(BMVelvetNoiseDecorrelator* this);
void BMVelvetNoiseDecorrelator_setDelayTimeNumTap(BMVelvetNoiseDecorrelator* this,float msDelayTime,int numImpulse,float sampleRate,float decayTimeMS);
void BMVelvetNoiseDecorrelator_processBufferMono(BMVelvetNoiseDecorrelator* this,float* input,float* output,size_t length);
void BMVelvetNoiseDecorrelator_processBufferStereo(BMVelvetNoiseDecorrelator* this,float* inputL,float* inputR,float* outputL,float* outputR,size_t length);

#endif /* BMVelvetNoiseDecorrelator_h */
