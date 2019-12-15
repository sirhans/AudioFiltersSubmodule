//
//  BMAudioStreamConverter.c
//  SaturatorAU
//
//  Created by hans anderson on 12/13/19.
//  Anyone may use this file - no restrictions.
//

#include "BMAudioStreamConverter.h"
#include "Constants.h"
#define BM_SRC_STAGE_1_OVERSAMPLE_FACTOR 4
#define BM_SRC_INPUT_BUFFER_LENGTH BM_SRC_MAX_OUTPUT_LENGTH * sizeof(float)
#define BM_SRC_OUTPUT_BUFFER_LENGTH BM_SRC_INPUT_BUFFER_LENGTH * BM_SRC_STAGE_1_OVERSAMPLE_FACTOR


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
	
	This->input = inputFormat;
	This->output = outputFormat;
	
	// is the resampler going to be stereo or mono?
	bool stereoResampling = false;
	if(outputFormat.mChannelsPerFrame == 2 && inputFormat.mChannelsPerFrame == 2)
		stereoResampling = true;
	
	// init the resampler
	BMUpsampler_init(&This->upsampler, stereoResampling, BM_SRC_STAGE_1_OVERSAMPLE_FACTOR, BMRESAMPLER_FULL_SPECTRUM);
	
	// init the fractional offset of the output buffer read index.
    // we init to 1 so that there is room for vDSP_vqint to read back one index
	This->nextStartIndex = 1.0;
	
	// set the conversion ratio
	This->conversionRatio = (double)outputFormat.mSampleRate / (double)inputFormat.mSampleRate;
	
	// init the input and output buffers
	size_t numChannels = 1;
    if(This->stereoResampling) numChannels = 2;
	for(size_t i=0; i<numChannels; i++){
		TPCircularBufferInit(&This->outputBuffers[i], (uint32_t)BM_SRC_OUTPUT_BUFFER_LENGTH);
		This->inputBuffers[i] = calloc(sizeof(float),BM_SRC_INPUT_BUFFER_LENGTH);
	}
}





void BMAudioStreamConverter_free(BMAudioStreamConverter *This){
	// get the number of channels
	size_t numChannels = 1; if(This->stereoResampling) numChannels = 2;
	
	// free the buffers
	for(size_t i=0; i<numChannels; i++){
		TPCircularBufferCleanup(&This->outputBuffers[i]);
		free(This->inputBuffers[i]);
		This->inputBuffers[i] = NULL;
	}
	
	// free the upsampler
	BMUpsampler_free(&This->upsampler);
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
	const void* inputPointers [2];
	void* outputPointers [2];
	for(size_t i=0; i<This->input.mChannelsPerFrame; i++){
		// get pointers to the buffers
		uint32_t bytesAvailable;
		inputPointers[i] = input[i];
		outputPointers[i] = TPCircularBufferHead(&This->outputBuffers[i], &bytesAvailable);
		// check that there is no buffer overrun
		assert(bytesAvailable > numSamplesIn * BM_SRC_STAGE_1_OVERSAMPLE_FACTOR * sizeof(float));
	}
	
	
	
	/***************************************
	 * convert short to float if necessary *
	 ***************************************/
	//
	if(0 == (This->input.mFormatFlags & kAudioFormatFlagIsFloat)){
		for(size_t i=0; i<This->input.mChannelsPerFrame; i++){

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
	if(This->input.mChannelsPerFrame == 2 &&
	   This->output.mChannelsPerFrame == 1){
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
										inputPointers[0], inputPointers[1],
										outputPointers[0], outputPointers[1],
										numSamplesIn);
	} else {
		BMUpsampler_processBufferMono(&This->upsampler,
									  inputPointers[0],
									  outputPointers[0],
									  numSamplesIn);
	}
	// mark bytes written
	for(size_t i=0; i<numChannelsResampling; i++){
		TPCircularBufferProduce(&This->outputBuffers[i],
								sizeof(float)*(uint32_t)numSamplesIn*BM_SRC_STAGE_1_OVERSAMPLE_FACTOR);
	}
	
	
	
	
	/***********************************************************
	 * calculate indices for linear interpolation downsampling *
	 ***********************************************************/
	//
	// get a pointer to the output buffer for reading
	uint32_t bytesAvailable;
	outputPointers[0] = TPCircularBufferTail(&This->outputBuffers[0], &bytesAvailable);
	size_t samplesAvailable = bytesAvailable / sizeof(float);
	//
	// calculate fractional start and end indices for interpolated read from the output buffers to the final output
	double startIndex = This->nextStartIndex;
	double maxEndIndex = (double)samplesAvailable;
	double readIndexStride = (double)BM_SRC_STAGE_1_OVERSAMPLE_FACTOR / This->conversionRatio;
	double remainder = fmod(maxEndIndex-startIndex,readIndexStride);
	double endIndex = maxEndIndex - remainder;
	// vDSP_vqint requires that the integer portion of endIndex be less than (samplesAvailable - 2)
	while((size_t)floor(endIndex) >= (samplesAvailable - 2))
		endIndex -= readIndexStride;
	//
	// calculate the number of samples we will output
	double numSamplesOut_d = (endIndex - startIndex) / readIndexStride;
	// numSamplesOut_d should already be an integer-valued double. We round it to eliminate numerical errors
	// before casting to size_t
	size_t numSamplesOut_i = (size_t)round(numSamplesOut_d);
	//
	// we generate evenly spaced fractional indices for linear interpolation downsampling
	// note: vgen output actually starts at start and ends at end.
	float startIndex_f = (float)startIndex;
	float endIndex_f = (float)endIndex;
	vDSP_vgen(&startIndex_f, &endIndex_f, This->indexBuffer, 1, numSamplesOut_i);
	//
	// what is the next fractional read index after the end index?
	double nextIndex = endIndex + readIndexStride;
	//
	// what is the first integer read index that will be used to produce the
	// next buffer of output? (we subtract 1 because vDSP_vqlint reads one index
    // behind the integer part of the given index)
	size_t firstReadIndexNextBuffer = (size_t)floor(nextIndex) - 1;
	//
	// how many of the bytes in the read buffer will not need to be read again next time?
	long bytesConsumed_sInt = sizeof(float) * firstReadIndexNextBuffer;
	uint32_t bytesConsumed_uInt = (uint32_t)BM_MAX(bytesConsumed_sInt, 0);
	
	
	
	/*****************************************************************
	 * quadratic interpolation downsample output buffers into output *
	 *****************************************************************/
	//
	for(size_t i=0; i<numChannelsResampling; i++){
		
		// use quadratic interpolation to copy from the output buffer to the output
		// at the fractional indices calculated above
		vDSP_vqint(outputPointers[i],
				   This->indexBuffer, 1,
				   output[i], 1,
				   numSamplesOut_i,
				   samplesAvailable);
		
		// mark the used bytes as consumed.
		TPCircularBufferConsume(&This->outputBuffers[i], bytesConsumed_uInt);
		
		if(i+1<numChannelsResampling){
			// get the read pointer to the next output buffer
			outputPointers[i+1] = TPCircularBufferTail(&This->outputBuffers[i+1], &bytesAvailable);
		}
	}
	//
	// set up the fractional read index offset for the next buffer
	This->nextStartIndex = nextIndex - (double)firstReadIndexNextBuffer;
	
	
	
	/************************************
	 * copy mono to stereo if necessary *
	 ************************************/
	//
	if(This->input.mChannelsPerFrame == 1 &&
	   This->output.mChannelsPerFrame == 2){
		memcpy(output[1],output[0],sizeof(float)*numSamplesOut_i);
	}
	
	
	return numSamplesOut_i;
}
