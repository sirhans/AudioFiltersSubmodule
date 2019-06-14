//
//  BMHoldPeak.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 6/14/19.
//  This file may be used by anyone without restrictions of any kind.
//

#include "BMHoldPeak.h"
#include <float.h>
#include <stdbool.h>

// forward declaration
void BMHoldPeak_updateOneChannel(BMHoldPeak* This,
                                 float currentVal,
                                 float* fixedPeak,
                                 float timeSinceLastUpdate_s,
                                 bool leftChannel);






void BMHoldPeak_init(BMHoldPeak* This, float holdTimeLimit_seconds){
    // float maxValL, maxValR, peakHoldTimeLimit, peakHoldTimeL, peakHoldTimeR;
    This->maxValL = FLT_MIN;
    This->maxValR = FLT_MIN;
    This->peakHoldTimeLimit = holdTimeLimit_seconds;
    This->peakHoldTimeL = 0.0f;
    This->peakHoldTimeR = 0.0f;
}




void BMHoldPeak_updateStereo(BMHoldPeak* This,
                       float currentValL, float currentValR,
                       float* fixedPeakL, float* fixedPeakR,
                       float timeSinceLastUpdate_s){
    
    BMHoldPeak_updateOneChannel(This, currentValL, fixedPeakL, timeSinceLastUpdate_s, true);
    BMHoldPeak_updateOneChannel(This, currentValR, fixedPeakR, timeSinceLastUpdate_s, false);
}




void BMHoldPeak_updateMono(BMHoldPeak* This,
                             float currentVal,
                             float* fixedPeak,
                             float timeSinceLastUpdate_s){
    
    BMHoldPeak_updateOneChannel(This, currentVal, fixedPeak, timeSinceLastUpdate_s, true);
}





inline void BMHoldPeak_updateOneChannel(BMHoldPeak* This,
                                        float currentVal,
                                        float* fixedPeak,
                                        float timeSinceLastUpdate_s,
                                        bool leftChannel){
    
    float* maxVal;
    float* peakHoldTime;
    
    // make aliases so that we can reuse the same code for R and L channels
    if (leftChannel){
        maxVal = &This->maxValL;
        peakHoldTime = &This->peakHoldTimeL;
    } else {
        maxVal = &This->maxValR;
        peakHoldTime = &This->peakHoldTimeR;
    }
    
    // if the current slowPeak is greater than the fixedPeak
    if (currentVal > *maxVal){
        // update the fixed peak
        *maxVal = currentVal;
        // and reset the clock
        *peakHoldTime = 0.0f;
    } else {
        // increment the clock
        *peakHoldTime += timeSinceLastUpdate_s;
        // if the hold time is greater than the limit
        if (*peakHoldTime > This->peakHoldTimeLimit){
            // reduce the peak value to the current slow peak value
            *maxVal = currentVal;
            // and reset the clock
            *peakHoldTime = 0.0f;
        }
    }
    
    *fixedPeak = *maxVal;
}
