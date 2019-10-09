//
//  BMVelvetNoiseDecorrelator.c
//  Saturator
//
//  Created by TienNM on 12/4/17
//  Rewritten by Hans on 9 October 2019
//  Anyone may use this file without restrictions
//

#include "BMVelvetNoiseDecorrelator.h"
#include <stdlib.h>
#include <assert.h>
#include "BMVelvetNoise.h"
#include "BMReverb.h"



/*!
 *BMVelvetNoiseDecorrelator_init
 */
void BMVelvetNoiseDecorrelator_init(BMVelvetNoiseDecorrelator* This,
                                    float maxDelaySeconds,
                                    size_t maxTapsPerChannel,
                                    float rt60DecayTimeSeconds,
									float wetMix,
									float sampleRate){
	This->sampleRate = sampleRate;
	
	// init the multi-tap delay in bypass mode
	bool stereo = true;
	size_t maxDelayLenth = ceil(maxDelaySeconds*sampleRate);
	BMMultiTapDelay_InitBypass(&This->multiTapDelay,
							   stereo,
							   maxDelayLenth,
							   maxTapsPerChannel + 1);

	
	// setup the delay for processing
	BMVelvetNoiseDecorrelator_update(This,
									 maxDelaySeconds,
									 maxTapsPerChannel,
									 rt60DecayTimeSeconds,
									 wetMix);
}





/*!
 *BMVelvetNoiseDecorrelator_free
 */
void BMVelvetNoiseDecorrelator_free(BMVelvetNoiseDecorrelator* This){
	BMMultiTapDelay_free(&This->multiTapDelay);
}






/*!
 *BMVelvetNoiseDecorrelator_update
 */
void BMVelvetNoiseDecorrelator_update(BMVelvetNoiseDecorrelator* This,
									  float diffusionSeconds,
									  size_t tapsPerChannel,
									  float rt60DecayTimeSeconds,
									  float wetMix){
	// keep the wet mix in bounds
	assert(0.0f <= wetMix & wetMix <= 1.0f);
	
	// using the VND to mix the wet and dry signals requires an additional
	// delay tap
	size_t tapsPerChannelWithDryTap = tapsPerChannel + 1;
	
    // allocate temporary memory
	size_t *delayLengthsL = malloc(sizeof(size_t)*tapsPerChannelWithDryTap);
	size_t *delayLengthsR = malloc(sizeof(size_t)*tapsPerChannelWithDryTap);
	float *gainsL = malloc(sizeof(float)*tapsPerChannelWithDryTap);
	float *gainsR = malloc(sizeof(float)*tapsPerChannelWithDryTap);
	
	
	// use the velvet noise algorithm to set delay times
	BMVelvetNoise_setTapIndices(0.0f,
								diffusionSeconds * 1000.0f,
								delayLengthsL+1,
								This->sampleRate,
								tapsPerChannel);
	BMVelvetNoise_setTapIndices(0.0f,
								diffusionSeconds * 1000.0f,
								delayLengthsR+1,
								This->sampleRate,
								tapsPerChannel);
	
	// set the first tap in each channel for the dry signal passthrough
	delayLengthsL[0] = 0;
	delayLengthsR[0] = 0;
	
	// set the delay times to the multitap delay
	BMMultiTapDelay_setDelayTimeNumTap(&This->multiTapDelay, delayLengthsL, delayLengthsR, tapsPerChannelWithDryTap);
	
	
	// use the velvet noise algorithm to set random delay tap signs
	BMVelvetNoise_setTapSigns(gainsL+1, tapsPerChannel);
	BMVelvetNoise_setTapSigns(gainsR+1, tapsPerChannel);
	
	// apply an exponential decay envelope to the gains
	for(size_t i=1; i<tapsPerChannel+1; i++){
		// left channel
		float delayTimeInSeconds = delayLengthsL[i] / This->sampleRate;
		float rt60Gain = BMReverbDelayGainFromRT60(rt60DecayTimeSeconds, delayTimeInSeconds);
		gainsL[i] *= rt60Gain;
		
		// right channel
		delayTimeInSeconds = delayLengthsR[i] / This->sampleRate;
		rt60Gain = BMReverbDelayGainFromRT60(rt60DecayTimeSeconds, delayTimeInSeconds);
		gainsR[i] *= rt60Gain;
	}
	
	// find the vector l2 norm of the delays in each channel for normalisation
	float leftTapsNorm = 0.0f;
	float rightTapsNorm = 0.0f;
	for(size_t i=1; i<tapsPerChannel+1; i++){
		leftTapsNorm += gainsL[i]*gainsL[i];
		rightTapsNorm += gainsR[i]*gainsR[i];
	}
	leftTapsNorm = sqrt(leftTapsNorm);
	rightTapsNorm = sqrt(rightTapsNorm);
	
	// set the dry bypass tap gains
	float dryGain = sqrt(1.0f - wetMix*wetMix);
	gainsL[0] = dryGain;
	gainsR[0] = dryGain;
	
	// normalize the other delay gains to get the specified wet / dry mix
	for(size_t i=1; i<tapsPerChannel+1; i++){
		gainsL[i] *= wetMix / leftTapsNorm;
		gainsR[i] *= wetMix / rightTapsNorm;
	}

	
	// set the gains to the multitap delay
	BMMultiTapDelay_setGains(&This->multiTapDelay, gainsL, gainsR);
	
	
	// free temporary memory
	free(delayLengthsL);
	free(delayLengthsR);
	free(gainsL);
	free(gainsR);
}





/*!
 *BMVelvetNoiseDecorrelator_processBufferStereo
 */
void BMVelvetNoiseDecorrelator_processBufferStereo(BMVelvetNoiseDecorrelator* This,
                                                   float* inputL,
                                                   float* inputR,
                                                   float* outputL,
                                                   float* outputR,
                                                   size_t length){
	// all processing is done by the multi-tap delay class
    BMMultiTapDelay_ProcessBufferStereo(&This->multiTapDelay,
										inputL, inputR,
										outputL, outputR,
										length);
}
