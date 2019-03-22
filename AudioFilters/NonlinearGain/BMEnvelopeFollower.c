//
//  BMEnvelopeFollower.c
//  AudioFiltersXCodeProject
//
//  Created by hans anderson on 2/26/19.
//
//  Anyone may use this file without restrictions.
//

#include "BMEnvelopeFollower.h"
#include <math.h>
#include <Accelerate/Accelerate.h>

// set all stages of smoothing filters to critically damped
#define BMENV_FILTER_Q 0.5

// set default time for attack and release
#define BMENV_ATTACK_TIME 1.0f / 1000.0f
#define BMENV_RELEASE_TIME 1.0f / 10.0f

// set default times for transient shaper attack
#define BMTRANS_ATTACK_PEAK_CONNECTION_TIME 1.0f / 2.0f
#define BMTRANS_ATTACK_SLOW_ATTACK_ENV_TIME 1.0f / 20.0f
#define BMTRANS_ATTACK_ONSET_TIME 1.0f / 2000.0f
#define BMTRANS_ATTACK_DURATION 1.0f / 5.0f
#define BMTRANS_DYNAMIC_SMOOTHING_SENSITIVITY 1.0f / 8.0f
#define BMTRANS_DYNAMIC_SMOOTHING_MIN_FC 1.0f

// set default times for transient shaper release





void BMAttackFilter_setCutoff(BMAttackFilter* This, float fc){
    // compute the input gain to the integrators
    float g = tanf(M_PI * fc / This->sampleRate);
    
    // update the first integrator state variable to ensure that the second
    // derivative remains continuous when changing the cutoff freuency
    This->ic1 *= This->g / g;
    
    // then save the new value of the gain coefficient
    This->g = g;
    
    // compute the three filter coefficients
    This->a1 = 1.0f / (1.0f + This->g * (This->g + This->k));
    This->a2 = This->a1 * This->g;
    This->a3 = This->a2 * This->g;
    
    // precompute a value that helps us set the state variable to keep the
    // gradient continuous when switching from release to attack mode
    This->gInv_2 = 1.0f / (2.0f * g);
}




void BMReleaseFilter_setCutoff(BMReleaseFilter* This, float fc){
    // compute the input gain to the integrators
    float g = tanf(M_PI * fc / This->sampleRate);
    
    // update the first integrator state variable to ensure that the second
    // derivative remains continuous when changing the cutoff freuency
    This->ic1 *= This->g / g;
    
    // then save the new value of the gain coefficient
    This->g = g;
    
    // compute the three filter coefficients
    This->a1 = 1.0f / (1.0f + This->g * (This->g + This->k));
    This->a2 = This->a1 * This->g;
    This->a3 = This->a2 * This->g;
}


/*!
 * ARTimeToCutoffFrequency
 * @param time       time in seconds
 * @param numStages  number of stages in the filter cascade
 * @abstract The -3db point of the filters shifts to a lower frequency each time we add another filter to the cascade. This function converts from time to cutoff frequency, taking into account the number of filters in the cascade.
 */
float ARTimeToCutoffFrequency(float time, size_t numStages){
    float correctionFactor = 0.472859f + 0.434404 * (float)numStages;
    if (time > 0) return 1.0f / (time / correctionFactor);
    else return FLT_MAX;
}




void BMAttackFilter_init(BMAttackFilter* This, float Q, float fc, float sampleRate){
    
    This->sampleRate = sampleRate;
    
    This->k = 1.0f / Q;
    This->g = 0.0f;
    This->previousOutputGradient = 0.0f;
    This->previousOutputValue = 0.0f;
    This->attackMode = false;
    
    BMAttackFilter_setCutoff(This, fc);
}



void BMReleaseFilter_init(BMReleaseFilter* This, float Q, float fc, float sampleRate){
    
    This->sampleRate = sampleRate;
    
    This->k = 1.0f / Q;
    This->g = 0.0f;
    This->previousOutputValue = 0.0f;
    This->attackMode = false;
    
    BMReleaseFilter_setCutoff(This, fc);
}




void BMAttackFilter_processBuffer(BMAttackFilter* This,
                                  const float* input,
                                  float* output,
                                  size_t numSamples){
    
    for (size_t i=0; i<numSamples; i++){
        float x = input[i];
        
        // if we are in attack mode,
        if (x > This->previousOutputValue){
            
            // if this is the first sample in attack mode
            if(!This->attackMode){
                // set the attack mode flag
                This->attackMode = true;
                // and update the state variables to get continuous gradient
                This->ic1 = This->previousOutputGradient * This->gInv_2;
                // and continuous output value
                This->ic2 = This->previousOutputValue;
            }
            
            // process the state variable filter
            float v3 = x - This->ic2;
            float v1 = This->a1 * This->ic1 + This->a2 * v3;
            float v2 = This->ic2 + This->a2 * This->ic1 + This->a3 * v3;
            
            // update the state variables
            This->ic1 = 2.0f * v1 - This->ic1;
            This->ic2 = 2.0f * v2 - This->ic2;
            
            // output
            output[i] = v2;
        }
        
        // if we are in release mode,
        else {
            // unset the attack mode flag
            This->attackMode = false;
            
            // copy the input to the output
            output[i] = x;
            
            // and save the gradient so that we have it ready in case the next
            // sample we switch into attack mode
            This->previousOutputGradient = x - This->previousOutputValue;
        }
        
        This->previousOutputValue = output[i];
    }
}




void BMReleaseFilter_processBuffer(BMReleaseFilter* This,
                                   const float* input,
                                   float* output,
                                   size_t numSamples){
    
    for (size_t i=0; i<numSamples; i++){
        float x = input[i];
        
        // if we are in attack mode,
        if (x > This->previousOutputValue){
            // set the attack mode flag
            This->attackMode = true;
            // copy the input to the output
            output[i] = x;
        }
        
        // if we are in release mode,
        else {
            // if this is the first sample in release mode
            if(This->attackMode){
                // unset the attack mode flag
                This->attackMode = false;
                // set the gradient to zero
                This->ic1 = 0.0f;
                // set the second state variable to the previous output
                // to keep the output function continuous
                This->ic2 = This->previousOutputValue;
            }
            
            // process the state variable filter
            float v3 = x - This->ic2;
            float v1 = This->a1 * This->ic1 + This->a2 * v3;
            float v2 = This->ic2 + This->a2 * This->ic1 + This->a3 * v3;
            
            // update the state variables
            This->ic1 = 2.0f * v1 - This->ic1;
            This->ic2 = 2.0f * v2 - This->ic2;
            
            // output
            output[i] = v2;
        }
        
        This->previousOutputValue = output[i];
    }
}





void BMEnvelopeFollower_init(BMEnvelopeFollower* This, float sampleRate){
    
    float attackFc = ARTimeToCutoffFrequency(BMENV_ATTACK_TIME, BMENV_NUM_STAGES);
    float releaseFc = ARTimeToCutoffFrequency(BMENV_ATTACK_TIME, BMENV_NUM_STAGES);
    
    // initialize all the filters
    for(size_t i=0; i<BMENV_NUM_STAGES; i++){
        BMAttackFilter_init(&This->attackFilters[i], BMENV_FILTER_Q, attackFc, sampleRate);
        BMReleaseFilter_init(&This->releaseFilters[i], BMENV_FILTER_Q, releaseFc, sampleRate);
    }
    
}




void BMTransientEnveloper_init(BMTransientEnveloper* This, float sampleRate){

    /*
     * attack transient filters
     */
    float attackPeakConnectionFc = ARTimeToCutoffFrequency(BMTRANS_ATTACK_PEAK_CONNECTION_TIME, BMENV_NUM_STAGES);
    float attackSlowAttackEnvFc = ARTimeToCutoffFrequency(BMTRANS_ATTACK_SLOW_ATTACK_ENV_TIME, BMENV_NUM_STAGES);
    float attackDurationFc = ARTimeToCutoffFrequency(BMTRANS_ATTACK_DURATION, BMENV_NUM_STAGES);
    float attackOnsetTimeFc = ARTimeToCutoffFrequency(BMTRANS_ATTACK_ONSET_TIME, BMENV_NUM_STAGES);
    
    for(size_t i=0; i<BMENV_NUM_STAGES; i++){
        BMReleaseFilter_init(&This->attackRF1[i],BMENV_FILTER_Q,attackPeakConnectionFc,sampleRate);
        BMAttackFilter_init(&This->attackAF1[i],BMENV_FILTER_Q,attackSlowAttackEnvFc,sampleRate);
        BMReleaseFilter_init(&This->attackRF2[i],BMENV_FILTER_Q,attackDurationFc,sampleRate);
        BMAttackFilter_init(&This->attackAF2[i],BMENV_FILTER_Q,attackOnsetTimeFc,sampleRate);
    }
    
    // if the attack onset time is zero, disable attack onset filtering
    This->filterAttackOnset = !(attackOnsetTimeFc == FLT_MAX);
    
    BMDynamicSmoothingFilter_init(&This->attackDSF,
                                  BMTRANS_DYNAMIC_SMOOTHING_SENSITIVITY,
                                  BMTRANS_DYNAMIC_SMOOTHING_MIN_FC,
                                  sampleRate);

    /*
     * release envelope filters
     */
    for(size_t i=0; i<BMENV_NUM_STAGES; i++){
        BMReleaseFilter_init(&This->releaseRF1[i],BMENV_FILTER_Q,0,sampleRate);
    }
    
    BMReleaseFilter_init(&This->releaseRF2,BMENV_FILTER_Q,0,sampleRate);
    
    BMDynamicSmoothingFilter_init(&This->attackDSF,
                                  BMTRANS_DYNAMIC_SMOOTHING_SENSITIVITY,
                                  BMTRANS_DYNAMIC_SMOOTHING_MIN_FC,
                                  sampleRate);
}





void BMEnvelopeFollower_setAttackTime(BMEnvelopeFollower* This, float attackTime){
    
    float attackFc = ARTimeToCutoffFrequency(attackTime,BMENV_NUM_STAGES);
    
    for(size_t i=0; i<BMENV_NUM_STAGES; i++)
        BMAttackFilter_setCutoff(&This->attackFilters[i],attackFc);
}




void BMEnvelopeFollower_setReleaseTime(BMEnvelopeFollower* This, float releaseTime){
    
    float releaseFc = ARTimeToCutoffFrequency(releaseTime,BMENV_NUM_STAGES);
    
    for(size_t i=0; i<BMENV_NUM_STAGES; i++)
        BMReleaseFilter_setCutoff(&This->releaseFilters[i],releaseFc);
}




void BMEnvelopeFollower_processBuffer(BMEnvelopeFollower* This, const float* input, float* output, size_t numSamples){
    
    // process all the release filters in series
    BMReleaseFilter_processBuffer(&This->releaseFilters[0], input, output, numSamples);
    for(size_t i=1; i<BMENV_NUM_STAGES; i++)
        BMReleaseFilter_processBuffer(&This->releaseFilters[i], output, output, numSamples);
    
    // process all the attack filters in series
    for(size_t i=0; i<BMENV_NUM_STAGES; i++)
        BMAttackFilter_processBuffer(&This->attackFilters[i], output, output, numSamples);
}




void BMTransientEnveloper_setAttackOnsetTime(BMTransientEnveloper* This, float seconds){
    
    float attackOnsetFc = ARTimeToCutoffFrequency(seconds, BMENV_NUM_STAGES);
    
    for(size_t i=0; i<BMENV_NUM_STAGES; i++)
        BMAttackFilter_setCutoff(&This->attackAF2[i],attackOnsetFc);
}




void BMTransientEnveloper_setAttackDuration(BMTransientEnveloper* This, float seconds){
    
    float attackDurationFc = ARTimeToCutoffFrequency(seconds, BMENV_NUM_STAGES);
    
    for(size_t i=0; i<BMENV_NUM_STAGES; i++)
        BMReleaseFilter_setCutoff(&This->attackRF2[i],attackDurationFc);
}




void BMTransientEnveloper_setReleaseDuration(BMEnvelopeFollower* This, float seconds){
    
}




void BMTransientEnveloper_processBuffer(BMTransientEnveloper* This,
                                             const float* input,
                                             float* attackEnvelope,
                                             float* afterAttackEnvelope,
                                             float* releaseEnvelope,
                                             size_t numSamples){
    assert(input != releaseEnvelope && input != attackEnvelope && input != afterAttackEnvelope);

    /***********************************
     *  Computing the attack envelope  *
     ***********************************/
    
    // get two aliases to make the code more readable
    float* slowAttack = attackEnvelope;
    float* fastAttack = releaseEnvelope;
    
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

    
    
    
    /************************************
     *  Computing the release envelope  *
     ************************************/
    
    // get aliases to improve code readability
    float* fastRelease = afterAttackEnvelope;
    float* slowRelease = releaseEnvelope;
    
    // reusing the fastAttack envelope from the previous section, attack filter
    // with fast time to get a complete volume envelope
    BMReleaseFilter_processBuffer(&This->releaseRF1[0], input, fastRelease, numSamples);
    for (size_t i=1; i<BMENV_NUM_STAGES; i++)
        BMReleaseFilter_processBuffer(&This->releaseRF1[i], fastRelease, fastRelease, numSamples);
    
    // release filter the complete envelope another time to get slower release
    BMReleaseFilter_processBuffer(&This->releaseRF2,fastRelease,slowRelease,numSamples);
    
    // releaseEnvelope = slowRelease - fastRelease
    vDSP_vsub(fastRelease, 1, slowRelease, 1, releaseEnvelope, 1, numSamples);
    
    // use a dynamic smoothing filter on the release envelope to reduce noise
    BMDynamicSmoothingFilter_processBuffer(&This->releaseDSF, releaseEnvelope, releaseEnvelope, numSamples);
    
    
    /***************************
     *  After-attack Envelope  *
     ***************************/
    
}
