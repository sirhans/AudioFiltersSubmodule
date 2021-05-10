//
//  BMFirstOrderVariableFilter.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 21/04/2021.
//  We hereby release this file into the public domain
//

#include "BMFirstOrderVariableFilter.h"
#include <Accelerate/Accelerate.h>

// This computes a third order approximation to the normalised value of the coefficient a1/a0
// The exact value we are approximating here is copied from BMMultilevelBiquad setHighpass6db
void BMFirstOrderVariableFilter_a1Normalised_hp6db(const float *fc01, float *a1normalized, size_t length){
	float coefficientsOrder3 [4] = {2.0f,-3.0f,3.0f,-1.0f};
	vDSP_vpoly(coefficientsOrder3, 1, fc01, 1, a1normalized, 1, length, 3);
}


// This computes a third order approximation to 1/a0
void BMFirstOrderVariableFilter_a0inv_hp6db(const float *fc01, float *a0inv, size_t length){
	float coefficientsOrder3 [4] = {-1.0f,1.5f,-1.5f,1.0f};
	vDSP_vpoly(coefficientsOrder3, 1, fc01, 1, a0inv, 1, length, 3);
}


// calculate the coefficients a1, b0, b1 for an array of cutoff frequencies
void BMFirstOrderVariableFilter_highpass6dBCoefficients(const float *fc, float *a1, float *b0, float *b1, float sampleRate, size_t length){
	
	// fc01 = fc / sampleRate
	float two_SampleRateInv = 2.0f / sampleRate;
	float *fc01 = b1; // use b1 as temporary storage
	vDSP_vsmul(fc, 1, &two_SampleRateInv, fc01, 1, length);
	
	// a1 = a1/a0
	BMFirstOrderVariableFilter_a1Normalised_hp6db(fc01, a1, length);
	
	// b0 = 1/a0
	BMFirstOrderVariableFilter_a0inv_hp6db(fc01, b0, length);
	
	// b1 = -b0
	vDSP_vneg(b0, 1, b1, 1, length);
}
