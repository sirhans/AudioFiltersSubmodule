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

void BMVelvetNoiseDecorrelator_initFullSettings(BMVelvetNoiseDecorrelator *This,
												float maxDelaySeconds,
												size_t numTaps,
												float rt60DecayTimeSeconds,
												bool hasDryTap,
												float sampleRate,
												bool evenTapDensity);

void BMVelvetNoiseDecorrelator_initMultiChannelInput(BMVelvetNoiseDecorrelator *This,
                                                float maxDelaySeconds,
                                                size_t numTaps,
                                                float rt60DecayTimeSeconds,
                                                bool hasDryTap,
                                                size_t numInput,
                                                float sampleRate,
                                                bool evenTapDensity);
/*!
 *BMVelvetNoiseDecorrelator_init
 */
void BMVelvetNoiseDecorrelator_init(BMVelvetNoiseDecorrelator *This,
                                    float maxDelaySeconds,
                                    size_t numTaps,
                                    float rt60DecayTimeSeconds,
									bool hasDryTap,
									float sampleRate){
	BMVelvetNoiseDecorrelator_initFullSettings(This, maxDelaySeconds, numTaps, rt60DecayTimeSeconds, hasDryTap, sampleRate, false);
}




/*!
 *BMVelvetNoiseDecorrelator_initWithEvenTapDensity
 */
void BMVelvetNoiseDecorrelator_initWithEvenTapDensity(BMVelvetNoiseDecorrelator *This,
                                    float maxDelaySeconds,
                                    size_t numTaps,
                                    float rt60DecayTimeSeconds,
									bool hasDryTap,
									float sampleRate){
	BMVelvetNoiseDecorrelator_initFullSettings(This, maxDelaySeconds, numTaps, rt60DecayTimeSeconds, hasDryTap, sampleRate, true);
}

void BMVelvetNoiseDecorrelator_initMultiChannelInputEvenTapDensity(BMVelvetNoiseDecorrelator *This,
                                    float maxDelaySeconds,
                                    size_t numTaps,
                                    float rt60DecayTimeSeconds,
                                    bool hasDryTap,
                                    size_t numInput,
                                    float sampleRate){
    BMVelvetNoiseDecorrelator_initMultiChannelInput(This, maxDelaySeconds, numTaps, rt60DecayTimeSeconds, hasDryTap,numInput, sampleRate, true);
}


/*!
 *BMVelvetNoiseDecorrelator_initFullSettings
 */
void BMVelvetNoiseDecorrelator_initFullSettings(BMVelvetNoiseDecorrelator *This,
												float maxDelaySeconds,
												size_t numTaps,
												float rt60DecayTimeSeconds,
												bool hasDryTap,
												float sampleRate,
												bool evenTapDensity){
	This->sampleRate = sampleRate;
	This->hasDryTap	= hasDryTap;
	This->wetMix = BM_VND_WET_MIX;
	This->rt60 = rt60DecayTimeSeconds;
	This->maxDelayTimeS = maxDelaySeconds;
	This->numWetTaps = numTaps;
	This->evenTapDensity = evenTapDensity;
    This->resetNumTaps = false;
    This->resetRT60DecayTime = false;
    This->fadeInSamples = 0;
	if (hasDryTap) This->numWetTaps--;
	
	// allocate memory for calculating delay setups
	This->delayLengthsL = calloc(numTaps, sizeof(size_t));
	This->delayLengthsR = calloc(numTaps, sizeof(size_t));
	This->gainsL = calloc(numTaps, sizeof(float));
	This->gainsR = calloc(numTaps, sizeof(float));
    This->tempBuffer = calloc(BM_BUFFER_CHUNK_SIZE, sizeof(float));
	
	// init the multi-tap delay in bypass mode
	size_t maxDelayLenth = ceil(maxDelaySeconds*sampleRate);
	BMMultiTapDelay_initBypass(&This->multiTapDelay,
							   true,
							   maxDelayLenth,
							   numTaps);
	
	// setup the delay for processing
	BMVelvetNoiseDecorrelator_randomiseAll(This);
}


void BMVelvetNoiseDecorrelator_initMultiChannelInput(BMVelvetNoiseDecorrelator *This,
                                                float maxDelaySeconds,
                                                size_t numTaps,
                                                float rt60DecayTimeSeconds,
                                                bool hasDryTap,
                                                size_t numInput,
                                                float sampleRate,
                                                bool evenTapDensity){
    This->sampleRate = sampleRate;
    This->hasDryTap    = hasDryTap;
    This->wetMix = BM_VND_WET_MIX;
    This->rt60 = rt60DecayTimeSeconds;
    This->maxDelayTimeS = maxDelaySeconds;
    This->numWetTaps = numTaps;
    This->evenTapDensity = evenTapDensity;
    This->resetNumTaps = false;
    This->resetRT60DecayTime = false;
    This->fadeInSamples = 0;
    if (hasDryTap) This->numWetTaps--;
    
    // allocate memory for calculating delay setups
    This->delayLengthsL = calloc(numTaps, sizeof(size_t));
    This->delayLengthsR = calloc(numTaps, sizeof(size_t));
    This->gainsL = calloc(numTaps, sizeof(float));
    This->gainsR = calloc(numTaps, sizeof(float));
    This->tempBuffer = calloc(BM_BUFFER_CHUNK_SIZE, sizeof(float));
    This->numInput = numInput;
    // init the multi-tap delay in bypass mode
    size_t maxDelayLenth = ceil(maxDelaySeconds*sampleRate);
    BMMultiTapDelay_initBypassMultiChannel(&This->multiTapDelay, true, maxDelayLenth, numInput, numTaps);
    
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
        float fadeIn = 1;
        if(This->fadeInSamples>0)
            fadeIn = MIN(This->delayLengthsL[i], This->fadeInSamples)/This->fadeInSamples;
		This->gainsL[i] *= rt60Gain * fadeIn;
		
		// right channel
		delayTimeInSeconds = This->delayLengthsR[i] / This->sampleRate;
		rt60Gain = BMReverbDelayGainFromRT60(This->rt60, delayTimeInSeconds);
        if(This->fadeInSamples>0)
            fadeIn = MIN(This->delayLengthsR[i], This->fadeInSamples)/This->fadeInSamples;
		This->gainsR[i] *= rt60Gain * fadeIn;
	}
    
    //Last tap gain
    This->lastTapGainL = This->gainsL[This->numWetTaps+shift-1];
    This->lastTapGainR = This->gainsR[This->numWetTaps+shift-1];
	
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
	
	// even tap density uses the velvet noise algorithm. The frequency response may be less even but it won't leave large gaps in the time domain
	if(This->evenTapDensity){
		// set the min delay time, depending on whether there is a dry tap
		float minDelayTimeS;
		if(This->hasDryTap) minDelayTimeS = This->maxDelayTimeS / (float)This->numWetTaps;
		else minDelayTimeS = 0.0f;
		
		// set the randomised tap indices
		BMVelvetNoise_setTapIndices(minDelayTimeS * 1000.0f,
									This->maxDelayTimeS * 1000.0f,
									This->delayLengthsL + shift,
									This->sampleRate, This->numWetTaps);
		BMVelvetNoise_setTapIndices(minDelayTimeS * 1000.0f,
									This->maxDelayTimeS * 1000.0f,
									This->delayLengthsR + shift,
									This->sampleRate, This->numWetTaps);
	}
	// uneven tap density may have more natural frequency response
	else {
		// set the min delay index depending on whether there is a dry tap
		size_t min;
		if(This->hasDryTap)
			min = ceil((This->maxDelayTimeS * This->sampleRate) / (float)This->numWetTaps);
		else
			min = 0;
		
		// set max delay index
		size_t max = ceil(This->maxDelayTimeS * This->sampleRate);
		
		// set the random delay times
		BMReverbRandomsInRange(min, max, This->delayLengthsL + shift, This->numWetTaps);
		BMReverbRandomsInRange(min, max, This->delayLengthsR + shift, This->numWetTaps);
	}
	
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


void BMVelvetNoiseDecorrelator_setNumTaps(BMVelvetNoiseDecorrelator *This, size_t numTaps){
    This->numWetTaps = numTaps;
    This->resetNumTaps = true;
}

void BMVelvetNoiseDecorrelator_resetNumTaps(BMVelvetNoiseDecorrelator *This){
    if(This->resetNumTaps){
        This->resetNumTaps = false;
        
        // init the multi-tap delay in bypass mode
        size_t maxDelayLenth = ceil(This->maxDelayTimeS*This->sampleRate);
        BMMultiTapDelay_initBypass(&This->multiTapDelay,
                                   true,
                                   maxDelayLenth,
                                   This->numWetTaps);
        
        // setup the delay for processing
        BMVelvetNoiseDecorrelator_randomiseAll(This);
    }
}

void BMVelvetNoiseDecorrelator_setRT60DecayTime(BMVelvetNoiseDecorrelator *This, float rt60DT){
    This->rt60 = rt60DT;
    This->resetRT60DecayTime = true;
}

void BMVelvetNoiseDecorrelator_resetRT60DecayTime(BMVelvetNoiseDecorrelator *This){
    if(This->resetRT60DecayTime){
        This->resetRT60DecayTime = false;
        BMVelvetNoiseDecorrelator_genRandGains(This);
    }
}

void BMVelvetNoiseDecorrelator_setFadeIn(BMVelvetNoiseDecorrelator *This,float fadeInS){
    This->fadeInSamples = fadeInS * This->sampleRate;
    This->resetFadeIn = true;
}

void BMVelvetNoiseDecorrelator_resetFadeIn(BMVelvetNoiseDecorrelator *This){
    if(This->resetFadeIn){
        This->resetFadeIn = false;
        BMVelvetNoiseDecorrelator_genRandGains(This);
    }
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
    free(This->tempBuffer);
    This->tempBuffer = NULL;
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
    //Reset numtap if needed
    BMVelvetNoiseDecorrelator_resetNumTaps(This);
    BMVelvetNoiseDecorrelator_resetRT60DecayTime(This);
    BMVelvetNoiseDecorrelator_resetFadeIn(This);
	// all processing is done by the multi-tap delay class
    BMMultiTapDelay_processBufferStereo(&This->multiTapDelay,
										inputL, inputR,
										outputL, outputR,
										length);
}

void BMVelvetNoiseDecorrelator_processMultiChannelInput(BMVelvetNoiseDecorrelator *This,
                                                   float** inputL,
                                                   float** inputR,
                                                   float* outputL,
                                                   float* outputR,
                                                   size_t length){
    //Reset numtap if needed
    BMVelvetNoiseDecorrelator_resetNumTaps(This);
    BMVelvetNoiseDecorrelator_resetRT60DecayTime(This);
    BMVelvetNoiseDecorrelator_resetFadeIn(This);
    // all processing is done by the multi-tap delay class
    BMMultiTapDelay_processMultiChannelInput(&This->multiTapDelay, inputL, inputR, outputL, outputR, This->numInput, length);
}

void BMVelvetNoiseDecorrelator_processBufferStereoWithFinalOutput(BMVelvetNoiseDecorrelator *This,
                                                   float* inputL,
                                                   float* inputR,
                                                   float* outputL,
                                                   float* outputR,
                                                   float* finalOutputL,
                                                   float* finalOutputR,
                                                   size_t length){
    //Reset numtap if needed
    BMVelvetNoiseDecorrelator_resetNumTaps(This);
    BMVelvetNoiseDecorrelator_resetRT60DecayTime(This);
    BMVelvetNoiseDecorrelator_resetFadeIn(This);
    // all processing is done by the multi-tap delay class
    BMMultiTapDelay_processStereoWithFinalOutput(&This->multiTapDelay,
                                                 inputL, inputR,
                                                 outputL, outputR, finalOutputL, finalOutputR, length);
    //Apply lasttap gain
    vDSP_vsmul(finalOutputL, 1, &This->lastTapGainL, finalOutputL, 1, length);
    vDSP_vsmul(finalOutputR, 1, &This->lastTapGainR, finalOutputR, 1, length);
}





void BMVelvetNoiseDecorrelator_processBufferMonoToStereo(BMVelvetNoiseDecorrelator *This,
														 float* inputL,
														 float* outputL, float* outputR,
														 size_t length){
    //Reset numtap if needed
    BMVelvetNoiseDecorrelator_resetNumTaps(This);
    BMVelvetNoiseDecorrelator_resetRT60DecayTime(This);
    BMVelvetNoiseDecorrelator_resetFadeIn(This);
	// all processing is done by the multi-tap delay class
	BMMultiTapDelay_processBufferStereo(&This->multiTapDelay,
										inputL, inputL,
										outputL, outputR,
										length);
}
