//
//  BMSmoothGain.h
//  AudioFiltersXcodeProject
//
//  Manages a gain control so that it can be adjusted without causing clicks
//
//  Created by hans anderson on 4/24/19.
//  Anyone may use this file without restrictions of any kind
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
    
    
    
    /*!
     * BMSmoothGain_init
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @param sampleRate  audio system sample rate; if you need to change this, you must re-initialize the struct
     */
    void BMSmoothGain_init(BMSmoothGain* This, float sampleRate);
    
    
    /*!
     * BMSmoothGain_processBuffer
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @param inputL      left channel input buffer length >= numSamples
     * @param inputR      right channel input buffer length >= numSamples
     * @param outputL     left channel output buffer length >= numSamples
     * @param outputR     right channel output buffer length >= numSamples
     */
    void BMSmoothGain_processBuffer(BMSmoothGain* This,
                                    float* inputL, float* inputR,
                                    float* outputL, float* outputR,
                                    size_t numSamples);
    
    
    /*!
     * BMSmoothGain_setGainDb
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @param gainDb      new target gain in decibels to be approached smoothly
     */
    void BMSmoothGain_setGainDb(BMSmoothGain* This, float gainDb);
    
    
    
    /*!
     * BMSmoothGain_getGainDb
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @return            gain in decibels
     */
    float BMSmoothGain_getGainDb(BMSmoothGain* This);

#endif /* BMSmoothGain_h */

#ifdef __cplusplus
}
#endif
