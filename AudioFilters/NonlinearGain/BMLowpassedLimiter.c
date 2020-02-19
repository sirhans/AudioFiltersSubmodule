//
//  BMLowpassedLimiter.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 2/19/20.
//  Anyone may use this file. No restrictions
//

#include "BMLowpassedLimiter.h"


void BMLowpassedLimiter_init(BMLowpassedLimiter *This, float sampleRate){
	BMMultiLevelBiquad_init(&This->lpf, 1, sampleRate, false, true, false);
}


void BMLowpassedLimiter_free(BMLowpassedLimiter *This){
	BMMultiLevelBiquad_free(&This->lpf);
}


void BMLopassedLimiter_setLowpassFc(BMLowpassedLimiter *This, float fc){
	BMMultiLevelBiquad_setLowPassQ12db(&This->lpf, fc, 0.5f, 0);
}


void BMLowpassedLimiter_process(BMLowpassedLimiter *This,
								const float* in, float *out,
								size_t numSamples){
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples);
		
		// rectify the input
		vDSP_vabs(in, 1, This->b1, 1, samplesProcessing);
		
		// lowpass to get the volume envelope
		BMMultiLevelBiquad_processBufferMono(&This->lpf, This->b1, This->b1, samplesProcessing);
		
		// apply soft clipping to the volume envelope
		BMAsymptoticLimitPositiveNoSag(This->b1, This->b2, samplesProcessing);
		
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
