//
//  BMDCBlocker.h
//  Saturator
//
//  Created by TienNM on 11/27/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMDCBlocker_h
#define BMDCBlocker_h

#include <stdio.h>

typedef struct BMDCBlocker {
    float filterData[2];         // [0] = x(n-1) //[1] = y[n-1]
    float r;
} BMDCBlocker;

void BMDCBlocker_init(BMDCBlocker *This);
void BMDCBlocker_process(BMDCBlocker *This,float* input,float* output,size_t processSample);

#endif /* BMDCBlocker_h */
