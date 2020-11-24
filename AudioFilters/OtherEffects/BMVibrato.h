//
//  BMvibrato.h
//  BMAudioFilters
//
//  Created by Hans on 17/6/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMMidiVibrato_h
#define BMMidiVibrato_h

#ifdef __cplusplus
extern "C" {
#endif
    
#define BMVB_MAX_CHANNELS 16
    
#include <stdio.h>
#include <stdbool.h>
    
    typedef struct BMVibrato {
        float LFO1Phase,
        LFO2Phase,
        centerRate,
        centerRateStart,
        centerRateEnd,
        modRate,
        fModDepth,
        wModDepth,
        widthScale,
        desiredWidthScale,
        fadeInRate,
        fadeInDepth,
        fadeInTime,
        fadeOutRate,
        fadeOutDepth,
        fadeOutRange,
        fadeOutMinDepth,
        sampleRate,
        centreWidth_notes,
        timeSinceStart_seconds,
        bufferSize_seconds;
        bool needsUpdate;
    } BMVibrato;
    
    
    
    /*
     * Uses a primary LFO to do vibrato and a secondary LFO to modulate the
     * depth and rate of the primary LFO.
     *
     * centreRate_hz: primary LFO speed in Hz
     * centreWidth_notes: primary LFO width in # of MIDI notes pitch bend
     * modRate_hz: modulation rate for both rate and depth
     * fModDepth_0to1: vibrato frequency modulation depth in the range 0 to 1
     * wModDepth_0to1: vibrato width modulation depth in the range 0 to 1
     * fadeInTime_seconds: starting from zero vibrato, how long before full width?
     * fadeOutTime_seconds: after the vibrato reaches full strength, how quickly does it fade?
     * sampleRate: audio system sample rate
     */
    void BMVB_init(BMVibrato* v,
                   float centreRate_hz_start,
                   float centreRate_hz_end,
                   float centreWidth_notes,
                   float modRate_hz,
                   float fModDepth_0to1,
                   float wModDepth_0to1,
                   float fadeInTime_seconds,
                   float fadeOutTine_seconds,
                   float fadeOutMinDepth,
                   float sampleRate);
    
    
    
    /*
     * init with good default settings for saxophone
     */
    void BMVB_initDefaultSax(BMVibrato* v, float sampleRate);
    
    
    
    /*
     * init with good default settings for bass
     */
    void BMVB_initDefaultBass(BMVibrato* v, float sampleRate);
    
    
    
    /*
     * call this before each new note to reset the fade in back to zero
     */
    void BMVB_newNote(BMVibrato* v);
    
    
    
    
    /*
     * Uses a primary LFO to do vibrato and a secondary LFO to modulate the
     * depth and rate of the primary LFO.
     *
     * Returns a value that can be multiplied directly by the stride when copying
     * from audio file to audio buffer to produce a frequency only vibrato.
     *
     * Monitors the size of the buffer to count time.
     */
    float BMVB_getStrideMultiplier(BMVibrato* v, size_t bufferSize_samples);
    
    
    
    
    /*
     * @param scale In [0,1]. Adjusts the width (actual witdh = wModDepth_0to1*scale)
     *
     * This is used for implementation of modulation wheel controllers
     */
    void BMVB_setWidthScale(BMVibrato* v, float scale);
    
    
    
    
#ifdef __cplusplus
}
#endif


#endif /* BMMidiVibrato_h */
