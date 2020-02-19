//
//  BMHugeReverb.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 12/6/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#include "BMHugeReverb.h"
#include <Accelerate/Accelerate.h>

#define BMHRV_DIFFUSER_TAPS 20
#define BMHRV_DIFFUSER_LENGTH 0.050f
#define BMHRV_PREDELAY_DEFAULT_LENGTH 0.030f
#define BMHRV_DELAY_LENGTH BMHRV_DIFFUSER_LENGTH * 4
#define BMHRV_TONE_FILTER_NUM_LEVELS 5
#define BMHRV_DEFAULT_RT60_DECAY 10.0f
#define BMHRV_LOWPASS_START_LEVEL 1
#define BMHRV_LOWPASS_NUM_LEVELS 2
#define BMHRV_LOW_END_START_LEVEL BMHRV_LOWPASS_START_LEVEL + BMHRV_LOWPASS_NUM_LEVELS
#define BMHRV_LOWPASS_MIN_FC 800.0f
#define BMHRV_LOWPASS_MAX_FC 11000.0f




void BMHugeReverb_init(BMHugeReverb *This, float sampleRate){
	This->sampleRate = sampleRate;
	
	// init diffusors
	BMEarlyReflections_init(&This->diff1, 0.0f, BMHRV_DIFFUSER_LENGTH,
							sampleRate, 1.0f, BMHRV_DIFFUSER_TAPS);
	BMEarlyReflections_init(&This->diff2, 0.0f, BMHRV_DIFFUSER_LENGTH,
							sampleRate, 1.0f, BMHRV_DIFFUSER_TAPS);
	BMEarlyReflections_init(&This->diff3, 0.0f, BMHRV_DIFFUSER_LENGTH,
							sampleRate, 1.0f, BMHRV_DIFFUSER_TAPS);
	BMEarlyReflections_init(&This->tankDiff, 0.0f, BMHRV_DIFFUSER_LENGTH,
							sampleRate, 1.0f, BMHRV_DIFFUSER_TAPS);
	This->loopDiffuserLength = BMHRV_DIFFUSER_LENGTH;
	
	// init delay
	This->delayLengthSamples = BMHRV_DELAY_LENGTH * sampleRate;
	BMSimpleDelayStereo_init(&This->delay, This->delayLengthSamples);
	
	// init predelay
	size_t predelayLengthSamples = BMHRV_PREDELAY_DEFAULT_LENGTH * sampleRate;
	BMSimpleDelayStereo_init(&This->preDelay, predelayLengthSamples);
	
	// clear the feedback buffers
	memset(This->feedbackBufferL, 0, sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	memset(This->feedbackBufferR, 0, sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	
	// init wet / dry mix to 100%
	BMWetDryMixer_init(&This->wetDryMix, sampleRate);
	
	// init the tone filter
	// the impulse response we are studying as a reference...
	//   cuts 12 dB per octave below 100 Hz
	//   cuts 24 dB per octave above 3500 Hz
	//   dips 10 dB around 1300 Hz
	//   low shelf +6 dB at 600 Hz
	BMMultiLevelBiquad_init(&This->toneFilter, BMHRV_TONE_FILTER_NUM_LEVELS, sampleRate, true, true, false);
	BMMultiLevelBiquad_setHighPass12db(&This->toneFilter, 100.0f, 0);
	BMMultiLevelBiquad_setHighOrderBWLP(&This->toneFilter, 3500.0f, BMHRV_LOWPASS_START_LEVEL, BMHRV_LOWPASS_NUM_LEVELS);
	BMMultiLevelBiquad_setLowShelfFirstOrder(&This->toneFilter, 600.0f, +6.0f, BMHRV_LOW_END_START_LEVEL);
	BMMultiLevelBiquad_setBellQ(&This->toneFilter, 1300.0f, 1.0f, -10.0f, BMHRV_LOW_END_START_LEVEL + 1);
	
	
	
	// init the decay filter
	// NOT DONE YET
}




void BMHugeReverb_setDecayTime(BMHugeReverb *This, float rt60DecaySeconds){
	// find the length of the decay
	float delayTimeInSeconds = This->delayLengthSamples / This->sampleRate;
	
	// add half the length of the diffuser inside the loop, because it
	// contributes to the average round - trip time
	delayTimeInSeconds += This->loopDiffuserLength * 0.5;
	
	// handle infinite sustain case
	if (rt60DecaySeconds == FLT_MAX) {
        This->decayCoefficient =  1.0f;
	}
	
	// handle finite decay time case
	else {
		This->decayCoefficient = pow( 10.0, (-3.0 * delayTimeInSeconds) / rt60DecaySeconds );
	}
}




void BMHugeReverb_setPredelay(BMHugeReverb *This, float predelaySeconds){
	This->preDelayTarget = predelaySeconds;
}




void BMHugeReverb_setDiffusion(BMHugeReverb	*This, float diffusion01){
	This->diffusionTarget = diffusion01;
}




void BMHugeReverb_setTone(BMHugeReverb *This, float tone01){
	// get a cutoff frequency on a logarithmic scale betweent the min and max settings
	double minFcLog2 = log2(BMHRV_LOWPASS_MIN_FC);
	double maxFcLog2 = log2(BMHRV_LOWPASS_MAX_FC);
	double toneControlExp = minFcLog2 + tone01 * (maxFcLog2 - minFcLog2);
	float toneControlFc = pow(2.0,toneControlExp);
	
	// set the filter
	BMMultiLevelBiquad_setHighOrderBWLP(&This->toneFilter, toneControlFc, BMHRV_LOWPASS_START_LEVEL, BMHRV_LOWPASS_NUM_LEVELS);
}




void BMHugeReverb_processBufferStereo(BMHugeReverb *This,
									  const float* inL, const float* inR,
									  float* outL, float* outR,
									  size_t numSamples){
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples);
		samplesProcessing = BM_MIN(samplesProcessing, This->delayLengthSamples);
		
		// copy the dry signal to the bypass buffer
		memcpy(This->bypassBufferL, inL, sizeof(float)*samplesProcessing);
		memcpy(This->bypassBufferR, inR, sizeof(float)*samplesProcessing);
		
		// apply the reverb tone control EQ
		BMMultiLevelBiquad_processBufferStereo(&This->toneFilter,
											   inL, inR,
											   This->processBufferL, This->processBufferR,
											   samplesProcessing);
		
		// process predelay
		BMSimpleDelayStereo_process(&This->preDelay,
									This->processBufferL, This->processBufferL,
									This->processBufferL, This->processBufferL,
									samplesProcessing);
	
		// process diffusers
		BMEarlyReflections_processBuffer(&This->diff1,
										 This->processBufferL, This->processBufferR,
										 This->processBufferL, This->processBufferR,
										 samplesProcessing);
		BMEarlyReflections_processBuffer(&This->diff2,
										 This->processBufferL, This->processBufferR,
										 This->processBufferL, This->processBufferR,
										 samplesProcessing);
		BMEarlyReflections_processBuffer(&This->diff3,
										 This->processBufferL, This->processBufferR,
										 This->processBufferL, This->processBufferR,
										 samplesProcessing);
		
		// mix feedback from the last processing chunk into the processBuffer
		vDSP_vadd(This->processBufferL, 1, This->feedbackBufferL,1, This->processBufferL, 1, samplesProcessing);
		vDSP_vadd(This->processBufferR, 1, This->feedbackBufferR,1, This->processBufferR, 1, samplesProcessing);
		
		// take output tap 1
		memcpy(This->outputBufferL, This->processBufferL, sizeof(float)*samplesProcessing);
		memcpy(This->outputBufferR, This->processBufferR, sizeof(float)*samplesProcessing);
		
		// process the circulating delays
		BMSimpleDelayStereo_process(&This->delay,
									This->processBufferL, This->processBufferR,
									This->processBufferL, This->processBufferR,
									samplesProcessing);
		
		// process the in-loop diffusion
		BMEarlyReflections_processBuffer(&This->tankDiff,
										 This->processBufferL, This->processBufferR,
										 This->processBufferL, This->processBufferR,
										 samplesProcessing);
		
		// apply broadband decay
		vDSP_vsmul(This->processBufferL, 1, &This->decayCoefficient, This->processBufferL, 1, samplesProcessing);
		vDSP_vsmul(This->processBufferR, 1, &This->decayCoefficient, This->processBufferR, 1, samplesProcessing);
		
		// apply the frequency-dependent decay
		// NOT DONE
		
		// take output tap 2 and adjust the gain to preserve the gain in the output buffer
		float sqrt1_2 = M_SQRT1_2;
		vDSP_vasm(This->outputBufferL, 1, This->processBufferL, 1, &sqrt1_2, This->outputBufferL, 1, samplesProcessing);
		vDSP_vasm(This->outputBufferR, 1, This->processBufferR, 1, &sqrt1_2, This->outputBufferR, 1, samplesProcessing);
		
		// cache the signal to feedback buffers, crossing left and right channels
		memcpy(This->feedbackBufferR, This->processBufferL, sizeof(float)*samplesProcessing);
		memcpy(This->feedbackBufferL, This->processBufferR, sizeof(float)*samplesProcessing);
		
		
		
		/**********************************
		 *     APPLY MODULATION HERE      *
		 **********************************/
		//   read from This->processBufferL and This->processBufferR and write back to the same place.
		
		
		
		// mix wet and dry signal
		BMWetDryMixer_processBufferRandomPhase(&This->wetDryMix,
											   This->outputBufferL, This->outputBufferR,
											   This->bypassBufferL, This->bypassBufferR,
											   outL, outR,
											   samplesProcessing);
		
		// update pointers
		numSamples -= samplesProcessing;
		inL += samplesProcessing;
		inR += samplesProcessing;
		outL += samplesProcessing;
		outR += samplesProcessing;
	}
}
