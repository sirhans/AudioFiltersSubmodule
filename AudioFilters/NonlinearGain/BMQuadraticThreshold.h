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


/*!
 *BMQuadraticThreshold_initLower
 *
 * @param threshold   the output will not go below this value
 * @prarm width       the output will be curved between threshold - width and threshold + width
 */
void BMQuadraticThreshold_initLower(BMQuadraticThreshold *This, float threshold, float width);


/*!
 *BMQuadraticThreshold_initUpper
 *
 * @param threshold   the output will not go above this value
 * @prarm width       the output will be curved between threshold - width and threshold + width
 */
void BMQuadraticThreshold_initUpper(BMQuadraticThreshold *This, float threshold, float width);



void BMQuadraticThreshold_lowerBuffer(BMQuadraticThreshold *This,
                                   const float* input,
                                   float* output,
                                   size_t numFrames);



void BMQuadraticThreshold_upperBuffer(BMQuadraticThreshold *This,
                                    const float* input,
                                    float* output,
                                    size_t numFrames);

#endif /* BMQuadraticThreshold_h */
