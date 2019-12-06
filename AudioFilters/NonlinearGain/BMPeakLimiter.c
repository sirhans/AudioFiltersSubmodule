//
//  BMPeakLimiter.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 12/5/19.
//  Anyone may use this file without restrictions.
//

#include "BMPeakLimiter.h"
#include <string.h>
#include <Accelerate/Accelerate.h>

#define BM_PEAK_LIMITER_LOOKAHEAD_TIME_DV 0.0005f
#define BM_PEAK_LIMITER_RELEASE_TIME_DV 0.150f
#define BM_PEAK_LIMITER_THRESHOLD_GAIN_DV 1.0f



void BMPeakLimiter_init(BMPeakLimiter *This, bool stereo, float sampleRate){
	This->sampleRate = sampleRate;
	This->lookaheadTime = BM_PEAK_LIMITER_LOOKAHEAD_TIME_DV;
	
	// init the envelope follower
	BMEnvelopeFollower_initWithCustomNumStages(&This->envelopeFollower, 2, 2, sampleRate);
	BMEnvelopeFollower_setAttackTime(&This->envelopeFollower, This->lookaheadTime);
	BMEnvelopeFollower_setReleaseTime(&This->envelopeFollower, BM_PEAK_LIMITER_RELEASE_TIME_DV);
	
	// init the delay
	size_t numChannels = (stereo == true) ? 2 : 1;
	size_t delayInSamples = This->lookaheadTime * sampleRate;
	BMShortSimpleDelay_init(&This->delay, numChannels, delayInSamples);
	
	// set the threshold
	This->thresholdGain = BM_PEAK_LIMITER_THRESHOLD_GAIN_DV;
}





void BMPeakLimiter_free(BMPeakLimiter *This){
	BMEnvelopeFollower_free(&This->envelopeFollower);
	BMShortSimpleDelay_free(&This->delay);
}





void BMPeakLimiter_setLookahead(BMPeakLimiter *This, float timeInSeconds){
	This->targetLookaheadTime = timeInSeconds;
}



void BMPeakLimiter_setReleaseTime(BMPeakLimiter *This, float timeInSeconds){
	BMEnvelopeFollower_setReleaseTime(&This->envelopeFollower, timeInSeconds);
}



void BMPeakLimiter_update(BMPeakLimiter *This){
	size_t delayInSamples = This->targetLookaheadTime * This->sampleRate;
	BMShortSimpleDelay_changeLength(&This->delay, delayInSamples);
	BMEnvelopeFollower_setAttackTime(&This->envelopeFollower, This->targetLookaheadTime);
	
	// mark the job done
	This->lookaheadTime = This->targetLookaheadTime;
}





void BMPeakLimiter_processStereo(BMPeakLimiter *This,
								 const float *inL, const float *inR,
								 float *outL, float *outR,
								 size_t numSamples){
	// if the lookahead needs an update, do that instead of processing audio
	if(This->lookaheadTime != This->targetLookaheadTime){
		memset(outL,0,numSamples*sizeof(float));
		memset(outR,0,numSamples*sizeof(float));
		BMPeakLimiter_update(This);
	}
	
	// if no update is necessary then process the audio
	else {
		// chunked processing
		while(numSamples > 0){
			size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples);
			
			// rectify the left and right channels into the buffers
			vDSP_vabs(inL, 1, This->bufferL, 1, samplesProcessing);
			vDSP_vabs(inR, 1, This->bufferR, 1, samplesProcessing);
			
			// take the max of the left and right channels into the control signal buffer
			vDSP_vmax(This->bufferL, 1, This->bufferR, 1, This->controlSignal, 1, samplesProcessing);
			
			// control signal = threshold / controlSignal
			// if we were to multiply the result by the input, the output would be nothing but +/- threshold
			vDSP_svdiv(&This->thresholdGain, This->controlSignal, 1, This->controlSignal, 1, samplesProcessing);
			
			// apply an envelope follower to the control signal to smooth it out
			BMEnvelopeFollower_processBuffer(&This->envelopeFollower, This->controlSignal, This->controlSignal, samplesProcessing);
			
			// delay the input
			const float* inputs [2] = {inL,inR};
			float* outputs [2] = {This->bufferL, This->bufferR};
			BMShortSimpleDelay_process(&This->delay, inputs, outputs, 2, samplesProcessing);
			
			// multiply the delayed inputs by the control signal and write to the output
			vDSP_vmul(This->bufferL,1,This->controlSignal,1,outL,1,samplesProcessing);
			vDSP_vmul(This->bufferR,1,This->controlSignal,1,outR,1,samplesProcessing);
			
			numSamples -= samplesProcessing;
			inL += samplesProcessing;
			inR += samplesProcessing;
			outL += samplesProcessing;
			outR += samplesProcessing;
		}
	}
}






void BMPeakLimiter_processMono(BMPeakLimiter *This,
							   const float *input,
							   float *output,
							   size_t numSamples){
	// if the lookahead needs an update, do that instead of processing audio
	if(This->lookaheadTime != This->targetLookaheadTime){
		memset(output,0,numSamples*sizeof(float));
		BMPeakLimiter_update(This);
	}
	
	// if no update is necessary then process the audio
	else {
		// chunked processing
		while(numSamples > 0){
			size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples);
			
			// rectify the input into the control signal buffer
			vDSP_vabs(input, 1, This->bufferL, 1, samplesProcessing);
			
			// control signal = threshold / controlSignal
			// if we were to multiply the result by the input, the output would be nothing but +/- threshold
			vDSP_svdiv(&This->thresholdGain, This->controlSignal, 1, This->controlSignal, 1, samplesProcessing);
			
			// apply an envelope follower to the control signal to smooth it out
			BMEnvelopeFollower_processBuffer(&This->envelopeFollower, This->controlSignal, This->controlSignal, samplesProcessing);
			
			// delay the input
			const float* inputs [1] = {input};
			float* outputs [1] = {This->bufferL};
			BMShortSimpleDelay_process(&This->delay, inputs, outputs, 1, samplesProcessing);
			
			// multiply the delayed input by the control signal and write to the output
			vDSP_vmul(This->bufferL,1,This->controlSignal,1,output,1,samplesProcessing);
			
			numSamples -= samplesProcessing;
			input += samplesProcessing;
			output += samplesProcessing;
		}
	}
}
