//
//  BMUpsampler2x.c
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
    
#include <stdlib.h>
#include <assert.h>
#include "BMUpsampler2x.h"
#include "BMSymmetricConv.h"
#include "BMFIRFilter.h"

    
    
    
    /*
     * @param coefficients0   first half of the polyphase filter coefficients
     * @param coeffieients1   second half of the polyphase filter coefficients
     * @param length          length of coefficients
     */
    void BMUpsampler2x_init(BMUpsampler2x* This,
                            float* coefficients0,
                            float* coefficients1,
                            size_t cf0Length,
                            size_t cf1Length){
        
        // confirm that the Antialiasing filter kernels are symmetric
        assert(BMFIRFilter_isSymmetric(coefficients0,cf0Length));
        assert(BMFIRFilter_isSymmetric(coefficients1,cf1Length));
        
        // set length variables
        This->cf0Length = cf0Length;
        This->cf1Length = cf1Length;
        This->maxLength = cf0Length < cf1Length ? cf0Length : cf1Length;
        This->IRLength = cf0Length + cf1Length;
        
        // init a circular buffer
        TPCircularBufferInit(&This->inputBuffer, sizeof(float)*((int)This->maxLength - 1 + BM_BUFFER_CHUNK_SIZE));
        TPCircularBufferClear(&This->inputBuffer);
        
        // copy the filter coefficients
        This->coefficients0 = malloc(sizeof(float)*cf0Length);
        This->coefficients1 = malloc(sizeof(float)*cf1Length);
        memcpy(This->coefficients0, coefficients0, sizeof(float)*cf0Length);
        memcpy(This->coefficients1, coefficients1, sizeof(float)*cf1Length);
        
        /*
         * write zeros into the circular buffer to prepare for the first
         * iteration of the filter
         */
        // get the head pointer
        int availableBytes;
        float* head = TPCircularBufferHead(&This->inputBuffer, &availableBytes);
        
        // confirm that the buffer has space as requested
        assert(availableBytes > sizeof(float)*(BM_BUFFER_CHUNK_SIZE + This->maxLength - 1));
        
        // write zeros
        memset(head, 0, sizeof(float)*(This->maxLength-1));
        
        // mark the bytes as written
        TPCircularBufferProduce(&This->inputBuffer,
                                sizeof(float)*((int)This->maxLength-1));
        
        // allocate a temp buffer for the convolver
        This->convolverTempBuffer = malloc(sizeof(float)*(BM_BUFFER_CHUNK_SIZE + This->maxLength - 1));
        
        // allocate space for output buffers
        This->outputBuffer0  = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->outputBuffer1 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->outputBuffers.imagp = This->outputBuffer0;
        This->outputBuffers.realp = This->outputBuffer1;
    }
    
    
    void BMUpsampler2x_process(BMUpsampler2x* This,
                               float* input,
                               float* output,
                               size_t numSamplesIn){
        
        // chunk processing loop
        while (numSamplesIn > 0){
            
            // get the write pointer for the buffer
            int bytesAvailable;
            float* head = TPCircularBufferHead(&This->inputBuffer, &bytesAvailable);
            
            // convert units from bytes to samples to get the size of the
            // space available for writing
            int samplesAvailable = bytesAvailable / sizeof(float);
            
            // don't process more than the available write space in the input buffer
            int samplesProcessing = numSamplesIn < samplesAvailable ? (int)numSamplesIn : samplesAvailable;
            
            // don't process more than the available space in the interleaving buffers
            samplesProcessing = samplesProcessing < BM_BUFFER_CHUNK_SIZE ? samplesProcessing : BM_BUFFER_CHUNK_SIZE;
            
            // convert samples to bytes for memcpy
            int bytesProcessing = samplesProcessing * sizeof(float);
            
            // copy the input into the buffer
            memcpy(head, input, bytesProcessing);
            
            // advance the write pointer
            TPCircularBufferProduce(&This->inputBuffer, bytesProcessing);
            
            // get the read pointer for the buffer
            float* tail = TPCircularBufferTail(&This->inputBuffer, &bytesAvailable);
            
            // do the convolution
            //vDSP_conv(tail, 1, This->coefficients0End, -1, output, 2, samplesProcessing, This->cf0Length);
            //vDSP_conv(tail, 1, This->coefficients1End, -1, output+1, 2, samplesProcessing, This->cf1Length);
            BMSymmetricConv(This->coefficients0, 1,
                            tail, 1,
                            This->outputBuffer0, 1,
                            This->convolverTempBuffer,
                            This->cf0Length,
                            samplesProcessing);
            BMSymmetricConv(This->coefficients1, 1,
                            tail, 1,
                            This->outputBuffer1, 1,
                            This->convolverTempBuffer,
                            This->cf1Length,
                            samplesProcessing);
            
            // interleave the buffered outputs into the audio output
            // ztoc function is actually used for split complex to
            // interleaved complex, but by casting the output pointer to
            // DSPComplex* we can use it.
            vDSP_ztoc(&This->outputBuffers, 1,
                      (DSPComplex*)output, 2, samplesProcessing);
            
            // advance the read pointer
            TPCircularBufferConsume(&This->inputBuffer, bytesProcessing);
            
            // advance pointers
            input += samplesProcessing;
            output += samplesProcessing;
            
            // how many samples are left?
            numSamplesIn -= samplesProcessing;
        }
    }
    
    
    void BMUpsampler2x_free(BMUpsampler2x* This){
        TPCircularBufferCleanup(&This->inputBuffer);
        
        free(&This->coefficients0);
        This->coefficients0 = NULL;
        
        free(&This->coefficients1);
        This->coefficients1 = NULL;
        
        free(&This->convolverTempBuffer);
        This->convolverTempBuffer = NULL;
        
        free(&This->outputBuffer0);
        This->outputBuffer0 = NULL;
        
        free(&This->outputBuffer1);
        This->outputBuffer1 = NULL;
    }
    
    
    
    /*
     * The calling function must ensure that the length of IR is at least
     * This->IRLength.
     */
    void BMUpsampler2x_impulseResponse(BMUpsampler2x* This, float* IR){
        
        // set up the input long enough to flush the zeros out of the buffer
        // and produce the IR
        float* impulseIn = malloc(sizeof(float)*(This->IRLength));
        memset(impulseIn,0,sizeof(float)*(This->IRLength));
        impulseIn[0] = 1.0;
        
        // enter the impulse input and flush out the zeros
        BMUpsampler2x_process(This, impulseIn, IR, This->IRLength);
        
        free(impulseIn);
    }
    
#ifdef __cplusplus
}
#endif
