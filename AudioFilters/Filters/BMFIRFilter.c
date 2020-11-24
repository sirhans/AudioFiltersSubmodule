//
//  BMFIRFilter.c
//  BMAudioFilters
//
//  Created by Hans on 18/9/17.
//  Anyone may use this code without restrictions
//



#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdlib.h>
#include <assert.h>
#include "BMFIRFilter.h"
#include "BMSymmetricConv.h"
#include <Accelerate/Accelerate.h>
    
    
    /*
     * check if the filter kernel is symmetric
     */
    bool BMFIRFilter_isSymmetric(float* kernel, size_t length){
        bool symmetric = true;
        
        for(size_t i=0; i<length/2; i++)
            if(kernel[i] != kernel[length - 1 - i])
                symmetric = false;
        
        return symmetric;
    }
    
    
    /*
     * @param coefficients   filter kernel
     * @param length         length of coefficients
     */
    void BMFIRFilter_init(BMFIRFilter *This,
                          float* coefficients,
                          size_t length){
        
        // if the filter kernel is symmetric, we can reduce the number of
        // multiplications in the convolution by about 50%. We check if
        // it is symmetric or not
        This->symmetricFilterKernel = BMFIRFilter_isSymmetric(coefficients, length);
        
        // set length variables
        This->length = length;
        
        // init a circular buffer
        TPCircularBufferInit(&This->inputBuffer, sizeof(float)*((int)This->length - 1 + BM_BUFFER_CHUNK_SIZE));
        TPCircularBufferClear(&This->inputBuffer);
        
        // copy the filter coefficients
        This->coefficients = malloc(sizeof(float)*length);
        memcpy(This->coefficients, coefficients, sizeof(float)*length);
        
        /*
         * write zeros into the circular buffer to prepare for the first
         * iteration of the filter
         */
        // get the head pointer
        uint32_t availableBytes;
        float* head = TPCircularBufferHead(&This->inputBuffer, &availableBytes);
        
        // confirm that the buffer has space as requested
        assert(availableBytes > sizeof(float)*(BM_BUFFER_CHUNK_SIZE + This->length - 1));
        
        // write zeros
        memset(head, 0, sizeof(float)*(This->length-1));
        
        // mark the bytes as written
        TPCircularBufferProduce(&This->inputBuffer,
                                sizeof(float)*((int)This->length-1));
        
        // for symmetric kernel, allocate a temp buffer for the convolver
        if (This->symmetricFilterKernel)
            This->convolverTempBuffer = malloc(sizeof(float)*(BM_BUFFER_CHUNK_SIZE + This->length - 1));
    }
    
    
    void BMFIRFilter_process(BMFIRFilter *This,
                             float* input,
                             float* output,
                             size_t numSamples){
        
        // chunk processing loop
        while (numSamples > 0){
            
            // get the write pointer for the buffer
            uint32_t bytesAvailable;
            float* head = TPCircularBufferHead(&This->inputBuffer, &bytesAvailable);
            
            // convert units from bytes to samples to get the size of the
            // space available for writing
            int samplesAvailable = bytesAvailable / sizeof(float);
            
            // don't process more than the available write space in the input buffer
            int samplesProcessing = numSamples < samplesAvailable ? (int)numSamples : samplesAvailable;
            
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
            
            // if the kernel is symmetric, do the convolution like this:
            if(This->symmetricFilterKernel)
                BMSymmetricConv(This->coefficients, 1,
                                tail, 1,
                                output, 1,
                                This->convolverTempBuffer,
                                This->length,
                                samplesProcessing);
            
            // for non-symmetric kernel, do it this way:
            else
                vDSP_conv(tail, 1, This->coefficients+This->length-1, -1, output, 1, samplesProcessing, This->length);
            
            
            // advance the read pointer
            TPCircularBufferConsume(&This->inputBuffer, bytesProcessing);
            
            // advance pointers
            input += samplesProcessing;
            output += samplesProcessing;
            
            // how many samples are left?
            numSamples -= samplesProcessing;
        }
    }
    
    
    void BMFIRFilter_free(BMFIRFilter *This){
        TPCircularBufferCleanup(&This->inputBuffer);
        
        free(This->coefficients);
        This->coefficients = NULL;
        
        if(This->symmetricFilterKernel){
            free(This->convolverTempBuffer);
            This->convolverTempBuffer = NULL;
        }
    }
    
    
    
    /*
     * The calling function must ensure that the length of IR is at least
      *This->length.
     */
    void BMFIRFilter_impulseResponse(BMFIRFilter *This, float* IR){
        
        // set up the input long enough to flush the zeros out of the buffer
        // and produce the IR
        float* impulseIn = malloc(sizeof(float)*(This->length));
        memset(impulseIn,0,sizeof(float)*(This->length));
        impulseIn[0] = 1.0;
        
        // enter the impulse input and flush out the zeros
        BMFIRFilter_process(This, impulseIn, IR, This->length);
        
        free(impulseIn);
    }
    
#ifdef __cplusplus
}
#endif
