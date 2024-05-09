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
    
#define BM_WETDRYMIXER_SMALL_CHUNK_SIZE 128
    
    
    
    void BMWetDryMixer_init(BMWetDryMixer *This, float sampleRate){
        This->wetMix = This->mixTarget = 1.0f;
        This->inTransition = false;
        
        // set the per sample difference to fade from 0 to 1 in *time*
        float time = 1.0f / 2.0f;
        This->perSampleDifference = 1.0f / (sampleRate * time);
    }
    
    
    
    void BMWetDryMixer_processBufferInPhase(BMWetDryMixer *This,
                                                float* inputWetL, float* inputWetR,
                                                float* inputDryL, float* inputDryR,
                                                float* outputL, float* outputR,
                                                size_t numSamples){
        // the dry input can not be in place with the output
        assert(inputDryL != outputL && inputDryR != outputR);
        
        // if we are now transitioning to a new gain setting do the crossfade
        if(This->inTransition){
            
            // how much do we have to change the mix control to reach the target?
            float error = This->mixTarget - This->wetMix;
            
            // how much will we move with each sample?
            float perSampleDifference = error > 0.0f ? This->perSampleDifference : -This->perSampleDifference;
            
            // how many samples will it take us to get there?
            size_t samplesTillTarget = error / perSampleDifference;
            
            // how many samples can we process right now?
            size_t samplesFading = BM_MIN(samplesTillTarget,numSamples);
            
            // fade the wet input into the output
            float wetMixCopy = This->wetMix; // vrampMul modifies wetMix, so we make a backup
            vDSP_vrampmul(inputWetL, 1, &wetMixCopy, &perSampleDifference, outputL, 1, samplesFading);
            vDSP_vrampmul(inputWetR, 1, &This->wetMix, &perSampleDifference, outputR, 1, samplesFading);
            
            // compute the dry mix at the end of the fade
            float newDryMix = 1.0f - This->wetMix;
            
            // fade the dry input, buffering onto itself
            perSampleDifference = (newDryMix - This->dryMix) / samplesFading;
            float dryMixCopy = This->dryMix; // vrampmul modifies dryMix so we make a backup
            vDSP_vrampmul(inputDryL, 1, &dryMixCopy, &perSampleDifference, inputDryL, 1, samplesFading);
            vDSP_vrampmul(inputDryR, 1, &This->dryMix, &perSampleDifference, inputDryR, 1, samplesFading);
            
            // mix the dry and wet inputs
            vDSP_vadd(inputDryL, 1, outputL, 1, outputL, 1, samplesFading);
            vDSP_vadd(inputDryR, 1, outputR, 1, outputR, 1, samplesFading);
            
            // exit the transition state if we are within the tolerance limit
            if(fabsf(This->wetMix - This->mixTarget) <= This->perSampleDifference){
                This->inTransition = false;
                This->wetMix = This->mixTarget;
                This->dryMix = 1.0f - This->mixTarget;
            }
            
            // if there are still samples left to be copied, finish them up
            if (samplesFading < numSamples) {
                size_t samplesLeft = numSamples - samplesFading;
                vDSP_vsmsma(inputWetL+samplesFading, 1, &This->wetMix, inputDryL+samplesFading, 1, &This->dryMix, outputL+samplesFading, 1, samplesLeft);
                vDSP_vsmsma(inputWetR+samplesFading, 1, &This->wetMix, inputDryR+samplesFading, 1, &This->dryMix, outputR+samplesFading, 1, samplesLeft);
            }
        }
        
        // if we aren't in the transition state, apply the mix by simple multiply and add
        else {
            vDSP_vsmsma(inputWetL, 1, &This->wetMix, inputDryL, 1, &This->dryMix, outputL, 1, numSamples);
            vDSP_vsmsma(inputWetR, 1, &This->wetMix, inputDryR, 1, &This->dryMix, outputR, 1, numSamples);
        }
    }
    
    
    
    
    void BMWetDryMixer_processBufferRandomPhase(BMWetDryMixer *This,
                                     float* inputWetL, float* inputWetR,
                                     float* inputDryL, float* inputDryR,
                                     float* outputL, float* outputR,
                                     size_t numSamples){
        // the dry input can not be in place with the output
        assert(inputDryL != outputL && inputDryR != outputR);
        
        // if we are now transitioning to a new gain setting do the crossfade
        if(This->inTransition){
            
            // process in smaller chunks to get a smoother fade
            while(numSamples > 0){
                size_t numSamplesThisChunk = BM_MIN(numSamples, BM_WETDRYMIXER_SMALL_CHUNK_SIZE);

                // how much do we have to change the mix control to reach the target?
                float error = This->mixTarget - This->wetMix;
                
                // how much will we move with each sample?
                float perSampleDifference = error > 0.0f ? This->perSampleDifference : -This->perSampleDifference;
                
                // how many samples will it take us to get there?
                size_t samplesTillTarget = error / perSampleDifference;
                
                // how many samples can we process right now?
                size_t samplesFading = BM_MIN(samplesTillTarget,numSamplesThisChunk);
                
                // fade the wet input into the output
                float wetMixCopy = This->wetMix; // vrampMul modifies wetMix, so we make a backup
                vDSP_vrampmul(inputWetL, 1, &wetMixCopy, &perSampleDifference, outputL, 1, samplesFading);
                vDSP_vrampmul(inputWetR, 1, &This->wetMix, &perSampleDifference, outputR, 1, samplesFading);
                
                // compute the dry mix at the end of the fade
                float newDryMix = sqrtf(1.0f - This->wetMix*This->wetMix);
                if (isnan(newDryMix)) newDryMix = 0.0f;
                
                // fade the dry input, buffering onto itself
                perSampleDifference = (newDryMix - This->dryMix) / samplesFading;
                float dryMixCopy = This->dryMix; // vrampmul modifies dryMix so we make a backup
                vDSP_vrampmul(inputDryL, 1, &dryMixCopy, &perSampleDifference, inputDryL, 1, samplesFading);
                vDSP_vrampmul(inputDryR, 1, &This->dryMix, &perSampleDifference, inputDryR, 1, samplesFading);
                
                // mix the dry and wet inputs
                vDSP_vadd(inputDryL, 1, outputL, 1, outputL, 1, samplesFading);
                vDSP_vadd(inputDryR, 1, outputR, 1, outputR, 1, samplesFading);
                
                // exit the transition state if we are within the tolerance limit
                if(fabsf(This->wetMix - This->mixTarget) <= This->perSampleDifference){
                    This->inTransition = false;
                    This->wetMix = This->mixTarget;
                    This->dryMix = sqrtf(1.0f - This->mixTarget*This->mixTarget);
                    if (isnan(This->dryMix)) This->dryMix = 0.0f;
                }
                
                // if there are still samples left to be copied, finish them up
                if (samplesFading < numSamplesThisChunk) {
                    size_t samplesLeft = numSamplesThisChunk - samplesFading;
                    vDSP_vsmsma(inputWetL+samplesFading, 1, &This->wetMix, inputDryL+samplesFading, 1, &This->dryMix, outputL+samplesFading, 1, samplesLeft);
                    vDSP_vsmsma(inputWetR+samplesFading, 1, &This->wetMix, inputDryR+samplesFading, 1, &This->dryMix, outputR+samplesFading, 1, samplesLeft);
                }
                
                // advance pointers
                inputDryL += numSamplesThisChunk;
                inputWetL += numSamplesThisChunk;
                inputDryR += numSamplesThisChunk;
                inputWetL += numSamplesThisChunk;
                outputL += numSamplesThisChunk;
                outputR += numSamplesThisChunk;
                numSamples -= numSamplesThisChunk;
            }
        }
        
        // if we aren't in the transition state, apply the mix by simple multiply and add
        else {
            vDSP_vsmsma(inputWetL, 1, &This->wetMix, inputDryL, 1, &This->dryMix, outputL, 1, numSamples);
            vDSP_vsmsma(inputWetR, 1, &This->wetMix, inputDryR, 1, &This->dryMix, outputR, 1, numSamples);
        }
    }
    
    
    
    
    void BMWetDryMixer_setMix(BMWetDryMixer *This, float mix){
        assert(0.0f <= mix && mix <= 1.0f);
        printf("setMix %f",mix);
        // if this is a legitimate gain change, set the new target and switch to
        // transition state
        if (mix != This->mixTarget){
            This->mixTarget = mix;
            This->inTransition = true;
        }
    }
    
    
    
    
    float BMWetDryMixer_getMix(BMWetDryMixer *This){
        return This->mixTarget;
    }
    
    
	
	bool BMWetDryMixer_isDry(BMWetDryMixer *This){
		// return false if the mixer is not in a stable state
		if (This->inTransition) return false;
		
		// return true if the mix target is very near zero
		if (This->mixTarget < BM_DB_TO_GAIN(-70.0))
			return true;
		
		// otherwise return false
		return false;
	}
    
    
#ifdef __cplusplus
}
#endif
