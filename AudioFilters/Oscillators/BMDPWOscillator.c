//
//  BMDPWOscillator.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 11/11/20.
//  Copyright © 2020 BlueMangoo. We hereby release this code into the public
//  domain with no restrictions.
//

#include "BMDPWOscillator.h"
#include <math.h>
#include <Accelerate/Accelerate.h>
#include "Constants.h"



void BMDPWOscillator_init(BMDPWOscillator *This,
						  enum BMDPWOscillatorType oscillatorType,
						  size_t integrationOrder,
						  float sampleRate){
	This->sampleRate = sampleRate;
	This->integrationOrder = integrationOrder;
	This->differentiationOrder = integrationOrder - 1;
	This->nextStartPhase = 0.0f;
	
	// this is the wavelength of the polynomial we use to generate the
	// integrated waveform.
	This->rawPolyWavelength = 2.0f;
	
	This->b1 = malloc(BM_BUFFER_CHUNK_SIZE * sizeof(float));
}




void BMDPWOscillator_free(BMDPWOscillator *This){
	free(This->b1);
	This->b1 = NULL;
}




size_t BMDPWFactorial(size_t N){
	if(N==0) return 0;
	
	size_t out = N;
	while (--N > 1)
		out *= N;
	
	return out;
}




/*
 * This is the scaling function given in eq (17) of the Valimaki DPW oscillator paper
 */
float BMDPWOscillator_valimakiScalingFunction(size_t N, double f, double sr){
	// Mathematica prototype:
	//
	//	P[f_, sr_] := sr/f
	//	c[n_, f_, sr_] := \[Pi]^(n - 1)/(
	//	 Factorial[n] (2 Sin[\[Pi]/P[f, sr]])^(n - 1))
	
	double numerator = pow(M_PI,(double)N-1.0);
	double P = sr/f;
	double denominator = (double)BMDPWFactorial(N) * pow(2.0 * sin(M_PI/P),(double)(N-1));
	
	return numerator / denominator;
}




/*
 * The integration operation significantly changes the amplitude of the wave.
 * For simplicity, we synthesize the integrated waveform with a constant
 * amplitude or 1. That results in a final differentiated output with wrong
 * amplitude. To get the correct amplitude we need to multiply the differentiated
 * output by the result of this scaling function.
 */
void BMDPWOscillator_scalingFunction(BMDPWOscillator *This, const float *frequencies, float *scales, size_t length){
	// TODO: should this be integration order or differentiation order?
	float N = This->integrationOrder;
	float sr = This->sampleRate;
	
	// the scaling functions given in the valimaki paper can be closely
	// approximated by the function c/f(N-1), where c is a scaling constant
	// we use a scaling function of this form for efficiency.
	
	// we start by finding the scaling constant
	float scalingConstant = BMDPWOscillator_valimakiScalingFunction(N,1.0,sr);
	
	// we raise the frequencies to the -(n-1th) power
	float oneMinusN = 1.0f-N;
	int lengthI = (int)length;
	vvpowsf(scales, &oneMinusN, frequencies, &lengthI);
	
	// scale and output
	vDSP_vsmul(scales, 1, &scalingConstant, scales, 1, length);
}





void BMDPWOscillator_freqsToPhases(BMDPWOscillator *This, const float *frequencies, float *phases, size_t length){
	
	// this scaling factor converts from frequency to phase increment.
	// We are generating waveforms on the interval from -1 to 1.
	// Therefore a freqeuency of f implies a phase increment of (f * rawPolyWavelength / sampleRate)
	// for each sample.
	float scalingFactor = This->rawPolyWavelength / This->sampleRate;
	
	// Here we want to running sum the phase increments into the phase buffer to
	// get the phases.
	//
	// Because the vrsum function requires a scaling factor, we conveniently
	// convert from frequency to phase increment and do the running sum in a
	// single function call.
	vDSP_vrsum(frequencies, 1, &scalingFactor, phases, 1, length);
	
	// To ensure phase continuity between buffers we adjust the start phase to
	// match the end phase
	vDSP_vsadd(This->b1, 1, &This->nextStartPhase, phases, 1, length);
	
	// compute the start phase of the next buffer.
	// note that we shift the phase to centre on zero after this. The formula
	// below takes into account that the shift has not yet occured at this point.
	This->nextStartPhase = phases[length-1] + frequencies[length-1]*scalingFactor;
	This->nextStartPhase = fmodf(This->nextStartPhase, This->rawPolyWavelength);
	
	// use floating point mod to wrap the phases into [0,rawPolyWavelength]
	int length_i = (int)length;
	vvfmodf(phases, phases, &This->rawPolyWavelength, &length_i);
	
	// subtract to shift the phases into [-rawPolyWavelength/2,rawPolyWavelength]
	float negHalfWavelength = This->rawPolyWavelength * -0.5f;
	vDSP_vsadd(phases, 1, &negHalfWavelength, phases, 1, length);
}



void BMDPWOscillator_integratedWaveform(BMDPWOscillator *This, const float *frequencies, float *output, size_t length){
	
}
