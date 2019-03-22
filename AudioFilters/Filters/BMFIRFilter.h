//
//  BMFIRFilter.h
//  BMAudioFilters
//
//  Created by Hans on 18/9/17.
//  Copyright © 2017 Hans. All rights reserved.
//


#ifdef __cplusplus
extern "C" {
#endif
    
    
#ifndef BMFIRFilter_h
#define BMFIRFilter_h
    
#include <stdio.h>
#include "Constants.h"
#include "TPCircularBuffer.h"
#include <Accelerate/Accelerate.h>
    
    typedef struct BMFIRFilter {
        TPCircularBuffer inputBuffer;
        float* convolverTempBuffer;
        float* coefficients;
        size_t length;
        bool symmetricFilterKernel;
    } BMFIRFilter;
    
    
    /*
     * @param coefficients   filter kernel
     * @param length         length of coefficients
     */
    void BMFIRFilter_init(BMFIRFilter* This,
                            float* coefficients,
                            size_t length);
    
    
    /*
     * @param input  length must be at least numSamples
     * @param output length must be at least numSamples
     */
    void BMFIRFilter_process(BMFIRFilter* This, float* input, float* output, size_t numSamples);
    
    
    void BMFIRFilter_free(BMFIRFilter* This);
    
    
    /*
     * The calling function must ensure that the length of IR is at least
     * This->IRLength.
     */
    void BMFIRFilter_impulseResponse(BMFIRFilter *This, float* IR);
    

    /*
     * returns true if kernel is symmetric around its centre
     */
    bool BMFIRFilter_isSymmetric(float* kernel, size_t length);
    

#endif /* BMFIRFilter_h */
    
    
#ifdef __cplusplus
}
#endif
