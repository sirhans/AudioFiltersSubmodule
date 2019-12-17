//
//  TestReverb.h
//  BMAudioFilters
//
//  Created by Nguyen Minh Tien on 4/26/18.
//  Anyone may use this file without restrictions of any kind
//

#ifndef TestReverb_h
#define TestReverb_h
#include <stdio.h>

#define Rev_DT_Min 7
#define Rev_DT_Max 100

typedef struct TestReverb {
    float** delayLines;
    size_t* readIdx;
    size_t* writeIdx;
    size_t numOfDelay;
    size_t* delayTimes;
    float* dlOut;
    float* temp;
    float* temp2;
} TestReverb;

void TestReverb_init(TestReverb *This,size_t numOfDelay,float sampleRate);
void TestReverb_process(TestReverb *This,float* input,float* output, size_t numSamples);
void TestReverb_impulseResponse(TestReverb *This,size_t frameCount);

#endif /* TestReverb_h */
