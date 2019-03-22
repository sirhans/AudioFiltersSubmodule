//
//  BMGainStage.h
//  BMAudioFilters
//
//  A gain stage has three components:
//    1. a gain boost
//    2. a slow limiter that emulates a vacuum tube
//    3. a lowpass filter for anti-aliasing the limiter output
//
//  BMGainStage responds well to input dynamics, giving harsher tone as you
//  drive the input harder, provided the input stays in the range [-1,1].
//  You may drive it harder to get more extreme distortion but then, of course,
//  the response will be less dynamic and may require oversampling to reduce
//  aliasing noise.
//
//  What is slow-limiting? A slow limiter attempts to put a hard limit on the
//  output signal amplitude, but makes changes to the gain reduction slowly,
//  rather than responding immediately to spikes in input amplitude. On an
//  oscilloscope, the response to a sine wave is higher on the leading edge and
//  lower on the trailing edge of the wave peaks. It produces a heavy
//  distortion at low frequencies but at high frequencies, we get a simple gain
//  reduction, with very little distortion at all.
//
//  BMGainStage is designed to be cascaded in series to produce harder clipping
//  with reduced aliasing.  When cascading gain stages, keep the input volume
//  strictly in [-1,1] and set the gain of all stages in the cascade to the same
//  value.  This evenly distributes the distortion over all stages in the
//  cascade, which is important for reducing aliasing.  If one of the gain
//  stages in a cascade is driven harder at the input than the others, that one
//  will produce more aliasing than the others, hence you need a higher sample
//  rate to avoid aliasing noise.  By keeping the input gain below 1 and
//  applying equal gain boost to all stages in the cascade, you ensure that each
//  gain stage contributes the same amount of aliasing noise, hence reducing the
//  need for oversampling.
//
//  If used in a series of FOUR gain stages, we can get 2^8 gain boost without
//  noticeable aliasing noise on guitar sounds, no need for oversampling.
//
//    (4x gain boost x 4 gain stages = 4^4 = 2^8)
//
//  Created by Hans on 2/8/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//
#ifndef BMGainStage_h
#define BMGainStage_h

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdio.h>
#include "BMMultiLevelBiquad.h"
#include "BMDCBlocker.h"
    
#define BMGAINSTAGE_AAFILTER_NUMLEVELS 4
    
    /*
     * TUBE type is a nonlinearity with state. Its shape is time-dependent so
     * the effect changes depending on frequency.
     *
     * ASYMPTOTIC type is a soft nonlinerity without state. Since the output
     * depends only on the value of one sample, the distortion of the wave
     * shape is not dependent on frequency.
     */
    typedef enum {
        BM_AMP_TYPE_DUAL_RES_TUBE,
        BM_AMP_TYPE_SINGLE_RES_TUBE,
        BM_AMP_TYPE_ASYMPTOTIC,
        BM_AMP_TYPE_LOWPASS_CLIP_DIFFERENCE
    } BMNonlinearityType ;
    
    typedef struct BMGainStage {
        float cPos,cNeg;              // charge reservoirs
        float t,tInv;                 // tightness: the capacity of the charge reservoirs
        float gain;                   // input gain
        float bias;                   // input bias
        float biasRatio;              // bias / gain in [0,1]
        float aaFc;                   // cutoff freq. of anti-alias filter
        float sampleRate;
        float* tempBuffer;
        BMNonlinearityType ampType;   // what type of nonlinearity to use
        BMMultiLevelBiquad aaFilter;  // anti-alias filter
        BMMultiLevelBiquad clipLP;    // filter for lowpass-ing the clipped signal
        BMDCBlocker dcFilter;         // DC blocking filter
    } BMGainStage;
    
    
    /*!
     * Setup a new gainstage object
     *
     * @param This        Pointer to the uninitialised gain stage
     * @param sampleRate  The sample rate of the input / output
     * @param AAFilterFc  cutoff frequency of antialiasing filter
     *                    for guitar, set this to 7000 hz.
     *                    for full frequency range set it to 20khz
     */
    void BMGainStage_init(BMGainStage* This,
                          float sampleRate,
                          float AAFilterFc);
    
    
    
    void BMGainStage_destroy(BMGainStage* This);
    
    
    
    /*!
     * Set the speed of limiting response
     *
     * @param This       Pointer to an initialised gain stage
     * @param tightness  In [0,1]. 0 = saggyTube; 1 = hardTransistor
     *
     */
    void BMGainStage_setTightness(BMGainStage* This, float tightness);
    
    
    
    /*!
     * Set the input gain
     *
     * @param This       Pointer to an initialised gain stage
     * @param gain       Input gain coefficient. Note that BMGainStage boosts at
     *                   the input but then squeezes output back down to [-2,2].
     *
     */
    void BMGainStage_setGain(BMGainStage* This, float gain);
    
    
    
    /*!
     * BMGainStage_setGain
     *
     * @param This       Pointer to an initialised gain stage
     * @param gain       Input gain coefficient.
     *
     */
    void BMGainStage_setGain(BMGainStage* This, float gain);
    
    
    
    
    
    
    /*!
     * BMGainStage_setBias
     *
     * @param This       Pointer to an initialised gain stage
     * @param biasRatio  biasRatio is bias / gain. Value should be in [0,1]
     *
     */
    void BMGainStage_setBiasRatio(BMGainStage* This, float biasRatio);
    
    
    
    
    /*!
     * BMGainStage_setAmpType
     *
     * @param This       Pointer to an initialised gain stage
     * @param type       BM_AMP_TYPE_TUBE | BM_AMP_TYPE_ASYMPTOTIC
     *
     */
    void BMGainStage_setAmpType(BMGainStage* This, BMNonlinearityType type);
    
    
    
    
    /*!
     * Set the cutoff frequency of the anti-aliasing lowpass filter
     *
     * @param This       Pointer to an initialised gain stage
     * @param fc         Anti-alias lowpass filter cutoff
     *
     */
    void BMGainStage_setAACutoff(BMGainStage* This, float fc);
    
    
    
    /*!
     * Set the audio input/output sample rate
     *
     * @param This       Pointer to an initialised gain stage
     * @param sampleRate The sample rate of the input/output
     *
     */
    void BMGainStage_setSampleRate(BMGainStage* This, float sampleRate);
    
    
    
    /*!
     * Process this gainstage on a buffer of audio samples. This function does
     * not work in place ( input != output).
     *
     * @param This    Pointer to an initialised gain stage
     * @param input   buffer of input samples
     * @param output  buffer of output samples
     * @param length  number of samples to process
     *                All buffers must be at least this long.
     *
     */
    void BMGainStage_processBufferMono(BMGainStage* This,
                                       float* input,
                                       float* output,
                                       size_t length);
    
    //    void BMGainStage_processAA(BMGainStage* This,
    //                               float* input,
    //                               float* output,
    //                               size_t numSamples);
    
#ifdef __cplusplus
}
#endif

#endif /* BMGainStage_h */
