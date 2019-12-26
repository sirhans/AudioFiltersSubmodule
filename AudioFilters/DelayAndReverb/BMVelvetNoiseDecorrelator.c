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
#include "BMVectorOps.h"
#include "BMReverb.h"
#include "BMSorting.h"


#define BM_VND_WET_MIX 0.40f




/*!
 *BMVelvetNoiseDecorrelator_init
 */
void BMVelvetNoiseDecorrelator_init(BMVelvetNoiseDecorrelator *This,
                                    float maxDelaySeconds,
                                    size_t numTaps,
                                    float rt60DecayTimeSeconds,
									bool hasDryTap,
									float sampleRate){
	This->sampleRate = sampleRate;
	This->hasDryTap	= true;
	This->wetMix = BM_VND_WET_MIX;
	This->rt60 = rt60DecayTimeSeconds;
	This->maxDelayTimeS = maxDelaySeconds;
	This->numWetTaps = numTaps;
	if (hasDryTap) This->numWetTaps--;
	
	// allocate memory for calculating delay setups
	This->delayLengthsL = calloc(numTaps, sizeof(size_t));
	This->delayLengthsR = calloc(numTaps, sizeof(size_t));
	This->gainsL = calloc(numTaps, sizeof(float));
	This->gainsR = calloc(numTaps, sizeof(float));
	
	// init the multi-tap delay in bypass mode
	size_t maxDelayLenth = ceil(maxDelaySeconds*sampleRate);
	BMMultiTapDelay_initBypass(&This->multiTapDelay,
							   true,
							   maxDelayLenth,
							   numTaps);
	
	// setup the delay for processing
	BMVelvetNoiseDecorrelator_randomiseAll(This);
}





/*!
 *BMVelvetNoiseDecorrelator_genRandGains
 *
 * @abstract generates random gains and normalises them and updates the delay
 */
void BMVelvetNoiseDecorrelator_genRandGains(BMVelvetNoiseDecorrelator *This){
	// prepare to skip an array index if there is a dry tap at the beginning
	size_t shift = This->hasDryTap ? 1 : 0;
	
	// use the velvet noise algorithm to set random delay tap signs
	BMVelvetNoise_setTapSigns(This->gainsL+shift, This->numWetTaps);
	BMVelvetNoise_setTapSigns(This->gainsR+shift, This->numWetTaps);
	
	
	// apply an exponential decay envelope to the gains
	for(size_t i=shift; i<This->numWetTaps+shift; i++){
		// left channel
		float delayTimeInSeconds = This->delayLengthsL[i] / This->sampleRate;
		float rt60Gain = BMReverbDelayGainFromRT60(This->rt60, delayTimeInSeconds);
		This->gainsL[i] *= rt60Gain;
		
		// right channel
		delayTimeInSeconds = This->delayLengthsR[i] / This->sampleRate;
		rt60Gain = BMReverbDelayGainFromRT60(This->rt60, delayTimeInSeconds);
		This->gainsR[i] *= rt60Gain;
	}
	
	// if there is is a dry tap,
	// set the balance between all of the wet taps against the single dry tap.
	if(This->hasDryTap)
		BMVelvetNoiseDecorrelator_setWetMix(This, This->wetMix);
	
	// else, if there is no dry tap, normalise so the wet gain is 1.0
	else {
		BMVectorNormalise(This->gainsL, This->numWetTaps);
		BMVectorNormalise(This->gainsR, This->numWetTaps);
		BMMultiTapDelay_setGains(&This->multiTapDelay, This->gainsL, This->gainsR);
	}
}






/*!
*BMVelvetNoiseDecorrelator_genRandTapTimes
*
* @abstract generates random delay tap times and updates the delay
*/
void BMVelvetNoiseDecorrelator_genRandTapTimes(BMVelvetNoiseDecorrelator *This){
	// prepare to skip an array index if there is a dry tap at the beginning
	size_t shift = This->hasDryTap ? 1 : 0;
	
	// set the dry tap delay time to zero
	if(This->hasDryTap){
		This->delayLengthsL[0] = 0;
		This->delayLengthsR[0] = 0;
	}
	
	// use the randoms-in-range algorithm to set delay tap times
	size_t min = ceil((This->maxDelayTimeS * This->sampleRate) / This->numWetTaps);
	size_t max = ceil(This->maxDelayTimeS * This->sampleRate);
	BMReverbRandomsInRange(min, max, This->delayLengthsL + shift, This->numWetTaps);
	BMReverbRandomsInRange(min, max, This->delayLengthsR + shift, This->numWetTaps);
	
	// sort the delay times for easier debugging
	BMInsertionSort_size_t(This->delayLengthsL + shift, This->numWetTaps);
	BMInsertionSort_size_t(This->delayLengthsR + shift, This->numWetTaps);
	
	
	BMMultiTapDelay_setDelayTimes(&This->multiTapDelay, This->delayLengthsL, This->delayLengthsR);
}





void BMVelvetNoiseDecorrelator_randomiseAll(BMVelvetNoiseDecorrelator *This){
	// randomise times
	BMVelvetNoiseDecorrelator_genRandTapTimes(This);
	
	// randomise gains
	BMVelvetNoiseDecorrelator_genRandGains(This);
}




/*!
 *BMVelvetNoiseDecorrelator_setWetMix
 *
 * @abstract sets the wet/dry mix and issues the command to update the multitap delay
 */
void BMVelvetNoiseDecorrelator_setWetMix(BMVelvetNoiseDecorrelator *This, float wetMix01){
	// we need a dry tap to set the mix; otherwise it's fixed at 100% wet
	// beacuse the wet/dry mix setting is meaningless
	assert(This->hasDryTap);
	
	// keep the wet mix in bounds
	assert(0.0f <= wetMix01 & wetMix01 <= 1.0f);
	
	This->wetMix = wetMix01;

	// what is the minimum gain we can set for the dry tap? It would not make
	// sense for the dry tap to have less gain than the wet taps, so the minimum
	// is not simply zero.
	float minDryGain = sqrtf(1.0f / (This->numWetTaps + 1.0f));
	
	// find the corrected dry mix, which is in [minDryGain,1] instead of [0,1]
	float dryGainUncorrected = sqrt(1.0f - wetMix01*wetMix01);
	float dryGainCorrected = minDryGain + dryGainUncorrected * (1.0f - minDryGain);
	
	// find the corrected wet gain
	float wetGainCorrected = sqrt(1.0f - dryGainCorrected*dryGainCorrected);
	
	// set the dry bypass tap gains
	This->gainsL[0] = dryGainCorrected;
	This->gainsR[0] = dryGainCorrected;
	
	// normalize so the norm of the vector of all wet taps is 1.0
	BMVectorNormalise(This->gainsL+1, This->numWetTaps);
	BMVectorNormalise(This->gainsR+1, This->numWetTaps);
	
	// scale the wet taps to get the wet gain as desired
	vDSP_vsmul(This->gainsL+1, 1, &wetGainCorrected, This->gainsL+1, 1, This->numWetTaps);
	vDSP_vsmul(This->gainsR+1, 1, &wetGainCorrected, This->gainsR+1, 1, This->numWetTaps);

	// set the gains to the multitap delay
	BMMultiTapDelay_setGains(&This->multiTapDelay, This->gainsL, This->gainsR);
}




/*!
 *BMVelvetNoiseDecorrelator_free
 */
void BMVelvetNoiseDecorrelator_free(BMVelvetNoiseDecorrelator *This){
	BMMultiTapDelay_free(&This->multiTapDelay);
	
	free(This->delayLengthsL);
	This->delayLengthsL = NULL;
	free(This->delayLengthsR);
	This->delayLengthsR = NULL;
	free(This->gainsL);
	This->gainsL = NULL;
	free(This->gainsR);
	This->gainsR = NULL;
}




/*!
 *BMVelvetNoiseDecorrelator_processBufferStereo
 */
void BMVelvetNoiseDecorrelator_processBufferStereo(BMVelvetNoiseDecorrelator *This,
                                                   float* inputL,
                                                   float* inputR,
                                                   float* outputL,
                                                   float* outputR,
                                                   size_t length){
	// all processing is done by the multi-tap delay class
    BMMultiTapDelay_processBufferStereo(&This->multiTapDelay,
										inputL, inputR,
										outputL, outputR,
										length);
}





void BMVelvetNoiseDecorrelator_processBufferMonoToStereo(BMVelvetNoiseDecorrelator *This,
														 float* inputL,
														 float* outputL, float* outputR,
														 size_t length){
	// all processing is done by the multi-tap delay class
	BMMultiTapDelay_processBufferStereo(&This->multiTapDelay,
										inputL, inputL,
										outputL, outputR,
										length);
}
