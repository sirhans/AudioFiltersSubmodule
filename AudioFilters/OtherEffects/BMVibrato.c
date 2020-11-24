//
//  BMVibrato.c
//  BMAudioFilters
//
//  Created by Hans on 17/6/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif
    
#include "BMVibrato.h"
#include <sys/time.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <stdlib.h>
    
    
    void BMVB_init(BMVibrato* v,
                   float centreRate_hz_start,
                   float centreRate_hz_end,
                   float centreWidth_notes,
                   float modRate_hz,
                   float fModDepth_0to1,
                   float wModDepth_0to1,
                   float fadeInTime_seconds,
                   float fadeOutTime_seconds,
                   float fadeOutMinDepth,
                   float sampleRate){
        
        v->LFO1Phase = 0.0f;
        v->LFO2Phase = 0.0f;
        v->centerRateStart = 2.0*M_PI*centreRate_hz_start;
        v->centerRate = v->centerRateStart;
        v->centerRateEnd = 2.0*M_PI*centreRate_hz_end;
        v->centreWidth_notes = centreWidth_notes;
        v->modRate = 2.0*M_PI*modRate_hz;
        v->fModDepth = fModDepth_0to1;
        v->wModDepth = wModDepth_0to1;
        v->fadeInDepth  = 0.0f;
        v->fadeInTime = fadeInTime_seconds;
        v->fadeOutDepth = 1.0f;
        v->fadeOutMinDepth = fadeOutMinDepth;
        v->sampleRate = sampleRate;
        v->timeSinceStart_seconds = 0.0f;
        v->bufferSize_seconds = 0.0f;
        v->widthScale = 1.0f;
        
        // solve for the fade in rate to get the specified fade in time
        //
        // define the smallest increment in 16 bit audio
        //#define BMVB_DELTA 0.000030517578125 // 2^(-15)
        //
        // BMVB_DELTA*2^(fadeInRate*fadeInTime) == 1
        // 1/BMVB_DELTA == 2^(fadeInRate*fadeInTime)
        // log2(1/BMVB_DELTA) / fadeInTime == fadeInRate
        // 15 / fadeInTime == fadeInRate
        if(fadeInTime_seconds > 0.0f)
            v->fadeInRate = 15.0f / fadeInTime_seconds;
        else
            v->fadeInRate = FLT_MAX;
        
        
        // solve for the fade in rate to get the specified fade in time
        //
        // 2^(fadeOutRate*fadeOutTime) = fadeOutMinDepth
        // fadeOutRate*fadeOutTime = log2(fadeOutMinDepth)
        // fadeOutRate = log2(fadeOutMinDepth)/fadeOutTime;
        if(fadeOutTime_seconds > 0.0f)
            v->fadeOutRate = log2f(fadeOutMinDepth)/fadeOutTime_seconds;
        else
            v->fadeOutRate = FLT_MAX;
        
        
        // find the range between pre-fade out and post fade out
        v->fadeOutRange = 1.0 - v->fadeOutMinDepth;
     
        
        // init temp variables
        BMVB_newNote(v);
    }
    
    
    
    
    void BMVB_initDefaultBass(BMVibrato* v, float sampleRate) {
        BMVB_init(v,
                  6.0,        // main LFO frequency at start
                  2.5,        // main LFO frequency at end
                  0.2,        // main LFO width in semitones
                  2.0/11.0,   // mod LFO frequency
                  0.1,        // frequency mod depth
                  0.1,        // amplitude mod depth
                  0.0,        // fade in time
                  1.0,        // fade out time
                  0.1,        // min fade out depth
                  sampleRate);
    }
    
    
    
    
    
    void BMVB_initDefaultSax(BMVibrato* v, float sampleRate) {
        BMVB_init(v,
                  4.5,        // main LFO frequency at start
                  4.5,        // main LFO frequency at end
                  0.2,        // main LFO width in semitones
                  2.0/11.0,   // mod LFO frequency
                  0.1,        // frequency mod depth
                  0.1,        // amplitude mod depth
                  0.75,       // fade in time
                  0.0,        // fade out time
                  1.0,        // min fade out depth (don't fade out)
                  sampleRate);
    }
    
    
    
    
    
    
    /*
     * Uses a primary LFO to do vibrato and a secondary LFO to modulate the
     * depth and rate of the primary LFO.
     *
     * Returns a value that can be multiplied directly by the stride when copying
     * from audio file to audio buffer to produce a frequency only vibrato.
     */
    float BMVB_getStrideMultiplier(BMVibrato* v, size_t bufferSize_samples){
        
        // if we haven't finished fading in yet, update the fade depth
        //
        // fadeInDepth == 2^(-15) * 2^(fadeInRate * timeSinceStart)
        // fadeInDepth == 2^(fadeInRate * timeSinceStart - 15.0)
        if (v->fadeInDepth < 1.0){
            // if the fade in rate is at maximum, go directly to full depth
            if(v->fadeInRate == FLT_MAX)
                v->fadeInDepth = 1.0;
            else {
                v->fadeInDepth = powf(2.0, v->fadeInRate *
                                      v->timeSinceStart_seconds -
                                      15.0);
                // clamp the fade at a maximum of 1.0
                if (v->fadeInDepth > 1.0) v->fadeInDepth = 1.0;
            }
        }
        
        // when done fading in consider fading out,
        else {
            // if not done fading out,
            if (v->fadeOutDepth > v->fadeOutMinDepth) {
                // if the fade out rate is at maximum, go directly to min depth
                if(v->fadeOutRate == FLT_MAX)
                    v->fadeOutDepth = v->fadeOutMinDepth;
                // otherwise do the fade normally
                else
                    v->fadeOutDepth = powf(2.0, v->fadeOutRate * (v->timeSinceStart_seconds - v->fadeInTime));
                
                // clamp the fade at the min
                if (v->fadeOutDepth < v->fadeOutMinDepth)
                    v->fadeOutDepth = v->fadeOutMinDepth;
            }
        }
        
        // fade the LF01 frequency according to the fadeOutDepth
        //
        // if there is a fade out
        if (v->fadeOutRange > 0.0001){
            // how far along in the fade are we?
            float fadeOutProgress = 1.0 - (v->fadeOutDepth - v->fadeOutMinDepth)/v->fadeOutRange;
            // adjust the centre rate proportional to the progress of the fade
            v->centerRate = v->centerRateStart + (v->centerRateEnd - v->centerRateStart)*fadeOutProgress;
        }
        
        // update the phase of LFO2
        float LFO2PhaseIncrement = v->modRate * v->bufferSize_seconds;
        v->LFO2Phase += LFO2PhaseIncrement;
        
        
        // get the value of LFO2
        float LFO2Val = sinf(v->LFO2Phase);
        
        
        // update the phase of LFO1
        float LFO1Rate = v->centerRate * (1.0 + (LFO2Val * v->fModDepth));
        float LFO1PhaseIncrement = LFO1Rate * v->bufferSize_seconds;
        v->LFO1Phase += LFO1PhaseIncrement;
        
        
        // get the current depth of LFO1
        float LFO1Depth = (1.0 - LFO2Val*v->wModDepth)*v->widthScale*v->fadeInDepth*v->fadeOutDepth;
        
        
        // get the value of LFO1
        float LFO1Val = sinf(v->LFO1Phase) * LFO1Depth;
        
        
        // get the time length for the current buffer
        v->bufferSize_seconds = (float)bufferSize_samples / v->sampleRate;
        
        
        // update the clock for the current note
        v->timeSinceStart_seconds += v->bufferSize_seconds;
        
        
        // return a value that can be multiplied directly with the stride
        // when copying from audio file to audio buffer
        return powf(2.0,(LFO1Val*v->centreWidth_notes)/12.0);
    }
    
    
    
    
    
    void BMVB_setWidthScale(BMVibrato* v, float scale){
        //assert(fabs(scale) <= 1.0f);
        v->widthScale = scale;
    }
    
    
    
    
    void BMVB_newNote(BMVibrato* v){
        v->fadeInDepth = 0.0;
        v->fadeOutDepth = 1.0;
        v->timeSinceStart_seconds = 0.0;
        v->LFO1Phase=2.0*M_PI*(double)arc4random()/UINT32_MAX;
        v->LFO2Phase=2.0*M_PI*(double)arc4random()/UINT32_MAX;
    }
    
#ifdef __cplusplus
}
#endif
