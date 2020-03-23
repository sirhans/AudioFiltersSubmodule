//
//  BMLongLoopFDN.c
//  AUCloudReverb
//
//  Created by hans anderson on 3/20/20.
//  This file is in the public domain.
//

#include "BMLongLoopFDN.h"
#include "BMReverb.h"

void BMLongLoopFDN_randomShuffle(bool* A, size_t length);

void BMLongLoopFDN_init(BMLongLoopFDN *This,
						size_t numDelays,
						float minDelaySeconds,
						float maxDelaySeconds,
						bool hasZeroTaps,
						enum BMLLFDNMixingMatrixBlockSize blockSize,
						enum BMLLFDNMixingMatrixBlockArrangement blockArrangement,
						float sampleRate){
	// require an even number of delays
	assert(numDelays % 2 == 0);
	
	This->blockSize = blockSize;
	This->blockArrangement = blockArrangement;
	
	// set the matrix attenuation
	if(blockSize == BMMM_1x1){
		This->matrixAttenuation = 1.0;
	}
	if(blockSize == BMMM_2x2){
		This->matrixAttenuation = sqrt(1.0/2.0);
	}
	if(blockSize == BMMM_4x4){
		This->matrixAttenuation = sqrt(1.0/4.0);
	}
	
	This->inverseMatrixAttenuation = 1.0f / This->matrixAttenuation;
	
	This->numDelays = numDelays;
	This->hasZeroTaps = hasZeroTaps;
	
	// get the lengths of the longest and shortest delays in samples
	size_t minDelaySamples = minDelaySeconds * sampleRate;
	size_t maxDelaySamples = maxDelaySeconds * sampleRate;
	This->minDelaySamples = minDelaySamples;
	
	// generate random delay times
	size_t *delayLengths = malloc(sizeof(size_t)*numDelays);
	BMReverbRandomsInRange(minDelaySamples, maxDelaySamples, delayLengths, numDelays);
	
	// confirm that the delay times are sorted
	bool sorted = true;
	for(size_t i=1; i<numDelays; i++)
		if(delayLengths[i]<delayLengths[i-1]) sorted = false;
	assert(sorted);
	
	// assign the delay times evenly between L and R channels so that each
	// channel gets some short ones and some long ones
	size_t *temp = malloc(sizeof(size_t)*numDelays);
	memcpy(temp,delayLengths,sizeof(size_t)*numDelays);
	size_t j=0;
	for(size_t i=0; i<numDelays; i+= 2){
		delayLengths[j] = temp[i];
		delayLengths[j+numDelays/2] = temp[i + 1];
		j++;
	}
	
	This->mixBuffers = malloc(sizeof(float*) * This->numDelays);
	This->mixBuffers[0] = malloc(sizeof(float) * This->numDelays * minDelaySamples);
	for(size_t i=1; i<numDelays; i++)
		This->mixBuffers[i] = This->mixBuffers[0] + i * minDelaySamples;
	
	// record delay times
	This->delayTimes = malloc(sizeof(float)*numDelays);
	for(size_t i=0; i<numDelays; i++)
		This->delayTimes[i] = (float)delayLengths[i] / sampleRate;
	
	// init the input buffers
	This->inputBufferL = malloc(sizeof(float) * minDelaySamples * 2);
	This->inputBufferR = This->inputBufferL + minDelaySamples;
	
	// init the delay buffers
	This->delays = malloc(numDelays * sizeof(TPCircularBuffer));
	for(size_t i=0; i<numDelays; i++){
		TPCircularBufferInit(&This->delays[i], 2 * (uint32_t)delayLengths[i] * sizeof(float));
	}
	
	// write zeros into the head of each delay buffer to advance it to the proper delay time
	float *writePointer;
	uint32_t bytesAvailable;
	for(size_t i=0; i<numDelays; i++){
		writePointer = TPCircularBufferHead(&This->delays[i], &bytesAvailable);
		vDSP_vclr(writePointer, 1, delayLengths[i]);
		TPCircularBufferProduce(&This->delays[i], (uint32_t)delayLengths[i] * sizeof(float));
	}
	
	// init the arrays of read and write pointers
	This->readPointers = malloc(sizeof(float*) * numDelays);
	This->writePointers = malloc(sizeof(float*) * numDelays);
	
	// how much does the input have to be attenuated to keep unity gain at the output?
	size_t numDelaysPerChannelWithZeroTap = (numDelays/2) + (hasZeroTaps ? 1 : 0);
	This->inputAttenuation = sqrt(1.0f / (float)numDelaysPerChannelWithZeroTap);
	
	// init the feedback coefficients
	float defaultDecayTime = 100.0f;
	This->feedbackCoefficients = malloc(sizeof(float)*numDelays);
	BMLongLoopFDN_setRT60Decay(This, defaultDecayTime);
	
	// set output tap signs
	This->tapSigns = malloc(numDelays * sizeof(bool));
	for(size_t i=0; i<numDelays; i++)
		This->tapSigns[i] = i % 2 == 0 ? false : true;
	
	// randomise the order of the output tap signs in each channel
	BMLongLoopFDN_randomShuffle(This->tapSigns, numDelays/2);
	BMLongLoopFDN_randomShuffle(This->tapSigns + numDelays/2, numDelays/2);
	
	This->inputPan = 0.2;
	
	free(delayLengths);
	delayLengths = NULL;
	free(temp);
	temp = NULL;
}





void BMLongLoopFDN_randomShuffle(bool* A, size_t length){
	// swap every element in A with another element in a random position
	for(size_t i=0; i<length; i++){
		size_t randomIndex = arc4random() % length;
		float temp = A[i];
		A[i] = A[randomIndex];
		A[randomIndex] = temp;
	}
}





void BMLongLoopFDN_free(BMLongLoopFDN *This){
	for(size_t i=0; i<This->numDelays; i++)
		TPCircularBufferCleanup(&This->delays[i]);
	free(This->delays);
	This->delays = NULL;
	
	free(This->feedbackCoefficients);
	This->feedbackCoefficients = NULL;
	
	free(This->delayTimes);
	This->delayTimes = NULL;
	
	free(This->tapSigns);
	This->tapSigns = NULL;
	
	free(This->readPointers);
	This->readPointers = NULL;
	
	free(This->writePointers);
	This->writePointers = NULL;
	
	free(This->inputBufferL);
	This->inputBufferL = NULL;
	
	free(This->mixBuffers[0]);
	free(This->mixBuffers);
	This->mixBuffers = NULL;
}





void BMLongLoopFDN_setRT60Decay(BMLongLoopFDN *This, float timeSeconds){
	for(size_t i=0; i<This->numDelays; i++){
		This->feedbackCoefficients[i] = This->matrixAttenuation * BMReverbDelayGainFromRT60(timeSeconds, This->delayTimes[i]);
	}
	
//	// prototype
//	for(size_t i=0; i<This->numDelays; i++){
//		This->feedbackCoefficients[i] =  BMReverbDelayGainFromRT60(timeSeconds, This->delayTimes[i]);
//	}
}




void BMLongLoopFDN_setInputPan(BMLongLoopFDN *This, float pan01){
	assert(0.0f <= pan01 && pan01 <= 1.0f);
	This->inputPan = pan01;
}




void BMLongLoopFDN_process(BMLongLoopFDN *This,
						   const float* inputL, const float* inputR,
						   float *outputL, float *outputR,
						   size_t numSamples){
	
	// limit the input to chunks of size <= minDelayLength
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(numSamples, This->minDelaySamples);
		
		
		// attenuate the input to keep the volume unitary between input and output and cache to buffers
		float leftAttenuation = This->inputAttenuation * sqrt((1.0 - This->inputPan) * 2.0);
		vDSP_vsmul(inputL, 1, &leftAttenuation, This->inputBufferL, 1, samplesProcessing);
		float rightAttenuation = This->inputAttenuation * sqrt(This->inputPan * 2.0);
		vDSP_vsmul(inputR, 1, &rightAttenuation, This->inputBufferR, 1, samplesProcessing);
		
		
		// set the outputs to zero
		vDSP_vclr(outputL, 1, samplesProcessing);
		vDSP_vclr(outputR, 1, samplesProcessing);
		
		
		// get read pointers for each delay and limit the number of samples processing
		// according to what's available in the delays
		for(size_t i=0; i<This->numDelays; i++){
			uint32_t bytesAvailable;
			uint32_t bytesProcessing = (uint32_t)sizeof(float)*(uint32_t)samplesProcessing;
			// get a read pointer
			This->readPointers[i] = TPCircularBufferTail(&This->delays[i], &bytesAvailable);
			// reduce bytesProcessing to not exceed bytesAvailable
			bytesProcessing = MIN(bytesProcessing,bytesAvailable);
			// get a write pointer
			This->writePointers[i] = TPCircularBufferHead(&This->delays[i], &bytesAvailable);
			// reduce bytesProcessing to not exceed bytesAvailable
			bytesProcessing = MIN(bytesProcessing,bytesAvailable);
			
			// reduce samples processing if the requested number of samples in unavailable
			samplesProcessing = bytesProcessing / sizeof(float);
			// printf("samplesProcessing: %zu\n", samplesProcessing);
		}
		
		
		// attenuate the output signal according to the RT60 decay time and the
		// mixing matrix attenuation
		for(size_t i=0; i<This->numDelays; i++){
			// attenuate the data at the read pointers to get desired decay time
			vDSP_vsmul(This->readPointers[i], 1, &This->feedbackCoefficients[i], This->readPointers[i], 1, samplesProcessing);
		
			// we will write the first half the delays to the left output and the second half to the right output
			float *outputPointer = (i < This->numDelays/2) ? outputL : outputR;
			// output mix: add the ith delay to the output if its tap sign is positive
			if(This->tapSigns[i])
				vDSP_vadd(outputPointer, 1, This->readPointers[i], 1, outputPointer, 1, samplesProcessing);
			// or subtract it if negative
			else
				vDSP_vsub(outputPointer, 1, This->readPointers[i], 1, outputPointer, 1, samplesProcessing);
		}
		
	
		// remove the matrix attenuation from the outputs by dividing it out
		vDSP_vsmul(outputL, 1, &This->inverseMatrixAttenuation, outputL, 1, samplesProcessing);
		vDSP_vsmul(outputR, 1, &This->inverseMatrixAttenuation, outputR, 1, samplesProcessing);
		
		
		// mix the zero taps to the output if we have them
		if(This->hasZeroTaps){
			vDSP_vadd(This->inputBufferL, 1, outputL, 1, outputL, 1, samplesProcessing);
			vDSP_vadd(This->inputBufferR, 1, outputR, 1, outputR, 1, samplesProcessing);
		}
		
		
		// apply the mixing matrix and cache to mixing buffers
		// This is a block circulant matrix of size numDelays consisting of two blocks of size numDelays/2
//		for(size_t i=0; i<This->numDelays; i++){
//			// shifting the index of the output buffer gives us the block circulant matrix
//			size_t shiftedIndex = (i + 1) % This->numDelays;
//			if(i<This->numDelays/2)
//				vDSP_vadd(This->readPointers[i], 1, This->readPointers[i+(This->numDelays/2)], 1, This->writePointers[shiftedIndex], 1, samplesProcessing);
//			else
//				vDSP_vsub(This->readPointers[i], 1, This->readPointers[i-(This->numDelays/2)], 1,
//						This->writePointers[shiftedIndex], 1, samplesProcessing);
//		}
//		for(size_t i=0; i<This->numDelays; i++){
//			size_t shiftedIndex = (i+0)%This->numDelays;
//			memcpy(This->writePointers[shiftedIndex], This->readPointers[i], sizeof(float)*samplesProcessing);
//		}
//		for(size_t i=0; i<This->numDelays; i+=2){
//			size_t shiftedIndex = (i+2) % This->numDelays;
//			vDSP_vadd(This->readPointers[i], 1, This->readPointers[i+1], 1, This->writePointers[shiftedIndex], 1, samplesProcessing);
//			vDSP_vsub(This->readPointers[i+1], 1, This->readPointers[i], 1, This->writePointers[shiftedIndex+1], 1, samplesProcessing);
//		}
		for(size_t i=0; i<This->numDelays; i+=4){
			// fast hadamard transform stage 1
			vDSP_vadd(This->readPointers[i], 1, This->readPointers[i+2], 1, This->mixBuffers[i], 1, samplesProcessing);
			vDSP_vadd(This->readPointers[i+1], 1, This->readPointers[i+3], 1, This->mixBuffers[i+1], 1, samplesProcessing);
			vDSP_vsub(This->readPointers[i+2], 1, This->readPointers[i], 1, This->mixBuffers[i+2], 1, samplesProcessing);
			vDSP_vsub(This->readPointers[i+3], 1, This->readPointers[i+1], 1, This->mixBuffers[i+3], 1, samplesProcessing);
			
			// FHT stage 2 with rotation
			size_t shiftedIndex = (i+4) % This->numDelays;
			vDSP_vadd(This->mixBuffers[i], 1, This->mixBuffers[i+1], 1, This->writePointers[shiftedIndex], 1, samplesProcessing);
			vDSP_vsub(This->mixBuffers[i+1], 1, This->mixBuffers[i], 1, This->writePointers[shiftedIndex+1], 1, samplesProcessing);
			vDSP_vadd(This->mixBuffers[i+2], 1, This->mixBuffers[i+3], 1, This->writePointers[shiftedIndex+2], 1, samplesProcessing);
			vDSP_vsub(This->mixBuffers[i+3], 1, This->mixBuffers[i+2], 1, This->writePointers[shiftedIndex+3], 1, samplesProcessing);
		}
		

		// mix inputs with feedback signals and write back into the delays
		uint32_t bytesProcessing = (uint32_t)samplesProcessing * sizeof(float);
		for(size_t i=0; i<This->numDelays; i++){
			// mix the inputL and inputR into the delay inputs
			if(i<This->numDelays/2)
				vDSP_vadd(This->writePointers[i], 1, This->inputBufferL, 1, This->writePointers[i], 1, samplesProcessing);
			else
				vDSP_vadd(This->writePointers[i], 1, This->inputBufferR, 1, This->writePointers[i], 1, samplesProcessing);
			
			// mark the delays read
			TPCircularBufferConsume(&This->delays[i], bytesProcessing);
		
			// mark the delays written
			TPCircularBufferProduce(&This->delays[i], bytesProcessing);
		}
		
		
		// advance pointers
		numSamples -= samplesProcessing;
		inputL  += samplesProcessing;
		inputR  += samplesProcessing;
		outputL += samplesProcessing;
		outputR += samplesProcessing;
	}
}

