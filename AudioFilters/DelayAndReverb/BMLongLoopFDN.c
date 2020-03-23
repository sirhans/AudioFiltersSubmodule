//
//  BMLongLoopFDN.c
//  AUCloudReverb
//
//  Created by hans anderson on 3/20/20.
//  This file is in the public domain.
//

#include "BMLongLoopFDN.h"
#include "BMReverb.h"
#include "BMIntegerMath.h"

#define BMLLFDN_MAX_BLOCK_SIZE 8

void BMLongLoopFDN_randomShuffle(bool* A, size_t length);

void BMLongLoopFDN_init(BMLongLoopFDN *This,
						size_t numDelays,
						float minDelaySeconds,
						float maxDelaySeconds,
						bool hasZeroTaps,
						size_t blockSize,
						size_t feedbackShiftByBlock,
						float sampleRate){
	// require block size to be a power of two
	assert(isPowerOfTwo(blockSize));
	// requre block size to be within the max size limit
	assert(1 <= blockSize && blockSize <= BMLLFDN_MAX_BLOCK_SIZE);
	// block size can not exceed network size
	assert(blockSize <= numDelays);
	// require an even number of delays
	assert(numDelays % 2 == 0);
	// numDelays must be divisible by blockSize
	assert(numDelays % blockSize == 0);
	
	This->blockSize = blockSize;
	This->feedbackShiftByDelay = feedbackShiftByBlock * blockSize;
	
	// set the matrix attenuation and inverse attenuation
	This->matrixAttenuation = sqrt(1.0/(float)blockSize);
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
		TPCircularBufferInit(&This->delays[i], ((uint32_t)delayLengths[i] + (uint32_t)minDelaySamples) * sizeof(float));
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
			
			if(samplesProcessing < This->minDelaySamples)
				printf("processing short\n");
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
		
		// rename some buffers to shorten the code in the following sections:
		float **wp, **rp, **mb;
		wp = This->writePointers;
		rp = This->readPointers;
		mb = This->mixBuffers;
		
		// apply the mixing matrix and write to write pointers
		if(This->blockSize == 1){
			for(size_t i=0; i<This->numDelays; i++){
				size_t shift = (i+This->feedbackShiftByDelay) % This->numDelays;
				memcpy(wp[shift],rp[i],sizeof(float)*samplesProcessing);
			}
		}
		if(This->blockSize == 2){
			for(size_t i=0; i<This->numDelays; i+=2){
				size_t shift = (i+This->feedbackShiftByDelay) % This->numDelays;
				
				// fast hadamard transform stage 1
				vDSP_vadd(rp[i+0],1,rp[i+1],1,wp[shift+0],1,samplesProcessing);
				vDSP_vsub(rp[i+1],1,rp[i+0],1,wp[shift+1],1,samplesProcessing);
			}
		}
		if(This->blockSize == 4){
			for(size_t i=0; i<This->numDelays; i+=4){
				// fast hadamard transform stage 1
				vDSP_vadd(rp[i+0], 1, rp[i+2], 1, mb[i+0], 1, samplesProcessing);
				vDSP_vadd(rp[i+1], 1, rp[i+3], 1, mb[i+1], 1, samplesProcessing);
				vDSP_vsub(rp[i+2], 1, rp[i+0], 1, mb[i+2], 1, samplesProcessing);
				vDSP_vsub(rp[i+3], 1, rp[i+1], 1, mb[i+3], 1, samplesProcessing);
				
				// fast hadamard transform stage 2 with rotation
				size_t shift = (i+This->feedbackShiftByDelay) % This->numDelays;
				vDSP_vadd(mb[i+0], 1, mb[i+1], 1, wp[shift+0], 1, samplesProcessing);
				vDSP_vsub(mb[i+1], 1, mb[i+0], 1, wp[shift+1], 1, samplesProcessing);
				vDSP_vadd(mb[i+2], 1, mb[i+3], 1, wp[shift+2], 1, samplesProcessing);
				vDSP_vsub(mb[i+3], 1, mb[i+2], 1, wp[shift+3], 1, samplesProcessing);
			}
		}
		if(This->blockSize == 8){
			for(size_t i=0; i<This->numDelays; i+=8){
				// fast hadamard transform stage 1
				vDSP_vadd(rp[i+0], 1, rp[i+4], 1, wp[i+0], 1, samplesProcessing);
				vDSP_vadd(rp[i+1], 1, rp[i+5], 1, wp[i+1], 1, samplesProcessing);
				vDSP_vadd(rp[i+2], 1, rp[i+6], 1, wp[i+2], 1, samplesProcessing);
				vDSP_vadd(rp[i+3], 1, rp[i+7], 1, wp[i+3], 1, samplesProcessing);
				vDSP_vsub(rp[i+4], 1, rp[i+0], 1, wp[i+4], 1, samplesProcessing);
				vDSP_vsub(rp[i+5], 1, rp[i+1], 1, wp[i+5], 1, samplesProcessing);
				vDSP_vsub(rp[i+6], 1, rp[i+2], 1, wp[i+6], 1, samplesProcessing);
				vDSP_vsub(rp[i+7], 1, rp[i+3], 1, wp[i+7], 1, samplesProcessing);
				
				// fast hadamard transform stage 2
				vDSP_vadd(wp[i+0], 1, wp[i+2], 1, mb[i+0], 1, samplesProcessing);
				vDSP_vadd(wp[i+1], 1, wp[i+3], 1, mb[i+1], 1, samplesProcessing);
				vDSP_vsub(wp[i+2], 1, wp[i+0], 1, mb[i+2], 1, samplesProcessing);
				vDSP_vsub(wp[i+3], 1, wp[i+1], 1, mb[i+3], 1, samplesProcessing);
				vDSP_vadd(wp[i+4], 1, wp[i+6], 1, mb[i+4], 1, samplesProcessing);
				vDSP_vadd(wp[i+5], 1, wp[i+7], 1, mb[i+5], 1, samplesProcessing);
				vDSP_vsub(wp[i+6], 1, wp[i+4], 1, mb[i+6], 1, samplesProcessing);
				vDSP_vsub(wp[i+7], 1, wp[i+5], 1, mb[i+7], 1, samplesProcessing);
				
				// fast hadamard transform stage 3 with rotation
				size_t shift = (i + This->feedbackShiftByDelay) % This->numDelays;
				vDSP_vadd(mb[i+0], 1, mb[i+1], 1, wp[shift+0], 1, samplesProcessing);
				vDSP_vsub(mb[i+1], 1, mb[i+0], 1, wp[shift+1], 1, samplesProcessing);
				vDSP_vadd(mb[i+2], 1, mb[i+3], 1, wp[shift+2], 1, samplesProcessing);
				vDSP_vsub(mb[i+3], 1, mb[i+2], 1, wp[shift+3], 1, samplesProcessing);
				vDSP_vadd(mb[i+4], 1, mb[i+5], 1, wp[shift+4], 1, samplesProcessing);
				vDSP_vsub(mb[i+5], 1, mb[i+4], 1, wp[shift+5], 1, samplesProcessing);
				vDSP_vadd(mb[i+6], 1, mb[i+7], 1, wp[shift+6], 1, samplesProcessing);
				vDSP_vsub(mb[i+7], 1, mb[i+6], 1, wp[shift+7], 1, samplesProcessing);
			}
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

