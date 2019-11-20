//
//  BMTransientShaper.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/17/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#include "BMTransientShaper.h"
#include <assert.h>
#include <Accelerate/Accelerate.h>
#include "BMIntegerMath.h"


// set defaults for transient shaper
#define BMTRANS_ATTACK_PEAK_CONNECTION_TIME 1.0f / 2.0f
#define BMTRANS_ATTACK_SLOW_ATTACK_ENV_TIME 1.0f / 20.0f
#define BMTRANS_ATTACK_ONSET_TIME 1.0f / 2000.0f
#define BMTRANS_ATTACK_DURATION 1.0f / 5.0f
#define BMTRANS_DYNAMIC_SMOOTHING_SENSITIVITY 1.0f / 8.0f
#define BMTRANS_DYNAMIC_SMOOTHING_MIN_FC 1.0f



void BMTransientEnveloper_init(BMTransientEnveloper *This, float sampleRate){
    
    /*
     * attack transient filters
     */
    float attackPeakConnectionFc = ARTimeToCutoffFrequency(BMTRANS_ATTACK_PEAK_CONNECTION_TIME, BMENV_NUM_STAGES);
    float attackSlowAttackEnvFc = ARTimeToCutoffFrequency(BMTRANS_ATTACK_SLOW_ATTACK_ENV_TIME, BMENV_NUM_STAGES);
    float attackDurationFc = ARTimeToCutoffFrequency(BMTRANS_ATTACK_DURATION, BMENV_NUM_STAGES);
    float attackOnsetTimeFc = ARTimeToCutoffFrequency(BMTRANS_ATTACK_ONSET_TIME, BMENV_NUM_STAGES);
    
    for(size_t i=0; i<BMENV_NUM_STAGES; i++){
        BMReleaseFilter_init(&This->attackRF1[i],attackPeakConnectionFc,sampleRate);
        BMAttackFilter_init(&This->attackAF1[i],attackSlowAttackEnvFc,sampleRate);
        BMReleaseFilter_init(&This->attackRF2[i],attackDurationFc,sampleRate);
        BMAttackFilter_init(&This->attackAF2[i],attackOnsetTimeFc,sampleRate);
    }
    
    // Since we aren't initializing with zero attack time, we set filterAttackOnset to true
    This->filterAttackOnset = true;
    
    BMDynamicSmoothingFilter_init(&This->attackDSF,
                                  BMTRANS_DYNAMIC_SMOOTHING_SENSITIVITY,
                                  BMTRANS_DYNAMIC_SMOOTHING_MIN_FC,
                                  sampleRate);
    
    /*
     * release envelope filters
     */
    for(size_t i=0; i<BMENV_NUM_STAGES; i++){
        BMReleaseFilter_init(&This->releaseRF1[i],0,sampleRate);
    }
    
    BMReleaseFilter_init(&This->releaseRF2,0,sampleRate);
    
    BMDynamicSmoothingFilter_init(&This->attackDSF,
                                  BMTRANS_DYNAMIC_SMOOTHING_SENSITIVITY,
                                  BMTRANS_DYNAMIC_SMOOTHING_MIN_FC,
                                  sampleRate);
}





/*!
 * BMTransientEnveloper_processBuffer
 * @abstract used as a component of a transient shaper
 *
 * @param This             pointer to an initialised BMEnvelopeFollower struct
 * @param input            single channel input array
 * @param attackEnvelope   positive values indicate that we are in the attack portion. output values in range: [0,input]
 * @param afterAttackEnvelope positive values indicate that we are in the portion immediately after the attack range: [0,+?]
 * @param releaseEnvelope  positive values indicate that we are in the release portion. output values in range: [0,input]
 */
void BMTransientEnveloper_processBuffer(BMTransientEnveloper *This,
                                        const float* input,
                                        float* attackEnvelope,
                                        float* afterAttackEnvelope,
                                        float* releaseEnvelope,
                                        size_t numSamples);


void BMTransientEnveloper_setAttackOnsetTime(BMTransientEnveloper *This, float seconds);

void BMTransientEnveloper_setAttackDuration(BMTransientEnveloper *This, float seconds);

void BMTransientEnveloper_setReleaseDuration(BMTransientEnveloper *This, float seconds);





void BMTransientEnveloper_setAttackOnsetTime(BMTransientEnveloper *This, float seconds){
    if(seconds > 0){
        float attackOnsetFc = ARTimeToCutoffFrequency(seconds, BMENV_NUM_STAGES);
    
        for(size_t i=0; i<BMENV_NUM_STAGES; i++)
        BMAttackFilter_setCutoff(&This->attackAF2[i],attackOnsetFc);
    }
    
    else This->filterAttackOnset = false;
}




void BMTransientEnveloper_setAttackDuration(BMTransientEnveloper *This, float seconds){
    
    float attackDurationFc = ARTimeToCutoffFrequency(seconds, BMENV_NUM_STAGES);
    
    for(size_t i=0; i<BMENV_NUM_STAGES; i++)
        BMReleaseFilter_setCutoff(&This->attackRF2[i],attackDurationFc);
}




void BMTransientEnveloper_setReleaseDuration(BMTransientEnveloper *This, float seconds){
    
}




void BMTransientEnveloper_processBuffer(BMTransientEnveloper *This,
                                        const float* input,
                                        float* attackEnvelope,
                                        float* afterAttackEnvelope,
                                        float* releaseEnvelope,
                                        size_t numSamples){
    assert(input != releaseEnvelope && input != attackEnvelope && input != afterAttackEnvelope);
    
    /************************************
     *  Computing the release envelope  *
     ************************************/
    
    // get aliases to improve code readability
    float* fastRelease = afterAttackEnvelope;
    float* slowRelease = releaseEnvelope;
    
    // compute the fast release envelope
    BMReleaseFilter_processBuffer(&This->releaseRF1[0], input, fastRelease, numSamples);
    for (size_t i=1; i<BMENV_NUM_STAGES; i++)
        BMReleaseFilter_processBuffer(&This->releaseRF1[i], fastRelease, fastRelease, numSamples);
    
    // release filter the complete envelope another time to get slower release
    BMReleaseFilter_processBuffer(&This->releaseRF2,fastRelease,slowRelease,numSamples);
    
    // releaseEnvelope = slowRelease - fastRelease
    vDSP_vsub(fastRelease, 1, slowRelease, 1, releaseEnvelope, 1, numSamples);
    
    // use a dynamic smoothing filter on the release envelope to reduce noise
    BMDynamicSmoothingFilter_processBuffer(&This->releaseDSF, releaseEnvelope, releaseEnvelope, numSamples);
    
    
    
    /***********************************
     *  Computing the attack envelope  *
     ***********************************/
    
    // get two aliases to make the code more readable
    float* slowAttack = attackEnvelope;
    float* fastAttack = afterAttackEnvelope;
    
    // Connecting the envelope across signal peaks
    //
    // release filter 1 connects across the peaks to filter most of the
    // audio-frequency content out of the envelope. Longer release time means
    // less bleed, but setting it too high means that attack transients that
    // follow closely after each other will not be detected at correct amplitude.
    //
    // Recommended setting: 1/2 second
    BMReleaseFilter_processBuffer(&This->attackRF1[0],
                                  input, fastAttack, numSamples);
    for(size_t i=1; i<BMENV_NUM_STAGES; i++)
        BMReleaseFilter_processBuffer(&This->attackRF1[i], fastAttack, fastAttack, numSamples);
    
    
    // Create a slower attack envelope to compare with the fast attack envelope
    //
    // Attack filter 1 has a slow attack time and it outputs into a separate buffer
    // the attack time of this filter controls the duration of the attack transients.
    // setting this time too long results in limited ability to detect multiple attack
    // transients in rapid succession. Too short and we won't catch the full amplitude
    // of the transients.
    //
    // Recommended setting: 1/20 second
    BMAttackFilter_processBuffer(&This->attackAF1[0], fastAttack, slowAttack, numSamples);
    for(size_t i=1; i<BMENV_NUM_STAGES; i++)
        BMAttackFilter_processBuffer(&This->attackAF1[i], slowAttack, slowAttack, numSamples);
    
    
    // take (fast - slow) to get the unfiltered attack transient envelope
    vDSP_vsub(slowAttack, 1, fastAttack, 1, attackEnvelope, 1, numSamples);
    
    
    // Implement attack transient duration
    //
    // release filter 2 smooths out the tail end of jitter caused by the unfiltered
    // attacks in the fast attack envelope. That jitter could have been eliminated
    // by attack filtering the the fastAttack envelope, however, doing that would
    // make it impossible to detect transients instantaneously. This method allows
    // us to set the onset time for attack transients right down to zero. A setting
    // of zero means the bleed from the control signal is full-bandwidth, but with
    // RF2 applied, the amplitude of that bleed through is reduced by an order of
    // magnitude. Setting this too low results in very ugly bleed. Setting it higher increases
    // the length of the attack transients.
    //
    // Recommended setting: in the range [1/8, 1/3] seconds
    for (size_t i=0; i<BMENV_NUM_STAGES; i++)
        BMReleaseFilter_processBuffer(&This->attackRF2[i], attackEnvelope, attackEnvelope, numSamples);
    
    
    // Implement attack onset time
    //
    // attack filter 2 controls the onset time of the attack transients. Setting
    // this faster increases the bandwidth of the distortion caused when the
    // control signal bleeds through into the audio band. Setting this too slow
    // results in slow response to transients. For some audio signals this can
    // be set to zero, in which case, we will not do the filtering.
    //
    // Recommended setting: in the range [0,1/200]
    if(This->filterAttackOnset){
        for (size_t i=0; i<BMENV_NUM_STAGES; i++)
            BMAttackFilter_processBuffer(&This->attackAF2[i], attackEnvelope, attackEnvelope, numSamples);
    }
    
    // Process a dynamic smoothing filter.
    // The purpose of this is to further reduce control signal bleeding through
    // to the audio frequencies. The cutoff frequency of the filter increases
    // automatically when the gradient of the input is high to allow instantaneous
    // changes when there is a big jump in the input value while still smoothing
    // out small changes at very low frequency settings. This allows the attack
    // transient to respond instantaneously to new events but still smooth out
    // the tiny jitters that are left over at this stage of the filtering
    // process.
    BMDynamicSmoothingFilter_processBuffer(&This->attackDSF, attackEnvelope, attackEnvelope, numSamples);
    
    
    
    /***************************
     *  After-attack Envelope  *
     ***************************/
    
    // get aliases to improve code readability
    fastAttack = attackEnvelope;
    slowAttack = afterAttackEnvelope;
    
    // release filter the attack envelope again to get a slower attack envelope
    BMReleaseFilter_processBuffer(&This->afterAttackRF, fastAttack, slowAttack, numSamples);
    
    // after-attack = slowAttack - fastAttack
    vDSP_vsub(fastAttack, 1, slowAttack, 1, afterAttackEnvelope, 1, numSamples);
    
}






/*!
 *BMTransientShaper_init
 */
void BMTransientShaper_init(BMTransientShaper *This, float sampleRate){
	BMTransientEnveloper_init(&This->enveloper, sampleRate);
}



/*!
 *BMTransientShaper_processBufferStereo
 */
void BMTransientShaper_processBufferStereo(BMTransientShaper *This,
										   const float* inL, const float* inR,
										   float* outL, float* outR,
										   size_t numSamples){
	while(numSamples > 0){
		size_t numSamplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
		
		// mix to mono
		float half = 0.5;
		vDSP_vasm(inL, 1, inR, 1, &half, This->buffer1, 1, numSamplesProcessing);
		
		// compute the three envelopes
		BMTransientEnveloper_processBuffer(&This->enveloper,
										   This->buffer1,
										   This->attackEnv,
										   This->postAttackEnv,
										   This->releaseEnv,
										   numSamplesProcessing);
		
		// create an alias for readibility
		float* controlSignal = This->buffer1;
		
		// apply the attack envelope to the control signal
		float one = 1.0f;
		vDSP_vsmsa(This->attackEnv, 1, &This->attackStrength, &one, controlSignal, 1, numSamplesProcessing);
		
		// apply the post-attack
		vDSP_vsma(This->postAttackEnv, 1, &This->postAttackStrength, controlSignal, 1, controlSignal, 1, numSamplesProcessing);
		
		// apply the release
		vDSP_vsma(This->releaseEnv, 1, &This->releaseStrength, controlSignal, 1, controlSignal, 1, numSamplesProcessing);
		
		// multiply the control signal by the input and write to output
		vDSP_vmul(controlSignal, 1, inL, 1, outL, 1, numSamplesProcessing);
		vDSP_vmul(controlSignal, 1, inR, 1, outR, 1, numSamplesProcessing);
		
		// advance pointers
		inL += numSamplesProcessing;
		inR += numSamplesProcessing;
		outL += numSamplesProcessing;
		outR += numSamplesProcessing;
		numSamples -= numSamplesProcessing;
	}
}


/*!
 *BMTransientShaper_setAttackStrength
 *
 * @param This pointer to an initialised struct
 * @param strength in -1,1
 */
void BMTransientShaper_setAttackStrength(BMTransientShaper *This, float strength){
	This->attackStrength = strength;
}


/*!
 *BMTransientShaper_setPostAttackStrength
 *
 * @param This pointer to an initialised struct
 * @param strength in -1,1
 */
void BMTransientShaper_setPostAttackStrength(BMTransientShaper *This, float strength){
	This->postAttackStrength = strength;
}


/*!
 *BMTransientShaper_setReleaseStrength
 *
 * @param This pointer to an initialised struct
 * @param strength in -1,1
 */
void BMTransientShaper_setReleaseStrength(BMTransientShaper *This, float strength){
	This->releaseStrength = strength;
}


/*!
 *BMTransientShaper_setAttackTime
 */
void BMTransientShaper_setAttackTime(BMTransientShaper *This, float timeInSeconds){
	BMTransientEnveloper_setAttackDuration(&This->enveloper, timeInSeconds);
}



/*!
 *BMTransientShaper_setPostAttackTime
 */
void BMTransientShaper_setPostAttackTime(BMTransientShaper *This, float timeInSeconds){
	// ?
}




/*!
 *BMTransientShaper_setReleaseTime
 */
void BMTransientShaper_setReleaseTime(BMTransientShaper *This, float timeInSeconds){
	BMTransientEnveloper_setReleaseDuration(&This->enveloper, timeInSeconds);
}

