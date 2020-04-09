//
//  BMHysteresisLimiter2.c
//  GainStageVintageCleanAU3
//
//  Created by hans anderson on 4/6/20.
//  This file is public domain with no restrictions
//

#include "BMHysteresisLimiter2.h"
#include "BMAsymptoticLimiter.h"
#include <math.h>
#include <assert.h>
#include "Constants.h"

#define BM_HYSTERESISLIMITER_DEFAULT_POWER_LIMIT -45.0f
#define BM_HYSTERESISLIMITER_DEFAULT_SAG 1.0f / (4000.0f)




void BMHysteresisLimiter2_processMonoRectified(BMHysteresisLimiter2 *This,
                                      const float *inputPos, const float *inputNeg,
                                      float* outputPos, float* outputNeg,
                                      size_t numSamples){
	assert(This->AAFilter1.numChannels == 2 && This->AAFilter2.numChannels == 2 &&
		   numSamples % 2 == 0);
	
	// create two alias pointers for code readability
	float* limitedPos = outputPos;
	float* limitedNeg = outputNeg;
	
	// antialiasing filter
	BMMultiLevelBiquad_processBufferStereo(&This->AAFilter1,
										   inputPos, inputNeg,
										   limitedPos, limitedNeg,
										   numSamples);
	
	// apply asymptotic limit
	BMAsymptoticLimitRectified(limitedPos, limitedNeg,
							   limitedPos, limitedNeg,
                               This->sampleRate,
                               This->sag,
							   numSamples);
	
	// antialiasing filter
	BMMultiLevelBiquad_processBufferStereo(&This->AAFilter2,
										   limitedPos, limitedNeg,
										   limitedPos, limitedNeg,
										   numSamples);
	
    for(size_t i=0; i<numSamples; i++){
		// we alternate the order in which the positive and negative signals
		// consume charge in order to avoid biasing the signal by always consuming
		// from one side before the other
		
		// positive, then negative
		//
		// positive output
		float charge = This->halfSR * (1.0f - This->c);
		float oPos = limitedPos[i] * (This->c + charge);
		// update charge
		This->c = This->c - (oPos * This->s) + charge;
		//
		// negative output
		charge = This->halfSR * (1.0f - This->c);
        float oNeg = limitedNeg[i] * (This->c + charge);
        // update charge
		This->c = This->c + (oNeg * This->s) + charge;
		// output
        outputPos[i] = oPos;
        outputNeg[i] = oNeg;
		
		
		i++;
		// Negative, then positive
		//
		// negative output
		charge = This->halfSR * (1.0f - This->c);
        oNeg = limitedNeg[i] * (This->c + charge);
        // update charge
		This->c = This->c + (oNeg * This->s) + charge;
		//
		// positive output
		charge = This->halfSR * (1.0f - This->c);
		oPos = limitedPos[i] * (This->c + charge);
		// update charge
		This->c = This->c - (oPos * This->s) + charge;
		// output
        outputPos[i] = oPos;
        outputNeg[i] = oNeg;
    }
	
	// scale to compensate for gain loss
	vDSP_vsmul(outputPos,1,&This->oneOverR,outputPos,1,numSamples);
	vDSP_vsmul(outputNeg,1,&This->oneOverR,outputNeg,1,numSamples);
}




void BMHysteresisLimiter2_processMonoSigned(BMHysteresisLimiter2 *This,
											const float *input,
											float* output,
											size_t numSamples){
	assert(This->AAFilter1.numChannels == 1 && This->AAFilter2.numChannels == 1);
	
	// create two alias pointers for code readability
	float* limited = output;
	
	// antialiasing filter
	BMMultiLevelBiquad_processBufferMono(&This->AAFilter1,
										 input,
										 limited,
										 numSamples);
	
	// apply asymptotic limit
	BMAsymptoticLimit(limited, limited, This->sampleRate, This->sag, numSamples);
	
	// antialiasing filter
	BMMultiLevelBiquad_processBufferMono(&This->AAFilter2,
										   limited,
										   limited,
										   numSamples);
	
    for(size_t i=0; i<numSamples; i++){
		float charge = This->halfSR * (1.0f - This->c);
		float o = limited[i] * (This->c + charge);
		// update charge
		This->c = This->c - (fabsf(o) * This->s) + charge;
		// output
        output[i] = o;
    }
	
	// scale to compensate for gain loss
	vDSP_vsmul(output,1,&This->oneOverR,output,1,numSamples);
}




void BMHysteresisLimiter2_processMonoClassA(BMHysteresisLimiter2 *This,
                                      const float *inputPos,
                                      float* outputPos,
                                      size_t numSamples){
	assert(This->AAFilter1.numChannels == 1 && This->AAFilter2.numChannels == 1 &&
		   numSamples % 2 == 0);
	
	// create an alias pointer for code readability
	float* buffer = outputPos;
	
	// antialiasing filter
	BMMultiLevelBiquad_processBufferMono(&This->AAFilter1,
										   inputPos,
										   buffer,
										   numSamples);
	
	// apply asymptotic limit
	BMAsymptoticLimitPositive(buffer,
							  buffer,
							  This->sampleRate,
							  This->sag,
							  numSamples);
	
	// antialiasing filter
	BMMultiLevelBiquad_processBufferMono(&This->AAFilter2,
										   buffer,
										   buffer,
										   numSamples);
	
    for(size_t i=0; i<numSamples; i++){
		// find output value
		float charge = This->halfSR * (1.0f - This->c);
		float oPos = buffer[i] * (This->c + charge);
		// update charge
		This->c = This->c - (oPos * This->s) + charge;
		// output
        buffer[i] = oPos;
    }
	
	// scale to compensate for gain loss
	vDSP_vsmul(buffer,1,&This->oneOverR,outputPos,1,numSamples);
}





void BMHysteresisLimiter2_processStereoRectified(BMHysteresisLimiter2 *This,
													  const float *inputPosL,
													  const float *inputPosR,
													  const float *inputNegL,
													  const float *inputNegR,
													  float *outputPosL, float *outputPosR,
													  float *outputNegL, float *outputNegR,
													  size_t numSamples){
	assert(This->AAFilter1.numChannels == 4 && This->AAFilter2.numChannels == 4 &&
		   numSamples % 2 == 0);
	
	// create some alias pointers for code readability
	float* limitedPosL = outputPosL;
	float* limitedNegL = outputNegL;
	float* limitedPosR = outputPosR;
	float* limitedNegR = outputNegR;
	
	// Antialiasing filter 1
	BMMultiLevelBiquad_processBuffer4(&This->AAFilter1,
										   inputPosL, inputNegL,
										   inputPosR, inputNegR,
										   limitedPosL, limitedNegL,
										   limitedPosR, limitedNegR,
										   numSamples);
	
	// apply asymptotic limit
	BMAsymptoticLimitRectified(limitedPosL, limitedNegL,
							   limitedPosL, limitedNegL,
                               This->sampleRate,
                               This->sag,
							   numSamples);
	BMAsymptoticLimitRectified(limitedPosR, limitedNegR,
							   limitedPosR, limitedNegR,
                               This->sampleRate,
                               This->sag,
							   numSamples);
	
	// Antialiasing filter 2
	BMMultiLevelBiquad_processBuffer4(&This->AAFilter2,
										   limitedPosL, limitedNegL,
										   limitedPosR, limitedNegR,
										   limitedPosL, limitedNegL,
										   limitedPosR, limitedNegR,
										   numSamples);
	
	
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
		This->cs = This->cs - (This->s * oPos) + charge;
		//
		// negative output
		charge = This->halfSR * (1.0f - This->cs);
		simd_float2 iNeg = simd_make_float2(limitedNegL[i], limitedNegR[i]);
		simd_float2 oNeg = iNeg * (This->cs + charge);
        // update charge
		This->cs = This->cs + (This->s * oNeg) + charge;
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
		This->cs = This->cs + (This->s * oNeg) + charge;
		//
		// positive output
		charge = This->halfSR * (1.0f - This->cs);
		iPos = simd_make_float2(limitedPosL[i], limitedPosR[i]);
		oPos = iPos * (This->cs + charge);
		// update charge
		This->cs = This->cs - (This->s * oPos) + charge;
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





void BMHysteresisLimiter2_processStereoClassA(BMHysteresisLimiter2 *This,
											 const float *inputPosL,
											 const float *inputPosR,
											 float *outputPosL,
											 float *outputPosR,
											 size_t numSamples){
	assert(This->AAFilter1.numChannels == 2 && This->AAFilter2.numChannels == 2 &&
		   numSamples % 2 == 0);
	
	// create some alias pointers for code readability
	float* limitedPosL = outputPosL;
	float* limitedPosR = outputPosR;
	
	// Antialiasing filter
	BMMultiLevelBiquad_processBufferStereo(&This->AAFilter1,
										   inputPosL, inputPosR,
										   limitedPosL, limitedPosR,
										   numSamples);
	
	// apply asymptotic limit
	BMAsymptoticLimitPositive(limitedPosL,
							  limitedPosL,
							  This->sampleRate,
							  This->sag,
							  numSamples);
	BMAsymptoticLimitPositive(limitedPosR,
							  limitedPosR,
							  This->sampleRate,
							  This->sag,
							  numSamples);
	
	// Antialiasing filter
	BMMultiLevelBiquad_processBufferStereo(&This->AAFilter2,
										   limitedPosL, limitedPosR,
										   limitedPosL, limitedPosR,
										   numSamples);
	
	
    for(size_t i=0; i<numSamples; i++){
		// find output value
		simd_float2 charge = This->halfSR * (1.0f - This->cs);
		simd_float2 iPos = simd_make_float2(limitedPosL[i], limitedPosR[i]);
		simd_float2 oPos = iPos * (This->cs + charge);
		// update charge
		This->cs = This->cs - (This->s * oPos) + charge;
        outputPosL[i] = oPos.x;
		outputPosR[i] = oPos.y;
    }
	
	// scale to compensate for gain loss
	vDSP_vsmul(outputPosL,1,&This->oneOverR,outputPosL,1,numSamples);
	vDSP_vsmul(outputPosR,1,&This->oneOverR,outputPosR,1,numSamples);
}





void BMHysteresisLimiter2_processStereoSimple(BMHysteresisLimiter2 *This,
											 const float *inputL, const float *inputR,
											 float *outputL, float *outputR,
											 size_t numSamples){
	assert(This->AAFilter1.numChannels == 2 && This->AAFilter2.numChannels == 2);
		   
	// create some aliases for code readability
	float *limitedL = outputL;
	float *limitedR = outputR;
	
	// Antialiasing filter
	BMMultiLevelBiquad_processBufferStereo(&This->AAFilter1,
										   inputL, inputR,
										   limitedL, limitedR,
										   numSamples);
	
	// apply asymptotic limit
	BMAsymptoticLimit(limitedL, limitedL, This->sampleRate, This->sag, numSamples);
	BMAsymptoticLimit(inputL, limitedR, This->sampleRate, This->sag, numSamples);
	
	// Antialiasing filter
	BMMultiLevelBiquad_processBufferStereo(&This->AAFilter2,
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



void BMHysteresisLimiter2_updateSettings(BMHysteresisLimiter2 *This){
    This->s = 1.0f / (This->sampleRate * This->sag);
    This->oneOverR = 1.0 / This->R;
    This->sR = This->s * This->R;
    This->halfSR = This->sR * 0.5f;
}



void BMHysteresisLimiter2_setPowerLimit(BMHysteresisLimiter2 *This, float limitDb){
    assert(limitDb <= 0.0f);
    This->R = BM_DB_TO_GAIN(limitDb);
    BMHysteresisLimiter2_updateSettings(This);
}





void BMHysteresisLimiter2_setSag(BMHysteresisLimiter2 *This, float sag){
    assert(sag > 0.0);
    This->sag = sag;
    BMHysteresisLimiter2_updateSettings(This);
}





void BMHysteresisLimiter2_setAAFilterFC(BMHysteresisLimiter2 *This, float fc){
    // safety check: don't let fc exceed 90% of the Nyquist frequency
    fc = BM_MIN(This->sampleRate * 0.5f * 0.9f, fc);
    // set the AA filters
	for(size_t i=0; i<This->AAFilter1.numLevels; i++){
		BMMultiLevelBiquad_setLowPass6db(&This->AAFilter1, fc, i);
		BMMultiLevelBiquad_setLowPass6db(&This->AAFilter2, fc, i);
	}
}




void BMHysteresisLimiter2_init(BMHysteresisLimiter2 *This,
							  float sampleRate,
							  size_t aaFilterNumLevels,
							  float aaFilterFc,
							  float hpFilterFc,
							  size_t numChannels){
	assert(numChannels == 1 || numChannels == 2 || numChannels == 4);
    
    This->sampleRate = sampleRate;
	
	// init the AA filter
    assert(aaFilterFc < sampleRate * 0.5f);
	if(numChannels == 1){
		BMMultiLevelBiquad_init(&This->AAFilter1, aaFilterNumLevels+1, sampleRate, false, false, false);
		BMMultiLevelBiquad_init(&This->AAFilter2, aaFilterNumLevels+1, sampleRate, false, false, false);
	}
	if(numChannels == 2){
		BMMultiLevelBiquad_init(&This->AAFilter1, aaFilterNumLevels+1, sampleRate, true, false, false);
		BMMultiLevelBiquad_init(&This->AAFilter2, aaFilterNumLevels+1, sampleRate, true, false, false);
	}
	if(numChannels == 4){
		BMMultiLevelBiquad_init4(&This->AAFilter1, aaFilterNumLevels+1, sampleRate, false);
		BMMultiLevelBiquad_init4(&This->AAFilter2, aaFilterNumLevels+1, sampleRate, false);
	}
	BMMultiLevelBiquad_setHighPass6db(&This->AAFilter1, hpFilterFc, aaFilterNumLevels);
	BMMultiLevelBiquad_setHighPass6db(&This->AAFilter2, hpFilterFc, aaFilterNumLevels);
	BMHysteresisLimiter2_setAAFilterFC(This,aaFilterFc);

	BMHysteresisLimiter2_setPowerLimit(This, BM_HYSTERESISLIMITER_DEFAULT_POWER_LIMIT);
    BMHysteresisLimiter2_setSag(This, BM_HYSTERESISLIMITER_DEFAULT_SAG);
	
	This->c = 0.0f;
	This->cs = 0.0f;
}





void BMHysteresisLimiter2_free(BMHysteresisLimiter2 *This){
	BMMultiLevelBiquad_free(&This->AAFilter1);
	BMMultiLevelBiquad_free(&This->AAFilter2);
}
