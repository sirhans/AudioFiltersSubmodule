//
//  BMBlip.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 26/3/21.
//  We hereby release this file into the public domain without restrictions
//

#include "BMBlip.h"
#include <Accelerate/Accelerate.h>
#include "BMIntegerMath.h"
#include "Constants.h"


#define BM_BLIP_MIN_OUTPUT 0.0000001 // -140 dB


void BMBlip_update(BMBlip *This, float lowpassFc, size_t filterOrder){
	
    // update p and n
    This->filterConfBack->p = 48000.0 / lowpassFc;
    This->filterConfBack->n_i = filterOrder;
    This->filterConfBack->n = This->filterConfBack->n_i;
    This->filterConfBack->negNOverP = -This->filterConfBack->n / This->filterConfBack->p;
    This->filterConfBack->pHatNegN = powf(This->filterConfBack->p, -This->filterConfBack->n);
    
    // fill the buffer
    float zero = 0.0f;
    float increment = -1.0 * This->filterConfBack->n * This->dt / This->filterConfBack->p;
    vDSP_vramp(&zero, &increment, This->filterConfBack->exp, 1, This->bufferLength);
    int bufferLengthI = (int)This->bufferLength;
    vvexpf(This->filterConfBack->exp, This->filterConfBack->exp, &bufferLengthI);
    
    // notify the oscillator to flip the buffers
    This->filterConfNeedsFlip = true;
}





void BMBlip_flipFilterConfig(BMBlip *This){
    BMBlipFilterConfig *temp = This->filterConf;
    This->filterConf = This->filterConfBack;
    This->filterConfBack = temp;
    
    This->filterConfNeedsFlip = false;
}





void BMBlip_restart(BMBlip *This, float offset){
    // offset in [0,1)
    assert(0.0f <= offset && offset < 1.0f);
    
    This->t0 = (1.0f - offset) * This->dt;
    
    // flip the buffers if necessary
    if(This->filterConfNeedsFlip)
        BMBlip_flipFilterConfig(This);
}





void BMBlip_init(BMBlip *This, size_t filterOrder, float lowpassFc, float sampleRate){
    assert(isPowerOfTwo(filterOrder));
    
    This->sampleRate = sampleRate;
	This->dt = This->sampleRate / 48000.0f;
    
    // allocate buffers for pre-computing the exp function
    This->bufferLength = BM_BUFFER_CHUNK_SIZE;
    This->filterConf1.exp = malloc(sizeof(float) * This->bufferLength);
    This->filterConf2.exp = malloc(sizeof(float) * This->bufferLength);
    This->filterConf = &This->filterConf1;
    This->filterConfBack = &This->filterConf2;
    This->b1 = malloc(sizeof(float) * This->bufferLength);
    This->b2 = malloc(sizeof(float) * This->bufferLength);
    
    // init some variables
    This->t0 = 0.0f;
    
    // fill the exp buffers. these buffers store pre-calculated values for an
    // array of output of the exp function. we do this to avoid computing that
    // function in real-time. We call this twice with a buffer flip in between
    // because there are two exp buffers.
    BMBlip_update(This, lowpassFc, filterOrder);
    BMBlip_flipFilterConfig(This);
    BMBlip_update(This, lowpassFc, filterOrder);
    BMBlip_flipFilterConfig(This);
}




void BMBlip_free(BMBlip *This){
    free(This->filterConf1.exp);
    free(This->filterConf2.exp);
    free(This->b1);
    free(This->b2);
    This->filterConf1.exp = NULL;
    This->filterConf2.exp = NULL;
    This->b1 = NULL;
    This->b2 = NULL;
}



float BMBlip_singleValue(float t, float n, float p){
	return expf(n - ((n * t) / p)) * powf(p,-n) * powf(t,n);
}



void BMBlip_processChunk(BMBlip *This, float *output, size_t length){
	assert(length <= This->bufferLength);
	
	// if the output level is already below the minimum, do nothing
	//
	// Note that we are checking two conditions here. The first is that t0 > p.
	// t0 == p is the peak of the impulse, where the derivative of the impulse
	// response is 0. We require t0 > p because there are small values both
	// before and after the peak. We are only concerned with small values after
	// the peak.
	float firstOutput = BMBlip_singleValue(This->t0, This->filterConf->n, This->filterConf->p);
	if(This->t0 > This->filterConf->p && firstOutput < BM_BLIP_MIN_OUTPUT) return;
	
//  // Code the slow way to test that the vectorised way below is correct
//	for(size_t i = 0; i<length; i++){
//		float t = This->t0 + (float)i * This->dt;
//		output[i] += BMBlip_singleValue(t, This->filterConf->n, This->filterConf->p);
//	}
//	This->t0 = This->t0 + (float)length * This->dt;
//	return;
	
	// Mathematica:
	//   E^(n - (n t)/p) p^-n t^n
	
	// b1 = E^(n - (n t)/p)
	//
	// In This->expb is a buffer containing E^(-t * n / p) for t = [0..length]
	// By multiplying This->expb by a constant scaling factor we can get
	// E^(n - (n t)/p) without doing exponentiation in real time.
	float exp_0 = expf(This->filterConf->n + (This->t0 * This->filterConf->negNOverP));
	float expScale = exp_0 / This->filterConf->exp[0];
	vDSP_vsmul(This->filterConf->exp, 1, &expScale, This->b1, 1, length);
	
	// fill a buffer with consecutive time values
	float *t = This->b2;
	vDSP_vramp(&This->t0, &This->dt, t, 1, length);
	
	// calculate the first time value for the following buffer
	float next_t0 = t[length-1] + This->dt;
	
	//
	// b2 = t^n
	//assert(isPowerOfTwo(n))
	vDSP_vsq(t, 1, This->b2, 1, length);
	size_t c = 2;
	while (c < This->filterConf->n_i){
		vDSP_vsq(This->b2, 1, This->b2, 1, length);
		c *= 2;
	}
	//
	// b2 = p^(-n) t^n
	vDSP_vsmul(This->b2, 1, &This->filterConf->pHatNegN, This->b2, 1, length);
	
	// output += b1 * b2 = (E^(n - (n t)/p))   *   ((p^-n) (t^n))
	vDSP_vma(This->b1, 1, This->b2, 1, output, 1, output, 1, length);
	
	// if t0 is negative, remove it from the output
//	if(This->t0 < 0.0f){
//		output[0] -= This->b1[0] * This->b2[0];
//	}
	
	// set the start value for the next time we call this function
	This->t0 = next_t0;
}






void BMBlip_process(BMBlip *This, float *output, size_t length){
    
        size_t samplesLeft = length;
        size_t samplesProcessed = 0;
	
        while(samplesLeft > 0){
            size_t samplesProcessing = BM_MIN(samplesLeft,This->bufferLength);
            BMBlip_processChunk(This, output + samplesProcessed, samplesProcessing);
            samplesLeft -= samplesProcessing;
            samplesProcessed += samplesProcessing;
        }
}
