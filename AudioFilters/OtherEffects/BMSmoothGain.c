//
//  BMSmoothGain.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 4/24/19.
//  Anyone may use this file without restrictions of any kind
//



#ifdef __cplusplus
extern "C" {
#endif
    
#include "BMSmoothGain.h"
#include <Accelerate/Accelerate.h>
#include "Constants.h"
    
    // returns the ratio that would affect a gain change of dB if applied
    // geometrically over a period of numSamples
    double getRatio(double dB,double numSamples){
        return pow(BM_DB_TO_GAIN(dB),1.0/numSamples);
    }
    
    
    void BMSmoothGain_init(BMSmoothGain *This, float sampleRate){
        This->gain = This->gainTarget = 1.0f;
        This->inTransition = false;
        
        // set the per sample ratio to fade 60 dB in *time*
        double time = 1.0 / 3.0;
        This->perSampleRatioUp = getRatio(60.0, sampleRate * time);
        
        // set the per sample ratio for fading -60 dB in *time*
        This->perSampleRatioDown = getRatio(-60.0, sampleRate * time);
    }
    
    
    void BMSmoothGain_processBuffer(BMSmoothGain *This,
                                    const float* inputL, const float* inputR,
                                    float* outputL, float* outputR,
                                    size_t numSamples){
        
        // if we are now transitioning to a new gain setting, fade geometrically
        if(This->inTransition){
            
            // update the gain target in case there was a change while the last
            // buffer was processing
            This->gainTarget = This->nextGainTarget;
            
            // what multiplier would get us to the target gain?
            float ratioToTarget = This->gainTarget / This->gain;
            
            // how many samples of multiplication by the perSampleRatio will
            // it take to get us to that multiplier?
            int samplesTillTarget = log2(ratioToTarget) / log2(This->perSampleRatioUp);
            
            // if we are increasing the gain, use the increasing ratio (> 1)
            // otherwise use the decreasing ratio (< 1)
            float perSampleRatio = samplesTillTarget > 0 ? This->perSampleRatioUp : This->perSampleRatioDown;
            
            // samplesTillTarget can be negative here. We only want positive values
            samplesTillTarget = abs(samplesTillTarget);
            
            // process only until we reach the target, or reach the end of the
            // buffer, whichever comes first
            size_t geometricFadeLength = BM_MIN(samplesTillTarget, numSamples);
            
            // fade geometrically, stopping at the sample nearest the target
            size_t i=0;
            while (i < geometricFadeLength) {
                This->gain *= perSampleRatio;
                outputL[i] = inputL[i]  *This->gain;
                outputR[i] = inputR[i]  *This->gain;
                i++;
            }
            
            // in case the geometric series above didn't stop near the target,
            // approach it asymptotically until we get within the tolerance.
            float error = This->gainTarget - This->gain;
            float tolerance = 0.00001;
            while (i < numSamples  &&  error > tolerance){
                This->gain += 0.05f * error;
                outputL[i] = inputL[i]  *This->gain;
                outputR[i] = inputR[i]  *This->gain;
                error = This->gainTarget - This->gain;
                i++;
            }
            
            // if we are finished, exit the transition state
            if(fabs(error) <= tolerance)
                This->inTransition = false;
            
            // if there are still samples left to be copied, finish them up
            if (i < numSamples) {
                size_t samplesLeft = numSamples - i;
                vDSP_vsmul(inputL+i, 1, &This->gain, outputL+i, 1, samplesLeft);
                vDSP_vsmul(inputR+i, 1, &This->gain, outputR+i, 1, samplesLeft);
            }
        }
        
        // if we aren't in transition, apply the gain by simple multiplication
        else {
            // if the gain is doing something, multiply
            if( fabsf(This->gain - 1.0f) > 0.0001){
                vDSP_vsmul(inputL, 1, &This->gain, outputL, 1, numSamples);
                vDSP_vsmul(inputR, 1, &This->gain, outputR, 1, numSamples);
            }
            // otherwise just copy
            else {
                // check that the arrays are not in place; if they are, do nothing
                if (inputL != outputL || inputR != outputR){
                    memcpy(outputL,inputL,sizeof(float)*numSamples);
                    memcpy(outputR,inputR,sizeof(float)*numSamples);
                }
            }
        }
    }
    
    
    
    
     void BMSmoothGain_processBuffers(BMSmoothGain *This,
                                      const float** inputs,
                                      float** outputs,
                                      size_t numChannels,
                                      size_t numSamples){
         
         // if we are now transitioning to a new gain setting, fade geometrically
         if(This->inTransition){
             
             // update the gain target in case there was a change while the last
             // buffer was processing
             This->gainTarget = This->nextGainTarget;
             
             // what multiplier would get us to the target gain?
             float ratioToTarget = This->gainTarget / This->gain;
             
             // how many samples of multiplication by the perSampleRatio will
             // it take to get us to that multiplier?
             int samplesTillTarget = log2(ratioToTarget) / log2(This->perSampleRatioUp);
             
             // if we are increasing the gain, use the increasing ratio (> 1)
             // otherwise use the decreasing ratio (< 1)
             float perSampleRatio = samplesTillTarget > 0 ? This->perSampleRatioUp : This->perSampleRatioDown;
             
             // samplesTillTarget can be negative here. We only want positive values
             samplesTillTarget = abs(samplesTillTarget);
             
             // process only until we reach the target, or reach the end of the
             // buffer, whichever comes first
             size_t geometricFadeLength = BM_MIN(samplesTillTarget, numSamples);
             
             // fade geometrically, stopping at the sample nearest the target
             size_t i=0;
             while (i < geometricFadeLength) {
                 This->gain *= perSampleRatio;
                 for(size_t j=0; j<numChannels; j++)
                     outputs[j][i] = inputs[j][i]  *This->gain;
                 i++;
             }
             
             // in case the geometric series above didn't stop near the target,
             // approach it asymptotically until we get within the tolerance.
             float error = This->gainTarget - This->gain;
             float tolerance = 0.00001;
             while (i < numSamples  &&  error > tolerance){
                 This->gain += 0.05f * error;
                 for(size_t j=0; j<numChannels; j++)
                     outputs[j][i] = inputs[j][i]  *This->gain;
                 error = This->gainTarget - This->gain;
                 i++;
             }
             
             // if we are finished, exit the transition state
             if(fabs(error) <= tolerance)
                 This->inTransition = false;
             
             // if there are still samples left to be copied, finish them up
             if (i < numSamples) {
                 size_t samplesLeft = numSamples - i;
                 for(size_t j=0; j<numChannels; j++)
                     vDSP_vsmul(inputs[j]+i, 1, &This->gain, outputs[j]+i, 1, samplesLeft);
             }
         }
         
         // if we aren't in transition, apply the gain by simple multiplication
         else {
             // if the gain is doing something, multiply
             if( fabsf(This->gain - 1.0f) > 0.0001){
                 for(size_t j=0; j<numChannels; j++)
                     vDSP_vsmul(inputs[j], 1, &This->gain, outputs[j], 1, numSamples);             }
             // otherwise just copy
             else {
                 // check that the arrays are not in place; if they are, do nothing
                 if (inputs != outputs){
                     for(size_t j=0; j<numChannels; j++)
                         memcpy(outputs[j],inputs[j],sizeof(float)*numSamples);
                 }
             }
         }
     }
     
    
    
    
    void BMSmoothGain_processBufferMono(BMSmoothGain *This,
                                        const float* input,
                                        float* output,
                                        size_t numSamples){
        // if we are now transitioning to a new gain setting, fade geometrically
        if(This->inTransition){
            
            // update the gain target in case there was a change while the last
            // buffer was processing
            This->gainTarget = This->nextGainTarget;
            
            // what multiplier would get us to the target gain?
            float ratioToTarget = This->gainTarget / This->gain;
            
            // how many samples of multiplication by the perSampleRatio will
            // it take to get us to that multiplier?
            int samplesTillTarget = log2(ratioToTarget) / log2(This->perSampleRatioUp);
            
            // if we are increasing the gain, use the increasing ratio (> 1)
            // otherwise use the decreasing ratio (< 1)
            float perSampleRatio = samplesTillTarget > 0 ? This->perSampleRatioUp : This->perSampleRatioDown;
            
            // samplesTillTarget can be negative here. We only want positive values
            samplesTillTarget = abs(samplesTillTarget);
            
            // process only until we reach the target, or reach the end of the
            // buffer, whichever comes first
            size_t geometricFadeLength = BM_MIN(samplesTillTarget, numSamples);
            
            // fade geometrically, stopping at the sample nearest the target
            size_t i=0;
            while (i < geometricFadeLength) {
                This->gain *= perSampleRatio;
                output[i] = input[i]  *This->gain;
                i++;
            }
            
            // in case the geometric series above didn't stop near the target,
            // approach it asymptotically until we get within the tolerance.
            float error = This->gainTarget - This->gain;
            float tolerance = 0.00001;
            while (i < numSamples  &&  error > tolerance){
                This->gain += 0.05f * error;
                output[i] = input[i]  *This->gain;
                error = This->gainTarget - This->gain;
                i++;
            }
            
            // if we are finished, exit the transition state
            if(fabs(error) <= tolerance)
                This->inTransition = false;
            
            // if there are still samples left to be copied, finish them up
            if (i < numSamples) {
                size_t samplesLeft = numSamples - i;
                vDSP_vsmul(input+i, 1, &This->gain, output+i, 1, samplesLeft);
            }
        }
        
        // if we aren't in transition, apply the gain by simple multiplication
        else {
            // if the gain is doing something, multiply
            if( fabsf(This->gain - 1.0f) > 0.0001){
                vDSP_vsmul(input, 1, &This->gain, output, 1, numSamples);
            }
            // otherwise just copy
            else {
                // check that the arrays are not in place; if they are, do nothing
                if (input != output){
                    memcpy(output,input,sizeof(float)*numSamples);                }
            }
        }
    }
    
    
    
    
    void BMSmoothGain_setGainDb(BMSmoothGain *This, float gainDb){
        // convert dB scale to linear scale gain
        float gain = BM_DB_TO_GAIN(gainDb);
        
		// turn off the gain completely if the input is FLT_MIN
        if(gainDb == FLT_MIN) gain = BM_DB_TO_GAIN(-110.0f);
        
        // if this is a legitimate gain change, set the new target and switch to
        // transition state
        if (gain != This->nextGainTarget) {
            This->nextGainTarget = gain;
            This->inTransition = true;
        }
    }
	
	
	
	
	/*!
	 * BMSmoothGain_setGainDbInstant
	 *
	 * @abstract This sets the gain immediately without smoothing the transition
	 *
	 * @param This        pointer to an initialized BMSmoothGain struct
	 * @param gainDb      new target gain in decibels to be approached smoothly
	 */
	void BMSmoothGain_setGainDbInstant(BMSmoothGain *This, float gainDb){
		// convert dB scale to linear scale gain
        float gain = BM_DB_TO_GAIN(gainDb);
        
		// turn off the gain completely if the input is FLT_MIN
        if(gainDb == FLT_MIN) gain = 0.0f;
		
        // if this is a legitimate gain change, set the gain immediately and exit the transition state
		This->gain = gain;
		This->nextGainTarget = gain;
		This->inTransition = false;
	}
    
    
    
    
    float BMSmoothGain_getGainDb(BMSmoothGain *This){
        return BM_GAIN_TO_DB(This->gainTarget);
    }
    
    
    
    float BMSmoothGain_getGainLinear(BMSmoothGain *This){
        return This->gainTarget;
    }
    
    
    
    
#ifdef __cplusplus
}
#endif
