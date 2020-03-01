//
//  BMAttackShaper.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 18/2/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include <Accelerate/Accelerate.h>
#include "BMAttackShaper.h"
#include "Constants.h"
#include "fastpow.h"

void BMAttackShaper_setAttackFrequency(BMAttackShaper *This, float attackFc){
	// set the attack filter
	BMAttackFilter_setCutoff(&This->af, attackFc);
}


void BMAttackShaper_init(BMAttackShaper *This, float sampleRate){
	This->sampleRate = sampleRate;
	
	// initialise filters
	for(size_t i=0; i<BMAS_RF_NUMLEVELS; i++)
		BMReleaseFilter_init(&This->rf[i], BMAS_RF_FC, sampleRate);
	BMAttackFilter_init(&This->af, BMAS_RF_FC, sampleRate);
	BMMultiLevelBiquad_init(&This->hpf, 1, sampleRate, false, true, false);
	BMMultiLevelBiquad_setHighPass6db(&This->hpf, 3000.0f, 0);
	
	// set default attack frequency
	BMAttackShaper_setAttackFrequency(This, BMAS_ATTACK_FC);
    
    // set the delay to 12 samples at 48 KHz sampleRate or
    // stretch appropriately for other sample rates
    size_t numChannels = 1;
    This->delaySamples = round(sampleRate * BMAS_DELAY_AT_48KHZ_SAMPLES / 48000.0f);
    BMShortSimpleDelay_init(&This->dly, numChannels, This->delaySamples);
	
	BMQuadraticLimiter_init(&This->ql, 1.0f, 0.5f);
	
	for(size_t i=0; i<BMAS_DSF_NUMLEVELS; i++)
		BMDynamicSmoothingFilter_init(&This->dsf[i], BMAS_DSF_SENSITIVITY, BMAS_DSF_FC, This->sampleRate);
}




void BMAttackShaper_setAttackTime(BMAttackShaper *This, float attackTime){
	// find the lpf cutoff frequency that corresponds to the specified attack time
	size_t attackFilterNumLevels = 1;
	float fc = ARTimeToCutoffFrequency(attackTime, attackFilterNumLevels);
	
	// set the attack filter
	BMAttackShaper_setAttackFrequency(This, fc);
}





void BMAttackShaper_process(BMAttackShaper *This,
							const float* input,
							float* output,
							size_t numSamples){
    
    while(numSamples > 0){
        size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE,numSamples);

		// highpass because we are mainly interested in HF attack sounds
		BMMultiLevelBiquad_processBufferMono(&This->hpf, input, This->b1, samplesProcessing);
		
        /*******************
         * volume envelope *
         *******************/
        // absolute value
        vDSP_vabs(This->b1, 1, This->b1, 1, samplesProcessing);
        //
        // release filter
		for(size_t i=0; i<BMAS_RF_NUMLEVELS; i++)
			BMReleaseFilter_processBuffer(&This->rf[i], This->b1, This->b1, samplesProcessing);
        //
        // attack filter, buffer to b2
		BMAttackFilter_processBuffer(&This->af, This->b1, This->b2, samplesProcessing);
        
        
        /*************************************************************
         * control signal to force the input down below the envelope *
         *************************************************************/
        // b1 = b2 / (b1 + 0.0001)
        float smallNumber = BM_DB_TO_GAIN(-60.0f);
        vDSP_vsadd(This->b1, 1, &smallNumber, This->b1, 1, samplesProcessing);
        vDSP_vdiv(This->b1, 1, This->b2, 1, This->b1, 1, samplesProcessing);
        //
        // limit the control signal value so that it can reduce but not increase volume
        vDSP_vneg(This->b1, 1, This->b1, 1, samplesProcessing);
        float limit = -BM_DB_TO_GAIN(-0.1); // this value is set to eliminate clicks in low-frequency signals
        vDSP_vthr(This->b1, 1, &limit, This->b1, 1, samplesProcessing);
        vDSP_vneg(This->b1, 1, This->b1, 1, samplesProcessing);
        
        
		
        /************************************************
         * filter the control signal to reduce aliasing *
         ************************************************/
        //
		// convert to decibel scale. this conversion ensures that the
		// dymanic smoothing filter treats transients the same way regardless
		// of the volume of the surrounding audio.
		smallNumber = BM_DB_TO_GAIN(-60.0f);
		vDSP_vsadd(This->b1, 1, &smallNumber, This->b1, 1, samplesProcessing);
		float one = 1.0f;
        vDSP_vdbcon(This->b1,1,&one,This->b1,1,samplesProcessing,0);
		//
        // dynamic smoothing
		for(size_t i=0; i< BMAS_DSF_NUMLEVELS; i++)
			BMDynamicSmoothingFilter_processBufferWithFastDescent(&This->dsf[i],
																  This->b1,
																  This->b1,
																  samplesProcessing);
		//
		// convert back to linear scale
		vector_fastDbToGain(This->b1,This->b1,samplesProcessing);
        
		
        /************************************************************************
         * delay the input signal to enable faster response time without clicks *
         ************************************************************************/
        const float **inputs = &input;
		float *t = This->b2;
        float **outputs = &t;
        BMShortSimpleDelay_process(&This->dly, inputs, outputs, 1, samplesProcessing);
        
        
        /********************************************************
         * apply the volume control signal to the delayed input *
         ********************************************************/
        vDSP_vmul(This->b2, 1, This->b1, 1, output, 1, samplesProcessing);
        
        
        /********************
         * advance pointers *
         ********************/
        input += samplesProcessing;
        output += samplesProcessing;
        numSamples -= samplesProcessing;
    }
}




void BMAttackShaper_free(BMAttackShaper *This){
	BMShortSimpleDelay_free(&This->dly);
}
