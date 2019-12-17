//
//  BMTremolo.h
//  BMAudioFilters
//
//  Created by Hans on 18/7/17.

//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//
//
//  Typical example usage:
//
//  float rate = 4.5f;  // tremolo rate in hz
//  float width = 3.0f; // +-3db variation in gain
//  float modRate = 2.0/11   // speed of secondary LFO in hz
//  float fModDepth = 0.1f;  // 10% modulation of primary LFO frequency
//  float wModDepth = 0.1f;  // 10% modulation of primary LFO width
//  float fadeTime = 0.75f;  // to full width 0.75 second after note start
//  float sampleRate = 44100.0f // sample rate at 44.1 khz
//
//   BMTremolo_init(BMTremolo *This,
//                  rate,
//                  width,
//                  float modRate,
//                  float fModDepth,
//                  float wModDepth,
//                  float fadeTime,
//                  float sampleRate);
//

#ifndef BMTremolo_h
#define BMTremolo_h

#ifdef __cplusplus
extern "C" {
#endif

#include "BMVibrato.h"
    
    typedef struct BMTremolo {
        BMVibrato state;
        float previousGain;
    } BMTremolo;
    
    
    
    /*
     * Uses a primary LFO to do tremolo and a secondary LFO to modulate the
     * depth and rate of the primary LFO.
     *
     * @param rate_hz - primary LFO speed in Hz
     * @param width_dB - primary LFO width in decibels
     * @param modRate_hz - modulation rate for both rate and depth
     * @param fModDepth_0to1 - tremolo frequency modulation depth in the range 0 to 1
     * @param wModDepth_0to1 - tremolo width modulation depth in the range 0 to 1
     * @param fadeInTime_seconds - starting from zero tremolo, how long before full width?
     * @param sampleRate - audio system sample rate
     */
    void BMTremolo_init(BMTremolo *This,
                    float rate_hz,
                    float width_dB,
                    float modRate_hz,
                    float fModDepth_0to1,
                    float wModDepth_0to1,
                    float fadeInTime_seconds,
                    float sampleRate);

    
    
    /*
     * Call this function before each new note to reset the fade
     */
    void BMTremolo_newNote(BMTremolo *This);

    
    
    
    /*
     * applies the tremolo directly to an audio buffer
     *
     * @param input   buffer of audio input samples
     * @param output  buffer of audio output samples, input == output is OK
     * @param numSamples length of input, output
     */
    void BMTremolo_processAudioMono(BMTremolo *This,
                                    float* input,
                                    float* output,
                                    size_t numSamples);
    
    
    
    /*
     * applies the tremolo directly to an audio buffer
     *
     * @param inL   left channel of buffer of audio input samples
     * @param inR   right channel of buffer of audio input samples
     * @param outL  left channel of output, input == output is OK
     * @param outR  right channel of output, input == output is OK
     * @param numSamples   length of inL, inR, outL, and outR
     */
    void BMTremolo_processAudioStereo(BMTremolo *This,
                                    float* inL, float* inR,
                                    float* outL, float* outR,
                                    size_t numSamples);
    
    
    /*
     * @param scale In [0,1]. Adjusts the width (actual witdh = wModDepth_0to1*scale)
     *
      *This is used for implementation of modulation wheel controllers
     */
    void BMTremolo_setWidthScale(BMTremolo *This, float scale);
    
    
#ifdef __cplusplus
}
#endif

#endif /* BMTremolo_h */
