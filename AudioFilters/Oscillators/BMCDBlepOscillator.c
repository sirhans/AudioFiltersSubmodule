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





void BMCDBlepOscillator_init(BMCDBlepOscillator *This, size_t numBleps, size_t filterOrder, size_t oversampleFactor, float sampleRate){
	assert(numBleps <= BMCDBLEP_MAX_BLEPS);
	assert(filterOrder <= BMCDBLEP_MAX_FILTER_ORDER);
	
	This->numBleps = numBleps;
	This->filterOrder = filterOrder;
	This->oversampleFactor = oversampleFactor;
	This->sampleRate = sampleRate;
    This->nextStartPhase = 0;
    This->oldestBlep = 0;
    This->previousEndValue = 0.0f;
	
	// start the bleps with the ramps at a large number so that their output
	// will be near zero
	for(size_t i=0; i<numBleps; i++)
		This->blepInputInitialValue[i] = sampleRate * 10;
	
	
	// allocate space for the blep ramp buffers
	for(size_t i=0; i<numBleps; i++)
		This->blepInputBuffers[i] = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE * oversampleFactor);
    
    // start the offset for writing to blep input buffers at zero
    for(size_t i=0; i<numBleps; i++)
        This->blepInputWriteOffset[i] = 0;
	
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
	This->b4 = malloc(sizeof(size_t) * BM_BUFFER_CHUNK_SIZE * oversampleFactor);
	This->archetypeWavelengths = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE * oversampleFactor);
	
	// the archetype wavelength is the wavelength of the basic wave shape before
	// we scale it to get the wavelength corresponding to the input frequency.
	// We need an entire vector containing the same value because there is no
	// vector - scalar floating point mod function in the Accelerate framework
	float archetypeWavelength = 2.0f;
	vDSP_vfill(&archetypeWavelength, This->archetypeWavelengths, 1, BM_BUFFER_CHUNK_SIZE * oversampleFactor);
	
	// calculate the BLEP ramp increment. This is the change in the BLEP input
	// for each sample of oversampled audio input.
	//
	// blepScale controls the cutoff frequency of the lowpass filter by setting
	// the time in seconds to reach 1 on the x-axis of the graph of the BLEP
	// function. For example, if blepScale is 1/1000 then it takes 1 ms for the
	// blep to reach 1 on the x axis.
	float blepScale = 1.0 / 1000.0;
	This->blepInputIncrement = 1.0f / (blepScale * sampleRate * (float)oversampleFactor);
}





void BMCDBlepOscillator_free(BMCDBlepOscillator *This){
	for(size_t i=0; i<This->numBleps; i++){
		free(This->blepInputBuffers[i]);
		This->blepInputBuffers[i] = NULL;
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
    free(This->b4);
    This->b4 = NULL;
	free(This->archetypeWavelengths);
	This->archetypeWavelengths = NULL;
    
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




//void BMCDBlepOscillator_blepInputs(BMCDBlepOscillator *This, size_t length){
//	for(size_t i=0; i<This->numBleps; i++){
//		vDSP_vramp(&This->blepInputStart[i], &This->blepInputIncrement, This->blepInputBuffers[i], 1, length);
//	}
//}





void BMCDBlepOscillator_processBlep(BMCDBlepOscillator *This, float *t, float *output, size_t numSamples){
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




void BMCDBlepOscillator_freqsToPhases(BMCDBlepOscillator *This, const float *frequencies, float *phases, size_t length){
    
    // this scaling factor converts from frequency to phase increment.
    // We are generating waveforms on the interval from -1 to 1.
    // Therefore a freqeuency of f implies a phase increment of (f * archetypeWavelength / sampleRate)
    // for each sample.
    float scalingFactor = This->archetypeWavelengths[0] / (This->sampleRate * (float)This->oversampleFactor);
    
    // Here we want to running sum the phase increments into the phase buffer to
    // get the phases.
    //
    // Because the vrsum function requires a scaling factor, we conveniently
    // convert from frequency to phase increment and do the running sum in a
    // single function call.
    vDSP_vrsum(frequencies, 1, &scalingFactor, phases, 1, length);
    
    // To ensure phase continuity between buffers we adjust the start phase to
    // align with the end phase of the previous buffer
    vDSP_vsadd(phases, 1, &This->nextStartPhase, phases, 1, length);
    
    // compute the start phase of the next buffer.
    // note that we shift the phase to centre on zero after this. The formula
    // below takes into account that the shift has not yet occured at this point.
    This->nextStartPhase = phases[length-1] + frequencies[length-1]*scalingFactor;
    This->nextStartPhase = fmodf(This->nextStartPhase, This->archetypeWavelengths[0]);
	This->nextStartPhase += This->archetypeWavelengths[0] * -0.5f;
    
    // use floating point mod to wrap the phases into [0,archetypeWavelength]
    int length_i = (int)length;
    vvfmodf(phases, phases, This->archetypeWavelengths, &length_i);
    
    // subtract to shift the phases into [-archetypeWavelength/2,archetypeWavelength/2]
    float negHalfWavelength = This->archetypeWavelengths[0] * -0.5f;
    vDSP_vsadd(phases, 1, &negHalfWavelength, phases, 1, length);
}



/*!
 *BMCDBlepOscillator_discontinuityOffset
 *
 * Given the ramp increment and the next value after the discontinuity,
 * return the offset between the last value before the discontinuity and the
 * discontinuity itself.
 */
float BMCDBlepOscillator_discontinuityOffset(float nextValue, float increment){
    return (increment - (nextValue + 1.0f)) / increment;
}



/*!
 *BMCDBlepOscillator_discontinuityOffsets
 *
 * does the same thing as discontinuityOffets (no s) but operates on vector-valued inputs
 */
void BMCDBlepOscillator_discontinuityOffsets(const size_t *indices, const float *inputs, float *buffer, float *output, float increment, size_t numNegCrossings){
    // return (increment - (nextValue + 1.0f)) / increment;
    // equivalent statement: ((increment - 1) + (-nextValue)) / increment
    
    // load all the values of the naive waveform just after
    // the discontinuity. Store in buffer.
    vDSP_vgathr(inputs, indices, 1, buffer, 1, numNegCrossings);
    
    
    // ((increment - 1) - nextValue) / increment
    // == ((increment - 1) / increment) + (nextValue * -1.0 / increment)
    float incrementMinusOneOverIncrement = (increment - 1.0f) / increment;
    float negOneOverIncrement = -1.0f / increment;
    vDSP_vsmsa(buffer, 1,
               &negOneOverIncrement,
               &incrementMinusOneOverIncrement,
               buffer, 1,
               numNegCrossings);
}




void BMCDBlepOscillator_generateblepInputs(BMCDBlepOscillator *This, const float *naiveWave, size_t length){
    // find the first order finite difference derivative of each element in the
    // naive waveform
    float *derivative = This->b2;
    vDSP_vsub(naiveWave, 1, naiveWave+1, 1, derivative+1, 1, length-1);
    
    // calculate the first element of the derivative using the last value from
    // the previous buffer call
    derivative[0] = naiveWave[0] - This->previousEndValue;
    
    // cache the last value from the current buffer call
    This->previousEndValue = naiveWave[length-1];
    
    // a phase reset occurs each time the derivative shifts from positive to
    // negative. we find the indices of each negative derivative and count the total
	size_t j = 0;
    size_t *negDerivIndices = This->b4;
	for (size_t i=0; i<length; i++)
		if(derivative[i] < 0.0f)
			negDerivIndices[j++] = i;
	size_t numNegDerivatives = j;
		
	
	
    // for each negative zero crossing, find the floating point offset between
    // the audio sample before the discontinuity and the discontinuity itself
    float *discontinuityOffsets = This->b2;
    for(size_t i=0; i<numNegDerivatives; i++){
        discontinuityOffsets[i] = BMCDBlepOscillator_discontinuityOffset(negDerivIndices[i], This->blepInputIncrement);
    }
    
    // for each negative zero crossing, calculate one of the blep inputs up to
    // the crossing point and reset that blep input's start value
    for(size_t i=0; i<numNegDerivatives; i++){
        // get a pointer to the place to start writing the ramp
        float *blepInputWritePointer = This->blepInputBuffers[This->oldestBlep];
        blepInputWritePointer += This->blepInputWriteOffset[This->oldestBlep];

        // how many samples are we writing?
        size_t samplesWriting = negDerivIndices[i] - This->blepInputWriteOffset[This->oldestBlep];
        
        // write the next segment in the ramp
        vDSP_vramp(&This->blepInputInitialValue[This->oldestBlep], &This->blepInputIncrement, blepInputWritePointer, 1, samplesWriting);
        
        // update the initial value for the next ramp
        This->blepInputInitialValue[This->oldestBlep] = discontinuityOffsets[i];
        
        // update the offset for the next ramp
        This->blepInputWriteOffset[This->oldestBlep] += samplesWriting;
        
        // advance to the next blep
        This->oldestBlep++;
        if(This->oldestBlep >= This->numBleps)
            This->oldestBlep = 0;
    }
    
    // after completing all the zero crossings, continue the blep inputs until
    // we reach the end of the buffer
    for(size_t i=0; i<This->numBleps; i++){
        // How many samples left till the end of the buffer?
        size_t samplesRemaining = length - This->blepInputWriteOffset[i];
        
        // where are we writing to?
        float *writePointer = This->blepInputBuffers[i];
        writePointer += This->blepInputWriteOffset[i];
        
        // complete the ramp
        vDSP_vramp(&This->blepInputInitialValue[i], &This->blepInputIncrement, writePointer, 1, samplesRemaining);
        
        // update the initial value for the next buffer. It's
        // equal to the last value in this buffer plus the increment.
        This->blepInputInitialValue[i] = This->blepInputIncrement + This->blepInputBuffers[i][length-1];
        
        // update the write offset for the next buffer. It's zero
        This->blepInputWriteOffset[i] = 0;
    }
}




void BMCDBlepOscillator_process(BMCDBlepOscillator *This, const float *frequencies, float *output, size_t numSamples){
	
	size_t samplesRemaining = numSamples;
	size_t i=0;
	while (samplesRemaining > 0){
		size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, samplesRemaining);
		size_t samplesProcessingOS = This->oversampleFactor * samplesProcessing;
		
		// upsample the frequencies
		float * frequenciesOS = This->b1;
		BMGaussianUpsampler_processMono(&This->upsampler, frequencies + i, frequenciesOS, samplesProcessing);
        
        // multiply the frequencies by the oversampling factor
        float osFactorF = This->oversampleFactor;
        vDSP_vsmul(frequenciesOS, 1, &osFactorF, frequenciesOS, 1, samplesProcessingOS);
		
		// convert frequencies to phases
        float *phasesOS = This->b1;
        BMCDBlepOscillator_freqsToPhases(This, frequenciesOS, phasesOS, samplesProcessingOS);
		
		// process naive saw oscillator
        float *outputOS = This->b1;
		BMCDBlepOscillator_naiveSaw(phasesOS, outputOS, samplesProcessingOS);
		
		// calculate the input indices for the BLEP filters and store them
        // in the BLEP input Buffers
        BMCDBlepOscillator_generateblepInputs(This, outputOS, samplesProcessingOS);
		
		// sum the blep outputs into the main output
		for(size_t j=0; j<This->numBleps; j++)
			BMCDBlepOscillator_processBlep(This, This->blepInputBuffers[j], outputOS, samplesProcessingOS);
		
		// downsample
		BMDownsampler_processBufferMono(&This->downsampler, outputOS, output+i, samplesProcessingOS);
		
		// advance indices
		samplesRemaining -= samplesProcessing;
		i += samplesProcessing;
	}
	
	// highpass filter
	BMMultiLevelBiquad_processBufferMono(&This->highpass, output, output, numSamples);
}
