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

void BMDCBlocker_init(BMDCBlocker* this){
    this->filterData[0] = 0.;
    this->filterData[1] = 0.;
    this->r = 0.995;
}

void BMDCBlocker_process(BMDCBlocker* this,float* input,float* output,size_t processSample){
    for(int i=0;i<processSample;i++){
        output[i] = input[i] - this->filterData[0] + this->r*this->filterData[1];
        this->filterData[0] = input[i];
        this->filterData[1] = output[i];
    }
    
}
