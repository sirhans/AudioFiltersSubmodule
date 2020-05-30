//
//  BMSpectrogram.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/15/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMSpectrogram.h"
#include "BMIntegerMath.h"
#include "Constants.h"

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



// http://www.chilliant.com/rgb2hsv.html
/*
 float3 HUEtoRGB(in float H)
  {
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3(R,G,B));
  }
 */
simd_float3 BMSpectrum_HUEtoRGB(float h) {
	simd_float3 a = {-1.0f, 2.0f, 2.0f};
	simd_float3 b = {-3.0f, -2.0f, -4.0f};
	simd_float3 c = {1.0f, -1.0f, -1.0f};
	simd_float3 rgb = simd_abs(6.0f * h + b) * c;
	return simd_clamp(rgb+a,0.0f, 1.0f);
}



// http://www.chilliant.com/rgb2hsv.html
simd_float3 BMSpectrum_HSLToRGB(simd_float3 hsl){
	// generate an rgb pixel with 100% saturation
	simd_float3 rgb = BMSpectrum_HUEtoRGB(hsl.x);
	
	// find out how much we need to scale down the saturated pixel to apply the
	// saturation and make headroom for the lightness
	float c = (1.0f - fabsf(2.0f * hsl.z - 1.0f)) * hsl.y;
	
	// scale the rgb pixel and mix with the lightness
	return (rgb - 0.5f) * c + hsl.z;
}



/*
 * since h and s are fixed, we can bake their values into the code and it runs
 * faster.
 */
simd_float3 BMSpectrum_HSLToRGB_baked(float l){
	// generate an rgb pixel with 100% saturation
	simd_float3 rgb = {0.0f, 0.625f, 1.0f};

	// find out how much we need to scale down the saturated pixel to apply the
	// saturation and make headroom for the lightness
	float s = 0.5f;
	float c = (1.0f - fabsf(2.0f * l - 1.0f)) * s;

	// scale the rgb pixel and mix with the lightness
	return (rgb - 0.5f) * c + l;
}



void BMSpectrogram_toRGBColour(float* input, BMRGBPixel* output, size_t length){
	// toHSBColourFunction[x_] := {7/12, 1 - x, x}
//	float h = (6.75/12.0);
//	float s = 0.5f;
//	simd_float3 hsl = {h,s,0.f};
	for(size_t i=0; i<length; i++){
		//hsl.z = input[i];
		//output[i] = BMSpectrum_HSLToRGB(hsl);
		output[i] = BMSpectrum_HSLToRGB_baked(input[i]);
	}
}





void BMSpectrogram_toDbScaleAndClip(const float* input, float* output, size_t fftSize, size_t length){
	// compensate for the scaling of the vDSP FFT
	float scale = 32.0f / (float)fftSize;
	vDSP_vsmul(input, 1, &scale, output, 1, length);
	
	// eliminate zeros and negative values
	float threshold = BM_DB_TO_GAIN(-110);
	vDSP_vthr(output, 1, &threshold, output, 1, length);
	
	// convert to db
	float zeroDb = sqrt((float)fftSize)/2.0f;
	uint32_t use20not10 = 1;
	vDSP_vdbcon(output, 1, &zeroDb, output, 1, length, use20not10);
	
	// clip to [-100,0]
	float min = -100.0f;
	float max = 0.0f;
	vDSP_vclip(output, 1, &min, &max, output, 1, length);
	
	// shift and scale to [0,1]
	scale = 1.0f/100.0f;
	float shift = 1.0f;
	vDSP_vsmsa(output, 1, &scale, &shift, output, 1, length);
}




float BMSpectrogram_getPaddingLeft(size_t fftSize){
	if(4 > fftSize)
		printf("[BMSpectrogram] WARNING: fftSize must be >=4\n");
	if(!isPowerOfTwo((size_t)fftSize))
		printf("[BMSpectrogram] WARNING: fftSize must be a power of two\n");
	return (fftSize/2) - 1;
}


float BMSpectrogram_getPaddingRight(size_t fftSize){
	if(4 > fftSize)
		printf("[BMSpectrogram] WARNING: fftSize must be >=4\n");
	if(!isPowerOfTwo((size_t)fftSize))
		printf("[BMSpectrogram] WARNING: fftSize must be a power of two\n");
	return fftSize/2;
}




void BMSpectrogram_process(BMSpectrogram *This,
						   const float* inputAudio,
						   SInt32 inputLength,
						   SInt32 startSampleIndex,
						   SInt32 endSampleIndex,
						   SInt32 fftSize,
						   BMRGBPixel** imageOutput,
						   SInt32 pixelWidth,
						   SInt32 pixelHeight,
						   float minFrequency,
						   float maxFrequency){
	assert(4 <= fftSize && fftSize <= This->maxFFTSize);
	assert(isPowerOfTwo((size_t)fftSize));
	assert(2 <= pixelHeight && pixelHeight < This->maxImageHeight);
	
	// we will need this later
	SInt32 fftOutputSize = 1 + fftSize / 2;
	
	// check that the fft start and end indices are within the bounds of the input array
	// note that we define the fft centred at sample n to mean that the fft is
	// calculated on samples in the interval [n-(fftSize/2)+1, n+(fftSize/2)]
	SInt32 fftStartIndex = startSampleIndex - (fftSize/2) + 1;
	SInt32 fftEndIndex = endSampleIndex + (fftSize/2);
	assert(fftStartIndex >= 0 && fftEndIndex < inputLength);
	
	// calculate the FFT stride to get the pixelWidth. this is the number of
	// samples we shift the fft window each time we compute it. It may take a
	// non-integer value. If stride is a non-integer then the actual stride used
	// will still be an integer but it will vary between two consecutive values.
	float sampleWidth = endSampleIndex - startSampleIndex + 1;
	float fftStride = sampleWidth / (float)(pixelWidth-1);
	
	
	// generate the image one column at a time
	for(SInt32 i=0; i<pixelWidth; i++){
		// find the index where the current fft window starts
		SInt32 fftCurrentWindowStart = (SInt32)roundf((float)i*fftStride + FLT_EPSILON) + fftStartIndex;
		
		// set the last window to start in the correct place in case the multiplication above isn't accurate
		if(i==pixelWidth-1)
			fftCurrentWindowStart = fftEndIndex - fftSize + 1;
		
		// take abs(fft(windowFunctin*windowSamples))
		This->b1[fftOutputSize-1] = BMSpectrum_processDataBasic(&This->spectrum,
														  inputAudio + fftCurrentWindowStart,
														  This->b1,
														  true,
														  fftSize);
		
		// write some zeros after the end as padding for the interpolation function
		memset(This->b1 + fftOutputSize, 0, sizeof(float)*This->fftBinInterpolationPadding);
		
		// convert to dB, scale to [0,1] and clip values outside that range
		BMSpectrogram_toDbScaleAndClip(This->b1, This->b1, fftSize, fftOutputSize);
		
		// interpolate the output from linear scale frequency to Bark Scale
		BMSpectrogram_fftBinsToBarkScale(This,
										 This->b1,
									 	 This->b2,
										 fftSize,
										 pixelHeight,
										 minFrequency,
										 maxFrequency);
		
		// clamp the outputs to [0,1]
		float lowerLimit = 0;
		float upperLimit = 1;
		vDSP_vclip(This->b2, 1, &lowerLimit, &upperLimit, This->b2, 1, pixelHeight);
		
		// convert to RGB colours and write to output
		BMSpectrogram_toRGBColour(This->b2,imageOutput[i],pixelHeight);
	}
}
