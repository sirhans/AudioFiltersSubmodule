//
//  BMBlipOscillator.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 25/02/2021.
//  We release this file into the public domain without restrictions
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
    This->previousPhaseIncrement = 0.0f;
    
    size_t bufferSize = BM_BUFFER_CHUNK_SIZE*oversampleFactor;
    This->b1 = malloc(sizeof(float)*bufferSize);
    This->b2 = malloc(sizeof(float)*bufferSize);
    This->b3i = malloc(sizeof(size_t)*bufferSize);
    
    float lowpassFc = sampleRate * 0.25f;
    for(size_t i=0; i<numBlips; i++)
        BMBlip_init(&This->blips[i], filterOrder, lowpassFc, sampleRate);
}





void BMBlipOscillator_free(BMBlipOscillator *This){
    free(This->b1);
    free(This->b2);
    free(This->b3i);
    This->b1 = NULL;
    This->b2 = NULL;
    This->b3i = NULL;
    
    for(size_t i=0; i<This->numBlips; i++)
        BMBlip_free(&This->blips[i]);
}





void BMBlipOscilaltor_setLowpassFc(BMBlipOscillator *This, float fc){
	for(size_t i=0; i<This->numBlips; i++)
		BMBlip_update(&This->blips[i], fc, This->filterOrder);
}





void BMBlipOscillator_processChunk(BMBlipOscillator *This, const float *log2Frequencies, float* output, size_t length){
    
    // convert logFrequencies to linear scale frequencies
    float *frequencies = This->b2;
    int length_i = (int)length;
    vvexp2f(frequencies, log2Frequencies, &length_i);
    
	// convert frequencies to phase increments, accounting for upsampling
    float *phaseIncrements = This->b1;
	float scale = 1.0 / (This->sampleRate * (float)This->upsampler.upsampleFactor);
	vDSP_vsmul(frequencies, 1, &scale, phaseIncrements, 1, length);
	
	// upsample the phase increments
	size_t lengthOS = length * This->upsampler.upsampleFactor;
    float *phaseIncrementsOS = This->b2;
	BMGaussianUpsampler_processMono(&This->upsampler, phaseIncrements, phaseIncrementsOS, length);
	
	// take running sum of the phase increments, taking note of integer and
	// fractional index of each place where the phase wraps around to zero
    float *fractionalOffsetsOS = This->b1;
	size_t *integerOffsetsOS = This->b3i;
	float phase = This->nextPhase;
	size_t j = 0;
	for(size_t i=0; i<lengthOS; i++){
		if(phase > 1.0f){
            // the phase must be in the range [0,1). We use the mod operation to wrap it around instead of just subtracting 1.0f in order to handle cases where the frequency of the oscillator exceeds the sample rate.
            phase = fmodf(phase, 1.0f);
            // the integer offset is the sample number immediately before the discontinuity. Note that by calculating it this way we are actually delaying the offset by one sample since we waited until the phase exceeds 1.0 before marking the offset.
			integerOffsetsOS[j] = i;
            // the fractional offset is the position of the discontinuity between integerOffset and integerOffset+1. Therefore the location of the discontinuity is integerOffset + fractionalOffset.
            float fractionalOffset = phase / fmodf(This->previousPhaseIncrement, 1.0f);
			fractionalOffsetsOS[j++] = fractionalOffset;
		}
        float phaseIncrement = phaseIncrementsOS[i];
		phase += phaseIncrement;
        This->previousPhaseIncrement = phaseIncrement;
	}
	size_t numImpulses = j;
	This->nextPhase = phase;
	
	// set the output to zero
    float *outputOS = This->b2;
	memset(outputOS,0,sizeof(float)*lengthOS);
	
	// process all the blips from start to end of the current buffer and sum into the output
	for(size_t i=0; i<This->numBlips; i++)
		BMBlip_process(&This->blips[i], outputOS, lengthOS);
	
	// for each phase discontinuity index, process a Blip from the index until the end of the buffer
	for(size_t i=0; i<numImpulses; i++){
        BMBlip_restart(&This->blips[This->nextBlip],fractionalOffsetsOS[i]);
        BMBlip_process(&This->blips[This->nextBlip], outputOS + integerOffsetsOS[i], lengthOS - integerOffsetsOS[i]);
	}
	
	// downsample
	BMDownsampler_processBufferMono(&This->downsampler, outputOS, output, lengthOS);
	
	// highpass
	BMMultiLevelBiquad_processBufferMono(&This->highpass, output, output, length);
}




/*
 * This is a wrapper for the processChunk that ensures the output buffer size does not exceed the length of the internal buffers.
 */
void BMBlipOscillator_process(BMBlipOscillator *This, const float *log2Frequencies, float* output, size_t length){
	size_t samplesLeft = length;
	size_t samplesProcessed = 0;
	
	while(samplesLeft > 0){
		size_t samplesProcessing = BM_MIN(samplesLeft,BM_BUFFER_CHUNK_SIZE);
		BMBlipOscillator_processChunk(This,
									  log2Frequencies + samplesProcessed,
									  output + samplesProcessed,
									  samplesProcessing);
		samplesLeft -= samplesProcessing;
		samplesProcessed += samplesProcessing;
	}
}
