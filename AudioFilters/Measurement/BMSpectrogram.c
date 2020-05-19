//
//  BMSpectrogram.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/15/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMSpectrogram.h"
#include "BMIntegerMath.h"

void BMSpectrogram_init(BMSpectrogram *This,
						size_t maxFFTSize,
						size_t maxImageHeight,
						float sampleRate){
	
	This->sampleRate = sampleRate;
	This->prevMinF = This->prevMaxF = 0.0f;
	This->prevImageHeight = This->prevFFTSize = 0;
	
	BMSpectrum_initWithLength(&This->spectrum, maxFFTSize);
	This->maxFFTSize = maxFFTSize;
	This->maxImageHeight = maxImageHeight;
	
	// we need some extra samples at the end of the fftBin array because the
	// interpolating function that converts from linear scale to bark scale
	// reads beyond the interpolation index
	This->fftBinInterpolationPadding = 2;
	
	size_t maxFFTOutput = 1 + maxFFTSize/2;
	This->b1 = malloc(sizeof(float)*(maxFFTOutput+This->fftBinInterpolationPadding));
	This->b2 = malloc(sizeof(float)*maxImageHeight);
	This->b3 = malloc(sizeof(float)*maxImageHeight);
}



float hzToBark(float hz){
	// hzToBark[hz_] := 6 ArcSinh[hz/600]
	return 6.0f * asinhf(hz / 600.0f);
}



float barkToHz(float bark){
	// barkToHz[bk_] := 600 Sinh[bk/6]
	return 600.0f * sinhf(bark / 6.0f);
}



float hzToFFTBin(float hz, float fftSize, float sampleRate){
	// fftBinZeroDC[f_, fftLength_, sampleRate_] := f/(sampleRate/fftLength)
	return hz * (fftSize / sampleRate);
}



void BMSpectrogram_fftBinsToBarkScale(BMSpectrogram *This,
									  const float* fftBins,
									  float *output,
									  size_t fftSize,
									  size_t outputLength,
									  float minFrequency,
									  float maxFrequency){
	// rename buffer 3 for readablility
	float* interpolatedIndices = This->b3;
	
	// if the settings have changed since last time we did this, recompute
	// the floating point array indices for interpolated reading of bark scale
	// frequency spectrum
	if(minFrequency != This->prevMinF ||
	   maxFrequency != This->prevMaxF ||
	   outputLength != This->prevImageHeight ||
	   fftSize != This->prevFFTSize){
		
		// find the min and max frequency in barks
		float minBark = hzToBark(minFrequency);
		float maxBark = hzToBark(maxFrequency);
		
		
		for(size_t i=0; i<outputLength; i++){
			// create evenly spaced values in bark scale
			float zeroToOne = (float)i / (float)(outputLength-1);
			float bark = minBark + (maxBark-minBark)*zeroToOne;
			
			// convert to Hz
			float hz = barkToHz(bark);
			
			// convert to FFT bin index (floating point interpolated)
			interpolatedIndices[i] = hzToFFTBin(hz, fftSize, This->sampleRate);
		}
	}
	
	// now that we have the fft bin indices we can interpolate and get the results
	size_t numFFTBins = 1 + fftSize/2;
	assert(numFFTBins + This->fftBinInterpolationPadding - 2 >= floor(interpolatedIndices[outputLength-1])); // requirement of vDSP_vqint. can be ignored if there is extra space at the end of the fftBins array and zeros are written there.
	vDSP_vqint(fftBins, interpolatedIndices, 1, output, 1, outputLength, numFFTBins);
}





void BMSpectrogram_toHSBColour(float* input, BMHSBPixel* output, size_t length){
	// toHSBColourFunction[x_] := {7/12, 1 - x, x}
	float h = 7.0/12.0;
	for(size_t i=0; i<length; i++)
		output[i] = simd_make_float3(h,1-input[i],input[i]);
}




void BMSpectrogram_toDbScaleAndClip(const float* input, float* output, size_t length){
	// convert to db
	float zeroDb = 1.0f;
	uint32_t use20not10 = 1;
	vDSP_vdbcon(input, 1, &zeroDb, output, 1, length, use20not10);
	
	// clip to [-100,0]
	float min = -100.0f;
	float max = 0.0f;
	vDSP_vclip(output, 1, &min, &max, output, 1, length);
	
	// shift and scale to [0,1]
	float scale = 1.0f/100.0f;
	float shift = 1.0f;
	vDSP_vsmsa(output, 1, &scale, &shift, output, 1, length);
}





void BMSpectrogram_process(BMSpectrogram *This,
						   const float* inputAudio,
						   SInt32 inputLength,
						   SInt32 startSampleIndex,
						   SInt32 endSampleIndex,
						   SInt32 fftSize,
						   BMHSBPixel** imageOutput,
						   SInt32 pixelWidth,
						   SInt32 pixelHeight,
						   float minFrequency,
						   float maxFrequency){
	assert(4 <= fftSize && fftSize <= This->maxFFTSize);
	assert(isPowerOfTwo((size_t)fftSize));
	assert(2 <= pixelHeight && pixelHeight < This->maxImageHeight);
	
	// we will need this later
	SInt32 fftOutputSize = 1 + fftSize / 2;
	
	// check that the fft start and end inices are within the bounds of the input array
	// note that we define the fft centred at sample n to mean that the fft is
	// calculated on samples in the interval [n-(fftSize/2)+1, n+(fftSize/2)]
	SInt32 fftStartIndex = startSampleIndex - (fftSize/2) + 1;
	SInt32 fftEndIndex = endSampleIndex + (fftSize/2);
	assert(fftStartIndex >= 0 && fftEndIndex < inputLength);
	
	// calculate the FFT stride to get the pixelWidth. this is the number of
	// samples we shift the fft window each time we compute it. It may take a
	// non-integer value. If stride is a non-integer then the actual stride used
	// will still be an integer but it will vary between two consecutive values.
	SInt32 sampleWidth = endSampleIndex - startSampleIndex + 1;
	float fftStride = (float)(sampleWidth) / (float)pixelWidth;
	
	// find the first sample of the first fft window
	SInt32 fftFirstWindowStart = fftStartIndex - fftSize/2 + 1;
	
	
	// generate the image one column at a time
	for(SInt32 i=0; i<pixelWidth; i++){
		// find the index where the fft window starts
		SInt32 fftCurrentWindowStart = (SInt32)roundf((float)i*fftStride + FLT_EPSILON) + fftFirstWindowStart;
		
		// set the last window to start in the correct place in case the multiplication above isn't accurate
		if(i==pixelWidth-1)
			fftCurrentWindowStart = fftEndIndex - fftSize/2 + 1;
		
		// take abs(fft(windowFunctin*windowSamples))
		This->b1[fftOutputSize-1] = BMSpectrum_processDataBasic(&This->spectrum,
														  inputAudio + fftCurrentWindowStart,
														  This->b1,
														  true,
														  fftSize);
		
		// write some zeros after the end as padding for the interpolation function
		memset(This->b1 + fftOutputSize, 0, sizeof(float)*This->fftBinInterpolationPadding);
		
		// interpolate the output from linear scale frequency to Bark Scale
		BMSpectrogram_fftBinsToBarkScale(This,
										 This->b1,
									 	 This->b2,
										 fftSize,
										 pixelHeight,
										 minFrequency,
										 maxFrequency);
		
		// convert to dB, scale to [0,1] and clip values outside that range
		BMSpectrogram_toDbScaleAndClip(This->b2, This->b2, pixelHeight);
		
		// convert to HSB colours and write to output
		BMSpectrogram_toHSBColour(This->b2,imageOutput[i],pixelHeight);
	}
}
