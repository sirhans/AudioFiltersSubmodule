//
//  BMDynamicSmoothingFilter.c
//  AudioFiltersXCodeProject
//
//  Created by hans anderson on 3/19/19.
//
//  The author places no restrictions on the use of this file.
//
//  See https://cytomic.com/files/dsp/DynamicSmoothing.pdf for the source of
//  of the ideas implemented here.
//
//  In summary: when operated as a lowpass filter, the value in the first
//  integrator of a second order state variable filter indicates the gradient
//  of the incoming signal. Using the absolute value of this gradient to control
//  the cutoff frequency of the filter, we get a lowpass filter that operates
//  on audio frequency signals safely with cutoff frequencies below 1 Hz and
//  is capable moving its cutoff frequency up near the Nyquist frequency when
//  there is a large spike in the input value. This way we can smooth out small
//  changes and still react immediately to large changes.

#include "BMDynamicSmoothingFilter.h"
#include <Accelerate/Accelerate.h>
#include "Constants.h"

#define BM_DSF_SENSITIVITY 0.125f
#define BM_DSF_MIN_FC 1.0f
#define BM_DSF_MAX_FC 500.0f




void BMDynamicSmoothingFilter_initDefault(BMDynamicSmoothingFilter *This,
										  float sampleRate){
	BMDynamicSmoothingFilter_init(This,
								  BM_DSF_SENSITIVITY,
								  BM_DSF_MIN_FC,
								  BM_DSF_MAX_FC,
								  sampleRate);
}


void BMDynamicSmoothingFilter_init(BMDynamicSmoothingFilter *This,
								   float sensitivity,
								   float minFc,
								   float maxFc,
								   float sampleRate){
	This->sampleRate = sampleRate;
	This->sensitivity = sensitivity;
	This->gMin = tanf(M_PI * minFc / sampleRate);
	This->gMax = tanf(M_PI * maxFc / sampleRate);
	This->low1z = 0.0f;
	This->low2z = 0.0f;
	This->g = This->gMin;
}



/*!
 *BMDynamicSmoothingFilter_setMinFc
 */
void BMDynamicSmoothingFilter_setMinFc(BMDynamicSmoothingFilter *This, float minFc){
	This->gMin = tanf(M_PI * minFc / This->sampleRate);
}


/*!
*BMDynamicSmoothingFilter_setMaxFc
*/
void BMDynamicSmoothingFilter_setMaxFc(BMDynamicSmoothingFilter *This, float maxFc){
	This->gMax = tanf(M_PI * maxFc / This->sampleRate);
}



void BMDynamicSmoothingFilter_processBufferFastAccent2(BMDynamicSmoothingFilter *This,
											const float* input,
											float* output,
											size_t numSamples){
    float threshold = This->sensitivity; //DB
	for(size_t i=0; i<numSamples; i++){
		float bandz = This->low1z - This->low2z;
        //If the input is far from the output & we are going up
        if(fabsf(input[i] - This->low2z)>threshold&&
           input[i] > This->low2z
           ){
            if(This->g!=This->gMax){
                This->low1z = ((This->gMax * This->low2z) - (This->g * bandz)) / This->gMax;
                This->g = This->gMax;
            }
            
        }else{
            //Decending
            //If we are going down and the input is close to the output
            if(This->g!=This->gMin){
                if(input[i] < This->low2z){
                    This->low1z = ((This->gMin * This->low2z) - (This->g * bandz)) / This->gMin;
                    This->g = This->gMin;
                }else{
                    //Ignore g
                }
            }
            
        }
		This->low2z += This->g * (bandz);
		This->low1z += This->g * (input[i] - This->low1z);
		output[i] = This->low2z;
	}
}

void BMDynamicSmoothingFilter_processBufferFastAccent3(BMDynamicSmoothingFilter *This,
                                            const float* input,
                                            float* output,
                                            size_t numSamples){
    float threshold = This->sensitivity; //DB
    for(size_t i=0; i<numSamples; i++){
        float bandz = This->low1z - This->low2z;
        //If the input is far from the output & we are going up
        if(input[i]>threshold
           ){
            if(This->g!=This->gMax){
                This->low1z = ((This->gMax * This->low2z) - (This->g * bandz)) / This->gMax;
                This->g = This->gMax;
            }
            
        }else{
            //Decending
            //If we are going down and the input is close to the output
            if(This->g!=This->gMin){
                if(input[i] < This->low2z){
                    This->low1z = ((This->gMin * This->low2z) - (This->g * bandz)) / This->gMin;
                    This->g = This->gMin;
                }else{
                    //Ignore g
                }
            }
            
        }
        This->low2z += This->g * (bandz);
        This->low1z += This->g * (input[i] - This->low1z);
        output[i] = This->low2z;
    }
}

float BMDSF_positiveOnly(float x){
    return 0.5 * (x + fabsf(x));
}

float BMDSF_negativeOnly(float x){
    return 0.5 * (x - fabsf(x));
}

void BMDynamicSmoothingFilter_processBufferFastAccent(BMDynamicSmoothingFilter *This,
                                            const float* input,
                                            float* output,
                                            size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
        float bandz = This->low1z - This->low2z;
        float g = This->gMin + This->sensitivity * BMDSF_positiveOnly(bandz);
        g = MIN(g, This->gMax);
        This->low2z += g * (bandz);
        This->low1z += g * (input[i] - This->low1z);
        output[i] = This->low2z;
    }
}





void BMDynamicSmoothingFilter_processBufferWithFastDescent(BMDynamicSmoothingFilter *This,
														   const float* input,
														   float* output,
														   size_t numSamples){
	for(size_t i=0; i<numSamples; i++){
		float bandz = This->low2z - This->low1z;
        
		float g = This->gMin + This->sensitivity * BMDSF_positiveOnly(bandz);
		g = MIN(g, This->gMax);
        
		This->low2z += g * (-bandz);
		This->low1z += g * (input[i] - This->low1z);
		output[i] = This->low2z;
	}
}


void BMDynamicSmoothingFilter_processBufferWithFastDescent2(BMDynamicSmoothingFilter *This,
														   const float* input,
														   float* output,
														   size_t numSamples){
	for(size_t i=0; i<numSamples; i++){
		float bandz = This->low2z - This->low1z;
		
		// adjust cutoff frequency depending on the direction of motion
		// if descending
		if(input[i] < This->low2z){
			// update g if it isn't already at the right value
			if(This->g != This->gMax){
				// this keeps the gradient continuous when changing g
				This->low1z = ((This->gMax * This->low2z) - (This->g * bandz)) / This->gMax;
				This->g = This->gMax;
			}
		}
		// if ascending
		else{
			if(This->g != This->gMin){
				// this sets the gradient to zero to avoid sharp discontinuity
				This->low1z = This->low2z;
				This->g = This->gMin;
			}
		}
		
		// lowpass filter
		This->low2z += This->g * (-bandz);
		This->low1z += This->g * (input[i] - This->low1z);
		output[i] = This->low2z;
	}
}
