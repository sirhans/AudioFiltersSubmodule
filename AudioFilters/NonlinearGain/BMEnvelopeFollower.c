//
//  BMEnvelopeFollower.c
//  AudioFiltersXCodeProject
//
//  Created by hans anderson on 2/26/19.
//
//  Anyone may use this file without restrictions.
//

#include <math.h>
#include <assert.h>
#include <Accelerate/Accelerate.h>
#include "BMEnvelopeFollower.h"


// set all stages of smoothing filters to critically damped
#define BMENV_FILTER_Q 0.5f

// set default time for attack and release
#define BMENV_ATTACK_TIME 1.0f / 1000.0f
#define BMENV_RELEASE_TIME 1.0f / 10.0f



void BMAttackFilter_setCutoff(BMAttackFilter *This, float fc){
    assert(fc > 0.0f);
    
    This->fc = fc;
    
    // compute the input gain to the integrators
    double g = tan(M_PI * (double)fc / (double)This->sampleRate);
	
	// we need double precision for the coefficient calculation but can use
	// single precision float for the filter processing
	double a1, a2, a3;
    
    // update the first integrator state variable to ensure that the second
    // derivative remains continuous when changing the cutoff freuency
    This->ic1 *= This->g / g;
    
    // then save the new value of the gain coefficient
    This->g = (float)g;
    
    // compute the three filter coefficients
    a1 = 1.0f / (1.0f + g * (g + (double)This->k));
    a2 = a1  * This->g;
    a3 = a2  * This->g;
	
	// copy the double precision filter coefficients in this function
	// to the single precision coefficients in the filter struct
	This->a1 = (float)a1;
	This->a2 = (float)a2;
	This->a3 = (float)a3;
    
    // precompute a value that helps us set the state variable to keep the
    // gradient continuous when switching from release to attack mode
    This->gInv_2 = 1.0f / (2.0f * g);
}




void BMReleaseFilter_setCutoff(BMReleaseFilter *This, float fc){
    assert(fc > 0.0f);
    
    This->fc = fc;
    
    // compute the input gain to the integrators
    double g = tanf(M_PI * fc / This->sampleRate);
	
	// we need double precision for the coefficient calculation but can use
	// single precision float for the filter processing
	double a1, a2, a3;
    
    // update the first integrator state variable to ensure that the second
    // derivative remains continuous when changing the cutoff freuency
    This->ic1 *= This->g / g;
    
    // then save the new value of the gain coefficient
    This->g = (float)g;
    
    // compute the three filter coefficients
    a1 = 1.0f / (1.0f + g * (g + (double)This->k));
    a2 = a1  * This->g;
    a3 = a2  * This->g;
	
	// copy the double precision filter coefficients in this function
	// to the single precision coefficients in the filter struct
	This->a1 = (float)a1;
	This->a2 = (float)a2;
	This->a3 = (float)a3;
}



void BMReleaseFilter_updateSampleRate(BMReleaseFilter *This, float sampleRate){
    This->sampleRate = sampleRate;
    BMReleaseFilter_setCutoff(This, This->fc);
}



void BMAttackFilter_updateSampleRate(BMAttackFilter *This, float sampleRate){
    This->sampleRate = sampleRate;
    BMAttackFilter_setCutoff(This, This->fc);
}



/*!
 * ARTimeToCutoffFrequency
 * @param time       time in seconds
 * @param numStages  number of stages in the filter cascade
 * @abstract The -3db point of the filters shifts to a lower frequency each time we add another filter to the cascade. This function converts from time to cutoff frequency, taking into account the number of filters in the cascade.
 */
float ARTimeToCutoffFrequency(float time, size_t numStages){
    assert (time > 0.0f);
    float correctionFactor = 0.472859f + 0.434404 * (float)numStages;
    return 1.0f / (time / correctionFactor);
}




void BMAttackFilter_init(BMAttackFilter *This, float fc, float sampleRate){
    
    This->sampleRate = sampleRate;
    
    This->k = 1.0f / BMENV_FILTER_Q;
    This->g = 0.0f;
    This->previousOutputGradient = 0.0f;
    This->previousOutputValue = 0.0f;
    This->attackMode = false;
    This->ic1 = This->ic2 = 0;
    
    BMAttackFilter_setCutoff(This, fc);
}



void BMReleaseFilter_init(BMReleaseFilter *This, float fc, float sampleRate){
    
    This->sampleRate = sampleRate;
    
    This->k = 1.0f / BMENV_FILTER_Q;
    This->g = 0.0f;
    This->previousOutputValue = 0.0f;
    This->attackMode = false;
    This->ic1 = This->ic2 = 0;
    
    BMReleaseFilter_setCutoff(This, fc);
}




void BMAttackFilter_processBuffer(BMAttackFilter *This,
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
                This->ic1 = This->previousOutputGradient  *This->gInv_2;
                // and continuous output value
                This->ic2 = This->previousOutputValue;
            }
            
            // process the state variable filter
            float v3 = x - This->ic2;
            float v1 = This->a1  *This->ic1 + This->a2 * v3;
            float v2 = This->ic2 + This->a2  *This->ic1 + This->a3 * v3;
            
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



void BMReleaseFilter_processBufferNegative(BMReleaseFilter *This,
                                   const float* input,
                                   float* output,
                                   size_t numSamples){
    // invert the sign of the signal
    vDSP_vneg(input, 1, output, 1, numSamples);
    BMReleaseFilter_processBuffer(This, output, output, numSamples);
    vDSP_vneg(output, 1, output, 1, numSamples);
}



void BMReleaseFilter_processBuffer(BMReleaseFilter *This,
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
            float v1 = This->a1  *This->ic1 + This->a2 * v3;
            float v2 = This->ic2 + This->a2  *This->ic1 + This->a3 * v3;
            
            // update the state variables
            This->ic1 = 2.0f * v1 - This->ic1;
            This->ic2 = 2.0f * v2 - This->ic2;
            
            // output
            output[i] = v2;
        }
        
        This->previousOutputValue = output[i];
    }
}





void BMEnvelopeFollower_init(BMEnvelopeFollower *This, float sampleRate){
    BMEnvelopeFollower_initWithCustomNumStages(This, BMENV_NUM_STAGES, BMENV_NUM_STAGES, sampleRate);
}





void BMEnvelopeFollower_free(BMEnvelopeFollower *This){
    free(This->attackFilters);
    free(This->releaseFilters);
    This->attackFilters = NULL;
    This->releaseFilters = NULL;
}



/*!
 *BMEnvelopeFollower_initWithCustomNumStages
 */
void BMEnvelopeFollower_initWithCustomNumStages(BMEnvelopeFollower *This,
                                                size_t numReleaseStages,
                                                size_t numAttackStages,
                                                float sampleRate){
    // there must be at least one release filter
    assert(numReleaseStages > 0);
    assert(numAttackStages >= 0);
    
    // don't process the attack filters if there aren't any
    if (numAttackStages == 0) This->processAttack = false;
    
    // remember how many stages we have
    This->numAttackStages = numAttackStages;
    This->numReleaseStages = numReleaseStages;
    
    // set the attack and release filter cutoff frequencies, adjusted for the
    // number of stages
    float releaseFc = ARTimeToCutoffFrequency(BMENV_RELEASE_TIME, This->numReleaseStages);
    float attackFc = 1.0f;
    if(numAttackStages > 0)
        attackFc = ARTimeToCutoffFrequency(BMENV_ATTACK_TIME, This->numAttackStages);
    
    // allocate memory for the filter structs
    This->attackFilters = malloc(sizeof(BMAttackFilter)*This->numAttackStages);
    This->releaseFilters = malloc(sizeof(BMReleaseFilter)*This->numReleaseStages);
    
    // initialize all the filters
    for(size_t i=0; i<This->numAttackStages; i++)
        BMAttackFilter_init(&This->attackFilters[i], attackFc, sampleRate);
    for(size_t i=0; i<This->numReleaseStages; i++)
        BMReleaseFilter_init(&This->releaseFilters[i], releaseFc, sampleRate);
}





void BMEnvelopeFollower_setAttackTime(BMEnvelopeFollower *This, float attackTimeSeconds){
    assert(attackTimeSeconds >= 0.0f);
    
    // if the attack time is zero, don't process the attack at all
    if(attackTimeSeconds == 0.0f || This->numAttackStages == 0){
        This->processAttack = false;
    }
    
    // for non-zero attack time, set the attack filter frequency
    else {
        float attackFc = ARTimeToCutoffFrequency(attackTimeSeconds,This->numAttackStages);
        This->processAttack = true;
        for(size_t i=0; i<This->numAttackStages; i++)
            BMAttackFilter_setCutoff(&This->attackFilters[i],attackFc);
    }
}




void BMEnvelopeFollower_setReleaseTime(BMEnvelopeFollower *This, float releaseTimeSeconds){
    
    float releaseFc = ARTimeToCutoffFrequency(releaseTimeSeconds,This->numReleaseStages);
    
    for(size_t i=0; i<This->numReleaseStages; i++)
        BMReleaseFilter_setCutoff(&This->releaseFilters[i],releaseFc);
}




void BMEnvelopeFollower_processBuffer(BMEnvelopeFollower *This, const float* input, float* output, size_t numSamples){
    
    // process all the release filters in series
    BMReleaseFilter_processBuffer(&This->releaseFilters[0], input, output, numSamples);
    for(size_t i=1; i<This->numReleaseStages; i++)
        BMReleaseFilter_processBuffer(&This->releaseFilters[i], output, output, numSamples);

    if(This->numAttackStages > 0){
        // process all the attack filters in series
        for(size_t i=0; i<This->numAttackStages; i++)
            BMAttackFilter_processBuffer(&This->attackFilters[i], output, output, numSamples);
    }
}


