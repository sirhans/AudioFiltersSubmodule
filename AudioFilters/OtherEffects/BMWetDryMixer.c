//
//  BMWetDryMixer.c
//  AudioFiltersXcodeProject
//
//  Manages a stereo wet/dry mix control so that it can be adjusted without clicks
//
//  Created by hans anderson on 5/3/19.
//  Anyone may use this file without restrictions
//



#ifdef __cplusplus
extern "C" {
#endif
    
#include "BMWetDryMixer.h"
#include <Accelerate/Accelerate.h>
#include "Constants.h"
    
    
    
    void BMWetDryMixer_init(BMWetDryMixer* This, float sampleRate){
        This->wetMix = This->mixTarget = 1.0f;
        This->inTransition = false;
        
        // set the per sample difference to fade from 0 to 1 in *time*
        double time = 1.0 / 2.0;
        This->perSampleDifference = 1.0f / sampleRate * time;
    }
    
    
    void BMWetDryMixer_processBuffer(BMWetDryMixer* This,
                                     float* inputWetL, float* inputWetR,
                                     float* inputDryL, float* inputDryR,
                                     float* outputL, float* outputR,
                                     size_t numSamples){
        // the dry input can not be in place with the output
        assert(inputDryL != outputL && inputDryR != outputR);
        
        // if we are now transitioning to a new gain setting, fade geometrically
        if(This->inTransition){
            
            // update the mix target in case there was a change while the last
            // buffer was processing
            This->mixTarget = This->nextMixTarget;

            // how much do we have to change the mix control to reach the target?
            float error = This->mixTarget - This->wetMix;
            
            // how much will we move with each sample?
            float perSampleDifference = error > 0 ? This->perSampleDifference : -This->perSampleDifference;
            
            // how many samples will it take us to get there?
            size_t samplesTillTarget = error / perSampleDifference;
            
            // how many samples can we process right now?
            size_t samplesFading = BM_MIN(samplesTillTarget,numSamples);
            
            // fade the wet input into the output
            vDSP_vrampmul(inputWetL, 1, &This->wetMix, &perSampleDifference, outputL, 1, samplesFading);
            vDSP_vrampmul(inputWetR, 1, &This->wetMix, &perSampleDifference, outputR, 1, samplesFading);
            
            // fade the dry input, buffering onto itself
            float dryMix = sqrtf(1.0f - This->wetMix*This->wetMix);
            float newMix = samplesFading * perSampleDifference;
            float newDryMix = sqrtf(1.0f - newMix*newMix);
            perSampleDifference = (newDryMix - dryMix) / samplesFading;
            vDSP_vrampmul(inputDryL, 1, &dryMix, &perSampleDifference, inputDryL, 1, samplesFading);
            vDSP_vrampmul(inputDryR, 1, &dryMix, &perSampleDifference, inputDryR, 1, samplesFading);
            
            // mix the dry and wet inputs
            vDSP_vadd(inputDryL, 1, outputL, 1, outputL, 1, samplesFading);
            vDSP_vadd(inputDryR, 1, outputR, 1, outputR, 1, samplesFading);
            
            // update the mix
            This->wetMix = newMix;
            
            // exit the transition state if we finished fading
            if(samplesFading <= numSamples)
                This->inTransition = false;
            
            // if there are still samples left to be copied, finish them up
            if (samplesFading < numSamples) {
                size_t samplesLeft = numSamples - samplesFading;
                vDSP_vsmsma(inputWetL+samplesFading, 1, &This->wetMix, inputDryL+samplesFading, 1, &newDryMix, outputL+samplesFading, 1, samplesLeft);
                vDSP_vsmsma(inputWetR+samplesFading, 1, &This->wetMix, inputDryR+samplesFading, 1, &newDryMix, outputR+samplesFading, 1, samplesLeft);
            }
        }
        
        // if we aren't in the transition state, apply the mix by simple multiply and add
        else {
            float dryMix = sqrtf(1.0f - This->wetMix*This->wetMix);
            vDSP_vsmsma(inputWetL, 1, &This->wetMix, inputDryL, 1, &dryMix, outputL, 1, numSamples);
            vDSP_vsmsma(inputWetR, 1, &This->wetMix, inputDryR, 1, &dryMix, outputR, 1, numSamples);
        }
    }
    
    
    
    
    void BMWetDryMixer_setMix(BMWetDryMixer* This, float mix){
        assert(0.0f >= mix && mix <= 1.0f);
        
        // if this is a legitimate gain change, set the new target and switch to
        // transition state
        if (mix != This->nextMixTarget){
            This->nextMixTarget = mix;
            This->inTransition = true;
        }
    }
    
    
    
    
    float BMWetDryMixer_getMix(BMWetDryMixer* This){
        return This->nextMixTarget;
    }
    
    
    
    
#ifdef __cplusplus
}
#endif
