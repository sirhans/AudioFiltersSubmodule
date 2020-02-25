//
//  BMExtremeCompressor.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 18/2/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMExtremeCompressor.h"

#define BMEC_GAIN_REDUCTION_BEFORE_SATURATOR -16.0f



void BMExtremeCompressor_init(BMExtremeCompressor *This, float sampleRate, bool isStereo, size_t oversampleFactor){
	This->osFactor = oversampleFactor;
	This->isStereo = isStereo;
	
	size_t sampleRateOS = oversampleFactor * sampleRate;
	
	// init attack shaper
	BMAttackShaper_init(&This->asL, sampleRateOS);
	if (isStereo) BMAttackShaper_init(&This->asR, sampleRateOS);
	
	// init lowpass limiter
	BMLowpassedLimiter_init(&This->llL, sampleRateOS);
	if (isStereo) BMLowpassedLimiter_init(&This->llR, sampleRateOS);
	
	// init sample rate converters
	enum resamplerType rsType = sampleRate > 50000.0f ? BMRESAMPLER_INPUT_96KHZ : BMRESAMPLER_FULL_SPECTRUM;
	BMUpsampler_init(&This->upsampler, isStereo, oversampleFactor, rsType);
	BMDownsampler_init(&This->downsampler, isStereo, oversampleFactor, rsType);
	
	// allocate buffers
	size_t bufferSize = sizeof(float)*BM_BUFFER_CHUNK_SIZE*oversampleFactor;
	This->b1L = malloc(bufferSize);
	This->b2L = malloc(bufferSize);
	if(isStereo){
		This->b1R = malloc(bufferSize);
		This->b2R = malloc(bufferSize);
	}
}



void BMExtremeCompressor_free(BMExtremeCompressor *This){
	BMAttackShaper_free(&This->asL);
	if(This->isStereo) BMAttackShaper_free(&This->asR);
	
	BMLowpassedLimiter_free(&This->llL);
	if(This->isStereo) BMLowpassedLimiter_free(&This->llR);
	
	BMUpsampler_free(&This->upsampler);
	BMDownsampler_free(&This->downsampler);
		
	free(This->b1L);
	This->b1L = NULL;
	if (This->isStereo){
		free(This->b1R);
		This->b1R = NULL;
	}
	
	free(This->b2L);
	This->b2L = NULL;
	if (This->isStereo){
		free(This->b2R);
		This->b2R = NULL;
	}
}




void BMExtremeCompressor_procesMono(BMExtremeCompressor *This,
									const float *in,
									float *out,
									size_t numSamples){
	assert(!This->isStereo);
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
	
		// upsample
		BMUpsampler_processBufferMono(&This->upsampler, in, This->b1L, samplesProcessing);
		
		// find the upsampled chunk length
		size_t lengthOS = This->osFactor * samplesProcessing;
	
		// apply attack transient shaper to smooth the attack transients
		BMAttackShaper_process(&This->asL, This->b1L, This->b1L, lengthOS);

		// apply lowpassed limiter to get compression without saturation
//		BMLowpassedLimiter_process(&This->llL, This->b1L, This->b1L, lengthOS);

		// apply a gain reduction
		float gainReduction = BM_DB_TO_GAIN(BMEC_GAIN_REDUCTION_BEFORE_SATURATOR);
//		vDSP_vsmul(This->b1L, 1, &gainReduction, This->b1L, 1, lengthOS);

		// apply a soft clipping limiter to tame the clipping level of the attack transients
//		BMAsymptoticLimitNoSag(This->b1L, This->b2L, lengthOS);
		memcpy(This->b2L, This->b1L, sizeof(float)*lengthOS);
		
		// downsample
		BMDownsampler_processBufferMono(&This->downsampler, This->b2L, out, lengthOS);
		
		// advance pointers
		in += samplesProcessing;
		out += samplesProcessing;
		numSamples -= samplesProcessing;
	}
}






void BMExtremeCompressor_procesStereo(BMExtremeCompressor *This,
									const float *inL, const float *inR,
									float *outL, float *outR,
									size_t numSamples){
	assert(This->isStereo);
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
	
		// upsample
		BMUpsampler_processBufferStereo(&This->upsampler,
										inL, inR,
										This->b1L, This->b1R,
										samplesProcessing);
		
		// what is the chunk length now that we have upsampled
		size_t lengthOS = This->osFactor * samplesProcessing;
	
		// apply attack transient shaper to smooth the attack transients
		BMAttackShaper_process(&This->asL, This->b1L, This->b1L, lengthOS);
		BMAttackShaper_process(&This->asR, This->b1R, This->b1R, lengthOS);
	
		// apply lowpassed limiter to get compression without saturation
		BMLowpassedLimiter_process(&This->llL, This->b1L, This->b1L, lengthOS);
		BMLowpassedLimiter_process(&This->llR, This->b1R, This->b1R, lengthOS);
		
		// apply a gain reduction
		float gainReduction = BM_DB_TO_GAIN(BMEC_GAIN_REDUCTION_BEFORE_SATURATOR);
		vDSP_vsmul(This->b1L, 1, &gainReduction, This->b1L, 1, lengthOS);
		vDSP_vsmul(This->b1R, 1, &gainReduction, This->b1R, 1, lengthOS);
	
		// apply a soft clipping limiter to tame the clipping level of the attack transients
		BMAsymptoticLimitNoSag(This->b1L, This->b2L, lengthOS);
		BMAsymptoticLimitNoSag(This->b1R, This->b2R, lengthOS);
	
		// downsample
		BMDownsampler_processBufferStereo(&This->downsampler,
										  This->b2L, This->b2R,
										  outL, outR,
										  lengthOS);
		
		// advance pointers
		inL += samplesProcessing;
		inR += samplesProcessing;
		outL += samplesProcessing;
		outR += samplesProcessing;
		numSamples -= samplesProcessing;
	}
}
