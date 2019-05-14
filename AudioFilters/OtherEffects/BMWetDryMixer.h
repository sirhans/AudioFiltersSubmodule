//
//  BMWetDryMixer.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/3/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifndef BMWetDryMixer_h
#define BMWetDryMixer_h

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdio.h>
#include <stdbool.h>
#include "Constants.h"
    
    
    typedef struct BMWetDryMixer {
        float wetMix, mixTarget, perSampleDifference;
        bool inTransition;
    } BMWetDryMixer;
    
    
    
    /*!
     * BMWetDryMixer_init
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @param sampleRate  audio system sample rate; if you need to change this, you must re-initialize the struct
     */
    void BMWetDryMixer_init(BMWetDryMixer* This, float sampleRate);
    
    
    /*!
     * BMWetDryMixer_processBuffer
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @param inputWetL   left channel wet input buffer length >= numSamples
     * @param inputWetR   right channel wet input buffer length >= numSamples
     * @param inputDryL   left channel dry input buffer length >= numSamples
     * @param inputDryR   right channel dry input buffer length >= numSamples
     * @param outputL     left channel output buffer length >= numSamples
     * @param outputR     right channel output buffer length >= numSamples
     * @param numSamples  to process completely, all input and output arrays should have this length
     */
    void BMWetDryMixer_processBuffer(BMWetDryMixer* This,
                                    float* inputWetL, float* inputWetR,
                                    float* inputDryL, float* inputDryR,
                                    float* outputL, float* outputR,
                                    size_t numSamples);
    
    
    /*!
     * BMWetDryMixer_setMix
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @param mix    in [0,1], 0 = dry, 1 = wet
     */
    void BMWetDryMixer_setMix(BMWetDryMixer* This, float mix);
    
    
    
    /*!
     * BMWetDryMixer_getMix
     *
     * @param This        pointer to an initialized BMWetDryMixer struct
     * @return            mix, 0 = dry, 1 = wet
     */
    float BMWetDryMixer_getMix(BMWetDryMixer* This);
    
    
#ifdef __cplusplus
}
#endif


#endif /* BMWetDryMixer_h */
