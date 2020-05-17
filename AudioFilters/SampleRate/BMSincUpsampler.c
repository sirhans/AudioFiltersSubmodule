//
//  BMSincUpsampler.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/16/20.
//  This file is released to the public domain.
//

#include "BMSincUpsampler.h"
#include <Accelerate/Accelerate.h>
#include "BMFFT.h"


void BMSincUpsampler_initFilterKernels(BMSincUpsampler *This);



void BMSincUpsampler_init(BMSincUpsampler *This,
						  size_t interpolationPoints,
						  size_t upsampleFactor){
	// even filter kernel lengths only
	assert(interpolationPoints % 2 == 0);
	
	This->upsampleFactor = upsampleFactor;
	This->kernelLength = interpolationPoints;
	This->inputPadding = (This->kernelLength) / 2;
	This->numKernels = This->upsampleFactor;
	
	This->filterKernels = malloc(sizeof(float*) * This->numKernels);
	for(size_t i=0; i<This->numKernels; i++)
		This->filterKernels[i] = malloc(sizeof(float*)*This->kernelLength);
	
	BMSincUpsampler_initFilterKernels(This);
}





void BMSincUpsampler_free(BMSincUpsampler *This){
	for(size_t i=0; i<This->numKernels; i++)
		free(This->filterKernels[i]);
	free(This->filterKernels);
	This->filterKernels = NULL;
}





size_t BMSincUpsampler_process(BMSincUpsampler *This,
							 const float* input,
							 float* output,
							 size_t inputLength){
	
	// how many samples of the input must be left out from the output at the beginning and end?
	size_t inputLengthMinusPadding = inputLength - 2*This->inputPadding;
	
	// copy the original samples from input to output, leaving spaces for the interpolated samples to be inserted
	float one = 1.0f;
	vDSP_vsmul(input+This->inputPadding, 1, &one, output, This->upsampleFactor, inputLengthMinusPadding);
	
	// insert the interpolated samples into the remaining spaces.
	// the index starts at 1 because the first filter kernel is replaced by the vsmul line above
	for(size_t i=1; i<This->upsampleFactor; i++){
		vDSP_conv(input, 1, This->filterKernels[i], 1, output+i, This->upsampleFactor, inputLengthMinusPadding, This->kernelLength);
	}
	
	return inputLengthMinusPadding * This->upsampleFactor;
}








void BMSincUpsampler_initFilterKernels(BMSincUpsampler *This){
	// allocate an array for the complete discretised sinc interpolation function
	size_t completeKernelLength = This->kernelLength * This->upsampleFactor;
	float *interleavedKernel = malloc(sizeof(float) * completeKernelLength);
	
	// generate the sinc function
	// Mathematica prototype:
	//    Table[Sinc[(t - K/2) \[Pi] / us], {t, 0, K}]
	for(size_t t=0; t<completeKernelLength; t++){
		// rename and cast variables for easier readability
		double td = t;
		double K = completeKernelLength;
		double us = This->upsampleFactor;
		double phi = (td - (K / 2.0)) * M_PI / us;
		
		// calculate the sinc function
		if(fabs(phi) > FLT_EPSILON)
			interleavedKernel[t] = sin(phi)/phi;
		else
			interleavedKernel[t] = 1.0;
		
		printf("%f,",interleavedKernel[t]);
	}
	printf("\n");
	
	// generate a blackman-harris window
	//
	// note that the window has an extra coefficient at the end that is not used
	// on the kernel. This is because the complete interleaved kernel is not
	// perfectly symmetric. It is missing a zero at the end that would make it
	// symmetric. That zero is missing because it doesn't fit into any of the
	// polyphase kernels. Technically, that final zero belongs to the first
	// polyphase kernel, the one that is all zeros except its centre element.
	//
	size_t bhWindowLength = completeKernelLength + 1;
	float* blackmanHarrisWindow = malloc(sizeof(float)*bhWindowLength);
	BMFFT_generateBlackmanHarrisCoefficients(blackmanHarrisWindow, bhWindowLength);
	
	// apply the blackman-harris window to the interleaved kernel
	vDSP_vmul(blackmanHarrisWindow,1,interleavedKernel,1,interleavedKernel,1,completeKernelLength);
	
	// copy from the complete kernel to the polyphase kernels
	for(size_t i=0; i<This->numKernels; i++)
		for(size_t j=0; j<This->kernelLength; j++)
			This->filterKernels[i][j] = interleavedKernel[j*This->numKernels + i];
	
	free(interleavedKernel);
	free(blackmanHarrisWindow);
}
