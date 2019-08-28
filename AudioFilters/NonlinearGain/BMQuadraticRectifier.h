//
//  BMQuadraticRectifier.h
//  AudioFiltersXcodeProject
//
//  Splits a signed signal into two signals, pos and neg such that
//  x >=0 for all x in pos and x<=0 for all x in neg.
//
//  It does this with a smooth transition at zero, meaning that some values from
//  the negative side of the input are pushed up into pos and vice versa for neg
//
//  It also has the property that pos + neg == input.
//
//  Created by hans anderson on 7/2/19.
//  Anyone may use this file without restrictions
//

#ifndef BMQuadraticRectifier_h
#define BMQuadraticRectifier_h

#include <stdio.h>
//#include <Accelerate/Accelerate.h>
#include "BMQuadraticThreshold.h"


typedef struct BMQuadraticRectifier {
    BMQuadraticThreshold qtPos, qtNeg;
} BMQuadraticRectifier;


/*!
 *BMQuadraticRectifier_init
 */
void BMQuadraticRectifier_init(BMQuadraticRectifier* This, float kneeWidth);


/*!
 *BMQuadraticRectifier_processBuffer
 */
void BMQuadraticRectifier_processBuffer(BMQuadraticRectifier* This,
                                        const float* input,
                                        float* outputPos, float* outputNeg,
                                        size_t numSamples);
/*!
 *BMQuadraticRectifier_processBufferStereo
 */
void BMQuadraticRectifier_processBufferStereo(BMQuadraticRectifier* This,
                                              const float* inputL, const float* inputR,
                                              float* outputPosL, float* outputNegL,
                                              float* outputPosR, float* outputNegR,
                                              size_t numSamples);

#endif /* BMQuadraticRectifier_h */
