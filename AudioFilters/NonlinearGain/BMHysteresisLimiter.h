//
//  BMHysteresis.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 29/10/19.
//  Anyone may use this file without restrictions
//

#ifndef BMHysteresis_h
#define BMHysteresis_h

#include <stdio.h>

typedef struct BMHysteresis {
    float cL, cR, r;
} BMHysteresis;

void BMHysteresis_processMono(BMHysteresis *This, const float *input, float* output, size_t numSamples);

#endif /* BMHysteresis_h */
