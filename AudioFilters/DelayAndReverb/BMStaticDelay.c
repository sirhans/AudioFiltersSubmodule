//
//  BMStaticDelay.c
//  BMAudioFilters
//
//  Created by Blue Mangoo on 5/7/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//


#include <Accelerate/Accelerate.h>
#include "BMStaticDelay.h"
#include "Constants.h"
#include "BMReverb.h"

#define BM_STATIC_DELAY_LOWPASS_FILTER_LEVEL 0
#define BM_STATIC_DELAY_HIGHPASS_FILTER_LEVEL 1


void BMStaticDelay_init(BMStaticDelay *This,
						float delayTimeSeconds,
						float rt60DecayTimeSeconds,
						float wetGainDb,
						float crossMixAmount,
						float lowpassFC,
						float highpassFC,
						float sampleRate){
	This->sampleRate = sampleRate;
	
	This->delayTimeInSeconds = delayTimeSeconds;
	size_t delayTimeInSamples = delayTimeSeconds * sampleRate;
	This->needsDelayTimeUpdate = false;
	
	BMSimpleDelayStereo_init(&This->delay, delayTimeInSamples);
	
	BMWetDryMixer_init(&This->bypassSwitch, sampleRate);
	
	// init the stereo matrix mixer
	float rotationAngle = M_PI_2 * crossMixAmount;
	BM2x2MatrixMixer_initWithRotation(&This->matrixMixer, rotationAngle);
	
	// init the filters
	size_t numFilters = 2;
	bool stereo = true;
	bool monoRealTimeUpdate = false;
	bool smoothUpdate = true;
	BMMultiLevelBiquad_init(&This->filter, numFilters, sampleRate, stereo, monoRealTimeUpdate, smoothUpdate);
	BMMultiLevelBiquad_setHighPass6db(&This->filter, highpassFC, BM_STATIC_DELAY_HIGHPASS_FILTER_LEVEL);
	BMMultiLevelBiquad_setLowPass6db(&This->filter, lowpassFC, BM_STATIC_DELAY_LOWPASS_FILTER_LEVEL);
	
	float panLFOFreqHz = 0.333;
	float panLFODepth = 0.9;
	BMLFOPan2_init(&This->pan, panLFOFreqHz, panLFODepth, sampleRate);
	
	// set feedback gain to get correct decay time
	This->feedbackGain = BMReverbDelayGainFromRT60(rt60DecayTimeSeconds, delayTimeSeconds);
	
	// init the wet and dry gain
	BMSmoothGain_init(&This->wetGain, sampleRate);
	BMSmoothGain_init(&This->dryGain, sampleRate);
	BMSmoothGain_setGainDb(&This->wetGain, wetGainDb);
	BMSmoothGain_setGainDb(&This->dryGain, 0.0f);
	
	This->mixingBufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->mixingBufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->feedbackBufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->feedbackBufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->outputBufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->outputBufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
}




void BMStaticDelay_free(BMStaticDelay *This){
	BMLFOPan2_free(&This->pan);
	BM2x2MatrixMixer_free(&This->matrixMixer);
	BMMultiLevelBiquad_free(&This->filter);
	BMSimpleDelayStereo_free(&This->delay);
	
	free(This->mixingBufferL);
	free(This->mixingBufferR);
	free(This->feedbackBufferL);
	free(This->feedbackBufferR);
	free(This->outputBufferL);
	free(This->outputBufferR);
	This->mixingBufferL = NULL;
	This->mixingBufferR = NULL;
	This->feedbackBufferL = NULL;
	This->feedbackBufferR = NULL;
	This->outputBufferL = NULL;
	This->outputBufferR = NULL;
}



/*!
 *BMStaticDelay_destroy
 *
 * Deprecated alternate name for BMStaticDelay_free
 */
void BMStaticDelay_destroy(BMStaticDelay *This){
	BMStaticDelay_free(This);
}



void BMStaticDelay_processBufferStereo(BMStaticDelay *This,
									   float* inL, float* inR,
									   float* outL, float* outR,
									   int numSamplesIn){
	// if a delay time update was requested, reinitialise the delay.
	if (This->needsDelayTimeUpdate) {
		size_t newDelayTimeSamples = This->sampleRate * This->delayTimeTargetInSeconds;
		BMSimpleDelayStereo_free(&This->delay);
		BMSimpleDelayStereo_init(&This->delay, newDelayTimeSamples);
		
		// update the delay time variable so that feedback time calculations work
		This->delayTimeInSeconds = This->delayTimeTargetInSeconds;
		
		// don't update again next time
		This->needsDelayTimeUpdate = false;
	}
	
	// Chunked processing
	size_t samplesToProcess = numSamplesIn;
	size_t samplesProcessed = 0;
	while (samplesToProcess > 0){
		// get the maximum number of samples that can be read from the delay
		size_t samplesAvailableInDelay = BMSimpleDelayStereo_outputCapacity(&This->delay);
		
		// get the maximum number of samples we can process this time through the loop
		size_t samplesProcessing = BM_MIN(samplesToProcess, BM_BUFFER_CHUNK_SIZE);
		samplesProcessing = BM_MIN(samplesProcessing, samplesAvailableInDelay);
		
		// If not in bypass mode, process the delay
		if(!BMWetDryMixer_isDry(&This->bypassSwitch)){
			
			// Delay output => output buffer
			size_t delayOutputRead = BMSimpleDelayStereo_output(&This->delay, This->outputBufferL, This->outputBufferR, samplesProcessing);
			
			// Output buffer => feedback buffer
			memcpy(This->feedbackBufferL, This->outputBufferL, samplesProcessing * sizeof(float));
			memcpy(This->feedbackBufferR, This->outputBufferR, samplesProcessing * sizeof(float));
		
			// reduce gain of delay feedback
			vDSP_vsmul(This->feedbackBufferL, 1, &This->feedbackGain, This->feedbackBufferL, 1, samplesProcessing);
			vDSP_vsmul(This->feedbackBufferR, 1, &This->feedbackGain, This->feedbackBufferR, 1, samplesProcessing);
			
			// apply the mixing matrix to the delay feedback buffer
			BM2x2MatrixMixer_processStereo(&This->matrixMixer,
										   This->feedbackBufferL, This->feedbackBufferR,
										   This->feedbackBufferL, This->feedbackBufferR,
										   samplesProcessing);
			
			// mix the input to one channel, then pan it back to stereo with an LFO
			BMLFOPan2_processStereo(&This->pan, inL + samplesProcessed, inR + samplesProcessed,
									This->mixingBufferL, This->mixingBufferR,
									samplesProcessing);
			
			// mix panned input + feedback
			vDSP_vadd(This->mixingBufferL, 1, This->feedbackBufferL, 1,
					  This->mixingBufferL, 1, samplesProcessing);
			vDSP_vadd(This->mixingBufferR, 1, This->feedbackBufferR, 1,
					  This->mixingBufferR, 1, samplesProcessing);
			
			// make sure there's space in the delay for writing the input
			size_t spaceAvailableInDelay = BMSimpleDelayStereo_inputCapacity(&This->delay);
			assert(spaceAvailableInDelay >= samplesProcessing);
			
			// mixingBuffer => delay input
			size_t delayInputWritten = BMSimpleDelayStereo_input(&This->delay, This->mixingBufferL, This->mixingBufferR, samplesProcessing);
			
			// make sure we read and wrote the same number of samples in and out of the delay
			assert(delayOutputRead == delayInputWritten);
			
			// filter the output
			BMMultiLevelBiquad_processBufferStereo(&This->filter,
												   This->outputBufferL, This->outputBufferR,
												   This->outputBufferL, This->outputBufferR,
												   samplesProcessing);
			
			// Adjust wet gain
			BMSmoothGain_processBuffer(&This->wetGain, This->outputBufferL, This->outputBufferR, This->outputBufferL, This->outputBufferR, samplesProcessing);
			
			// Adjust the dry gain and send the dry signal to the mixing buffer
			BMSmoothGain_processBuffer(&This->dryGain,
									   inL + samplesProcessed, inR + samplesProcessed,
									   This->mixingBufferL, This->mixingBufferR,
									   samplesProcessing);
			
			// mix the wet and dry signals to the output buffer
			vDSP_vadd(This->mixingBufferL, 1, This->outputBufferL, 1,
					  This->outputBufferL, 1,
					  samplesProcessing);
			vDSP_vadd(This->mixingBufferR, 1, This->outputBufferR, 1,
					  This->outputBufferR, 1,
					  samplesProcessing);
		}
		// if we are in bypass mode, send silence to the delay so it doesn't make sound when we turn it back on.
		else {
			vDSP_vclr(This->mixingBufferL, 1, samplesProcessing);
			BMSimpleDelayStereo_process(&This->delay, This->mixingBufferL, This->mixingBufferL, This->outputBufferL, This->outputBufferR, samplesProcessing);
		}
		
		// Bypass switch
		BMWetDryMixer_processBufferInPhase(&This->bypassSwitch,
										   This->outputBufferL, This->outputBufferR,
										   inL + samplesProcessed, inR + samplesProcessed,
										   outL + samplesProcessed, outR + samplesProcessed,
										   samplesProcessing);
		
		// Update indices
		samplesToProcess -= samplesProcessing;
		samplesProcessed += samplesProcessing;
	}
}



void BMStaticDelay_setFeedbackCrossMix(BMStaticDelay *This, float amount){
	BM2x2MatrixMixer_setRotation(&This->matrixMixer, amount * M_PI_2);
}




void BMStaticDelay_setRT60DecayTime(BMStaticDelay *This, float rt60DecayTime){
	BMReverbDelayGainFromRT60(rt60DecayTime, This->delayTimeInSeconds);
}



void BMStaticDelay_setWetGain(BMStaticDelay *This, float gainDb){
	BMSmoothGain_setGainDb(&This->wetGain, gainDb);
}



void BMStaticDelay_setDryGain(BMStaticDelay *This, float gainDb){
	BMSmoothGain_setGainDb(&This->dryGain, gainDb);
}



void BMStaticDelay_setLowpassFc(BMStaticDelay *This, float fc){
	BMMultiLevelBiquad_setLowPass6db(&This->filter, fc, BM_STATIC_DELAY_LOWPASS_FILTER_LEVEL);
}



void BMStaticDelay_setHighpassFc(BMStaticDelay *This, float fc){
	BMMultiLevelBiquad_setLowPass6db(&This->filter, fc, BM_STATIC_DELAY_HIGHPASS_FILTER_LEVEL);
}



void BMStaticDelay_setDelayTime(BMStaticDelay *This, float timeInSeconds){
	This->delayTimeTargetInSeconds = timeInSeconds;
	This->needsDelayTimeUpdate = true;
}



void BMStaticDelay_setBypassed(BMStaticDelay *This){
	BMWetDryMixer_setMix(&This->bypassSwitch, 0.0f);
}


void BMStaticDelay_setActive(BMStaticDelay *This){
	BMWetDryMixer_setMix(&This->bypassSwitch, 1.0f);
}
