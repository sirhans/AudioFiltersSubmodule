//
//  BMMonoToStereo.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/9/19.
//  Anyone may use this file without restrictions of any kind
//

#include "BMMonoToStereo.h"

#define BM_MTS_RT60 0.5f
#define BM_MTS_DIFFUSION_WIDTH 0.025f
#define BM_MTS_LOW_CROSSOVER_FC 350.0f
#define BM_MTS_HIGH_CROSSOVER_FC 1600.0f
#define BM_MTS_TAPS_PER_CHANNEL 20
#define BM_MTS_WET_MIX 0.9f



/*!
 *BMMonoToStereo_init
 */
void BMMonoToStereo_init(BMMonoToStereo *This, float sampleRate){
	
	// initialise the velvet noise decorrelator that does the main work
	BMVelvetNoiseDecorrelator_init(&This->vnd,
								   BM_MTS_DIFFUSION_WIDTH,
								   BM_MTS_TAPS_PER_CHANNEL,
								   BM_MTS_RT60,
								   BM_MTS_WET_MIX,
								   sampleRate);
	
	// initialise a pair of crossover filters that will isolate only the midrange
	// frequencies for mono-to-stereo conversion
	BMCrossover_init(&This->lowCrossover, BM_MTS_LOW_CROSSOVER_FC, sampleRate, false, true);
	BMCrossover_init(&This->highCrossover, BM_MTS_HIGH_CROSSOVER_FC, sampleRate, false, true);
	
	// allocate memory for buffers
	size_t numBuffers = 6;
	This->b1L = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE * numBuffers);
	This->b1R = This->b1L + BM_BUFFER_CHUNK_SIZE;
	This->b2L = This->b1R + BM_BUFFER_CHUNK_SIZE;
	This->b2R = This->b2L + BM_BUFFER_CHUNK_SIZE;
	This->b3L = This->b2R + BM_BUFFER_CHUNK_SIZE;
	This->b3R = This->b3L + BM_BUFFER_CHUNK_SIZE;
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
	
		// split the low frequencies off and save them for later
		float *lowL = This->b1L;
		float *lowR = This->b1R;
		float *midL = This->b2L;
		float *midR = This->b2R;
		BMCrossover_processStereo(&This->lowCrossover,
								  inputL, inputR,
								  lowL, lowR, midL, midR,
								  samplesProcessing);
		
		// split the high frequencies off and save them for later
		float *highL = This->b3L;
		float *highR = This->b3R;
		BMCrossover_processStereo(&This->highCrossover,
								  midL, midR,
								  midL, midR, highL, highR,
								  samplesProcessing);
		
		// process the Velvet Noise Decorrelator on the mid frequencies
		BMVelvetNoiseDecorrelator_processBufferStereo(&This->vnd,
													  midL, midR,
													  midL, midR,
													  samplesProcessing);
		
		// combine the low and mid back together
		vDSP_vadd(lowL, 1, midL, 1, midL, 1, samplesProcessing);
		vDSP_vadd(lowR, 1, midR, 1, midR, 1, samplesProcessing);
		
		// combine the mid and high back together
		vDSP_vadd(midL, 1, highL, 1, outputL, 1, samplesProcessing);
		vDSP_vadd(midR, 1, highR, 1, outputR, 1, samplesProcessing);
		
		// advance pointers
		numSamples -= samplesProcessing;
		inputL     += samplesProcessing;
		inputR     += samplesProcessing;
		outputL    += samplesProcessing;
		outputR    += samplesProcessing;
	}
}





/*!
 *BMMonoToStereo_free
 */
void BMMonoToStereo_free(BMMonoToStereo *This){
	BMVelvetNoiseDecorrelator_free(&This->vnd);
	BMCrossover_destroy(&This->lowCrossover);
	BMCrossover_destroy(&This->highCrossover);
	
	// free buffer memory
	// we only call free once because all buffers were allocated with a single
	// call to malloc
	free(This->b1L);
	This->b1L = NULL;
}
