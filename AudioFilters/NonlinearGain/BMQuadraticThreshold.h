//
//  BMQuadraticThreshold.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/2/19.
//  Anyone may use this file without restrictions.
//

#ifndef BMQuadraticThreshold_h
#define BMQuadraticThreshold_h

#include <Accelerate/Accelerate.h>

typedef struct BMQuadraticThreshold {
    float lPlusW, lMinusW;
    float coefficients [3];
    bool isUpper;
} BMQuadraticThreshold;


void BMQuadraticThreshold_initLower(BMQuadraticThreshold* This, float threshold, float width);
void BMQuadraticThreshold_initUpper(BMQuadraticThreshold* This, float threshold, float width);


/*
 * Mathematica code:
 *
 * qThreshold[x_, l_, w_] :=
 *       If[x < l - w, l, If[x > l + w, x, (- l x + (l + w + x)^2/4)/w]]
 */
void quadraticThreshold_lowerBuffer(BMQuadraticThreshold* This,
                                   const float* input,
                                   float* output,
                                   size_t numFrames);



void quadraticThreshold_upperBuffer(BMQuadraticThreshold* This,
                                    const float* input,
                                    float* output,
                                    size_t numFrames);

#endif /* BMQuadraticThreshold_h */
