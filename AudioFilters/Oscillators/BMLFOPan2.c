//
//  BMLFOPan2.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/8/24.
//

#include "BMLFOPan2.h"
#include <assert.h>
#include <Accelerate/Accelerate.h>



/*!
 *BMLFOPan2_processStereo
 *
 * Input is mixed to mono before processing.
 */
void BMLFOPan2_processStereo(BMLFOPan2 *This,
								  float *inL, float *inR,
								  float *outL, float *outR,
								  size_t numSamples){
	// chunked processing
	size_t samplesRemaining = numSamples;
	size_t samplesProcessed = 0;
	
	while(samplesRemaining > 0){
		size_t samplesProcessing = BM_MIN(samplesRemaining, BM_BUFFER_CHUNK_SIZE);
		
		// mix input to mono and buffer
		float gain = M_SQRT1_2; // reduce the gain a bit when mixing because hard pan of the mixed singal will have higher peak volume
		vDSP_vasm(inL + samplesProcessed, 1, inR + samplesProcessed, 1,
				  &gain,
				  This->buffer, 1,
				  samplesProcessing);
		
		// generate gain control signal for mixing to L channel
		BMLFO_process(&This->lfo, This->mixControlSignalL, samplesProcessing);
		
		// generate gain control signal for mixing to R channel by doing 1.0 - L
		float negOne = -1.0f;
		vDSP_vsadd(This->mixControlSignalL, 1, &negOne, This->mixControlSignalR, 1, samplesProcessing);
		vDSP_vneg(This->mixControlSignalR, 1, This->mixControlSignalR, 1, samplesProcessing);
		
		// mix output to R channel
		vDSP_vmul(This->buffer, 1,
				  This->mixControlSignalR, 1,
				  outR + samplesProcessed, 1,
				  samplesProcessing);
		
		// mix output to L channel
		vDSP_vmul(This->buffer, 1,
				  This->mixControlSignalL, 1,
				  outL + samplesProcessed, 1,
				  samplesProcessing);
		
		samplesProcessed += samplesProcessing;
		samplesRemaining -= samplesProcessing;
	}
}

/*!
 *BMLFOPan2_init
 *
 * @param This pointer to a struct
 * @param LFOFreqHz the LFO frequency in Hz
 * @param depth in [0,1]. 0 means always panned center. 1 means full L-R panning
 * @param sampleRate sample rate in Hz
 */
void BMLFOPan2_init(BMLFOPan2 *This, float LFOFreqHz, float depth, float sampleRate){
	assert(depth >= 0.0f && depth <= 1.0f);

	float min = 0.5f - (0.5f * depth);
	float max = 0.5f + (0.5f * depth);
	BMLFO_init(&This->lfo, LFOFreqHz, min, max, sampleRate);
	
	This->mixControlSignalL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->mixControlSignalR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->buffer = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
}



/*!
 *BMLFOPan2_free
 */
void BMLFOPan2_free(BMLFOPan2 *This){
	BMLFO_free(&This->lfo);
	
	free(This->mixControlSignalL);
	free(This->mixControlSignalR);
	free(This->buffer);
	This->mixControlSignalL = NULL;
	This->mixControlSignalR = NULL;
	This->buffer = NULL;
}
