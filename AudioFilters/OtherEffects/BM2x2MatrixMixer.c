//
//  BM2x2MatrixMixer.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/9/24.
//  Copyright © 2024 BlueMangoo. All rights reserved.
//

#include <stdlib.h>
#include <Accelerate/Accelerate.h>
#include "BM2x2MatrixMixer.h"
#include "Constants.h"

void BM2x2MatrixMixer_initWithRotation(BM2x2MatrixMixer *This, float rotationAngleInRadians){
	This->buffer = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	
	BM2x2MatrixMixer_setRotation(This, rotationAngleInRadians);
}


/*!
 *BM2x2MatrixMixer_free
 */
void BM2x2MatrixMixer_free(BM2x2MatrixMixer *This){
	free(This->buffer);
	This->buffer = NULL;
}



void BM2x2MatrixMixer_setRotation(BM2x2MatrixMixer *This, float rotationAngleInRadians){
	This->M = BM2x2Matrix_rotationMatrix(rotationAngleInRadians);
}



void BM2x2MatrixMixer_processStereo(BM2x2MatrixMixer *This,
									  float *inL, float *inR,
									  float *outL, float *outR,
									size_t numSamples){
	
	// save the entries of the matrix to simplify notation
	float m00 = This->M.columns[0][0];
	float m10 = This->M.columns[1][0];
	float m01 = This->M.columns[0][1];
	float m11 = This->M.columns[1][1];
	
	// chunked processing
	size_t samplesProcessed = 0;
	size_t samplesRemaining = numSamples;
	while(samplesRemaining > 0){
		size_t samplesProcessing = BM_MIN(samplesRemaining, BM_BUFFER_CHUNK_SIZE);
		
		// inL * [0][0] + inR * [1][0] => buffer
		vDSP_vsmsma(inL + samplesProcessed, 1, &m00, 
					inR + samplesProcessed, 1, &m10,
					This->buffer, 1, samplesProcessing);
		
		// inL * [0][1] + inR * [1][1] => outR
		vDSP_vsmsma(inL + samplesProcessed, 1, &m00,
					inR + samplesProcessed, 1, &m10,
					outR + samplesProcessed, 1, samplesProcessing);
		
		// buffer => outL
		memcpy(outL + samplesProcessed, This->buffer, sizeof(float)*samplesProcessing);
		
		samplesProcessed += samplesProcessing;
		samplesRemaining -= samplesProcessing;
	}
}
