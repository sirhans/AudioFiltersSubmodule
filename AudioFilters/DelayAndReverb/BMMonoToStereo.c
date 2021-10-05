//
//  BMMonoToStereo.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/9/19.
//  Anyone may use this file without restrictions of any kind
//

#include "BMMonoToStereo.h"


#define BM_MTS_RT60 0.50f
#define BM_MTS_LOW_CROSSOVER_FC 350.0f
#define BM_MTS_HIGH_CROSSOVER_FC 1200.0f
#define BM_MTS_TAPS_PER_CHANNEL 24
#define BM_MTS_TAPS_PER_CHANNEL_BIG 128
#define BM_MTS_WET_MIX 0.92f
#define BM_MTS_DECORRELATOR_FREQUENCY_BAND_WIDTH 50.0f
#define BM_MTS_DIFFUSION_TIME 1.0f / BM_MTS_DECORRELATOR_FREQUENCY_BAND_WIDTH
#define BM_MTS_DIFFUSION_TIME_BIG 2.0f / BM_MTS_DECORRELATOR_FREQUENCY_BAND_WIDTH



/*!
 *BMMonoToStereo_init
 */
void BMMonoToStereo_init(BMMonoToStereo *This, float sampleRate, bool stereoInput){
	This->stereoInput = stereoInput;
	
	// initialise the velvet noise decorrelator that does the main work
	BMVelvetNoiseDecorrelator_init(&This->vnd,
								   BM_MTS_DIFFUSION_TIME,
								   BM_MTS_TAPS_PER_CHANNEL,
								   BM_MTS_RT60,
								   true,
								   sampleRate);
	
	// set the wet/dry mix
	BMVelvetNoiseDecorrelator_setWetMix(&This->vnd, BM_MTS_WET_MIX);
	
	// initialise a pair of crossover filters that will isolate only the midrange
	// frequencies for mono-to-stereo conversion
    BMCrossover3way_init(&This->crossover,
                         BM_MTS_LOW_CROSSOVER_FC,
                         BM_MTS_HIGH_CROSSOVER_FC,
                         sampleRate,
                         false,
                         stereoInput);
	
	// allocate memory for buffers
	size_t numBuffers = 6;
	This->lowL = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE * numBuffers);
	This->lowR = This->lowL + BM_BUFFER_CHUNK_SIZE;
	This->midL = This->lowR + BM_BUFFER_CHUNK_SIZE;
	This->midR = This->midL + BM_BUFFER_CHUNK_SIZE;
	This->highL = This->midR + BM_BUFFER_CHUNK_SIZE;
	This->highR = This->highL + BM_BUFFER_CHUNK_SIZE;
}


void BMMonoToStereo_initBigger(BMMonoToStereo *This, float sampleRate, bool stereoInput){
    This->stereoInput = stereoInput;
    
    // initialise the velvet noise decorrelator that does the main work
    BMVelvetNoiseDecorrelator_init(&This->vnd,
                                   BM_MTS_DIFFUSION_TIME_BIG,
                                   BM_MTS_TAPS_PER_CHANNEL_BIG,
                                   BM_MTS_RT60,
                                   true,
                                   sampleRate);
    
    // set the wet/dry mix
    BMVelvetNoiseDecorrelator_setWetMix(&This->vnd, BM_MTS_WET_MIX);
    
    // initialise a pair of crossover filters that will isolate only the midrange
    // frequencies for mono-to-stereo conversion
    BMCrossover3way_init(&This->crossover,
                         BM_MTS_LOW_CROSSOVER_FC,
                         BM_MTS_HIGH_CROSSOVER_FC,
                         sampleRate,
                         false,
                         stereoInput);
    
    // allocate memory for buffers
    size_t numBuffers = 6;
    This->lowL = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE * numBuffers);
    This->lowR = This->lowL + BM_BUFFER_CHUNK_SIZE;
    This->midL = This->lowR + BM_BUFFER_CHUNK_SIZE;
    This->midR = This->midL + BM_BUFFER_CHUNK_SIZE;
    This->highL = This->midR + BM_BUFFER_CHUNK_SIZE;
    This->highR = This->highL + BM_BUFFER_CHUNK_SIZE;
}



/*!
 *BMMonoToStereo_processBuffer
 */
void BMMonoToStereo_processBuffer(BMMonoToStereo *This,
								  const float* inputL, const float* inputR,
								  float* outputL, float* outputR,
								  size_t numSamples){
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
	
		if(This->stereoInput){
			// split low, mid, and high
			BMCrossover3way_processStereo(&This->crossover,
										  inputL, inputR,
										  This->lowL, This->lowR,
										  This->midL, This->midR,
										  This->highL, This->highR,
										  samplesProcessing);
			
			
			// process the Velvet Noise Decorrelator on the mid frequencies
			BMVelvetNoiseDecorrelator_processBufferStereo(&This->vnd,
														  This->midL, This->midR,
														  This->midL, This->midR,
														  samplesProcessing);
			
			// combine the low, mid, and high back together
			BMCrossover3way_recombine(This->lowL, This->lowR,
									  This->midL, This->midR,
									  This->highL, This->highR,
									  outputL, outputR,
									  samplesProcessing);
			
			// advance pointers
			numSamples -= samplesProcessing;
			inputL     += samplesProcessing;
			inputR     += samplesProcessing;
			outputL    += samplesProcessing;
			outputR    += samplesProcessing;
		}
		
		// mono input
		else {
			// split low, mid, and high
			BMCrossover3way_processMono(&This->crossover,
										inputL,
										This->lowL,
										This->midL,
										This->highL,
										samplesProcessing);
			
			
			// process the Velvet Noise Decorrelator on the mid frequencies
			BMVelvetNoiseDecorrelator_processBufferMonoToStereo(&This->vnd,
																This->midL,
																This->midL, This->midR,
																samplesProcessing);
			
			// combine the low, mid, and high back together
			BMCrossover3way_recombine(This->lowL, This->lowL,
									  This->midL, This->midR,
									  This->highL, This->highL,
									  outputL, outputR,
									  samplesProcessing);
			
			// advance pointers
			numSamples -= samplesProcessing;
			inputL     += samplesProcessing;
			outputL    += samplesProcessing;
			outputR    += samplesProcessing;
		}
	}
}





/*!
 *BMMonoToStereo_free
 */
void BMMonoToStereo_free(BMMonoToStereo *This){
	BMVelvetNoiseDecorrelator_free(&This->vnd);
	BMCrossover3way_free(&This->crossover);
	
	// free buffer memory
	// we only call free once because all buffers were allocated with a single
	// call to malloc
	free(This->lowL);
	This->lowL = NULL;
}





void BMMonoToStereo_setWetMix(BMMonoToStereo *This, float wetMix01){
	BMVelvetNoiseDecorrelator_setWetMix(&This->vnd, wetMix01);
}
