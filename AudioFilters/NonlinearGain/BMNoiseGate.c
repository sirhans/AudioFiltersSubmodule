//
//  BMNoiseGate.c
//  AudioUnitTest
//
//  Created by TienNM on 9/25/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#include "BMNoiseGate.h"
#include "BMEnvelopeFollower.h"
#include <Accelerate/Accelerate.h>

    
#define BM_NOISE_GATE_ATTACK_TIME 0.003
#define BM_NOISE_GATE_RELEASE_TIME 1.0 / 10.0
    
    

    void BMNoiseGate_init(BMNoiseGate *This, float thresholdDb, float sampleRate){
        BMEnvelopeFollower_initWithCustomNumStages(&This->envFollower, 3, 3, sampleRate);
        BMNoiseGate_setThreshold(This, thresholdDb);
        BMNoiseGate_setReleaseTime(This, BM_NOISE_GATE_RELEASE_TIME);
        BMNoiseGate_setAttackTime(This, BM_NOISE_GATE_ATTACK_TIME);
        BMMultiLevelBiquad_init(&This->sidechainFilter, 2, sampleRate, false, false, false);
		
		// init the sidechain filter group delay compensation delay
		This->sidechainMinFreq = 20.0f;
		This->sidechainMaxFreq = 10000.0f;
		BMShortSimpleDelay_init(&This->delay, 2, 10);
        
        // bypass the sidechain filters
		BMMultiLevelBiquad_setBypass(&This->sidechainFilter, 0);
		BMMultiLevelBiquad_setBypass(&This->sidechainFilter, 1);
        BMNoiseGate_setSidechainLowpass(This, 0.0f);
        BMNoiseGate_setSidechainHighpass(This, 0.0f);
        
        This->lastState = 0.0f;
        This->sidechainInputLeveldB = -128.0f;
        This->controlSignalLeveldB = -128.0f;
    }
    
    
	
	void BMNoiseGate_free(BMNoiseGate *This){
		BMMultiLevelBiquad_destroy(&This->sidechainFilter);
		BMShortSimpleDelay_free(&This->delay);
	}

    
	
	
	// returns the group delay in samples of the sidechain filter at the log
	// scale midpoint between the highpass and lowpass cutoff frequencies
	size_t BMNoiseGate_sidechainFilterGroupDelay(BMNoiseGate *This){
		
		// we use the geometric mean to get the midpoint between the
		// lower and upper bounds on a log frequency scale
		double centreFreq = BMGeometricMean2(This->sidechainMinFreq,
											   This->sidechainMaxFreq);
		
		// return the group delay in samples, rounded to the nearest integer
		return (size_t)round(BMMultiLevelBiquad_groupDelay(&This->sidechainFilter, centreFreq));
	}
	
	
	
	
	
	void BMNoiseGate_updateDelay(BMNoiseGate *This){
		// find the group delay of the sidechain filter at its centre frequency
		size_t groupDelay = BMNoiseGate_sidechainFilterGroupDelay(This);
		
		// set the delay to compensate for group delay
		BMShortSimpleDelay_changeLength(&This->delay, groupDelay);
	}
	
	
	
    
    /*!
     *BMNoiseGate_thresholdClosedOpen
     *
     * @abstract convert values < threshold to closedGain; >= threshold to 1.
     */
    void BMNoiseGate_thresholdClosedOpen(BMNoiseGate *This, const float* input, float* output, size_t numSamples){
        // int version of numSamples for vForce functions
        int numSamplesI = (int)numSamples;
        
        // rectify
        vvfabsf(This->buffer, input, &numSamplesI);
        
        // shift so that threshold is at 0
        float negThreshold = -This->thresholdGain;
        vDSP_vsadd(This->buffer, 1, &negThreshold, This->buffer, 1, numSamples);
        
        // clip outside of [0,1]
        float zero = 0.0f; float one = 1.0f;
        vDSP_vclip(This->buffer, 1, &zero, &one, This->buffer, 1, numSamples);
        
        // ceiling to get only values from the set {0.0,1.0}
        vvceilf(This->buffer, This->buffer, &numSamplesI);
        
        // scale and shift to replace zeros with This->closedGain
        float scale = 1.0f - This->closedGain;
        vDSP_vsmsa(This->buffer, 1, &scale, &This->closedGain, This->buffer, 1, numSamples);
    }
    
    
    
    
    
    void BMNoiseGate_processStereo(BMNoiseGate *This,
                                   const float* inputL, const float* inputR,
                                   float* outputL, float* outputR,
                                   size_t numSamples){
        
        // begin chunked processing
        while (numSamples > 0){
            size_t samplesProcessing = BM_MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
            
            // mix stereo input to mono and store into buffer
            float half = 0.5f;
            vDSP_vasm(inputL, 1, inputR, 1, &half, This->buffer, 1, samplesProcessing);
            
            // apply sidechain filters to the buffer
            BMMultiLevelBiquad_processBufferMono(&This->sidechainFilter, This->buffer, This->buffer, samplesProcessing);
            
            // measure the peak level of the sidechain input
            float unused;
			BMLevelMeter_peakLevelMono(&This->sidechainInputMeter,
									   This->buffer,
									   &This->sidechainInputLeveldB,
									   &unused,
									   samplesProcessing);
            
            // if abs(input) > threshold, buffer = 1, else buffer = closedGain
            BMNoiseGate_thresholdClosedOpen(This, This->buffer, This->buffer, samplesProcessing);
            
            // Filter the buffer, which now contains only 1 and closedGain values, to
            // generate a gain control signal
            BMEnvelopeFollower_processBuffer(&This->envFollower, This->buffer, This->buffer, samplesProcessing);
            
            // save the last value in the control signal buffer for use in level
            // metre display
            This->controlSignalLeveldB = BM_GAIN_TO_DB(This->buffer[samplesProcessing-1]);
            
            // Apply the gain control signal to the input and write to output
            vDSP_vmul(This->buffer, 1, inputL, 1, outputL, 1, samplesProcessing);
            vDSP_vmul(This->buffer, 1, inputR, 1, outputR, 1, samplesProcessing);
            
            // record the last gain value for use in apps that require a gain
            // indicator in the UI
            This->lastState = This->buffer[samplesProcessing-1];
            
            // advance pointers
            inputL += samplesProcessing;
            outputL += samplesProcessing;
            inputR += samplesProcessing;
            outputR += samplesProcessing;
            numSamples -= samplesProcessing;
        }
    }
    
    
    
    
    
    void BMNoiseGate_processMono(BMNoiseGate *This,
                                 const float* input,
                                 float* output,
                                 size_t numSamples){
        
        // begin chunked processing
        while (numSamples > 0){
            size_t samplesProcessing = BM_MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
            
            // apply sidechain filters to the buffer
            BMMultiLevelBiquad_processBufferMono(&This->sidechainFilter, input, This->buffer, samplesProcessing);
            
            // if abs(input) > threshold, buffer = 1, else buffer = 0
            BMNoiseGate_thresholdClosedOpen(This, This->buffer, This->buffer, samplesProcessing);
            
            //Filter the buffer, which now contains only closedGain and 0 values, to
            //generate a gain control signal
            BMEnvelopeFollower_processBuffer(&This->envFollower, This->buffer, This->buffer, samplesProcessing);
            
            // Apply the gain control signal to the input and write to output
            vDSP_vmul(This->buffer, 1, input, 1, output, 1, samplesProcessing);
            
            // record the last gain value for use in apps that require a gain
            // indicator in the UI
            This->lastState = This->buffer[samplesProcessing-1];
            
            // advance pointers
            input += samplesProcessing;
            output += samplesProcessing;
            numSamples -= samplesProcessing;
        }
    }
    
    
    
    
    void BMNoiseGate_setAttackTime(BMNoiseGate *This,float attackTimeSeconds){
        BMEnvelopeFollower_setAttackTime(&This->envFollower, attackTimeSeconds);
    }
    
    
    
    
    
    void BMNoiseGate_setReleaseTime(BMNoiseGate *This,float releaseTimeSeconds){
        BMEnvelopeFollower_setReleaseTime(&This->envFollower, releaseTimeSeconds);
    }
    
    
    
    
    void BMNoiseGate_setThreshold(BMNoiseGate *This,float thresholdDb){
        This->thresholdGain = BM_DB_TO_GAIN(thresholdDb);
    }
    
    
    
 
    
    float BMNoiseGate_getGateVolumeDB(BMNoiseGate *This){
        return This->controlSignalLeveldB;
    }



    
    
    float BMNoiseGate_getSidechainInputLevelDB(BMNoiseGate *This){
        return This->sidechainInputLeveldB;
    }
    
    
    


    void BMNoiseGate_setSidechainHighCut(BMNoiseGate *This, float fc){
		assert(fc <= This->sidechainFilter.sampleRate/2.0f);
		
        if (fc > 0.0f && fc != This->sidechainFilter.sampleRate/2.0f){
            BMMultiLevelBiquad_setActiveOnLevel(&This->sidechainFilter, true, 1);
            BMMultiLevelBiquad_setLowPass12db(&This->sidechainFilter, fc, 1);
			This->sidechainMaxFreq = fc;
        }
		else {
			BMMultiLevelBiquad_setActiveOnLevel(&This->sidechainFilter, false, 1);
			This->sidechainMaxFreq = This->sidechainFilter.sampleRate/2.0;
		}
		
		BMNoiseGate_updateDelay(This);
    }



 
    void BMNoiseGate_setSidechainLowCut(BMNoiseGate *This, float fc){
		assert(fc <= This->sidechainFilter.sampleRate/2.0f);
		
        if (fc > 0.0f && fc != This->sidechainFilter.sampleRate/2.0f){
            BMMultiLevelBiquad_setActiveOnLevel(&This->sidechainFilter, true, 0);
            BMMultiLevelBiquad_setHighPass12db(&This->sidechainFilter, fc, 1);
			This->sidechainMinFreq = fc;
        }
		else {
			BMMultiLevelBiquad_setActiveOnLevel(&This->sidechainFilter, false, 0);
			This->sidechainMinFreq = 0.0f;
		}
		
		BMNoiseGate_updateDelay(This);
    }
    
    
    

    
    void BMNoiseGate_setClosedGain(BMNoiseGate *This, float gainDb){
        This->closedGain = BM_DB_TO_GAIN(gainDb);
    }
    
    
    
#ifdef __cplusplus
}
#endif
