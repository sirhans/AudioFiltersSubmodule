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



void BMAttackShaper_init(BMAttackShaper *This, float sampleRate){
	This->sampleRate = sampleRate;
	
	// initialise filters
	BMReleaseFilter_init(&This->rf1, BMAS_RF_FC, sampleRate);
	for(size_t i=0; i<BMAS_RF2_NUMLEVELS; i++)
		BMReleaseFilter_init(&This->rf2[i], BMAS_RF_FC, sampleRate);
	BMMultiLevelBiquad_init(&This->lpf, 3, sampleRate, false, true, false);
	
	// set default attack time
	BMAttackShaper_setAttackTime(This, BMAS_ATTACK_TIME_DEFAULT);
    
    // set the delay to 12 samples at 48 KHz sampleRate or
    // stretch appropriately for other sample rates
    size_t numChannels = 1;
    This->delaySamples = round(sampleRate * BMAS_DELAY_AT_48KHZ_SAMPLES / 48000.0f);
    BMShortSimpleDelay_init(&This->dly, numChannels, This->delaySamples);
}




void BMAttackShaper_setAttackTime(BMAttackShaper *This, float attackTime){
	// find the lpf cutoff frequency that corresponds to the specified attack time
	float fc = ARTimeToCutoffFrequency(attackTime, BMAS_LPF_NUMLEVELS + BMAS_RF2_NUMLEVELS);
	
	// set all levels of the filter
	for(size_t i=0; i<BMAS_LPF_NUMLEVELS; i++)
		BMMultiLevelBiquad_setLowPassQ12db(&This->lpf, fc, 0.5f, i);
	
	// set the inverted release filter to the same cutoff as the lowpass filter.
	for(size_t i=0; i<BMAS_RF2_NUMLEVELS; i++)
		BMReleaseFilter_setCutoff(&This->rf2[i], fc);
	
	BMDynamicSmoothingFilter_init(&This->dsf, BMAS_DSF_SENSITIVITY, fc / 2.0f, This->sampleRate);
}




void BMAttackShaper_process(BMAttackShaper *This,
							const float* input,
							float* output,
							size_t numSamples){
    
    while(numSamples > 0){
        size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE,numSamples);

        /*******************
         * volume envelope *
         *******************/
        // absolute value
        vDSP_vabs(input, 1, This->b1, 1, samplesProcessing);
        //
        // release filter
        BMReleaseFilter_processBuffer(&This->rf1, This->b1, This->b1, samplesProcessing);
        //
        // lowpass filter
        BMMultiLevelBiquad_processBufferMono(&This->lpf, This->b1, This->b1, samplesProcessing);
        
        
        /*************************************************************
         * control signal to force the input down below the envelope *
         *************************************************************/
        // b1 / (abs(input) + 0.0001)
        float smallNumber = 0.0001;
        vDSP_vabs(input,1,This->b2,1,samplesProcessing);
        vDSP_vsadd(This->b2, 1, &smallNumber, This->b2, 1, samplesProcessing);
        vDSP_vdiv(This->b2, 1, This->b1, 1, This->b1, 1, samplesProcessing);
        //
        // limit the control signal value below 1 so that it can reduce but not increase volume
        vDSP_vneg(This->b1, 1, This->b1, 1, samplesProcessing);
        float negOne = -1.0f;
        vDSP_vthr(This->b1, 1, &negOne, This->b1, 1, samplesProcessing);
        vDSP_vneg(This->b1, 1, This->b1, 1, samplesProcessing);
        
        
        /************************************************
         * filter the control signal to reduce aliasing *
         ************************************************/
        // inverted release filter
		vDSP_vneg(This->b1, 1, This->b1, 1, samplesProcessing);
		for(size_t i=0; i<BMAS_RF2_NUMLEVELS; i++)
			BMReleaseFilter_processBuffer(&This->rf2[i], This->b1, This->b1, samplesProcessing);
		vDSP_vneg(This->b1, 1, This->b1, 1, samplesProcessing);
        //
        // dynamic smoothing
        BMDynamicSmoothingFilter_processBuffer(&This->dsf, This->b1, This->b1, samplesProcessing);
        
        
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
	BMMultiLevelBiquad_free(&This->lpf);
	BMShortSimpleDelay_free(&This->dly);
}
