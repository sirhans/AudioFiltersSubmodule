//
//  BMPanMixer.c
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 8/8/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#include "BMPanMixer.h"
#import <math.h>
#include <Accelerate/Accelerate.h>

void BMPanMixer_init(BMPanMixer* This,PanMode mode, float sampleRate){
    This->panMix = 0.5;
    This->panMode = mode;
    This->reachTarget = true;
    
    // set the per sample difference to fade from 0 to 1 in *time*
    float time = 1.0f / 2.0f;
    This->perSampleDiff = 1.0f / (sampleRate * time);
}

void BMPanMixer_setPanMix(BMPanMixer* This,float mix){
    This->panMix = mix;
    if(This->panMode==PM_Mode1){
        //Mode 1 : left edge : panleft  = 1. Mid : panleft = 0.5; rightEdge = 1.
        This->panLeft = mix;
        This->panRight = 1.0f - mix;
    }else if(This->panMode==PM_Mode2){
        //Mode 2
        This->panLeft = sqrtf(mix);
        This->panRight = sqrtf(1-mix);
    }else{
        //Mode 3
    }
}

void BMPanMixer_processStereo(BMPanMixer* This,float* inL,float* inR,float* outL,float* outR,size_t frameCount){
    vDSP_vsmul(inL, 1, &This->panLeft, outL, 1, frameCount);
    vDSP_vsmul(inR, 1, &This->panRight, outR, 1, frameCount);
}
