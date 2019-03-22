//
//  BMDiffusion.h
//  AUStereoModulator
//
//  Created by Nguyen Minh Tien on 3/29/18.
//  Copyright © 2018 Nguyen Minh Tien. All rights reserved.
//

#ifndef BMDiffusion_h
#define BMDiffusion_h

#include <stdio.h>
#include <stdbool.h>

typedef struct{
    float* delayLine;
    float coefficient;
    size_t readIndex;
    size_t writeIndex;
    size_t delayEndIndex;
} BMAllPassFilter;

typedef struct{
    float* delayLine;
    float coefficient;
    size_t readIndex;
    size_t writeIndex;
    size_t delayEndIndex;
    BMAllPassFilter nestedAP;
    bool preNesting;
} BMNestedAllPass2;

typedef struct{
    float* delayLine;
    float coefficient;
    size_t readIndex;
    size_t writeIndex;
    size_t delayEndIndex;
    BMNestedAllPass2 nestedAP2;
    bool preNesting;
} BMNestedAllPass3;

typedef struct{
    int dt1;
    int dt2;
    int dt3;
    BMNestedAllPass3 nestedAPFilterL;
    BMNestedAllPass3 nestedAPFilterR;
} BMDiffusion;

void BMDiffusion_init(BMDiffusion* this,float totalDT);
void BMDiffusion_destroy(BMDiffusion* this);
void BMDiffusion_process(BMDiffusion* this,float* inDataL,float* inDataR,float* outDataL,float* outDataR,size_t frameCount);

#endif /* BMDiffusion_h */
