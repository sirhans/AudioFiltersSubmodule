//
//  BMLFO.c
//  BMAudioFilters
//
//  Created by Hans on 13/5/16.
//  Copyright © 2016 Hans. All rights reserved.
//

#include "BMLFO.h"
#include <Accelerate/Accelerate.h>
#include <math.h>
#define BMLFO_PARAMETER_UPDATE_TIME_SECONDS 1.0

void BMLFO_init(BMLFO *This, float frequency, float minVal, float maxVal, float sampleRate){
	BMQuadratureOscillator_init(&This->osc, frequency, sampleRate);
	BMSmoothValue_init(&This->minValue, BMLFO_PARAMETER_UPDATE_TIME_SECONDS, sampleRate);
	BMSmoothValue_init(&This->scale, BMLFO_PARAMETER_UPDATE_TIME_SECONDS, sampleRate);
	BMLFO_setMinMaxImmediately(This, minVal, maxVal);
	
	This->b1 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->b2 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
}



void BMLFO_free(BMLFO *This){
	free(This->b1);
	free(This->b2);
	This->b1 = NULL;
	This->b2 = NULL;
}



void BMLFO_setFrequency(BMLFO *This, float frequency){
	BMQuadratureOscillator_setFrequency(&This->osc, frequency);
}



void BMLFO_setTimeInSamples(BMLFO *This, size_t timeInSamples){
	BMQuadratureOscillator_setTimeInSamples(&This->osc, timeInSamples);
}



void BMLFO_setMinMaxSmoothly(BMLFO *This, float minVal, float maxVal){
	
	float scale = maxVal - minVal;
	
	// the scale of the quadrature oscillator output is [-1,1] so we need to
	// multiply by 0.5 here
	BMSmoothValue_setValueSmoothly(&This->scale, 0.5 * scale);
	//
	// after multiplying by this scale, the range of oscillator output will be
	// [-scale/2, scale/2]
	
	// because the quadrature oscillator outputs in the range [-1,1], this
	// math is more complex than we originally hoped
	float minShifted = (scale * 0.5) + minVal;
	BMSmoothValue_setValueSmoothly(&This->minValue, minShifted);
	//
	// after adding this minShifted, the range of the oscillator output will be
	//    [-scale/2, scale/2] + minShifted
	// == [-(max-min)*0.5,(max-min)*0.5] + ((max - min) * 0.5) + min
	// == [-(max-min)*0.5 + ((max - min) * 0.5), (max-min)*0.5 + ((max - min) * 0.5)] + min
	// == [0,(max-min)] + min
	// == [min,max]
}




void BMLFO_setMinMaxImmediately(BMLFO *This, float minVal, float maxVal){
	
	float scale = maxVal - minVal;
	
	// the scale of the quadrature oscillator output is [-1,1] so we need to
	// multiply by 0.5 here
	BMSmoothValue_setValueImmediately(&This->scale, 0.5 * scale);
	
	// because the quadrature oscillator outputs in the range [-1,1], this
	// math is more complex than we originally hoped
	float minShifted = (scale * 0.5) + minVal;
	BMSmoothValue_setValueImmediately(&This->minValue, minShifted);
	//
	// after adding this minShifted, the range of the oscillator output will be
	//    [-scale/2, scale/2] + minShifted
	// == [-(max-min)*0.5,(max-min)*0.5] + ((max - min) * 0.5) + min
	// == [-(max-min)*0.5 + ((max - min) * 0.5), (max-min)*0.5 + ((max - min) * 0.5)] + min
	// == [0,(max-min)] + min
	// == [min,max]
}



void BMLFO_setUpdateTime(BMLFO *This, float timeSeconds){
	BMSmoothValue_setUpdateTime(&This->scale, timeSeconds);
	BMSmoothValue_setUpdateTime(&This->minValue, timeSeconds);
}



void BMLFO_process(BMLFO *This, float *output, size_t numSamples){
	size_t samplesRemaining = numSamples;
	size_t indexShift = 0;
	while(samplesRemaining > 0){
		size_t samplesProcessing = BM_MIN(samplesRemaining, BM_BUFFER_CHUNK_SIZE);
		
		// generate a sine wave, output to b1. (b2 is also filled but will not be used)
		BMQuadratureOscillator_process(&This->osc, This->b1, This->b2, samplesProcessing);
		
		// scale the sine wave, output to b1 (overwrite b2 as temp buffer)
		BMSmoothValue_process(&This->scale, This->b2, samplesProcessing);
		vDSP_vmul(This->b1, 1, This->b2, 1, This->b1, 1, samplesProcessing);
		
		// shift the sine wave, output to output (again overwriting b2 for temp)
		BMSmoothValue_process(&This->minValue, This->b2, samplesProcessing);
		vDSP_vadd(This->b1, 1, This->b2, 1, output + indexShift, 1, samplesProcessing);
		
		// advance the output index shift
		indexShift += samplesProcessing;
		
		// update the progress count
		samplesRemaining -= samplesProcessing;
	}
	
	This->lastOutput = output[numSamples-1];
}



float BMLFO_advance(BMLFO *This, size_t numSamples){
	
	// get values from the quadrature oscillator, then skip ahead numSamples
	// q is the quadrature signal, which we will not use
	float r,q;
	BMQuadratureOscillator_advance(&This->osc, &r, &q, numSamples);
	
	// get the scale and apply it to the oscillator output.
	float scale = BMSmoothValue_advance(&This->scale, numSamples);
	r *= scale;
		
	// get the shift and apply it to the oscillator output.
	float shift = BMSmoothValue_advance(&This->minValue, numSamples);
	r += shift;
	
	This->lastOutput = r;

	return r;
}




float BMLFO_getLastValue(BMLFO *This){
	return This->lastOutput;
}




float BMLFO_getAngle(BMLFO *This){
	return BMQuadratureOscillator_getAngle(&This->osc);
}
