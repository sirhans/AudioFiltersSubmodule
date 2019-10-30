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
		float oPos = inputPos[i] * This->cL;
		// update charge
		This->cL = This->cL - oPos + This->halfR*(1.0f - This->cL);
		// negative output
        float oNeg = inputNeg[i] * This->cL;
        // update charge
		This->cL = This->cL + oNeg + This->halfR*(1.0f - This->cL);
								  
        outputPos[i] = oPos;
        outputNeg[i] = oNeg;
    }
}


float chargeRate(float timeTo90PercentChargeSeconds, float sampleRate){
	// Mathematica: {{R -> -((Log[-0.9 + m])/t)}}
	float t = timeTo90PercentChargeSeconds * sampleRate;
	return -logf(1.0f - 0.90f) / t;
}


void BMHysteresisLimiter_init(BMHysteresisLimiter *This,
							  float chargeTimeSeconds,
							  float sampleRate){
	This->R = chargeRate(chargeTimeSeconds, sampleRate);
	This->halfR = This->R * 0.5f;
	This->cL = This->cR = 0.0f;
}
