//
//  BMLongLoopFDN.c
//  AUCloudReverb
//
//  Created by hans anderson on 3/20/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMLongLoopFDN.h"
#include "BMReverb.h"



void BMLongLoopFDN_init(BMLongLoopFDN *This, size_t numDelays, float minDelaySeconds, float maxDelaySeconds, float sampleRate){
	This->numDelays = numDelays;
	
	// get the lengths of the longest and shortest delays in samples
	size_t minDelaySamples = minDelaySeconds * sampleRate;
	size_t maxDelaySamples = maxDelaySeconds * sampleRate;
	
	// generate random delay times
	size_t *delayLengths = malloc(sizeof(float)*numDelays);
	BMReverbRandomsInRange(minDelaySamples, maxDelaySamples, delayLengths, numDelays);
	
	// init the delays
	This->delays = malloc(sizeof(BMSimpleDelayMono)*numDelays);
	for(size_t i=0; i<numDelays; i++)
		BMSimpleDelayMono_init(&This->delays[i], delayLengths[i]);
	
	// init the feedback buffers
	This->feedbackBuffers = malloc(numDelays * sizeof(TPCircularBuffer));
	for(size_t i=1; i<numDelays; i++)
		TPCircularBufferInit(&This->feedbackBuffers[i], (uint32_t)minDelaySamples);
	
	// write zeros into the head of each feedback buffer
	float *writePointer;
	uint32_t bytesAvailable;
	for(size_t i=1; i<numDelays; i++){
		writePointer = TPCircularBufferHead(&This->feedbackBuffers[i], &bytesAvailable);
		vDSP_vclr(writePointer, 1, minDelaySamples);
		TPCircularBufferProduce(&This->feedbackBuffers[i], (uint32_t)minDelaySamples * sizeof(float));
	}
	This->samplesInFeedbackBuffer = minDelaySamples;

	// init the input buffer
	This->inputBuffer = malloc(sizeof(float)*minDelaySamples);
	
	// how much does the input have to be attenuated to keep unity gain at the output?
	This->inputAttenuation = sqrt(1.0f / (float)numDelays);
}


void BMLongLoopFDN_free(BMLongLoopFDN *This){
	// free feedback buffers
	for(size_t i=0; i<This->numDelays; i++)
		TPCircularBufferCleanup(&This->feedbackBuffers[i]);
	
	free(This->inputBuffer);
	This->inputBuffer = NULL;
	
	// free delays
	for(size_t i=0; i<This->numDelays; i++)
		BMSimpleDelayMono_free(&This->delays[i]);
}


void BMLongLoopFDN_setRT60Decay(BMLongLoopFDN *This, float timeSeconds);


void BMLongLoopFDN_process(BMLongLoopFDN *This,
						   const float* inputL, const float* inputR,
						   float *outputL, float *outputR,
						   size_t numSamples){
	
	// we can only process a buffer that's no longer than the shortest delay
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(numSamples,This->samplesInFeedbackBuffer);
		
		// attenuate the left input into the input buffer
		vDSP_vsmul(inputL, 1, &This->inputAttenuation, This->inputBuffer, 1, samplesProcessing);
		
		// mix feedback from last chunk with input and process input / output from delays
		uint32_t bytesAvailable;
		for(size_t i=0; i < This->numDelays/2; i++){
			// get the read pointer for feedback buffer # i
			float *bufferReadPointer = TPCircularBufferHead(This->feedbackBuffers, &bytesAvailable);
			// for the first buffer only, confirm that there is sufficient data in the buffer
			if(i==0) assert(bytesAvailable >= samplesProcessing * sizeof(float));
			// mix the input back into the feedback buffer
			vDSP_vadd(This->inputBuffer, 1, bufferReadPointer, 1, bufferReadPointer, 1, samplesProcessing);
			// write the mixed input to the delay and read the output back from the delay
			BMSimpleDelayMono_process(&This->delays[i], <#const float *input#>, <#float *output#>, <#size_t numSamples#>);
			// mark the used bytes as read
			TPCircularBufferConsume(This->feedbackBuffers, (uint32_t)samplesProcessing * sizeof(float));
		}
		
		// attenuate to get the desired RT60 decay time
		
		
		// apply the mixing matrix and cache the output until the next chunk
		
		numSamples -= samplesProcessing;
		inputL  += samplesProcessing;
		inputR  += samplesProcessing;
		outputL += samplesProcessing;
		outputR += samplesProcessing;
	}
}
