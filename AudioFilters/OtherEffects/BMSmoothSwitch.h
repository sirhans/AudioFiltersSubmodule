//
//  BMSmoothSwitch.h
//  SaturatorAU
//
//  This is an On / Off switch that smoothly opens and closes without clicking
//
//  Created by Hans on 10/12/19.
//  Anyone may use this file without restrictions
//

#ifndef BMSmoothSwitch_h
#define BMSmoothSwitch_h

#include <stdio.h>
#include "BMSmoothGain.h"

typedef struct BMSmoothSwitch {
    BMSmoothGain gainControl;
} BMSmoothSwitch;


enum BMSmoothSwitchState {BMSwitchOn, BMSwitchOff, BMSwitchInTransition};


/*!
 *BMSmoothSwitch_init
 */
void BMSmoothSwitch_init(BMSmoothSwitch *This, float sampleRate);


/*!
 *BMSmoothSwitch_processBufferMono
 */
void BMSmoothSwitch_processBufferMono(BMSmoothSwitch *This,
                                      const float *input, float* output,
                                      size_t numSamples);


/*!
 *BMSmoothSwitch_processBufferStereo
 */
void BMSmoothSwitch_processBufferStereo(BMSmoothSwitch *This,
                                      const float *inL, const float *inR,
                                      float* outL, float* outR,
                                      size_t numSamples);


/*!
 *BMSmoothSwitch_setState
 */
void BMSmoothSwitch_setState(BMSmoothSwitch *This, bool stateOnOff);


/*!
 *BMSmoothSwitch_getState
 */
enum BMSmoothSwitchState BMSmoothSwitch_getState(BMSmoothSwitch *This);


#endif /* BMSmoothSwitch_h */
