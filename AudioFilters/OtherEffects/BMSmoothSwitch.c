//
//  BMSmoothSwitch.c
//  SaturatorAU
//
//  This is an On / Off switch that smoothly opens and closes without clicking
//
//  Created by Hans on 10/12/19.
//  Anyone may use this file without restrictions
//

#include "BMSmoothSwitch.h"
#include <float.h>
#include <string.h>
#include <math.h>




void BMSmoothSwitch_init(BMSmoothSwitch *This, float sampleRate){
    // init the gain control
    BMSmoothGain_init(&This->gainControl, sampleRate);
    
    // start at unity gain
    BMSmoothGain_setGainDbInstant(&This->gainControl, 0.0f);
	
	// speed up the fade rate to 10x faster than the default setting
	This->gainControl.perSampleRatioDown = pow(This->gainControl.perSampleRatioDown,10.0);
	This->gainControl.perSampleRatioUp = pow(This->gainControl.perSampleRatioUp,10.0);
}





void BMSmoothSwitch_processBufferMono(BMSmoothSwitch *This,
                                      const float *input, float* output,
                                      size_t numSamples){
    if(This->gainControl.inTransition)
        BMSmoothGain_processBufferMono(&This->gainControl, input, output, numSamples);
    
    // if we aren't in a transition state then we are either on or off
    else {
        // if the switch is on
        if(BMSmoothSwitch_getState(This) == BMSwitchOn){
            // if the input and output are in separate locations, copy from input to output
            if(input != output)
                memcpy(output,input,sizeof(float)*numSamples);
            
            // otherwise do nothing
        }
        
        // if the switch is off
        else {
            // set the output to zero
            memset(output,0,sizeof(float)*numSamples);
        }
    }
}





void BMSmoothSwitch_processBufferStereo(BMSmoothSwitch *This,
                                      const float *inL, const float *inR,
                                      float* outL, float* outR,
                                        size_t numSamples){
    if(This->gainControl.inTransition)
        BMSmoothGain_processBuffer(&This->gainControl, inL, inR, outL, outR, numSamples);
    
    // if we aren't in a transition state then we are either on or off
    else {
        // if the switch is on
        if(BMSmoothSwitch_getState(This) == BMSwitchOn){
            // if the input and output are in separate locations, copy from input to output
            if(inL != outL){
                memcpy(outL,inL,sizeof(float)*numSamples);
                memcpy(outR,inR,sizeof(float)*numSamples);
            }
            
            // otherwise do nothing
        }
        
        // if the switch is off set the output to zero
        else {
            memset(outL,0,sizeof(float)*numSamples);
            memset(outR,0,sizeof(float)*numSamples);
        }
    }
}





void BMSmoothSwitch_setState(BMSmoothSwitch *This, bool stateOnOff){
    // turn off
    if(stateOnOff == false)
        BMSmoothGain_setGainDb(&This->gainControl, FLT_MIN);
    
    // turn on
    else
        BMSmoothGain_setGainDb(&This->gainControl, 0.0f);
}




enum BMSmoothSwitchState BMSmoothSwitch_getState(BMSmoothSwitch *This){
    if(This->gainControl.inTransition)
        return BMSwitchInTransition;
    
    // ideally the gain should be at 1.0 when the switch is open, but in case
    // it stopped at 0.999... we can consider it on if it is out of the
    // transition state (see above) and non-zero (see below).
    if(This->gainControl.gain > 0.5f)
        return BMSwitchOn;
    
    // finally, if the switch is not in transition and not on then it's off.
    return BMSwitchOff;
}
