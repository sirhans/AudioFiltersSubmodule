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

void BMLongLoopFDN_init(BMLongLoopFDN *This, size_t numDelays, float minDelaySeconds, float maxDelaySeconds, bool hasZeroTaps, float sampleRate){
	// require an even number of delays
	assert(numDelays % 2 == 0);
	
	This->numDelays = numDelays;
	This->hasZeroTaps = hasZeroTaps;
	
	// get the lengths of the longest and shortest delays in samples
	size_t minDelaySamples = minDelaySeconds * sampleRate;
	size_t maxDelaySamples = maxDelaySeconds * sampleRate;
	
	// generate random delay times
	size_t *delayLengths = malloc(sizeof(size_t)*numDelays);
	BMReverbRandomsInRange(minDelaySamples, maxDelaySamples, delayLengths, numDelays);
	
	// allocate the delay times evenly between L and R channels so that each
	// channel gets some short ones and some long ones
	size_t *temp = malloc(sizeof(size_t)*numDelays);
	memcpy(temp,delayLengths,sizeof(size_t)*numDelays);
	size_t j=0;
	for(size_t i=0; i<numDelays; i+= 2){
		delayLengths[j] = temp[i];
		delayLengths[j+numDelays/2] = temp[i + 1];
		j++;
	}
	
	// record delay times
	This->delayTimes = malloc(sizeof(float)*numDelays);
	for(size_t i=0; i<numDelays; i++)
		This->delayTimes[i] = (float)delayLengths[i] / sampleRate;
	
	// init the delays
	This->delays = malloc(sizeof(BMSimpleDelayMono)*numDelays);
	for(size_t i=0; i<numDelays; i++)
		BMSimpleDelayMono_init(&This->delays[i], delayLengths[i]);
	
	// init the feedback buffers
	This->feedbackBuffers = malloc(numDelays * sizeof(TPCircularBuffer));
	for(size_t i=1; i<numDelays; i++)
		TPCircularBufferInit(&This->feedbackBuffers[i], (uint32_t)minDelaySamples*sizeof(float));
	
	// write zeros into the head of each feedback buffer
	float *writePointer;
	uint32_t bytesAvailable;
	for(size_t i=1; i<numDelays; i++){
		writePointer = TPCircularBufferHead(&This->feedbackBuffers[i], &bytesAvailable);
		vDSP_vclr(writePointer, 1, minDelaySamples);
		TPCircularBufferProduce(&This->feedbackBuffers[i], (uint32_t)minDelaySamples * sizeof(float));
	}
	This->samplesInFeedbackBuffer = minDelaySamples;
	
	// how much does the input have to be attenuated to keep unity gain at the output?
	size_t numDelaysPerChannelWithZeroTap = (numDelays/2) + (hasZeroTaps ? 1 : 0);
	This->inputAttenuation = sqrt(1.0f / (float)numDelaysPerChannelWithZeroTap);
	
	// init the feedback coefficients
	float defaultDecayTime = 3.5f;
	This->feedbackCoefficients = malloc(sizeof(float)*numDelays);
	BMLongLoopFDN_setRT60Decay(This, defaultDecayTime);
	
	// init the mixing buffers
	This->mixingBuffers = malloc(sizeof(float*)*numDelays);
	This->mixingBuffers[0] = malloc(sizeof(float)*minDelaySamples);
	for(size_t i=1; i<numDelays; i++)
		This->mixingBuffers[i] = This->mixingBuffers[0] + i * minDelaySamples;
	
	// set output tap signs
	This->tapSigns = malloc(numDelays * sizeof(bool));
	for(size_t i=0; i<numDelays; i++)
		This->tapSigns[i] = i % 2 == 2 ? false : true;
	
	// randomise the order of the output tap signs in each channel
	BMLongLoopFDN_randomShuffle(This->tapSigns, numDelays/2);
	BMLongLoopFDN_randomShuffle(This->tapSigns + numDelays/2, numDelays/2);
	
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
	// free feedback buffers
	for(size_t i=0; i<This->numDelays; i++)
		TPCircularBufferCleanup(&This->feedbackBuffers[i]);
	
	free(This->feedbackCoefficients);
	This->feedbackCoefficients = NULL;
	
	free(This->delayTimes);
	This->delayTimes = NULL;
	
	free(This->mixingBuffers[0]);
	This->mixingBuffers[0] = NULL;
	free(This->mixingBuffers);
	This->mixingBuffers = NULL;
	
	free(This->tapSigns);
	This->tapSigns = NULL;
	
	// free delays
	for(size_t i=0; i<This->numDelays; i++)
		BMSimpleDelayMono_free(&This->delays[i]);
}





void BMLongLoopFDN_setRT60Decay(BMLongLoopFDN *This, float timeSeconds){
	float matrixAttenuation = sqrt(0.5);
	for(size_t i=0; i<This->numDelays; i++){
		This->feedbackCoefficients[i] = BMReverbDelayGainFromRT60(1.0f, This->delayTimes[i]);
		This->feedbackCoefficients[i] *= matrixAttenuation;
	}
}






void BMLongLoopFDN_process(BMLongLoopFDN *This,
						   const float* inputL, const float* inputR,
						   float *outputL, float *outputR,
						   size_t numSamples){
	
	while(numSamples > 0){
		// we can only process in chunks that are no longer than the shortest delay
		size_t samplesProcessing = BM_MIN(numSamples,This->samplesInFeedbackBuffer);
		
		// attenuate the left input into the output
		vDSP_vsmul(inputL, 1, &This->inputAttenuation, outputL, 1, samplesProcessing);
		vDSP_vsmul(inputR, 1, &This->inputAttenuation, outputR, 1, samplesProcessing);
		
		
		/****************************************************
		 * mix feedback from the previous chunk with input  *
		 * and process input / output from delays           *
		 ****************************************************/
		//
		uint32_t bytesAvailable;
		for(size_t i=0; i < This->numDelays; i++){
			// get the read pointer for feedback buffer # i
			float *bufferReadPointer = TPCircularBufferTail(This->feedbackBuffers, &bytesAvailable);
			
			// for the first buffer only, confirm that there is sufficient data in the buffer
			if(i==0) assert(bytesAvailable >= samplesProcessing * sizeof(float));
			
			// mix the input with the feedback signal and save to a mixing buffer
			if(i<This->numDelays/2)
				vDSP_vadd(outputL, 1, bufferReadPointer, 1, This->mixingBuffers[i], 1, samplesProcessing);
			else
				vDSP_vadd(outputR, 1, bufferReadPointer, 1, This->mixingBuffers[i], 1, samplesProcessing);
			
			// mark the used bytes as read
			TPCircularBufferConsume(&This->feedbackBuffers[i], (uint32_t)samplesProcessing * sizeof(float));
			
			// write the mixed input to the delay and read the output back from the delay
			BMSimpleDelayMono_process(&This->delays[i], This->mixingBuffers[i], This->mixingBuffers[i], samplesProcessing);
			
			// attenuate the output to get the desired RT60 decay time
			vDSP_vsmul(This->mixingBuffers[i], 1, &This->feedbackCoefficients[i], This->mixingBuffers[i], 1, samplesProcessing);
		}
		
		
		/**************************
		 * mix to L and R outputs *
		 **************************/
		//
		// Note that the output buffers already contain the data for the zero
		// taps so if we have zero taps in the output then we want to leave
		// that data there when we mix the feedback in.
		//
		// if don't have zero taps then the first mixing buffer we copy into the
		// output buffers must be copied, not added
		if(!This->hasZeroTaps){
			// if the left zero tap is positive
			if(This->tapSigns[0]){
				memcpy(outputL, This->mixingBuffers[0], sizeof(float)*samplesProcessing);
			}
			// left zero tap is negative
			else{
				vDSP_vneg(This->mixingBuffers[0], 1, outputL, 1, samplesProcessing);
			}
			// right zero tap is positive
			if(This->tapSigns[This->numDelays/2]){
				memcpy(outputR, This->mixingBuffers[This->numDelays/2], sizeof(float)*samplesProcessing);
			}
			// right zero tap is negative
			else{
				vDSP_vneg(This->mixingBuffers[This->numDelays/2], 1, outputR, 1, samplesProcessing);
			}
		}
		//
		// mix left channel output, starting with index 0 if there is a zero-tap
		// or 1 if there isn't
		size_t i = This->hasZeroTaps ? 0 : 1;
		for(; i<This->numDelays/2; i++){
			if(This->tapSigns[i])
				vDSP_vadd(outputL, 1, This->mixingBuffers[i], 1, outputL, 1, samplesProcessing);
			else
				vDSP_vsub(outputL, 1, This->mixingBuffers[i], 1, outputL, 1, samplesProcessing);
		}
		// mix right channel output
		i = This->hasZeroTaps ? This->numDelays/2 : 1 + This->numDelays/2;
		for(; i<This->numDelays; i++){
			if(This->tapSigns[i])
				vDSP_vadd(outputR, 1, This->mixingBuffers[i], 1, outputL, 1, samplesProcessing);
			else
				vDSP_vsub(outputR, 1, This->mixingBuffers[i], 1, outputL, 1, samplesProcessing);
		}
		
		
		/**********************************************************
		 * apply mixing matrix and write to the feedback buffers. *
		 * This is a block-circulant mixing matrix with two       *
		 * n/2 x n/2 blocks. We implement it efficiently by       *
		 * applying the first stage of a fast hadamard transform  *
		 * and rotating the output index by one positin.          *
		 **********************************************************/
		//
		for(size_t i=0; i<This->numDelays; i++){
			// get the write pointer for feedback buffer # (i+1) % numDelays.
			// adding +1 to the index rotates the output so that we never write
			// into the same delay the signal came out of. This increases the
			// time before a signal passes from a given delay back to itself.
			size_t fbBufferIndex = (int)(i+1) % (int)This->numDelays;
			float *bufferWritePointer = TPCircularBufferHead(&This->feedbackBuffers[fbBufferIndex], &bytesAvailable);
			
			// for the first buffer only, confirm that there is sufficient space
			// to write to the buffer
			if(i==0) assert(bytesAvailable >= samplesProcessing * sizeof(float));
			
			// for the first numDelays/2 we add the buffer i to buffer i+numDelays/2
			size_t halfDelays = This->numDelays/2;
			if(i<halfDelays)
				vDSP_vadd(This->mixingBuffers[i], 1, This->mixingBuffers[i+halfDelays], 1, bufferWritePointer, 1, samplesProcessing);
			// and for the second half we subtract buffer i - buffer i-numDelays/2
			else
				vDSP_vsub(This->mixingBuffers[i], 1, This->mixingBuffers[i-halfDelays], 1, bufferWritePointer, 1, samplesProcessing);
			
			// mark the data written
			TPCircularBufferProduce(&This->feedbackBuffers[i], (uint32_t)samplesProcessing * sizeof(float));
		}
		
		This->samplesInFeedbackBuffer = samplesProcessing;
		
		numSamples -= samplesProcessing;
		inputL  += samplesProcessing;
		inputR  += samplesProcessing;
		outputL += samplesProcessing;
		outputR += samplesProcessing;
	}
}
