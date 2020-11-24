//
//  BMPanMixer.h
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 8/8/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifndef BMPanMixer_h
#define BMPanMixer_h

#include <stdio.h>
#include <stdbool.h>
#include "Constants.h"

typedef enum{
    PM_Mode1,PM_Mode2,PM_Mode3
} PanMode;

typedef struct BMPanMixer {
    float perSampleDiff;
    bool reachTarget;
    float panMix;
    PanMode panMode;
    float panLeft,panRight;
} BMPanMixer;

void BMPanMixer_init(BMPanMixer* This,PanMode mode, float sampleRate);
void BMPanMixer_setPanMix(BMPanMixer* This,float mix);
void BMPanMixer_processStereo(BMPanMixer* This,float* inL,float* inR,float* outL,float* outR,size_t frameCount);

#endif /* BMPanMixer_h */
