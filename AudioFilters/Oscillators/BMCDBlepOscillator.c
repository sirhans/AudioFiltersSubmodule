//
//  BMCDBlepOscillator.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 11/26/20.
//  Copyright © 2020 BlueMangoo. We hereby release this file into the public
//  domain without restrictions.
//

#include "BMCDBlepOscillator.h"
#include <assert.h>
#include "BMIntegerMath.h"
#include "Constants.h"





/*
 * The impulse response of a lowpass filter has the following form,
 *
 *   x[s_] := (a + b*s + c*s^2 + d*s^3) * E^(r*s)
 *
 * Setting boundary conditions x[0] == 0, x[1] == 1, x'[0] == 0, x''[0] == 0,
 * we solve,
 *
 *   x[s_] := s^3 * E^(-s)
 *
 * More generally, the impulse response of the nth order filter is
 *
 *   x[s_] := s^n * E^(-s)
 *
 * When we integrate this to get the step response we have,
 *
 *   X[t_] := poly[t] * E^(-t)
 *
 * where poly[t] is an nth order polynomial whose coefficients are given by the
 * following formula,
 *
 *   integratedPolyCoefficientList[n_] := Join[{1}, Table[1/Factorial[m], {m, 1, n}]]
 *
 * the following function computes this list of coefficients
 */
void intPolyCoefList(float *coefList, size_t order, float sampleRate){
	assert(order > 0);
	
	coefList[0] = 1.0f;
	
	for(size_t i=1; i<order; i++)
	coefList[i] = 1.0f / (float)BMFactorial(i);
	
	// a canonical saw wave in the range [-1,1] requires the step response to
	// go from 2 to 0. We scale it up:
	float two = 2.0f;
	vDSP_vsmul(coefList, 1, &two, coefList, 1, order);
}



/*
 * Plan for implementing the oscillator
 *
 * 1. Upsample the frequency input arrays
 * 2. Convert frequency to phase
 * 3. Generate naive saw wave and write to buffer
 * 4. Add the output of N critically-damped lowpass filtered step functions to the naive waveform
 * 5. Downsample
 * 6. Highpass filter to remove DC offset caused by summing the BLEPS
 *
 * Step 2 is the tricky part. First, for each sample of output we need to know
 * the input variable offset that puts each of the N step functions in its right
 * position relative to the naive oscillator output.
 *
 * We calculate the N offsets as follows:
 *
 *   The input argument for each step function a linear ramp that resets its
 *   position occasionally. We start by generating the linear ramps without the
 *   reset points. Then we scan through the naive oscillator output and each
 *   time we reach a discontinuity we subtract a constant from the input array
 *   of the oldest ramp, starting at the discontinuity and extending until the
 *   end of the array. At the end of the buffer we save the next value in the
 *   ramp so that we can continue on where we left off.
 */





void BMCDBlepOscillator_init(BMCDBlepOscillator *This, size_t numBleps, size_t filterOrder, size_t oversampleFactor, float sampleRate){
	assert(numBleps <= BMCDBLEP_MAX_BLEPS);
	assert(filterOrder <= BMCDBLEP_MAX_FILTER_ORDER);
	
	This->numBleps = numBleps;
	This->filterOrder = filterOrder;
	This->oversampleFactor = oversampleFactor;
	This->sampleRate = sampleRate;
	
	// start the bleps with the ramps at a large number so that their output
	// will be near zero
	for(size_t i=0; i<numBleps; i++)
		This->blepInputStart[i] = sampleRate * 10;
	
	
	// allocate space for the blep ramp buffers
	for(size_t i=0; i<numBleps; i++)
		This->blepRampBuffers[i] = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE * oversampleFactor);
	
	// compute the step response coefficients
	intPolyCoefList(This->stepResponseCoefficients, filterOrder, sampleRate * (float)oversampleFactor);
	
	// init the upsampler
	BMGaussianUpsampler_init(&This->upsampler, oversampleFactor, 3);
	
	// init the downsampler
	bool stereo = false;
	BMDownsampler_init(&This->downsampler, stereo, oversampleFactor, BMRESAMPLER_FULL_SPECTRUM);
	
	// init the DC blocking filter
	BMMultiLevelBiquad_init(&This->highpass, 1, sampleRate, stereo, true, false);
	BMMultiLevelBiquad_setHighPass6db(&This->highpass, 40.0f, 0);
	
	// allocate some buffers
	This->b1 = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE * oversampleFactor);
	This->b2 = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE * oversampleFactor);
	This->b3 = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE * oversampleFactor);
	
	// calculate the BLEP ramp increment. This is the change in the BLEP input
	// for each sample of oversampled audio input.
	//
	// blepScale controls the cutoff frequency of the lowpass filter by setting
	// the time in seconds to reach 1 on the x-axis of the graph of the BLEP
	// function. For example, if blepScale is 1/1000 then it takes 1 ms for the
	// blep to reach 1 on the x axis.
	float blepScale = 1.0 / 1000.0;
	This->blepRampIncrement = 1.0f / (blepScale * sampleRate * (float)oversampleFactor);
}





void BMCDBlepOscillator_free(BMCDBlepOscillator *This){
	for(size_t i=0; i<This->numBleps; i++){
		free(This->blepRampBuffers[i]);
		This->blepRampBuffers[i] = NULL;
	}
	
	BMDownsampler_free(&This->downsampler);
	BMGaussianUpsampler_free(&This->upsampler);
	BMMultiLevelBiquad_free(&This->highpass);
	
	free(This->b1);
	This->b1 = NULL;
	free(This->b2);
	This->b2 = NULL;
	free(This->b3);
	This->b3 = NULL;
}




/*!
 *BMCDBlepOscillator_naiveSaw
 *
 * PHASES MUST BE IN RANGE [-1,1]
 */
void BMCDBlepOscillator_naiveSaw(const float *phases, float *output, size_t length){
	if(phases != output)
		memcpy(output, phases, sizeof(float)*length);
	
	// if processing in place then we do nothing.
}


void BMCDBlepOscillator_blepRamps(BMCDBlepOscillator *This, size_t length){
	for(size_t i=0; i<This->numBleps; i++){
		vDSP_vramp(&This->blepInputStart[i], &This->blepRampIncrement, This->blepRampBuffers[i], 1, length);
	}
}



void BMDCBlepOscillator_processBlep(BMCDBlepOscillator *This, float *t, float *output, size_t numSamples){
	// evaluate the polynomial
	vDSP_vpoly(This->stepResponseCoefficients, 1, t, 1, This->b2, 1, numSamples, This->filterOrder-1);
	
	// invert the sign of the input
	vDSP_vneg(t, 1, t, 1, numSamples);
	
	// evaluate the exponential
	int numSamplesI = (int)numSamples;
	vvexpf(This->b3, t, &numSamplesI);
	
	// multiply the polynomial by the exponential and sum into the output
	vDSP_vma(This->b2, 1, This->b3, 1, output, 1, output, 1, numSamples);
}




void BMCDBlepOscillator_process(BMCDBlepOscillator *This, const float *frequencies, float *output, size_t numSamples){
	
	size_t samplesRemaining = numSamples;
	size_t i=0;
	while (samplesRemaining > 0){
		size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, samplesRemaining);
		size_t samplesProcessingOS = This->oversampleFactor * samplesProcessing;
		
		// upsample the frequencies
		float * frequenciesUS = This->b1;
		BMGaussianUpsampler_processMono(&This->upsampler, frequencies + i, frequenciesUS, samplesProcessing);
		
		// convert frequencies to phases
		
		// process naive oscillator
		BMCDBlepOscillator_naiveSaw(const float *phases, float *output, samplesProcessingOS);
		
		// fill the blep input buffers with smoothly ascending ramps. These are
		// simply counting the passing of time with each sample.
		BMCDBlepOscillator_blepRamps(This, samplesProcessingOS);
		
		// scan the naive oscillator output for discontinuities. Each time you
		// find one, subtract a constant from one of the BLEP input buffers,
		// starting from the time index of the discontinuity and continuing to
		// the end of the buffer.
		
		
		// sum the blep ramps into the output
		for(size_t i=0; i<This->numBleps; i++)
		BMDCBlepOscillator_processBlep(This, This->blepRampBuffers[i], <#float *output#>, samplesProcessingOS);
		
		// downsample
		BMDownsampler_processBufferMono(&This->downsampler, <#float *input#>, output+i, samplesProcessingOS);
		
		// advance indices
		samplesRemaining -= samplesProcessing;
		i += samplesProcessing;
	}
	
	// highpass filter
	BMMultiLevelBiquad_processBufferMono(&This->highpass, output, output, numSamples);
}
