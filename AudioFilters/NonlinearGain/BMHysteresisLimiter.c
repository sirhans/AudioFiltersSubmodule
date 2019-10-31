//
//  BMHysteresisLimiter.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 29/10/19.
//  Anyone may use this file without restrictions
//

#include "BMHysteresisLimiter.h"
#include <math.h>
#include "BMAsymptoticLimiter.h"

//void BMHysteresisLimiter_processMono(BMHysteresisLimiter *This, const float *input, float* output, size_t numSamples){
//    for(size_t i=0; i<numSamples; i++){
//        float o = input[i]*This->cL;
//        This->cL += This->r - fabsf(o);
//        output[i] = o;
//    }
//
//    // output = input[i]*(This->cL - output + r);
//    // o = i*(c + r - o);
//    // o = i*(c+r) - i*o;
//    // o (1+i) = i*(c+r);
//    // o = (c+r) * i/(1+i)
//}
//
//void BMHysteresisLimiter_processMonoRectified(BMHysteresisLimiter *This,
//                                      const float *inputPos, const float *inputNeg,
//                                      float* outputPos, float* outputNeg,
//                                      size_t numSamples){
//    for(size_t i=0; i<numSamples; i++){
//        float oPos = inputPos[i]*This->cL;
//        float oNeg = inputNeg[i]*This->cL;
//        This->cL += This->r - oPos + oNeg;
//        outputPos[i] = oPos;
//        outputNeg[i] = oNeg;
//    }
//
//    // output = input[i]*(This->cL - output + r);
//    // oPos = iPos*(c + r - oPos + oNeg);
//    // oNeg = iNeg*(c + r - oPos + oNeg);
//    // oPos + iPos*oPos = iPos*(c + r + oNeg);
//    // oPos = (iPos / (1 + iPos)) * (c + r + oNeg);
//    // oNeg = (iNeg / (1 - iNeg)) * (c + r + oPos);
//    //
//    //   * iPA = (iPos / (1 + iPos))  && * iNA = (iNeg / (1 - iNeg))
//    // oPos = iPA * (c + r + iNA * (c + r + oPos));
//    // oPos - iPA*iNA*oPos = iPA *(c + r + iNA(c+r));
//    // oPos * (1-iPA*iNA) = iPA * (c+r)*(1+iNA);
//    // oPos = iPA * (c+r) * (1+iNA) / (1 - iPA*iNA);
//    //
//    // oNeg = iNA * (c + r + iPA * (c + r + oNeg))
//    // oNeg - iNA*iPA*oNeg = iNA * ( c + r + iPA*(c+r))
//    // oNeg = iNA * (c+r) * (1+iPA) / (1 - iPA*iNA);
//}


void BMHysteresisLimiter_processMonoRectifiedSimple(BMHysteresisLimiter *This,
                                      const float *inputPos, const float *inputNeg,
                                      float* outputPos, float* outputNeg,
                                      size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
		// positive output
		float oPos = inputPos[i] * This->c;
		// update charge
		This->c = This->c - oPos + This->halfR*(1.0f - This->c);
		// negative output
        float oNeg = inputNeg[i] * This->c;
        // update charge
		This->c = This->c + oNeg + This->halfR*(1.0f - This->c);
		// output
        outputPos[i] = oPos;
        outputNeg[i] = oNeg;
    }
}



void BMHysteresisLimiter_processStereoRectifiedSimple(BMHysteresisLimiter *This,
													  const float *inputPosL, const float *inputPosR,
													  const float *inputNegL, const float *inputNegR,
													  float *outputPosL, float *outputPosR,
													  float *outputNegL, float *outputNegR,
													  size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
		// positive output
		simd_float2 iPos = simd_make_float2(inputPosL[i], inputPosR[i]);
		simd_float2 oPos = iPos * This->cs;
		// update charge
		This->cs = This->cs - oPos + This->halfR * (1.0f - This->cs);
		// negative output
		simd_float2 iNeg = simd_make_float2(inputNegL[i], inputNegR[i]);
		simd_float2 oNeg = iNeg * This->cs;
        // update charge
		This->cs = This->cs + oNeg + This->halfR * (1.0f - This->cs);
		// output
        outputPosL[i] = oPos.x;
		outputPosR[i] = oPos.y;
        outputNegL[i] = oNeg.x;
		outputNegR[i] = oNeg.y;
    }
}


void BMHysteresisLimiter_setChargeTime(BMHysteresisLimiter *This, float timeInSeconds){
	float t = timeInSeconds * This->sampleRate;
	
	// Mathematica: {{R -> -((Log[-0.9 + m])/t)}}
	This->R = -logf(1.0f - 0.90f) / t;
	This->halfR = This->R * 0.5f;
}




void BMHysteresisLimiter_init(BMHysteresisLimiter *This,
							  float chargeTimeSeconds,
							  float sampleRate){
	
	This->sampleRate = sampleRate;
	BMHysteresisLimiter_setChargeTime(This, chargeTimeSeconds);
	
	This->c = 0.0f;
	This->cs = 0.0f;
}
