//
//  BMBlipOscillator.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 25/02/2021.
//  Copyright © 2021 BlueMangoo. All rights reserved.
//

#include "BMBlipOscillator.h"
#include <string.h>
#include "Constants.h"

void BMBlipOscillator_init(BMBlipOscillator *This, float sampleRate, size_t oversampleFactor, size_t filterOrder, size_t numBlips){
	This->numBlips = numBlips;
	This->nextBlip = 0;
	This->sampleRate = sampleRate;
	This->nextPhase = 0.0f;
	This->filterOrder = filterOrder;
}





void BMBlipOscillator_free(BMBlipOscillator *This);





void BMBlipOscilaltor_setLowpassFc(BMBlipOscillator *This, float fc){
	for(size_t i=0; i<This->numBlips; i++)
		BMBlip_update(&This->blips[i], fc, This->filterOrder);
}





void BMBlipOscillator_processChunk(BMBlipOscillator *This, const float *frequencies, float* output, size_t length){
	// convert frequencies to phase increments, accounting for upsampling
	float *phaseIncrements; 													//TODO
	float scale = 1.0 / (This->sampleRate * (float)This->upsampler.upsampleFactor);
	vDSP_vsmul(frequencies, 1, &scale, phaseIncrements, 1, length);
	
	// upsample the phase increments
	size_t lengthOS = length * This->upsampler.upsampleFactor;
	float *phaseIncrementsOS;
	BMGaussianUpsampler_processMono(&This->upsampler, phaseIncrements, phaseIncrementsOS, length);
	
	// take running sum of the phase increments, taking note of integer and
	// fractional index of each place where the phase wraps around to zero
	float *fractionalOffsets; 													//TODO
	size_t *integerOffsets;
	float phase = This->nextPhase;
	size_t j = 0;
	for(size_t i=0; i<lengthOS; i++){
		if(phase > 1.0f){
			phase = fmodf(phase,1.0f);
			integerOffsets[j] = i;
			fractionalOffsets[j++] = phase;
		}
		phase += phaseIncrementsOS[i];
	}
	size_t numImpulses = j;
	This->nextPhase = phase;
	
	
	// set the output to zero
	float *outputOS; 															//TODO
	memset(outputOS,0,sizeof(float)*length);
	
	// process all the blips from start to end of the current buffer and sum into the output
	for(size_t i=0; i<This->numBlips; i++)
		BMBlip_process(&This->blips[i], outputOS, lengthOS);
	
	// for each phase discontinuity index, process a Blip from the index until the end of the buffer
	for(size_t i=0; i<numImpulses; i++){
		BMBlip_restart(&This->blips[This->nextBlip], fractionalOffsets[i]);
		BMBlip_process(&This->blips[This->nextBlip], outputOS + integerOffsets[i], lengthOS - integerOffsets[i]);
	}
	
	// downsample
	BMDownsampler_processBufferMono(&This->downsampler, outputOS, output, lengthOS);
	
	// highpass
	BMMultiLevelBiquad_processBufferMono(&This->highpass, output, output, length);
}





void BMBlipOscillator_process(BMBlipOscillator *This, const float *frequencies, float* output, size_t length){
	size_t samplesLeft = length;
	size_t samplesProcessed = 0;
	
	while(samplesLeft > 0){
		size_t samplesProcessing = BM_MIN(samplesLeft,BM_BUFFER_CHUNK_SIZE);
		BMBlipOscillator_processChunk(This,
									  frequencies + samplesProcessed,
									  output + samplesProcessed,
									  samplesProcessing);
		samplesLeft -= samplesProcessing;
		samplesProcessed += samplesProcessing;
	}
}
