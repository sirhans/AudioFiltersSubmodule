//
//  BMSpectralGain.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 27/08/2021.
//  Copyright © 2021 BlueMangoo. All rights reserved.
//

#include "BMSpectralGain.h"
#include "BMIntegerMath.h"
#include "Constants.h"

#define BMSG_KAISER_BETA 16.0


void BMSpectralGain_init(BMSpectralGain *This, size_t maxFFTSize){
//	typedef struct BMSpectralGain {
//		BMFFT fft;
//		float *b1r, *kaiserWindow, *kaiserToHann;
//		float *outputRight, *outputCentre, *outputLeft;
//		float *inputRight, *inputCentre, *inputLeft;
//		DSPSplitComplex *b2c;
//		size_t kaiserWindowLength, hannWindowLength;
//	} BMSpectralGain;
	
	BMFFT_init(&This->fft, maxFFTSize);
	
	float *floatBuffers = malloc(sizeof(float)*maxFFTSize);
	This->b1r = malloc(sizeof(float)*maxFFTSize);
	This->kaiserWindow = malloc(sizeof(float)*maxFFTSize);
	This->kaiserToHann = malloc(sizeof(float)*maxFFTSize);
	This->outputRight = malloc(sizeof(float)*maxFFTSize);
	This->outputCentre = malloc(sizeof(float)*maxFFTSize);
	This->outputLeft = malloc(sizeof(float)*maxFFTSize);
	This->inputLeft	= malloc(sizeof(float)*maxFFTSize);
	This->inputCentre = malloc(sizeof(float)*maxFFTSize);
	This->inputRight = malloc(sizeof(float)*maxFFTSize);
	This->b2c = malloc(sizeof(DSPSplitComplex)*maxFFTSize);
	
	This->kaiserWindowLength = This->hannWindowLength = 0;
}




void BMSpectralGain_free(BMSpectralGain *This){
	BMFFT_free(&This->fft);
	
	free(This->b1r);
	free(This->kaiserWindow);
	free(This->kaiserToHann);
	free(This->outputRight);
	free(This->outputCentre);
	free(This->outputLeft);
	free(This->inputLeft);
	free(This->inputCentre);
	free(This->inputRight);
	free(This->b2c);
	This->b1r = NULL;
	This->kaiserWindow = NULL;
	This->kaiserToHann = NULL;
	This->outputRight = NULL;
	This->outputCentre = NULL;
	This->outputLeft = NULL;
	This->inputLeft = NULL;
	This->inputCentre = NULL;
	This->inputRight = NULL;
	This->b2c = NULL;
}




void BMSpectralGain_kaiserToHann(BMSpectralGain *This,
								 float *inputWithKaiser,
								 float *outputWithHann,
								 size_t inputLength,
								 size_t outputLength){
	// find the section of the input that will actually be used for the output
	size_t offset = (inputLength - outputLength) / 2;
	float *offsetInput = inputWithKaiser + offset;
	
	// if either of the window lengths has changed, recompute the window
	if (inputLength != This->kaiserWindowLength ||
		outputLength != This->hannWindowLength){
		
		// update the lengths
		This->kaiserWindowLength = inputLength;
		This->hannWindowLength = outputLength;
		
		// generate kaiser window => kaiserWindow
		BMFFT_generateKaiserCoefficients(This->kaiserWindow, BMSG_KAISER_BETA, This->kaiserWindowLength);
		
		// genearte hann window => hann
		float *hann = This->kaiserToHann;
		BMFFT_generateHannCoefficients(hann, This->hannWindowLength);
		
		// kaiserToHann = hann window / kaiser window
		vDSP_vdiv(This->kaiserWindow, 1, hann, 1, This->kaiserToHann, 1, outputLength);
	}
	
	// remove the kaiser window, apply a hann window, and copy to output
	vDSP_vmul(This->kaiserToHann, 1, offsetInput, 1, outputWithHann, 1, outputLength);
}





void BMSpectralGain_processThreeBuffers(BMSpectralGain *This,
										const float *gainLeft, const float *inputLeft,
										const float *gainCentre, const float *inputCentre,
										const float *gainRight, const float *inputRight,
										size_t fftSize, size_t sideBufferLength, size_t centreBufferLength,
										float *output
										){
	// the length of the two side buffers should be half the fft size. This can
	// be proven mathematically but the proof is too long to write here.
	assert (sideBufferLength == fftSize / 2);
	
	// how many samples needed to the left and right of the centre buffer to fill the FFT?
	size_t padding = (fftSize - centreBufferLength) / 2;
	
	// if the inputs are contiguous, set up pointers to use them in place
	const float *left, *centre, *right;
	if(inputLeft + sideBufferLength + 1 == inputCentre &&
	   inputCentre + centreBufferLength + 1 == inputRight){
		// set the left pointer to the left input
		left = inputLeft;
		
		// set the centre pointer to the centre input minus the padding
		centre = inputCentre - padding;
		
		// set the right pointer to the right input minus the side buffer length
		// (because the side buffers are exactly half the FFT size)
		right = inputRight - sideBufferLength;
		
	// if the inputs are not contiguous, copy to buffers and set pointers to the buffers
	} else {
		// LEFT BUFFER
		// how many samples to copy from right, left, centre?
		size_t samplesFromLeft = sideBufferLength;
		size_t samplesFromCentre = BM_MIN(fftSize - sideBufferLength,centreBufferLength);
		size_t samplesFromRight = fftSize - samplesFromLeft - samplesFromCentre;
		// from left
		memcpy(This->inputLeft,inputLeft,sizeof(float)*samplesFromLeft);
		// from centre
		memcpy(This->inputLeft+samplesFromLeft,inputCentre,sizeof(float)*samplesFromCentre);
		// from right
		memcpy(This->inputLeft+samplesFromLeft+samplesFromCentre,inputRight,sizeof(float)*samplesFromRight);
		
		// CENTRE BUFFER
		// from left
		memcpy(This->inputCentre,inputLeft+sideBufferLength-padding,sizeof(float)*padding);
		// from centre
		memcpy(This->inputCentre+padding,inputCentre,sizeof(float)*centreBufferLength);
		// from right
		memcpy(This->inputCentre+padding+centreBufferLength,inputRight,sizeof(float)*padding);
		
		// RIGHT BUFFER
		// how many samples to copy from right, left, centre?
		samplesFromRight = sideBufferLength;
		samplesFromCentre = BM_MIN(fftSize - sideBufferLength,centreBufferLength);
		samplesFromLeft = fftSize - samplesFromRight - samplesFromCentre;
		// from left
		memcpy(This->inputRight,inputLeft+sideBufferLength-samplesFromLeft,sizeof(float)*samplesFromLeft);
		// from centre
		memcpy(This->inputRight+samplesFromLeft,inputLeft,sizeof(float)*samplesFromCentre);
		// from right
		memcpy(This->inputRight + samplesFromLeft + samplesFromCentre,inputLeft,sizeof(float)*samplesFromRight);
		
		left = This->inputLeft;
		centre = This->inputCentre;
		right = This->inputRight;
	}
	
	// process the three buffers
	BMSpectralGain_processOneBuffer(This, gainLeft, inputLeft, This->outputLeft, fftSize, centreBufferLength);
	BMSpectralGain_processOneBuffer(This, gainCentre, inputCentre, This->outputCentre, fftSize, centreBufferLength);
	BMSpectralGain_processOneBuffer(This, gainRight, inputRight, This->outputRight, fftSize, centreBufferLength);
	
	// sum the last half of left and the first half of centre to the output
	size_t halfOutputLength = centreBufferLength/2;
	vDSP_vadd(This->outputLeft+halfOutputLength, 1, This->outputCentre, 1, output, 1, halfOutputLength);
	
	// sum the last half of centre and the first half of right to output
	vDSP_vadd(This->outputRight,1, This->outputCentre+halfOutputLength,1, output + halfOutputLength, 1, halfOutputLength);
}







void BMSpectralGain_processOneBuffer(BMSpectralGain *This,
							const float *gain,
							const float *input,
							float *output,
							size_t inputLength,
							size_t outputLength){
	assert (outputLength < inputLength);
	assert (outputLength % 2 == 0);
	assert (isPowerOfTwo(inputLength));
	
	// apply a kaiser window to the input
	double kaiserBeta = BMSG_KAISER_BETA;
	BMFFT_kaiserWindow(&This->fft, input, This->b1r, kaiserBeta, inputLength);
	
	// compute the fft
	BMFFT_FFTComplexOutput(&This->fft, This->b1r, This->b2c, inputLength);
	
	// multiply by the gain
	vDSP_zrvmul(This->b2c+1, 1, gain+1, 1, This->b2c+1, 1, inputLength/2);
	// multiply gain for DC and Nyquist terms
	float *dc = This->b2c[0].realp;
	(*dc) *= gain[0];
	float *nyquist = This->b2c[0].imagp;
	(*nyquist) *= gain[inputLength/2];
	
	// inverse fft
	BMFFT_IFFT(&This->fft, This->b2c, This->b1r, inputLength/2);
	
	// remove the kaiser window and apply hann window instead
	BMSpectralGain_kaiserToHann(This, This->b1r, output, inputLength, outputLength);
}
