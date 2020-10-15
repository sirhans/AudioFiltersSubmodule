//
//  BMSpectrum.c
//  Saturator
//
//  Created by Nguyen Minh Tien on 1/31/18.
//  This file is public domain. No restrictions.
//

#include "BMSpectrum.h"
#include <assert.h>
#include "Constants.h"
#include "BMUnitConversion.h"

#define BM_SPECTRUM_DEFAULT_BUFFER_LENGTH 4096

void BMSpectrum_setupFFT(BMSpectrum* This,size_t n);


bool IsPowerOfTwo(size_t x)
{
	return (x & (x - 1)) == 0;
}



void BMSpectrum_init(BMSpectrum* This){
	BMSpectrum_initWithLength(This, BM_SPECTRUM_DEFAULT_BUFFER_LENGTH);
}



void BMSpectrum_free(BMSpectrum* This){
	BMFFT_free(&This->fft);
	free(This->buffer);
	This->buffer = NULL;
}



void BMSpectrum_initWithLength(BMSpectrum* This, size_t maxInputLength){
	This->maxInputLength = maxInputLength;
	BMFFT_init(&This->fft, maxInputLength);
	This->buffer = malloc(sizeof(float)*maxInputLength);
}





bool BMSpectrum_processData(BMSpectrum* This,
							const float* input,
							float* output,
							size_t inputLength,
							size_t *outputLength,
							float* nyquist){
	if(inputLength > 0){
		*outputLength = inputLength / 2;
		
		// calculate the FFT
		*nyquist = BMFFT_absFFTReturnNyquist(&This->fft, input, output, inputLength);
		
		// Limit the range of output values
		float minValue = BM_DB_TO_GAIN(-60.0f);
		float maxValue = BM_DB_TO_GAIN(60.0f);
		vDSP_vclip(output, 1, &minValue, &maxValue, output, 1, *outputLength);
		float b = 0.01;
		vDSP_vdbcon(output, 1, &b, output, 1, *outputLength, 1);
		
		return true;
	}
	return false;
}




float BMSpectrum_processDataBasic(BMSpectrum* This,
								  const float* input,
								  float* output,
								  bool applyWindow,
								  size_t inputLength){
	assert(inputLength > 0);
	assert(IsPowerOfTwo(inputLength));
	assert(inputLength <= This->maxInputLength);
	
	// apply a Kaiser window to the input
	const float *windowedInput = input;
	if(applyWindow){
		//        BMFFT_blackmanHarrisWindow(&This->fft, input, This->buffer, inputLength);
		double beta = 15.0;
		BMFFT_kaiserWindow(&This->fft, input, This->buffer, beta, inputLength);
		windowedInput = This->buffer;
	}
	
	// compute abs(fft(input)) and save the nyquist term
	float absNyquist = BMFFT_absFFTReturnNyquist(&This->fft, windowedInput, output, inputLength);
	
	return absNyquist;
}




void BMSpectrum_barkScaleFFTBinIndices(float *A, size_t fftSize, float minHz, float maxHz, float sampleRate, size_t length){
	// find the min and max frequency in barks
	float minBark = BMConv_hzToBark(minHz);
	float maxBark = BMConv_hzToBark(maxHz);
	
	
	for(size_t i=0; i<length; i++){
		// create evenly spaced values in bark scale
		float zeroToOne = (float)i / (float)(length-1);
		float bark = minBark + (maxBark-minBark)*zeroToOne;
		
		// convert to Hz
		float hz = BMConv_barkToHz(bark);
		
		// convert to FFT bin index (floating point interpolated)
		A[i] = BMConv_hzToFFTBin(hz, fftSize, sampleRate);
	}
}





void BMSpectrum_fftDownsamplingIndices(size_t *startIndices,
									   size_t *groupLengths,
									   const float *interpolatedIndices,
									   float minHz,
									   float maxHz,
									   float sampleRate,
									   size_t fftSize,
									   size_t length){
	
	// find the min and max frequency in barks
	float minBark = BMConv_hzToBark(minHz);
	float maxBark = BMConv_hzToBark(maxHz);
	
	
	for(size_t i=0; i<length; i++){
		// create evenly spaced values in bark scale
		float zeroToOne = (float)i / (float)(length-1);
		float bark = minBark + (maxBark-minBark)*zeroToOne;
		
		// convert to Hz
		float hz = BMConv_barkToHz(bark);
		// calculate bins per pixel at this index
		float binsPerPixel_f = BMConv_fftBinsPerPixel(hz, fftSize, length, minHz, maxHz, sampleRate);
		
		// calculate the start index for this pixel
		if(i==0) startIndices[i] = BMConv_hzToFFTBin(minHz, fftSize, sampleRate);
		else startIndices[i] = startIndices[i-1]+groupLengths[i-1];
		
		// calculate the end index for this pixel
		size_t endIndex = (size_t)roundf(interpolatedIndices[i] + binsPerPixel_f*0.5f);
		
		// calculate the number of pixels in [startIndices[i],endIndex]
		groupLengths[i] = endIndex - startIndices[i];
		
		// don't let the end index go above the limit
		if(endIndex > fftSize-1) groupLengths[i] = fftSize - startIndices[i];
	}
}




// returns the frequency at which one interpolated output array element is equal to two FFT bins.
float pixelBinParityFrequency(float fftSize, float pixelHeight,
							  float minFrequency, float maxFrequency,
							  float sampleRate){
	// initial guess
	float f = sampleRate / 4.0;
	
	// numerical search for a value of f where binsPerPixel is near 1.0
	while(true){
		// find out how many bins per pixel at frequency f
		float binsPerPixel = BMConv_fftBinsPerPixel(f, fftSize, pixelHeight, minFrequency, maxFrequency, sampleRate);
		
		// this is the number of bins per pixel we are looking for
		float targetParity = 2.0f;
		
		// if binsPerPixel < targetParity, increase f. else decrease f
		f /= (binsPerPixel / targetParity);
		
		// if binsPerPixel is close to 2, return
		if (fabsf(binsPerPixel - targetParity) < 0.001) return f;
		
		// if f is below half of minFrequency, stop
		if (f < minFrequency * 0.5f) return minFrequency;
		
		// if f is above 2x maxFrequency, stop
		if (f > maxFrequency * 2.0f) return maxFrequency;
	}
	
	// satisfy the compiler
	return f;
}



size_t BMSpectrum_numUpsampledPixels(size_t fftSize, float minF, float maxF, float sampleRate, size_t outputLength){
	// find the min and max frequency in barks
	float minBark = BMConv_hzToBark(minF);
	float maxBark = BMConv_hzToBark(maxF);
	
	// find the frequency at which 2 fft bins = 1 screen pixel
	float pixelBinParityFreq = pixelBinParityFrequency(fftSize, outputLength, minF, maxF, sampleRate);
	
	// find the number of pixels we can interpolate by upsampling
	float parityFrequencyBark = BMConv_hzToBark(pixelBinParityFreq);
	float upsampledPixelsFraction = (parityFrequencyBark - minBark) / (maxBark - minBark);
	size_t upsampledPixels = 1 + round(1.0 * upsampledPixelsFraction * outputLength);
	if(upsampledPixels > outputLength) upsampledPixels = outputLength;
	
	return upsampledPixels;
}
