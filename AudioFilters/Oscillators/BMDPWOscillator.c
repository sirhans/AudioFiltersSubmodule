//
//  BMDPWOscillator.c
//  AudioFiltersXcodeProject
//
//  WE HAVE ABANDONED DEVELOPMENT OF THIS CLASS BECAUSE IT REQUIRES GREATER
//  PRECISION THAN THE 64 BIT DOUBLE DATA TYPE PROVIDES. MATHEMATICA PROTOTYPES
//  SUGGEST THAT 128 BIT PRECISION WOULD BE SUFFICIENT.
//
//  Created by hans anderson on 11/11/20.
//  Copyright © 2020 BlueMangoo. We hereby release this code into the public
//  domain with no restrictions.
//

#include "BMDPWOscillator.h"
#include <math.h>
#include <Accelerate/Accelerate.h>
#include "Constants.h"
#include "BMIntegerMath.h"

void BMDPWOscillator_initDifferentiator(BMDPWOscillator *This);

void BMDPWOscillator_init(BMDPWOscillator *This,
						  enum BMDPWOscillatorType oscillatorType,
						  size_t integrationOrder,
						  size_t oversampleFactor,
						  float sampleRate){
	// make sure the integration order is supported
	assert(integrationOrder == 10 ||
		   integrationOrder == 8 ||
		   integrationOrder == 6);
	
	This->outputSampleRate = sampleRate;
	This->oversampledSampleRate = sampleRate * oversampleFactor;
	This->integrationOrder = integrationOrder;
	This->differentiationOrder = integrationOrder - 1;
	This->oversampleFactor = oversampleFactor;
	This->nextStartPhase = 0.0f;
	

    
    This->bufferLength = BM_BUFFER_CHUNK_SIZE * oversampleFactor;
	
	// allocate memory for two buffers
	This->b1 = malloc(This->bufferLength * sizeof(float));
	This->b2 = malloc(This->bufferLength * sizeof(float));
    This->b3 = malloc(This->bufferLength * sizeof(float));
    This->rawPolyWavelength = malloc(This->bufferLength * sizeof(float));
    
    // this is the wavelength of the polynomial we use to generate the
    // integrated waveform.
    float two = 2.0f;
    vDSP_vfill(&two, This->rawPolyWavelength, 1, This->bufferLength);
	
	// init the downsampler for oversampled processing
	bool stereo = false;
	assert(isPowerOfTwo(oversampleFactor) && oversampleFactor > 0);
	BMDownsampler_init(&This->downsampler, stereo, oversampleFactor, BMRESAMPLER_FULL_SPECTRUM);
	
	// init the upsampler for matching the sample rate of the input to the internal
	// oversampled rate of the oscillators
	size_t numLevels = 3;
	BMGaussianUpsampler_init(&This->upsampler, oversampleFactor,numLevels);
	
	// init the finite-difference differentiator
	BMDPWOscillator_initDifferentiator(This);
}





void BMDPWOscillator_free(BMDPWOscillator *This){
	BMGaussianUpsampler_free(&This->upsampler);
	BMDownsampler_free(&This->downsampler);
	BMFIRFilter_free(&This->differentiator);
    BMFIRFilter_free(&This->scalingFilter);
	
	free(This->b1);
	This->b1 = NULL;
	free(This->b2);
	This->b2 = NULL;
    free(This->b3);
    This->b3 = NULL;
    
    free(This->rawPolyWavelength);
    This->rawPolyWavelength = NULL;
}





void BMDPWOscillator_initDifferentiator(BMDPWOscillator *This){
	
	// allocate an array on the stack for temporary storage of the kernel
	assert(This->differentiationOrder < 16);
	float finiteDifferenceKernel [16];
	size_t kernelLength = 16;
	
	/**************************************************************************
	 *         HOW WE CALCULATE FINITE-DIFFERENCE KERNEL COEFFICIENTS
	 *
	 * Mathematica:
	 *
	 * UFDWeights[m_, n_, s_] := CoefficientList[Normal[Series[x^s Log[x]^m, {x, 1, n}]/h^m], x]
	 *
	 * FiniteDifferenceCoefficientList[differentialOrder_, kernelLength_] :=
	 *            UFDWeights[differentialOrder,
	 *                       kernelLength,
	 *                       If[OddQ[differentialOrder],
	 *                          (kernelLength - 1)/2,
	 *                          kernelLength/2]
	 *                       ] /. h -> 1
	 *
	 *
	 *    9TH ORDER 16 POINT FINITE DIFFERENCE DERIVATIVE:
	 * kernel = N[FiniteDifferenceCoefficientList[9, 16], 16]
	 *
	 **************************************************************************/
	
	if (This->differentiationOrder == 9){
		
		float k [16] = {0.02641679067460317, -0.5009393601190476, 4.509700520833333,
			-25.31026475694444, 95.62454427083333, -252.0561848958333,
			474.6988498263889, -648.8857979910714, 648.8857979910714,
			-474.6988498263889, 252.0561848958333, -95.62454427083333,
			25.31026475694444, -4.509700520833333, 0.5009393601190476,
			-0.02641679067460317};
		
		memcpy(finiteDifferenceKernel,k,kernelLength*sizeof(float));
	}
	
	if (This->differentiationOrder == 7){
		float k [16] = {-0.003472870249807099, 0.06771081995081019, -0.6369531702112269,
			3.851987560884452, -16.64204485857928, 50.52541969581887,
			-105.8411557044512, 152.9011318630642, -152.9011318630642,
			105.8411557044512, -50.52541969581887, 16.64204485857928,
			-3.851987560884452, 0.6369531702112269, -0.06771081995081019,
			0.003472870249807099};
		
		memcpy(finiteDifferenceKernel,k,kernelLength*sizeof(float));
	}
	
	if (This->differentiationOrder == 5){
		float k [16] = {0.0003442369682977051, -0.006817176675727701, 0.06575932248158200,
			-0.4156998407680815, 1.964794636736012, -7.482530006529793,
			19.33192750682977, -31.23528918311709, 31.23528918311709,
			-19.33192750682977, 7.482530006529793, -1.964794636736012,
			0.4156998407680815, -0.06575932248158200, 0.006817176675727701,
			-0.0003442369682977051};
		
		memcpy(finiteDifferenceKernel,k,kernelLength*sizeof(float));
	}
	
	// init the FIR filter that will process the differentiation
	BMFIRFilter_init(&This->differentiator, finiteDifferenceKernel, kernelLength);
    
    // normalise the difference kernel to get a vector magnitude of 1
	float sumsq;
	vDSP_svesq(finiteDifferenceKernel, 1, &sumsq, kernelLength);
	float normalizer = 1.0f/sqrtf(sumsq);
	vDSP_vsmul(finiteDifferenceKernel, 1, &normalizer, finiteDifferenceKernel, 1, kernelLength);
	
	// take the absolute value of the difference kernel to get a smoothing filter
	vDSP_vabs(finiteDifferenceKernel, 1, finiteDifferenceKernel, 1, kernelLength);
    
    // init the smoothing filter
    BMFIRFilter_init(&This->scalingFilter, finiteDifferenceKernel, kernelLength);
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
	double denominator = (double)BMFactorial(N) * pow(2.0 * sin(M_PI/P),(double)(N-1));
	
	return numerator / denominator;
}




/*
 * The integration operation significantly changes the amplitude of the wave.
 * For simplicity, we synthesize the integrated waveform with a constant
 * amplitude or 1. That results in a final differentiated output with wrong
 * amplitude. To get the correct amplitude we need to multiply the differentiated
 * output by the result of this scaling function.
 */
void BMDPWOscillator_ampScales(BMDPWOscillator *This, const float *frequencies, float *scales, size_t length){
	// TODO: should this be integration order or differentiation order?
	float N = This->integrationOrder;
	float sr = This->oversampledSampleRate;
	
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
	float scalingFactor = This->rawPolyWavelength[0] / This->oversampledSampleRate;
	
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
	This->nextStartPhase = fmodf(This->nextStartPhase, This->rawPolyWavelength[0]);
	
	// use floating point mod to wrap the phases into [0,rawPolyWavelength]
	int length_i = (int)length;
	vvfmodf(phases, phases, This->rawPolyWavelength, &length_i);
	
	// subtract to shift the phases into [-rawPolyWavelength/2,rawPolyWavelength/2]
	float negHalfWavelength = This->rawPolyWavelength[0] * -0.5f;
	vDSP_vsadd(phases, 1, &negHalfWavelength, phases, 1, length);
}




void BMDPWOscillator_sawWaveInt10(BMDPWOscillator *This, const float *phases, float *output, size_t length){
	// x^2 (381 + x^2 (-310 + x^2 (98 + x^2 (-15 + x^2))))
	
	// x^2
	float *x2 = This->b1;
	vDSP_vsq(phases, 1, x2, 1, length);
	
	// t = (-15 + x^2)
	float neg15 = -15.0f;
	float *t = This->b2;
	vDSP_vsadd(x2, 1, &neg15, t, 1, length);
	
	// t = 98 + x^2 (t)
	float ninetyEight = 98.0f;
	vDSP_vmsa(t, 1, x2, 1, &ninetyEight, t, 1, length);
	
	// t = -310 + x^2 (t)
	float negThreeTen = -310.0;
	vDSP_vmsa(t, 1, x2, 1, &negThreeTen, t, 1, length);
	
	// t = 381 + x^2 (t)
	float threeEightyOne = 381.0f;
	vDSP_vmsa(t, 1, x2, 1, &threeEightyOne, t, 1, length);
	
	// output = x^2 (t)
	vDSP_vmul(x2, 1, t, 1, output, 1, length);
}




void BMDPWOscillator_sawWaveInt8(BMDPWOscillator *This, const float *phases, float *output, size_t length){
	// x^2 (-(124/3) + x^2 (98/3 + x^2 (-(28/3) + x^2)))
	
	// x^2
	float *x2 = This->b1;
	vDSP_vsq(phases, 1, x2, 1, length);
	
	// t = (-(28/3) + x^2)
	float fn28_3 = -28.0f / 3.0f;
	float *t = This->b2;
	vDSP_vsadd(x2, 1, &fn28_3, t, 1, length);
	
	// t = 98/3 + x^2 (t)
	float f98_3 = 98.0f / 3.0f;
	vDSP_vmsa(t, 1, x2, 1, &f98_3, t, 1, length);
	
	// t = -(124/3) + x^2 (t)
	float fn124_3 = -124.0f / 3.0f;
	vDSP_vmsa(t, 1, x2, 1, &fn124_3, t, 1, length);
	
	// output = x^2 (t)
	vDSP_vmul(x2, 1, t, 1, output, 1, length);
}




void BMDPWOscillator_sawWaveInt6(BMDPWOscillator *This, const float *phases, float *output, size_t length){
	// x^2 (7 + x^2 (-5 + x^2))
	
	// x^2
	float *x2 = This->b1;
	vDSP_vsq(phases, 1, x2, 1, length);
	
	// t = (-5 + x^2)
	float neg5 = -5.0f;
	float *t = This->b2;
	vDSP_vsadd(x2, 1, &neg5, t, 1, length);
	
	// t = 7 + x^2 (t)
	float seven = 7.0f;
	vDSP_vmsa(t, 1, x2, 1, &seven, t, 1, length);
	
	// output = x^2 (t)
	vDSP_vmul(x2, 1, t, 1, output, 1, length);
}





void BMDPWOscillator_integratedWaveform(BMDPWOscillator *This, const float *frequencies, float *output, size_t length){
	// compute the phases
	BMDPWOscillator_freqsToPhases(This, frequencies, This->b1, length);
	
	if(This->integrationOrder == 10){
		BMDPWOscillator_sawWaveInt10(This, This->b1,output,length);
		return;
	}
	if(This->integrationOrder == 8){
		BMDPWOscillator_sawWaveInt10(This, This->b1,output,length);
		return;
	}
	if(This->integrationOrder == 6){
		BMDPWOscillator_sawWaveInt10(This, This->b1,output,length);
		return;
	}
	
	// if we get here then the integration order isn't supported.
	assert(false);
}





void BMDPWOscillator_process(BMDPWOscillator *This, const float *frequencies, float *output, size_t length){
    
    size_t samplesRemainingOS = length * This->oversampleFactor;
    size_t i=0;
    //size_t iOS=0;
    
    // process in limited-length chunks
    while(samplesRemainingOS > 0){
        size_t samplesProcessingOS = BM_MIN(samplesRemainingOS,This->bufferLength);
        size_t samplesProcessing = samplesProcessingOS / This->oversampleFactor;
        
        // upsample the frequency data
        BMGaussianUpsampler_processMono(&This->upsampler, frequencies+i, This->b3, samplesProcessing);
        
        // generate the integrated waveform
        BMDPWOscillator_integratedWaveform(This, This->b3, This->b1, samplesProcessingOS);
        
        // get the volume scaling signal
        BMDPWOscillator_ampScales(This, This->b3, This->b2, samplesProcessingOS);
        
        // apply a smoothing filter to the scaling signal
        BMFIRFilter_process(&This->scalingFilter, This->b2, This->b2, samplesProcessingOS);
        
        // differentiate
        BMFIRFilter_process(&This->differentiator, This->b1, This->b1, samplesProcessingOS);
        
        // apply the volume scaling to the integrated waveform signal
        vDSP_vmul(This->b1, 1, This->b2, 1, This->b1, 1, samplesProcessingOS);
        
        // downsample
        BMDownsampler_processBufferMono(&This->downsampler, This->b1, output+i, samplesProcessingOS);
        
        // advance pointers
        samplesRemainingOS -= samplesProcessingOS;
        i += samplesProcessing;
    }
}
