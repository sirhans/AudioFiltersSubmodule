//
//  BMAudioStreamConverter.c
//  SaturatorAU
//
//  Created by hans anderson on 12/13/19.
//  Anyone may use this file - no restrictions.
//

#include "BMAudioStreamConverter.h"
#include "Constants.h"
#include <simd/simd.h>

#define BM_SRC_STAGE_1_OVERSAMPLE_FACTOR 8
#define BM_SRC_INPUT_BUFFER_LENGTH BM_SRC_MAX_OUTPUT_LENGTH * sizeof(float)
#define BM_SRC_OUTPUT_BUFFER_LENGTH BM_SRC_INPUT_BUFFER_LENGTH * BM_SRC_STAGE_1_OVERSAMPLE_FACTOR
#define BM_SRC_SAMPLES_CLEARANCE_BEFORE 1
#define BM_SCR_SAMPLES_CLEARANCE_AFTER 2
#define BM_SCR_AAFILTER_NUM_LEVELS 4


void BMAudioStreamConverter_init(BMAudioStreamConverter *This,
								AudioStreamBasicDescription inputFormat,
								 AudioStreamBasicDescription outputFormat){
	
	/***********************************************************
	 * confirm that the input and output formats are supported *
	 ***********************************************************/
	
	// we should be increasing the sample rate, not decreasing
	assert(inputFormat.mSampleRate < outputFormat.mSampleRate);
	
	// we don't support more than two channels
	assert(inputFormat.mChannelsPerFrame <= 2);
	assert(outputFormat.mChannelsPerFrame <= 2);
	
	// we only support linear PCM
	assert(inputFormat.mFormatID == kAudioFormatLinearPCM);
	assert(outputFormat.mFormatID == kAudioFormatLinearPCM);
	
	// we only support packed formats
	assert(inputFormat.mFormatFlags & kAudioFormatFlagIsPacked);
	assert(outputFormat.mFormatFlags & kAudioFormatFlagIsPacked);
	
	// we only support non-interleaved formats
	assert(inputFormat.mFormatFlags & kAudioFormatFlagIsNonInterleaved);
	assert(outputFormat.mFormatFlags & kAudioFormatFlagIsNonInterleaved);
	
	// we only support one frame per packet
	assert(inputFormat.mFramesPerPacket == 1);
	assert(outputFormat.mFramesPerPacket == 1);
	
	// if the input is integer format, it must be short int
	if(kAudioFormatFlagIsFloat != (inputFormat.mFormatFlags & kAudioFormatFlagIsFloat))
		assert(inputFormat.mBytesPerFrame = sizeof(short) * inputFormat.mFramesPerPacket);
	// otherwise it must be 32 bit float
	else
		assert(inputFormat.mBytesPerFrame = sizeof(float) * inputFormat.mFramesPerPacket);
	
	// output must be floating point
	assert(outputFormat.mFormatFlags & kAudioFormatFlagIsFloat);
	
	
	/****************************
	 * initialise the resampler *
	 ****************************/
	
	This->inputFormat = inputFormat;
	This->outputFormat = outputFormat;
	
	// is the resampler going to be stereo or mono?
	bool stereoResampling = false;
	if(outputFormat.mChannelsPerFrame == 2 && inputFormat.mChannelsPerFrame == 2)
		stereoResampling = true;
	
	// init the resampler
	BMUpsampler_init(&This->upsampler, stereoResampling, BM_SRC_STAGE_1_OVERSAMPLE_FACTOR, BMRESAMPLER_FULL_SPECTRUM_NO_STAGE2_FILTER);
	
	// init the fractional offset of the output buffer read index.
    // we init to 1 so that there is room for vDSP_vqint to read back one index
	This->nextStartIndex = (float)BM_SRC_SAMPLES_CLEARANCE_BEFORE;
	
	// set the conversion ratio
	This->conversionRatio = (double)outputFormat.mSampleRate / (double)inputFormat.mSampleRate;
	
	// init the input and output buffers
	size_t numChannels = 1;
    if(This->stereoResampling) numChannels = 2;
	for(size_t i=0; i<numChannels; i++){
		TPCircularBufferInit(&This->upsampledBuffers[i], (uint32_t)BM_SRC_OUTPUT_BUFFER_LENGTH);
		This->inputBuffers[i] = calloc(sizeof(float),BM_SRC_INPUT_BUFFER_LENGTH);
	}
	
	// init the antialiasing filter
	BMMultiLevelBiquad_init(&This->aaFilter,
							BM_SCR_AAFILTER_NUM_LEVELS,
							outputFormat.mSampleRate,
							stereoResampling,
							true,
							false);
	BMMultiLevelBiquad_setLegendreLP(&This->aaFilter,
									 inputFormat.mSampleRate * 0.5f * 0.90f,
									 0,
									 BM_SCR_AAFILTER_NUM_LEVELS);
}





void BMAudioStreamConverter_free(BMAudioStreamConverter *This){
	// get the number of channels
	size_t numChannels = 1; if(This->stereoResampling) numChannels = 2;
	
	// free the buffers
	for(size_t i=0; i<numChannels; i++){
		TPCircularBufferCleanup(&This->upsampledBuffers[i]);
		free(This->inputBuffers[i]);
		This->inputBuffers[i] = NULL;
	}
	
	// free the upsampler
	BMUpsampler_free(&This->upsampler);
}




void BM_qint(const float *A, const float *indices, float* output, size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
        float index_f = indices[i];
		float integerPart = floorf(index_f);
		float fractionalPart = index_f - integerPart;
        size_t integerPart_i = (size_t)integerPart;
        simd_float3 a;
        float delta;
		
		// the following code is slightly better than VDSP output
		a = simd_make_float3(A[integerPart_i-1],
							A[integerPart_i],
							A[integerPart_i+1]);
		delta = fractionalPart;
		
		simd_float3 x = simd_make_float3((delta - 1.0f) * (delta - 2.0f),
										 -2.0f * delta * (delta - 2.0f),
										 delta * (delta - 1.0f));
        output[i] = simd_dot(a, x) * 0.5f;
    }
}



void BM_cint(const float *A, const float *indices, float* output, size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
        float index_f = indices[i];
		float integerPart = floorf(index_f);
		float fractionalPart = index_f - integerPart;
        size_t integerPart_i = (size_t)integerPart;
        simd_float4 a;
        float delta;
		
		a = simd_make_float4(A[integerPart_i-1],
							A[integerPart_i],
							A[integerPart_i+1],
							A[integerPart_i+2]);
		delta = fractionalPart + 1.0f;
		
        simd_float4 x = simd_make_float4(-0.16666666666f * (delta - 1.0f) * (delta - 2.0f) * (delta - 3.0f),
                                         0.5 * delta * (delta - 2.0f) * (delta - 3.0f),
                                         -0.5 * delta * (delta - 1.0f) * (delta - 3.0f),
										 0.16666666666f * delta * (delta - 1.0f) * (delta - 2.0f));
        output[i] = simd_dot(a, x);
    }
}



/*!
 *BM_intAsDoubleFix
 *
 * @abstract with floating point arithmetic we sometimes get integer values that are slightly below the exact result. If this happens when using floating point numbers to calculate integer array indices, this will cause an error when we truncate to integer. This function prevents those errors by pushing near-integer values up to the high side of the nearest integer.
 */
double BM_intAsDoubleFix(double x){
	// if x is just below the next integer
	if(x - floor(x) >= 1.0 - FLT_EPSILON){
		// return a value just slightly above the nearest integer
		return round(x) + DBL_EPSILON;
	}
	// otherwise do nothing
	return x;
}



size_t BMAudioStreamConverter_convert(BMAudioStreamConverter *This,
									  const void **input,
									  void **output,
									  size_t numSamplesIn){
	
	// get the number of channels for the resampling process
	size_t numChannelsResampling = 1; if(This->stereoResampling) numChannelsResampling = 2;
	
	
	
	/************************************
	 * set up input and output pointers *
	 ************************************/
	//
	const float* inputPointers [2];
	float* upsampledBufferWritePointers [2];
	for(size_t i=0; i<This->inputFormat.mChannelsPerFrame; i++){
		// get pointers to the buffers
		uint32_t bytesAvailable;
		inputPointers[i] = input[i];
		upsampledBufferWritePointers[i] = TPCircularBufferHead(&This->upsampledBuffers[i], &bytesAvailable);
		// check that there is no buffer overrun
		assert(bytesAvailable > numSamplesIn * BM_SRC_STAGE_1_OVERSAMPLE_FACTOR * sizeof(float));
	}
	
	
	
	/***************************************
	 * convert short to float if necessary *
	 ***************************************/
	//
	if(0 == (This->inputFormat.mFormatFlags & kAudioFormatFlagIsFloat)){
		for(size_t i=0; i<This->inputFormat.mChannelsPerFrame; i++){

			// convert from short to float
			vDSP_vflt16(input[i], 1, This->inputBuffers[i], 1, numSamplesIn);
			
			// scale the range down to [-1,1]
			float scale = (float)(-1.0 / (double)INT16_MIN);
			vDSP_vsmul(This->inputBuffers[i], 1, &scale, This->inputBuffers[i], 1, numSamplesIn);
			
			// redirect the input pointers to the inputBuffers
			inputPointers[i] = This->inputBuffers[i];
		}
	}
	
	
	
	/*********************************
	 * mix down to mono if necessary *
	 *********************************/
	//
	if(This->inputFormat.mChannelsPerFrame == 2 &&
	   This->outputFormat.mChannelsPerFrame == 1){
		float half = 0.5f;
		vDSP_vasm(inputPointers[0], 1, inputPointers[1], 1, &half, This->inputBuffers[0], 1, numSamplesIn);
		
		// redirect the input pointer to the input buffer
		inputPointers[0] = This->inputBuffers[0];
	}
	
	
	
	/*******************************************************************
	 * upsample the input into the output buffers (integer power of 2) *
	 *******************************************************************/
	//
	if(This->stereoResampling){
		BMUpsampler_processBufferStereo(&This->upsampler,
										inputPointers[0],
										inputPointers[1],
										upsampledBufferWritePointers[0],
										upsampledBufferWritePointers[1],
										numSamplesIn);
	} else {
		BMUpsampler_processBufferMono(&This->upsampler,
									  inputPointers[0],
									  upsampledBufferWritePointers[0],
									  numSamplesIn);
	}
	// mark bytes written
	for(size_t i=0; i<numChannelsResampling; i++){
		TPCircularBufferProduce(&This->upsampledBuffers[i],
								sizeof(float)*(uint32_t)numSamplesIn*BM_SRC_STAGE_1_OVERSAMPLE_FACTOR);
	}
	
	
	
	
	/*************************************************************
	 * calculate indices for lagrange interpolation downsampling *
	 *************************************************************/
	//
	// get a pointer to the output buffer for reading
	uint32_t bytesAvailable;
	float *upsampledBufferReadPointers [2];
	upsampledBufferReadPointers[0] = TPCircularBufferTail(&This->upsampledBuffers[0], &bytesAvailable);
	size_t samplesAvailable = bytesAvailable / sizeof(float);
	//
	// calculate fractional start and end indices for interpolated read from the output buffers to the final output
	double startIndex = This->nextStartIndex;
	double maxEndIndex = (double)samplesAvailable;
	double readIndexStride = (double)BM_SRC_STAGE_1_OVERSAMPLE_FACTOR / This->conversionRatio;
	double remainder = fmod(maxEndIndex-startIndex, readIndexStride);
	double endIndex = BM_intAsDoubleFix(maxEndIndex - remainder);
	// our interpolation function requires that the integer portion of endIndex be less than (samplesAvailable - BM_SCR_SAMPLES_CLEARANCE_AFTER)
	while((size_t)endIndex > (samplesAvailable - BM_SCR_SAMPLES_CLEARANCE_AFTER))
		endIndex -= readIndexStride;
	//
	// calculate the number of samples we will output
	double numSamplesOut_d = 1.0 + ((endIndex - startIndex) / readIndexStride);
	size_t numSamplesOut_i = (size_t)BM_intAsDoubleFix(numSamplesOut_d);
	//
	// we generate evenly spaced fractional indices for linear interpolation downsampling
	// note: vgen output actually starts at start and ends at end.
	float startIndex_f = (float)startIndex;
	float endIndex_f = (float)endIndex;
	vDSP_vgen(&startIndex_f, &endIndex_f, This->indexBuffer, 1, numSamplesOut_i);
	//
	// what is the next fractional read index after the end index?
	double nextIndex = endIndex + readIndexStride;
	nextIndex = BM_intAsDoubleFix(nextIndex);
	//
	// what is the first integer read index that will be used to produce the
	// next buffer of output? (we subtract BM_SCR_SAMPLES_CLEARANCE_BEFORE
    // because our interpolation function may read some indices before the
    // integer part of the given index)
	size_t firstReadIndexNextBuffer = (size_t)floor(nextIndex) - BM_SRC_SAMPLES_CLEARANCE_BEFORE;
	//
	// how many of the bytes in the read buffer will not need to be read again next time?
	long bytesConsumed_sInt = sizeof(float) * (firstReadIndexNextBuffer);
	uint32_t bytesConsumed_uInt = (uint32_t)BM_MAX(bytesConsumed_sInt, 0);
	
	
	
	/****************************************************************
	 * lagrange interpolation downsample output buffers into output *
	 ****************************************************************/
	//
	for(size_t i=0; i<numChannelsResampling; i++){
		
		// use quadratic interpolation to copy from the output buffer to the output
		// at the fractional indices calculated above
		vDSP_vqint(upsampledBufferReadPointers[i],
				   This->indexBuffer, 1,
				   output[i], 1,
				   numSamplesOut_i,
				   samplesAvailable);
//		BM_qint(upsampledBufferReadPointers[i], This->indexBuffer, output[i], numSamplesOut_i);
		
		// mark the used bytes as consumed.
		TPCircularBufferConsume(&This->upsampledBuffers[i], bytesConsumed_uInt);
		
		if(i+1 < numChannelsResampling){
			// get the read pointer to the next output buffer
			upsampledBufferWritePointers[i+1] = TPCircularBufferTail(&This->upsampledBuffers[i+1], &bytesAvailable);
		}
	}
	//
	// set up the fractional read index offset for the next buffer
	This->nextStartIndex = (nextIndex - (double)firstReadIndexNextBuffer);
	This->nextStartIndex = BM_intAsDoubleFix(This->nextStartIndex);
	
	
	/**********************************************************************
	 * antialiasing filter to remove artefacts of low-order interpolation *
	 **********************************************************************/
	//
	if(This->stereoResampling)
		BMMultiLevelBiquad_processBufferStereo(&This->aaFilter,
											   output[0], output[1],
											   output[0], output[1],
											   numSamplesOut_i);
	else
		BMMultiLevelBiquad_processBufferMono(&This->aaFilter,
											 output[0],
											 output[0],
											 numSamplesOut_i);
	
	
	/************************************
	 * copy mono to stereo if necessary *
	 ************************************/
	//
	if(This->inputFormat.mChannelsPerFrame == 1 &&
	   This->outputFormat.mChannelsPerFrame == 2){
		memcpy(output[1],output[0],sizeof(float)*numSamplesOut_i);
	}
	
	
	return numSamplesOut_i;
}
