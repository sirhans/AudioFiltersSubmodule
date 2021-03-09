//
//  BMBlipOscillator.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 25/02/2021.
//  Copyright © 2021 BlueMangoo. All rights reserved.
//

#include "BMBlipOscillator.h"
#include <string.h>
#include "BMIntegerMath.h"
#include "Constants.h"

#define BM_BLIP_MIN_OUTPUT 0.0000001 // -140 dB


void BMBlip_update(BMBlip *This, float lowpassFc, size_t filterOrder){
    // update p and n
    
   // TODO: update p and n
    
    // set a pointer to the currently unused exp buffer
    float *exp_backBuffer = This->exp_b1;
    if(This->exp_ptr == This->exp_b1)
        exp_backBuffer = This->exp_b2;
    
    // fill the buffer
    float zero = 0.0f;
    float increment = -1.0 * This->n * (This->sampleRate / 48000.0f) / This->p;
    vDSP_vramp(&zero, &increment, exp_backBuffer, 1, This->bufferLength);
    int bufferLengthI = (int)This->bufferLength;
    vvexpf(exp_backBuffer, exp_backBuffer, &bufferLengthI);
    
    // notify the oscillator to flip the buffers
    This->expBufferNeedsFlip = true;
}


void BMBlip_init(BMBlip *This, size_t filterOrder, size_t oversampleFactor, float lowpassFc, float sampleRate){
	assert(isPowerOfTwo(filterOrder));
	
    This->expBufferNeedsFlip = false;
	This->p = sampleRate / lowpassFc;
	This->n_i = filterOrder;
	This->n = This->n_i;
	This->bufferLength = oversampleFactor * BM_BUFFER_CHUNK_SIZE;
	int bufferLengthI = (int)This->bufferLength;
	
    // allocate buffers for pre-computing the exp function
    This->exp_b1 = malloc(sizeof(float) * This->bufferLength);
    This->exp_b2 = malloc(sizeof(float) * This->bufferLength);
    
	// fill the buffer exp_b1 with a decaying exponential to avoid doing exponentiation in real time
	float zero = 0.0f;
	float increment = -1.0 * This->n * (sampleRate / 48000.0f) / This->p;
	vDSP_vramp(&zero, &increment, This->exp_b1, 1, This->bufferLength);
	vvexpf(This->exp_b1, This->exp_b1, &bufferLengthI);
    
    // set the exp pointer to b1
    This->exp_ptr = This->exp_b1;
	
	// init variables
	This->nextIndex = 0;
	This->negNOverP = -This->n / This->p;
	This->pHatNegN = powf(This->p, -This->n);
	This->lastOutput = 1.0f;
	This->sampleRate = sampleRate;
}



/*!
 *BMBlip_process
 *
 * @param This pointer to an initialised struct
 * @param t the time parameter
 * @param b1 buffer of length length
 * @param b2 buffer of length length
 * @param output array of length length. output is summed into here so it should be initialised to zero before calling
 * @param length length of buffers
 */
void BMBlip_process(BMBlip *This, const float *t, float *b1, float *b2, float *output, size_t length){
	
	// Mathematica:
	//   E^(n - (n t)/p) p^-n t^n
	
	// E^(n - (n t)/p)
	//
	// In This->expb is a buffer containing E^(- n t / p) for t = [0..length]
	// By multiplying This->expb by a constant scaling factor we can get
	// E^(n - (n t)/p) without doing exponentiation in real time.
	float expScale = expf(This->n - (This->n * t[0] / This->p));
	vDSP_vsmul(This->exp_ptr, 1, &expScale, b1, 1, length);
	
	//
	// b2 = t^n
	// assert(isPowerOfTwo(n))
	vDSP_vsq(t, 1, b2, 1, length);
	size_t c = 2;
	while (c<This->n_i)
		vDSP_vsq(b2, 1, b2, 1, length);
	//
	// b2 = p^(-n) t^n
	vDSP_vsmul(b2, 1, &This->pHatNegN, b2, 1, length);
	
	//
	// output += b1 * b2
	vDSP_vma(b1, 1, b2, 1, output, 1, output, 1, length);
	
	// cache the last value
	This->lastOutput = b1[length-1] * b2[length-1];
}


void BMBlip_processBuffer(BMBlip *This, const size_t *integerOffsets, const float *fractionalOffsets, float *output, float *b1, float *b2, float *b3, size_t length){
	// TODO: don't process these if the output is too small to be significant
	
	// process from the zero sample until the first integer offset
	size_t samplesTillNextBlip = integerOffsets[0] + 1;
	float increment = 1.0f * This->sampleRate / 48000.0f;
	vDSP_vramp(&This->nextIndex, &increment, b3, 1, samplesTillNextBlip);
	if(This->lastOutput > BM_BLIP_MIN_OUTPUT){
		BMBlip_process(This, b3, b1, b2, output, samplesTillNextBlip);
		
		// save the last output from the previous call
		This->lastOutput = 0.0f;
		BMBlip_process(This, b3 + samplesTillNextBlip-1, b1 + samplesTillNextBlip-1, b2 + samplesTillNextBlip-1, &This->lastOutput, 1);
	}
	
	// process from the nth offset to the next (n+1) offset
	while( // Handle the case where there is no integer offset left or none appears in this buffer call
}



void BMBlipOscillatorProcess(BMBlipOscillator *This, const float *frequencies, float *output, size_t length){
	
	// convert from frequencies to phase increments
	
	// find the integer and fractional parts of the sample index of each impulse.
	// store in a multi-dimensional array with one row for each Blip generator.
	BMBlipOscillator_impulseIndices(This, This->phaseIncrements, length);
	
	// upsample
	BMGaussianUpsampler_processMono(&This->upsampler, This->phaseIncrements, This->phaseIncrementsOS, length);
	size_t lengthOS = length * This->upsampler.upsampleFactor;
	
	// initialise the output to zero
	memset(This->outputOS,0,sizeof(float)*lengthOS);
	
	// for each row of indices, process a Blip generator and sum its output into the mix
	for (size_t i=0; i<This->numBlips; i++){
		BMBlip_process(This->blips[i], This->integerOffsets[i], This->fractionalOffsets[i], This->outputOS, lengthOS);
	}
	
	// downsample
	BMDownsampler_processBufferMono(&This->downsampler, This->outputOS, output, lengthOS);
	
	// highpass filter to eliminate DC offset
	BMMultiLevelBiquad_processBufferMono(&This->highpass, output, output, length);
}
