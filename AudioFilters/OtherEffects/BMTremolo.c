//
//  BMTremolo.c
//  BMAudioFilters
//
//  Created by Hans on 18/7/17.

//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//


#ifdef __cplusplus
extern "C" {
#endif
    
#include "BMTremolo.h"
#include <Accelerate/Accelerate.h>
    
    /*
     * Uses a primary LFO to do tremolo and a secondary LFO to modulate the
     * depth and rate of the primary LFO.
     *
     * rate_hz: primary LFO speed in Hz
     * width_dB: primary LFO width in decibels
     * modRate_hz: modulation rate for both rate and depth
     * fModDepth_0to1: tremolo frequency modulation depth in the range 0 to 1
     * wModDepth_0to1: tremolo width modulation depth in the range 0 to 1
     * fadeInTime_seconds: starting from zero tremolo, how long before full width?
     * sampleRate: audio system sample rate
     */
    void BMTremolo_init(BMTremolo* This,
                        float rate_hz,
                        float width_dB,
                        float modRate_hz,
                        float fModDepth_0to1,
                        float wModDepth_0to1,
                        float fadeInTime_seconds,
                        float sampleRate){
        
        
        
        
        // convert the tremolo width in dB units to units of midi notes
        // * Yes, this is wierd, but if we want to reuse the vibrato class
        // * for tremolo then it's required.
        //
        // A change in frequency of one n midi notes is achieved by
        // multiplying the frequency by 2^(n/12). A multiplicative change
        // of this amount corresponds to how many decibels?
        //
        // A change of x decibels is a linear scale change given by,
        //     linearMultiplier == 10^(dbScaleChange/20)
        //
        // We solve for n, the number of midi notes:
        //   2^(n/12) == linearMultiplier
        //     n/12   == log_2(linearMultiplier)
        //      n     == 12 * log_2(linearMultiplier)
        float linearMultiplier = powf(10.0f,width_dB/20.0f);
        float centreWidth_notes = 12.0f * log2(linearMultiplier);
        
        
        
        
        // solve for the fade in rate to get the specified fade in time
        //
        // define the smallest increment in 16 bit audio
        //#define BMVB_DELTA 0.000030517578125 // 2^(-15)
        //
        // BMVB_DELTA*2^(fadeInRate*fadeInTime) == 1
        // 1/BMVB_DELTA == 2^(fadeInRate*fadeInTime)
        // log2(1/BMVB_DELTA) / fadeInTime == fadeInRate
        // 15 / fadeInTime == fadeInRate
        //if(fadeInTime_seconds > 0.0f)
        //This->state.fadeInRate = 15.0f / fadeInTime_seconds;
        //else
        //This->state.fadeInRate = FLT_MAX;
        
        
        
        // init the vibrato struct
        BMVB_init(&This->state,
                  rate_hz,
                  centreWidth_notes,
                  modRate_hz,
                  fModDepth_0to1,
                  wModDepth_0to1,
                  fadeInTime_seconds,
                  sampleRate);
        
        
        
        // to get a smooth fade in volume between frames, we save
        // the end gain of the previous frame at each update.
        // at startup, the gain should be 1.
        This->previousGain = 1.0;
    }
    
    
    
    /*
     * Call this function before each new note to reset the fade
     */
    void BMTremolo_newNote(BMTremolo* This){
        BMVB_newNote(&This->state);
        This->previousGain = 1.0;
    }
    
    
    
    
    /*
     * applies the tremolo directly to an audio buffer
     *
     * @param input   buffer of audio input samples
     * @param output  buffer of audio output samples, input == output is OK
     * @param numSamples length of input, output
     */
    void BMTremolo_processAudioMono(BMTremolo* This,
                                    float* input,
                                    float* output,
                                    size_t numSamples){
        
        // get start and end gain for the tremolo effect fade
        float startGain = This->previousGain;
        float endGain = BMVB_getStrideMultiplier(&This->state,
                                                 numSamples);
        float increment =
        (endGain - startGain) / (float) numSamples;
        
        // multiply the input by a ramp from startGain to endGain
        vDSP_vrampmul(input, 1,
                      &startGain,
                      &increment,
                      output, 1,
                      numSamples);
        
        // update the previous gain so we start out in the right place
        // for the next buffer
        This->previousGain = endGain;
    }
    
    
    
    
    /*
     * applies the tremolo directly to an audio buffer
     *
     * @param inL   left channel of buffer of audio input samples
     * @param inR   right channel of buffer of audio input samples
     * @param outL  left channel of output, input == output is OK
     * @param outR  right channel of output, input == output is OK
     * @param numSamples   length of inL, inR, outL, and outR
     */
    void BMTremolo_processAudioStereo(BMTremolo* This,
                                      float* inL, float* inR,
                                      float* outL, float* outR,
                                      size_t numSamples){
        
        // get start and end gain for the tremolo effect fade
        float startGain = This->previousGain;
        float endGain = BMVB_getStrideMultiplier(&This->state,
                                                 numSamples);
        float increment =
        (endGain - startGain) / (float) numSamples;
        
        // multiply the input by a ramp from startGain to endGain
        vDSP_vrampmul2(inL, inR, 1,
                       &startGain,
                       &increment,
                       outL, outR, 1,
                       numSamples);
        
        
        // update the previous gain so we start out in the right place
        // for the next buffer
        This->previousGain = endGain;
    }
    
    
    
    /*
     * @param scale In [0,1]. Adjusts the width (actual witdh = wModDepth_0to1*scale)
     *
     * This is used for implementation of modulation wheel controllers
     */
    void BMTremolo_setWidthScale(BMTremolo* This, float scale){
        BMVB_setWidthScale(&This->state, scale);
    }
    
    
    
#ifdef __cplusplus
}
#endif
