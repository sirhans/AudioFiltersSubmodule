//
//  BMHysteresisLimiter.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 29/10/19.
//  Anyone may use this file without restrictions
//

#include "BMHysteresisLimiter.h"
#include "BMAsymptoticLimiter.h"
#include <math.h>
#include <assert.h>
#include "Constants.h"

#define BM_HYSTERESISLIMITER_DEFAULT_POWER_LIMIT -45.0f
#define BM_HYSTERESISLIMITER_AA_FILTER_FC 12000.0f
#define BM_HYSTERESISLIMITER_DEFAULT_SAG 1.0f / (4000.0f)




void BMHysteresisLimiter_processMonoRectified(BMHysteresisLimiter *This,
                                      const float *inputPos, const float *inputNeg,
                                      float* outputPos, float* outputNeg,
                                      size_t numSamples){
	assert(This->AAFilter.numChannels == 2 &&
		   numSamples % 2 == 0);
	
	// create two alias pointers for code readability
	float* limitedPos = outputPos;
	float* limitedNeg = outputNeg;
	
	// apply asymptotic limit
	BMAsymptoticLimitRectified(inputPos, inputNeg,
							   limitedPos, limitedNeg,
                               This->sampleRate,
                               This->sag,
							   numSamples);
	
	// antialiasing filter
	BMMultiLevelBiquad_processBufferStereo(&This->AAFilter,
										   limitedPos, limitedNeg,
										   limitedPos, limitedNeg,
										   numSamples);
	
    for(size_t i=0; i<numSamples; i++){
		// we alternate the order in which the positive and negative signals
		// consume charge in order to avoid biasing the signal by always consuming
		// from one side before the other
		
		// Positive, then negative
		
//		// positive output
//		float oPos = limitedPos[i] * This->c;
//		// update charge
//		This->c -= oPos * This->s;
//		This->c += This->halfSR*(1.0f - This->c);
//		// negative output
//        float oNeg = limitedNeg[i] * This->c;
//        // update charge
//		This->c += oNeg * This->s;
//		This->c += This->halfSR*(1.0f - This->c);
		// positive output
		float charge = This->halfSR * (1.0f - This->c);
		float oPos = limitedPos[i] * (This->c + charge);
		// update charge
		This->c -= oPos * This->s;
		This->c += charge;
		// negative output
		charge = This->halfSR * (1.0f - This->c);
        float oNeg = limitedNeg[i] * (This->c + charge);
        // update charge
		This->c += oNeg * This->s;
		This->c += charge;
		// output
        outputPos[i] = oPos;
        outputNeg[i] = oNeg;
		
		
		// Negative, then positive
		i++;
//		// negative output
//        oNeg = limitedNeg[i] * This->c;
//        // update charge
//		This->c += oNeg * This->s;
//		This->c += This->halfSR*(1.0f - This->c);
//		// positive output
//		oPos = limitedPos[i] * This->c;
//		// update charge
//		This->c -= oPos * This->s;
//		This->c += This->halfSR*(1.0f - This->c);
		// negative output
		charge = This->halfSR * (1.0f - This->c);
        oNeg = limitedNeg[i] * (This->c + charge);
        // update charge
		This->c += oNeg * This->s;
		This->c += charge;
		// positive output
		charge = This->halfSR * (1.0f - This->c);
		oPos = limitedPos[i] * (This->c + charge);
		// update charge
		This->c -= oPos * This->s;
		This->c += charge;
		// output
        outputPos[i] = oPos;
        outputNeg[i] = oNeg;
    }
	
	// scale to compensate for gain loss
	vDSP_vsmul(outputPos,1,&This->oneOverR,outputPos,1,numSamples);
	vDSP_vsmul(outputNeg,1,&This->oneOverR,outputNeg,1,numSamples);
}





void BMHysteresisLimiter_processStereoRectified(BMHysteresisLimiter *This,
													  const float *inputPosL,
													  const float *inputPosR,
													  const float *inputNegL,
													  const float *inputNegR,
													  float *outputPosL, float *outputPosR,
													  float *outputNegL, float *outputNegR,
													  size_t numSamples){
	assert(This->AAFilter.numChannels == 4 &&
		   numSamples % 2 == 0);
	
	// create some alias pointers for code readability
	float* limitedPosL = outputPosL;
	float* limitedNegL = outputNegL;
	float* limitedPosR = outputPosR;
	float* limitedNegR = outputNegR;
	
	// apply asymptotic limit
	BMAsymptoticLimitRectified(inputPosL, inputNegL,
							   limitedPosL, limitedNegL,
                               This->sampleRate,
                               This->sag,
							   numSamples);
	BMAsymptoticLimitRectified(inputPosR, inputNegR,
							   limitedPosR, limitedNegR,
                               This->sampleRate,
                               This->sag,
							   numSamples);
	
	// Antialiasing filter
	BMMultiLevelBiquad_processBuffer4(&This->AAFilter,
										   limitedPosL, limitedNegL,
										   limitedPosR, limitedNegR,
										   limitedPosL, limitedNegL,
										   limitedPosR, limitedNegR,
										   numSamples);
	
//	// bypass the transformer sag model
//	memcpy(outputPosL, limitedPosL, numSamples*sizeof(float));
//	memcpy(outputPosR, limitedPosR, numSamples*sizeof(float));
//	memcpy(outputNegL, limitedNegL, numSamples*sizeof(float));
//	memcpy(outputNegR, limitedNegR, numSamples*sizeof(float));
	
	
    for(size_t i=0; i<numSamples; i++){
		// we alternate the order in which the positive and negative signals
		// consume charge in order to avoid biasing the signal by always consuming
		// from one side before the other
		
		// Positive, then negative
		//
		// positive output
		simd_float2 charge = This->halfSR * (1.0f - This->cs);
		simd_float2 iPos = simd_make_float2(limitedPosL[i], limitedPosR[i]);
		simd_float2 oPos = iPos * (This->cs + charge);
		// update charge
		This->cs -= This->s * oPos;
		This->cs += charge;
		// negative output
		charge = This->halfSR * (1.0f - This->cs);
		simd_float2 iNeg = simd_make_float2(limitedNegL[i], limitedNegR[i]);
		simd_float2 oNeg = iNeg * (This->cs + charge);
        // update charge
		This->cs += This->s * oNeg;
		This->cs += charge;
		// output
        outputPosL[i] = oPos.x;
		outputPosR[i] = oPos.y;
        outputNegL[i] = oNeg.x;
		outputNegR[i] = oNeg.y;
		
		
		// negative, then positive
		i++;
		// negative output
		charge = This->halfSR * (1.0f - This->cs);
		iNeg = simd_make_float2(limitedNegL[i], limitedNegR[i]);
		oNeg = iNeg * (This->cs + charge);
        // update charge
		This->cs += This->s * oNeg;
		This->cs += charge;
		// positive output
		charge = This->halfSR * (1.0f - This->cs);
		iPos = simd_make_float2(limitedPosL[i], limitedPosR[i]);
		oPos = iPos * (This->cs + charge);
		// update charge
		This->cs -= This->s * oPos;
		This->cs += charge;
		// output
        outputPosL[i] = oPos.x;
		outputPosR[i] = oPos.y;
        outputNegL[i] = oNeg.x;
		outputNegR[i] = oNeg.y;
    }
	
	// scale to compensate for gain loss
	vDSP_vsmul(outputPosL,1,&This->oneOverR,outputPosL,1,numSamples);
	vDSP_vsmul(outputPosR,1,&This->oneOverR,outputPosR,1,numSamples);
	vDSP_vsmul(outputNegL,1,&This->oneOverR,outputNegL,1,numSamples);
	vDSP_vsmul(outputNegR,1,&This->oneOverR,outputNegR,1,numSamples);
}





void BMHysteresisLimiter_processStereoSimple(BMHysteresisLimiter *This,
											 const float *inputL, const float *inputR,
											 float *outputL, float *outputR,
											 size_t numSamples){
	assert(This->AAFilter.numChannels == 2);
		   
	// create some aliases for code readability
	float *limitedL = outputL;
	float *limitedR = outputR;
	
	// apply asymptotic limit
	BMAsymptoticLimit(inputL, limitedL, This->sampleRate, This->sag, numSamples);
	BMAsymptoticLimit(inputR, limitedR, This->sampleRate, This->sag, numSamples);
	
	// Antialiasing filter
	BMMultiLevelBiquad_processBufferStereo(&This->AAFilter,
										   limitedL, limitedR,
										   limitedL, limitedR,
										   numSamples);
	
	for(size_t i=0; i<numSamples; i++){
		// output
		simd_float2 in = simd_make_float2(limitedL[i], limitedR[i]);
		simd_float2 out = in * This->cs;
		// update charge
		This->cs -= This->s * simd_abs(out);
		This->cs += This->sR * (1.0f - This->cs);
		// scale to compensate for gain loss
		out *= This->oneOverR;
		// output
		outputL[i] = out.x;
		outputR[i] = out.y;
	}
}



void BMHysteresisLimiter_updateSettings(BMHysteresisLimiter *This){
    This->s = 1.0f / (This->sampleRate * This->sag);
    This->oneOverR = 1.0 / This->R;
    This->sR = This->s * This->R;
    This->halfSR = This->sR * 0.5f;
}



void BMHysteresisLimiter_setPowerLimit(BMHysteresisLimiter *This, float limitDb){
    assert(limitDb <= 0.0f);
    This->R = BM_DB_TO_GAIN(limitDb);
    BMHysteresisLimiter_updateSettings(This);
}





void BMHysteresisLimiter_setSag(BMHysteresisLimiter *This, float sag){
    assert(sag > 0.0);
    This->sag = sag;
    BMHysteresisLimiter_updateSettings(This);
}





void BMHysteresisLimiter_setAAFilterFC(BMHysteresisLimiter *This, float fc){
    // safety check: don't let fc exceed 90% of the Nyquist frequency
    fc = BM_MIN(This->sampleRate * 0.5f * 0.9f, fc);
    // set the AA filters
	for(size_t i=0; i<This->AAFilter.numLevels; i++){
		BMMultiLevelBiquad_setLowPass6db(&This->AAFilter, fc, i);
	}
}




void BMHysteresisLimiter_init(BMHysteresisLimiter *This, float sampleRate, size_t numChannels){
	assert(numChannels == 1 || numChannels == 2 || numChannels == 4);
    
    This->sampleRate = sampleRate;
	
	// init the AA filter
	size_t numLevels = 1;
	if(numChannels == 1)
		BMMultiLevelBiquad_init(&This->AAFilter, numLevels, sampleRate, false, false, false);
	if(numChannels == 2)
		BMMultiLevelBiquad_init(&This->AAFilter, numLevels, sampleRate, true, false, false);
	if(numChannels == 4)
		BMMultiLevelBiquad_init4(&This->AAFilter, numLevels, sampleRate, false);
	BMHysteresisLimiter_setAAFilterFC(This,BM_HYSTERESISLIMITER_AA_FILTER_FC);

	BMHysteresisLimiter_setPowerLimit(This, BM_HYSTERESISLIMITER_DEFAULT_POWER_LIMIT);
    BMHysteresisLimiter_setSag(This, BM_HYSTERESISLIMITER_DEFAULT_SAG);
	
	This->c = 0.0f;
	This->cs = 0.0f;
}





void BMHysteresisLimiter_free(BMHysteresisLimiter *This){
	BMMultiLevelBiquad_destroy(&This->AAFilter);
}
