//
//  BMHysteresisLimiter.h
//  AudioFiltersXcodeProject
//
//  This simulates the amplifier sag from the power supply output being drained
//  by the tubes. In the context of this model it does not matter whether the
//  sag is caused by a transformer or by a capacitor or some other phenomenon.
//
//  The code generically models any effect where high level output from an amp
//  temporarily causes a reduction its output capacity, but after allowed to
//  rest it recovers its capacity again. If multiple sources of hysteresis are
//  required, for example, both capacitor and transformer, preamp and power amp,
//  then you should run several of these limiters in series, each with
//  appropriate charge rates.
//
//  Note that the correct mathematical model for this requires that the input
//  be processed through an asymptotic limiter beforehand:
//
//    limitedOut = input / (1.0 + abs(input)
//
//  This class does not include the limiter because in the case where you want
//  to simulate multiple stages of hysteresis it is not strictly necessary to
//  re-apply the asymptotic limiter between stages as long as the input to each
//  stage is strictly within (-1,1).
//
//  Created by Hans on 29/10/19.
//  Anyone may use this file without restrictions
//

#ifndef BMHysteresisLimiter_h
#define BMHysteresisLimiter_h

#include <stdio.h>

typedef struct BMHysteresisLimiter {
    float cL, cR, R, halfR;
} BMHysteresisLimiter;


/*!
 *BMHysteresisLimiter_init
 *
 * @param This pointer to a BMHysteresisLimiter struct
 * @param chargeTimeSeconds seconds to go from 0 to 90% charge with the amp at rest
 * @param sampleRate audio sample rate
 *
 * @notes This function does not allocate dynamic memory on the heap
 */
void BMHysteresisLimiter_init(BMHysteresisLimiter *This,
							  float chargeTimeSeconds,
							  float sampleRate);


/*!
 *BMHysteresisLimiter_processMonoRectifiedSimple
 *
 * @notes this is a simpler hysteresis model in the sense that it does not do zero-delay-feedback modeling of charge consumption. Rather the charge consumed at the current sample index removes charge from the capacitor for the next sample index.
 *
 * @param This pointer to an initialised struct
 * @param inputPos positive side rectified input. MUST BE IN [0,1)
 * @param inputNeg negative side rectified input. MUST BE IN (-1,0]
 * @param outputPos output array with length numSamples
 * @param outputNeg output array with length numSamples
 * @param numSamples length of inputs and outputs
 */
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

#endif /* BMHysteresisLimiter_h */
