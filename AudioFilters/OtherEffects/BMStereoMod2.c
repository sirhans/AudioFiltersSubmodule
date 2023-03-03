//
//  BMStereoMod2.c
//  SaturatorAU
//
//  Created by hans anderson on 10/14/19.
//  Copyright © 2019 bluemangoo. All rights reserved.
//

#include "BMStereoMod2.h"
#include "BMMonoToStereo.h"
#include "Constants.h"

// decorrelator settings
#define BM_SM2_RT60 0.3f
#define BM_SM2_LOW_CROSSOVER_FC 350.0f
#define BM_SM2_HIGH_CROSSOVER_FC 1200.0f
#define BM_SM2_TAPS_PER_CHANNEL 3
#define BM_SM2_DECORRELATOR_FREQUENCY_BAND_WIDTH 40.0f
#define BM_SM2_DIFFUSION_TIME 1.0f / BM_SM2_DECORRELATOR_FREQUENCY_BAND_WIDTH

// LFO settings
#define BM_SM2_MOD_RATE 0.6f

// other settings
#define BM_SM2_WET_MIX 0.40f


/*!
 *BMStereoMod2_init
 */
void BMStereoMod2_init(BMStereoMod2 *This, float sampleRate){
	
	// init the 3-way crossover
	BMCrossover3way_init(&This->crossover,
						 BM_SM2_LOW_CROSSOVER_FC,
						 BM_SM2_HIGH_CROSSOVER_FC,
						 sampleRate,
						 false,
						 true);
	
	// init four velvet noise decorrelators with wet gain at zero
	for(size_t i=0; i<4; i++)
		BMVelvetNoiseDecorrelator_init(&This->decorrelators[i],
									   BM_SM2_DIFFUSION_TIME,
									   BM_SM2_TAPS_PER_CHANNEL,
									   BM_SM2_RT60,
									   false,
									   sampleRate);
	
	// init the quadrature oscillator
	BMQuadratureOscillator_init(&This->qOscillator,
								BM_SM2_MOD_RATE,
								sampleRate);
	
	// init the wet / dry mixer
	BMWetDryMixer_init(&This->wetDryMixer, sampleRate);
	BMWetDryMixer_setMix(&This->wetDryMixer, BM_SM2_WET_MIX);
	
	// init temp buffers for bass, mid, treble
	This->lowL  = calloc(BM_BUFFER_CHUNK_SIZE * 8, sizeof(float));
	This->lowR  = This->lowL + BM_BUFFER_CHUNK_SIZE;
	This->midLwet  = This->lowR + BM_BUFFER_CHUNK_SIZE;
	This->midRwet  = This->midLwet + BM_BUFFER_CHUNK_SIZE;
	This->midLdry  = This->midRwet + BM_BUFFER_CHUNK_SIZE;
	This->midRdry  = This->midLdry + BM_BUFFER_CHUNK_SIZE;
	This->highL = This->midRdry + BM_BUFFER_CHUNK_SIZE;
	This->highR = This->highL + BM_BUFFER_CHUNK_SIZE;
	
	// init temp buffers for decorrelators
	This->vndL[0] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE * 4);
	This->vndR[0] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE * 4);
	for(size_t i=1; i<4; i++){
		This->vndL[i] = i*BM_BUFFER_CHUNK_SIZE + This->vndL[0];
		This->vndR[i] = i*BM_BUFFER_CHUNK_SIZE + This->vndR[0];
	}
}




/*!
 *BMStereoMod2_free
 */
void BMStereoMod2_free(BMStereoMod2 *This){
	BMCrossover3way_free(&This->crossover);
	for(size_t i=0; i<4; i++)
		BMVelvetNoiseDecorrelator_free(&This->decorrelators[i]);
	
	free(This->lowL);
	free(This->vndL[0]);
	free(This->vndR[0]);
}




/*!
 *BMStereoMod2_setModRate
 */
void BMStereoMod2_setModRate(BMStereoMod2 *This, float rateHz){
	BMQuadratureOscillator_setFrequency(&This->qOscillator, rateHz);
}




/*!
 *BMStereoMod2_setWetMix
 */
void BMStereoMod2_setWetMix(BMStereoMod2 *This, float wetMix01){
	BMWetDryMixer_setMix(&This->wetDryMixer, wetMix01);
}





void BM4ChannelSum(const float** buffersL, const float** buffersR,
				   float* outL, float* outR,
				   size_t numSamples){
	// sum buffers 1 and 2
	vDSP_vadd(buffersL[0], 1, buffersL[1], 1, outL, 1, numSamples);
	vDSP_vadd(buffersR[0], 1, buffersR[1], 1, outR, 1, numSamples);
	
	// add buffer 3
	vDSP_vadd(buffersL[2], 1, outL, 1, outL, 1, numSamples);
	vDSP_vadd(buffersR[2], 1, outR, 1, outR, 1, numSamples);
	
	// add buffer 4
	vDSP_vadd(buffersL[3], 1, outL, 1, outL, 1, numSamples);
	vDSP_vadd(buffersR[3], 1, outR, 1, outR, 1, numSamples);
}



/*!
 *BMStereoMod2processWithoutMod
 *
 * @abstract a special processing function that is more efficient when the mod rate is zero
 */
void BMStereoMod2processWithoutMod(BMStereoMod2 *This,
								const float* inL, const float* inR,
								float* outL, float* outR,
								size_t numSamples){
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
		
		// split into bass, mid, treble
		BMCrossover3way_processStereo(&This->crossover,
									  inL, inR,
									  This->lowL, This->lowR,
									  This->midLdry, This->midRdry,
									  This->highL, This->highR,
									  samplesProcessing);
		
		
		// run one decorrelator on the mid channel
		BMVelvetNoiseDecorrelator_processBufferStereo(&This->decorrelators[0],
													  This->midLdry, This->midRdry,
													  This->vndL[0], This->vndR[0],
													  samplesProcessing);
		
		// mix the wet and dry signal for the mid frequencies
		BMWetDryMixer_processBufferRandomPhase(&This->wetDryMixer,
											   This->vndL[0], This->vndR[0],
											   This->midLdry, This->midRdry,
											   This->midLwet, This->midLwet,
											   samplesProcessing);
		
		// recombine the bass, mid, and treble channels
		BMCrossover3way_recombine(This->lowL, This->lowR,
								  This->midLwet, This->midRwet,
								  This->highL, This->highR,
								  outL, outR,
								  samplesProcessing);
			
		// advance pointers
		numSamples -= samplesProcessing;
		inL += samplesProcessing;
		inR += samplesProcessing;
		outL += samplesProcessing;
		outR += samplesProcessing;
	}
}





/*!
 *BMStereoMod2_processWithMod
 *
 * @abstract the standard process function for stereo mod 2
 */
void BMStereoMod2_processWithMod(BMStereoMod2 *This,
						  const float* inL, const float* inR,
						  float* outL, float* outR,
						  size_t numSamples){
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
		
		// split into bass, mid, treble
		BMCrossover3way_processStereo(&This->crossover,
									  inL, inR,
									  This->lowL, This->lowR,
									  This->midLdry, This->midRdry,
									  This->highL, This->highR,
									  samplesProcessing);
		
		
		// run four channels of decorrelators on the mid channel
		for(size_t i=0; i<4; i++)
			BMVelvetNoiseDecorrelator_processBufferStereo(&This->decorrelators[i],
													  This->midLdry, This->midRdry,
													  This->vndL[i], This->vndR[i],
													  samplesProcessing);
		
		// apply quadrature phase volume envelopes to each decorrelator output
		bool zeros [4];
		BMQuadratureOscillator_volumeEnvelope4Stereo(&This->qOscillator,
													 This->vndL,
													 This->vndR,
													 zeros,
													 samplesProcessing);
		
		// update the decorrelators that are currently at zero gain to get more
		// organic variation
		for(size_t i=0; i<4; i++)
			if(zeros[i] == true)
				BMVelvetNoiseDecorrelator_randomiseAll(&This->decorrelators[i]);
		
		// mix the signal from the four decorrelators into mid wet buffers
		BM4ChannelSum((const float**)This->vndL, (const float**)This->vndR,
					  This->midLwet, This->midRwet,
					  samplesProcessing);
		
		// mix the wet and dry signal for the mid frequencies
		BMWetDryMixer_processBufferRandomPhase(&This->wetDryMixer,
											   This->midLwet, This->midRwet,
											   This->midLdry, This->midRdry,
											   This->midLwet, This->midLwet,
											   samplesProcessing);
		
		// recombine the bass, mid, and treble channels
		BMCrossover3way_recombine(This->lowL, This->lowR,
								  This->midLwet, This->midRwet,
								  This->highL, This->highR,
								  outL, outR,
								  samplesProcessing);
			
		// advance pointers
		numSamples -= samplesProcessing;
		inL += samplesProcessing;
		inR += samplesProcessing;
		outL += samplesProcessing;
		outR += samplesProcessing;
	}
}





/*!
 *BMStereoMod2_process
 */
void BMStereoMod2_process(BMStereoMod2 *This,
						  const float* inL, const float* inR,
						  float* outL, float* outR,
						  size_t numSamples){
	if(This->qOscillator.oscFreq > 0.0f)
		BMStereoMod2_processWithMod(This,
									inL, inR,
									outL, outR,
									numSamples);
	else
		BMStereoMod2processWithoutMod(This,
									  inL, inR,
									  outL, outR,
									  numSamples);
}
