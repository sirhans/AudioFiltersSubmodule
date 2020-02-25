//
//  BMLowpassedLimiter.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 2/19/20.
//  Anyone may use this file. No restrictions
//

#include "BMLowpassedLimiter.h"

#define BMLPL_LPF_FC 10.0f
#define BMLPL_RELEASE_FC 4.0f
#define BMLPL_LOWPASS_NUMLEVELS 1

void BMLowpassedLimiter_init(BMLowpassedLimiter *This, float sampleRate){
	BMMultiLevelBiquad_init(&This->lpf, BMLPL_LOWPASS_NUMLEVELS, sampleRate, false, true, false);
	BMLopassedLimiter_setLowpassFc(This, BMLPL_LPF_FC);
	BMReleaseFilter_init(&This->rf, BMLPL_RELEASE_FC, sampleRate);
}


void BMLowpassedLimiter_free(BMLowpassedLimiter *This){
	BMMultiLevelBiquad_free(&This->lpf);
}


void BMLopassedLimiter_setLowpassFc(BMLowpassedLimiter *This, float fc){
	for(size_t i=0; i<BMLPL_LOWPASS_NUMLEVELS; i++)
		BMMultiLevelBiquad_setLowPassQ12db(&This->lpf, fc, 0.5f, i);
	
}


void BMLowpassedLimiter_process(BMLowpassedLimiter *This,
								const float* in, float *out,
								size_t numSamples){
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples);
		
		// rectify the input
		vDSP_vabs(in, 1, This->b1, 1, samplesProcessing);
		
		// release filter, then lowpass to get the volume envelope
		BMMultiLevelBiquad_processBufferMono(&This->lpf, This->b1, This->b1, samplesProcessing);
		//BMReleaseFilter_processBuffer(&This->rf, This->b1, This->b1, samplesProcessing);
		
		// apply soft clipping to the volume envelope
		BMAsymptoticLimitPositiveNoSag(This->b1, This->b2, samplesProcessing);
		
		// add a small number to the volume envelope to prevent divide by zero
		float smallNumber = BM_DB_TO_GAIN(-60.0f);
		vDSP_vsadd(This->b1, 1, &smallNumber, This->b1, 1, samplesProcessing);
		
		// find a control signal that would get us from the volume envelope
		// to the clipped volume envelope
		vDSP_vdiv(This->b1, 1, This->b2, 1, This->b2, 1, samplesProcessing);
		
		// apply the control signal to the input signal
		vDSP_vmul(in, 1, This->b2, 1, out, 1, samplesProcessing);
		
		in += samplesProcessing;
		out += samplesProcessing;
		numSamples -= samplesProcessing;
	}
}
