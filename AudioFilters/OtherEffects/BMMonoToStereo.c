//
//  BMMonoToStereo.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/9/19.
//  Anyone may use this file without restrictions of any kind
//

#include "BMMonoToStereo.h"

#define BM_MTS_RT60 0.4f
#define BM_MTS_LOW_CROSSOVER_FC 350.0f
#define BM_MTS_HIGH_CROSSOVER_FC 1500.0f
#define BM_MTS_TAPS_PER_CHANNEL 10
#define BM_MTS_WET_MIX 0.33f

// the decorrelator will be able to influence the spectrum in frequency bands
// whose width are integer multiples of the base frequency. The base frequency
// should be set low enough so that the first few bands above
// BM_MTS_DECORRELATOR_BASE_FREQUENCY are not so wide that they distort the
// perceived EQ balance of the left and right channel. Setting the base frequency
// too low smears transients in the time domain.
#define BM_MTS_DECORRELATOR_BASE_FREQUENCY 20.0f
#define BM_MTS_DIFFUSION_TIME 1.0f / BM_MTS_DECORRELATOR_BASE_FREQUENCY



/*!
 *BMMonoToStereo_init
 */
void BMMonoToStereo_init(BMMonoToStereo *This, float sampleRate){
	
	// initialise the velvet noise decorrelator that does the main work
	BMVelvetNoiseDecorrelator_init(&This->vnd,
								   BM_MTS_DIFFUSION_TIME,
								   BM_MTS_TAPS_PER_CHANNEL,
								   BM_MTS_RT60,
								   BM_MTS_WET_MIX,
								   sampleRate);
	
	// initialise a pair of crossover filters that will isolate only the midrange
	// frequencies for mono-to-stereo conversion
    BMCrossover3way_init(&This->crossover,
                         BM_MTS_LOW_CROSSOVER_FC,
                         BM_MTS_HIGH_CROSSOVER_FC,
                         sampleRate,
                         false,
                         true);
	
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
		
		// combine the low and mid back together
		vDSP_vadd(This->lowL, 1, This->midL, 1, This->midL, 1, samplesProcessing);
		vDSP_vadd(This->lowR, 1, This->midR, 1, This->midR, 1, samplesProcessing);
		
		// combine the mid and high back together
		vDSP_vadd(This->midL, 1, This->highL, 1, outputL, 1, samplesProcessing);
		vDSP_vadd(This->midR, 1, This->highR, 1, outputR, 1, samplesProcessing);
		
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
	BMCrossover3way_free(&This->crossover);
	
	// free buffer memory
	// we only call free once because all buffers were allocated with a single
	// call to malloc
	free(This->lowL);
	This->lowL = NULL;
}
