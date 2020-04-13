//
//  BMAllpassNestedFilter.h
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 4/10/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMAllpassNestedFilter_h
#define BMAllpassNestedFilter_h

#include <stdio.h>
#include "TPCircularBuffer.h"

typedef struct BMAllpassFilterData{
    TPCircularBuffer circularBufferL;
    TPCircularBuffer circularBufferR;
    size_t delaySamples;
    float decay1,decay2;
    float* tempL;
    float* tempR;
} BMAllpassFilterData;

typedef struct BMAllpassNestedFilter{
    size_t numLevel;
    BMAllpassFilterData* nestedData;
} BMAllpassNestedFilter;


void BMAllpassNestedFilter_init(BMAllpassNestedFilter* This,size_t delaySamples,float dc1, float dc2,size_t nestedLevel);
void BMAllpassNestedFilter_destroy(BMAllpassNestedFilter* This);

void BMAllpassNestedFilter_processBufferStereo(BMAllpassNestedFilter* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t level,size_t numSamples);

#endif /* BMAllpassNestedFilter_h */
