//
//  BMHarmonicityMeasure.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 11/3/20.
//  This file is public domain. No restrictions.
//

#include "BMHarmonicityMeasure.h"

void BMHarmonicityMeasure_init(BMHarmonicityMeasure *This, size_t bufferLength, float sampleRate){
    BMCepstrum_init(&This->cepstrum, bufferLength);
    BMSFM_init(&This->sfm, bufferLength);
    This->input = malloc(sizeof(float)*bufferLength);
    This->output = malloc(sizeof(float)*bufferLength);
}

void BMHarmonicityMeasure_free(BMHarmonicityMeasure *This){
    BMCepstrum_free(&This->cepstrum);
    BMSFM_free(&This->sfm);
    
    free(This->input);
    This->input = NULL;
    free(This->output);
    This->output = NULL;
}

float BMHarmonicityMeasure_processStereoBuffer(BMHarmonicityMeasure *This,float* inputL,float* inputR,size_t length){
    vDSP_vadd(inputL, 1, inputR, 1, This->input, 1, length);
    
	return BMHarmonicityMeasure_processMonoBuffer(This, This->input, length);
}

float BMHarmonicityMeasure_processMonoBuffer(BMHarmonicityMeasure *This,float* input,size_t length){
    BMCepstrum_getCepstrum(&This->cepstrum, input, This->output, false, length);
	
	// the cepstrum outputs half as many elements as its input
	size_t cepstrumOutputLength = length/4;
	
	// it is possible that we might want to use only part of the cepstrum output
	size_t GMAMInputLength = cepstrumOutputLength;
    
	// Take the geometric mean / arithmetic mean of the output, skipping the first element
	return BMGeometricArithmeticMean(This->output + 1, This->sfm.b2, GMAMInputLength);
}
