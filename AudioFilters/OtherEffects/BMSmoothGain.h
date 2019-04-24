//
//  BMSmoothGain.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 4/24/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMSmoothGain_h
#define BMSmoothGain_h

#include <stdio.h>
#include <stdbool.h>

    
    typedef struct BMSmoothGain {
        float gain, gainTarget, nextGainTarget, perSampleRatioUp, perSampleRatioDown;
        bool inTransition;
    } BMSmoothGain;
    
    
    void BMSmoothGain_init(BMSmoothGain* This, float sampleRate);
    
    
    void BMSmoothGain_processBuffer(BMSmoothGain* This,
                                    float* inputL, float* inputR,
                                    float* outputL, float* outputR,
                                    size_t numSamples);
    
    
    void BMSmoothGain_setGainDb(BMSmoothGain* This, float gainDb);

#endif /* BMSmoothGain_h */

#ifdef __cplusplus
}
#endif
