//
//  BMStaticDelay.h
//  BMAudioFilters
//
//  Use this class if you need a non-modulated delay whose length is substantial relative
//  to the audio buffer size.  This class uses a circular buffer that is effecient
//  for long contiguous reads and writes.
//
//  This delay is simply a delay, without feedback or mixing parameters.  Use this as a
//  building block in more complext effects.
//
//  For feedback delay lines on the order of a few samples in length, this may not be
//  efficient due to function calls incurred when switching between read and write
//  modes.
//
//
//  ************************************************
//  *  BASIC USAGE EXAMPLE: MONO IN => STEREO OUT  *
//  ************************************************
//
//  // initialisation
//  BMStaticDelay sd;
//  BMStaticDelayInit(1,         // mono in
//                    2,         // stereo out
//                    0.400f,    // 400 milliseconds of delay
//                    2.0f,      // RT60 decay time of 2 seconds
//                    0.10f,     // 10% wet
//                    0.9f,      // 90% cross stereo mixing
//                    2000.0f,   // wet signal lowpass cutoff at 2000Hz
//                    48000.0f); // 48khz sampling rate
//
//  // audio processing in render callback
//  BMStaticDelay_processBufferMonoToStereo(sd,
//                                          inputBufferMono,
//                                          outputBufferRight,
//                                          outputBufferLeft,
//                                          numFrames);
//
//
//  Created by Hans on 5/7/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMStaticDelay_h
#define BMStaticDelay_h

#ifdef __cplusplus
extern "C" {
#endif
    
    
#include <stdio.h>
#include "TPCircularBuffer+AudioBufferList.h"
#include "BM2x2Matrix.h"
#include "BMMultiLevelBiquad.h"
#include "BMQuadratureOscillator.h"
#include "BMSmoothGain.h"
    
#define Filter_LP_Level 0
#define Filter_HP_Level 1
#define Filter_Count 2
    
#define BMSD_INIT_COMPLETED 9913156
    
    
    typedef struct BMStaticDelay {
        TPCircularBuffer bufferL, bufferR;
        size_t numChannelsIn, numChannelsOut;
        int delayTimeSamples;
        float sampleRate, feedbackGain, delayTimeInSeconds, crossMixAngle, crossMixAmount, lowpassFC, decayTime, lfoFreq, highpassFC;
        bool updateLFO;
        float *feedbackBufferL, *feedbackBufferR, *tempBufferL, *tempBufferR, *LFOBufferR, *LFOBufferL;
        BM2x2Matrix feedbackMatrix;
        BMMultiLevelBiquad filter;
        BMQuadratureOscillator qosc;
        BMSmoothGain wetGain, dryGain;
        uint32_t initComplete;
        bool delayTimeNeedsUpdate;
    } BMStaticDelay;
    
    
    
    /*
     * @param numChannelsIn  1:mono, 2:stereo
     * @param numChannelsOut 1:mono, 2:stereo
     * @param delayTime delay time in seconds
     * @param decayTime Time for feedback to decay from unity gain to -60db. Set 0 for no feedback.
     * @param wetGainDb sets the gain level of the first echo out of the delay
     * @param crossMixAmount In [0,1]. Set to 0 for no cross mix, 1 for 100% cross, 0.5 for even mixing.
     * @param lowpassFC The cutoff frequency of a 12db lowpass filter on the delay signal output
     * @param highpassFC The cutoff frequency of a 12db highpass filter on the delay signal output
     * @param sampleRate
     */
    void BMStaticDelay_init(BMStaticDelay* This,
                            size_t numChannelsIn, size_t numChannelsOut,
                            float delayTime,
                            float decayTime,
                            float wetGainDb,
                            float crossMixAmount,
                            float lowpassFC,
                            float highpassFC,
                            float sampleRate);
    
    void BMStaticDelay_destroy(BMStaticDelay* This);
    
    
    
    /*!
     *BMStaticDelay_processBufferStereo
     *
     */
    void BMStaticDelay_processBufferStereo(BMStaticDelay* This,
                                           const float* inL, const float* inR,
                                           float* outL, float* outR,
                                           int numFrames);
    
    
    /*!
     * BMStaticDelay_setFeedbackCrossMix
     *
     * @abstract sets the amount of mixing from left channel to right and right channel to left
     *
     * @param amount In [0,1]. Set to 0 for no cross mix, 1 for 100% cross, 0.5 for even mixing.
     */
    void BMStaticDelay_setFeedbackCrossMix(BMStaticDelay* This, float amount);
    
    
    
    /*!
     *BMStaticDelay_setRT60DecayTime
     *
     * @abstract Sets the time for the gain of the feedback signal to decay from unity down to -60db
     */
    void BMStaticDelay_setRT60DecayTime(BMStaticDelay* This, float rt60DecayTime);
    
    
    
    /*!
     *BMStaticDelay_setWetGain
     *
     * @abstract sets the wet mixture
     *
     * @param gainDb  sets the gain of the wet signal in dB
     */
    void BMStaticDelay_setWetGain(BMStaticDelay* This, float gainDb);
    
    
    
    /*!
     *BMStaticDelay_setLowpassFc
     *
     * @param fc The cutoff frequency of a 12db lowpass filter on the delay signal output
     */
    void BMStaticDelay_setLowpassFc(BMStaticDelay* This, float fc);
    
    
    
    /*!
     *BMStaticDelay_setHighpassFc
     *
     * @param fc The cutoff frequency of a 12db highpass filter on the delay signal output
     */
    void BMStaticDelay_setHighpassFc(BMStaticDelay* This, float fc);
    
    
    
    /*!
     *BMStaticDelay_setDelayTime
     *
     * @abstract set the delay time. The delay time must be updated on the audio thread and this will cause a click.  Calling this function queues the update.  The update will take effect next time the process function is called.
     *
     * @param timeInSeconds delay time in seconds
     */
    void BMStaticDelay_setDelayTime(BMStaticDelay* This, float timeInSeconds);
    
    void BMStaticDelay_setBypass(BMStaticDelay* This, size_t level);
    
    void BMStaticDelay_setDryGain(BMStaticDelay* This, float gainDb);
    
#ifdef __cplusplus
}
#endif

#endif /* BMStaticDelay_h */
