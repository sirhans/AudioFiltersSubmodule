//
//  BMDCBlocker.c
//  Saturator
//
//  Created by TienNM on 11/27/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#include "BMDCBlocker.h"

void BMDCBlocker_init(BMDCBlocker *This){
    This->filterData[0] = 0.;
    This->filterData[1] = 0.;
    This->r = 0.995;
}

void BMDCBlocker_process(BMDCBlocker *This,float* input,float* output,size_t processSample){
    for(int i=0;i<processSample;i++){
        output[i] = input[i] - This->filterData[0] + This->r*This->filterData[1];
        This->filterData[0] = input[i];
        This->filterData[1] = output[i];
    }
    
}
