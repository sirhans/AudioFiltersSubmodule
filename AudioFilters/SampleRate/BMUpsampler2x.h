//
//  BMUpsampler2x.h
//  BMAudioFilters
//
//  This file defines a single stage of 2x upsampling. It does not contain
//  the antialiasing filter coefficients because it is not intended to be
//  used as a standalone upsampling function.
//
//  Please use the fucntions in this file via the intervae in BMUpsampler
//
//  Created by Hans on 11/9/17.
//  Anyone may use this code without restrictions
//

#ifdef __cplusplus
extern "C" {
#endif
    
#ifndef BMUpsampler2x_h
#define BMUpsampler2x_h
    
#include "Constants.h"
#include "TPCircularBuffer.h"
#include <Accelerate/Accelerate.h>
    
    typedef struct BMUpsampler2x {
        TPCircularBuffer inputBuffer;
        float* convolverTempBuffer;
        float* outputBuffer0;
        float* outputBuffer1;
        float* coefficients0;
        float* coefficients1;
        DSPSplitComplex outputBuffers;
        size_t maxLength, cf0Length, cf1Length, IRLength;
    } BMUpsampler2x;
    
    
    /*
     * @param coefficients0   first half of the polyphase filter coefficients
     * @param coeffieients1   second half of the polyphase filter coefficients
     * @param cf0Length       length of coefficients0
     * @param cf1Length       length of coefficients1
     */
    void BMUpsampler2x_init(BMUpsampler2x* This,
                            float* coefficients0,
                            float* coefficients1,
                            size_t cf0Length,
                            size_t cf1Length);
    
    
    /*
     * @param input  length must be at least numSamplesIn
     * @param output length must be at least 2*numSamplesIn
     */
    void BMUpsampler2x_process(BMUpsampler2x* This, float* input, float* output, size_t numSamplesIn);
    
    
    void BMUpsampler2x_free(BMUpsampler2x* This);
    
    
    /*
     * The calling function must ensure that the length of IR is at least
     * This->IRLength.
     */
    void BMUpsampler2x_impulseResponse(BMUpsampler2x *This, float* IR);
    
    
#endif /* BMUpsampler2x_h */
    
    
#ifdef __cplusplus
}
#endif
