//
//  BMDownsampler.c
//  BMAudioFilters
//
//  Created by Hans on 13/9/17.
//  Anyone may use this file without restrictions
//

#ifdef __cplusplus
extern "C" {
#endif
    
#include <assert.h>
#include "BMDownsampler.h"
#include "Constants.h"
#include <Accelerate/Accelerate.h>
#include <stdlib.h>
    
#define BMDOWNSAMPLER_2X_FILTER_LENGTH 48
    
    
    /*
     * @param decimationFactor   set this to N to keep 1 in N samples
     */
    void BMDownsampler_init(BMDownsampler* This,
                            size_t decimationFactor){
        
        // currently we only support decimation by 2
        assert(decimationFactor == 2);
        
        This->decimationFactor = decimationFactor;
        
        float filterCoefficients2x [BMDOWNSAMPLER_2X_FILTER_LENGTH] = {-0.000139553, -0.000209157, 0.000312496, 0.000873898, -0.0000216733, -0.00192026, -0.00145017, 0.0025341, 0.00448678, -0.00111919, -0.00830354, -0.00405208, 0.0103703, 0.0135405, -0.00664352, -0.0253336, -0.00733062, 0.0339704, 0.0354092, -0.0297469, -0.0835031, -0.00966769, 0.196183, 0.38174, 0.38174, 0.196183, -0.00966769, -0.0835031, -0.0297469, 0.0354092, 0.0339704, -0.00733062, -0.0253336, -0.00664352, 0.0135405, 0.0103703, -0.00405208, -0.00830354, -0.00111919, 0.00448678, 0.0025341, -0.00145017, -0.00192026, -0.0000216733, 0.000873898, 0.000312496, -0.000209157, -0.000139553};
        
        // malloc the filter coefficient array
        This->AAFilter = malloc(sizeof(float)*BMDOWNSAMPLER_2X_FILTER_LENGTH);
        
        // copy the coefficients into the filter coefficient array
        memcpy(This->AAFilter,filterCoefficients2x,sizeof(float)*BMDOWNSAMPLER_2X_FILTER_LENGTH);
        
        // set length of filter
        This->filterLength = BMDOWNSAMPLER_2X_FILTER_LENGTH;
        
        // vDSP_desamp requires the input length to be
        //    DF*(outputLength-1) + filterLength
        // we compute this value below
        size_t minBufferSize = (BM_BUFFER_CHUNK_SIZE*This->decimationFactor) + This->filterLength;
        TPCircularBufferInit(&This->buffer, (int)(sizeof(float)*minBufferSize));
        TPCircularBufferClear(&This->buffer);
        
        /*
         * write zeros into the circular buffer to prepare for the first
         * iteration of the filter
         */
        // get the head pointer
        int availableBytes;
        float* head = TPCircularBufferHead(&This->buffer, &availableBytes);
        
        // confirm that the buffer has space as requested
        assert(availableBytes > sizeof(float)*minBufferSize);
        
        // find the offset between the write and read pointers
        size_t offset = sizeof(float)*This->filterLength;
        
        // write zeros
        memset(head, 0, offset);
        
        // mark the bytes as written
        TPCircularBufferProduce(&This->buffer, (int)offset);
        
        // precompute the length of the impulse response
        This->IRLength = This->filterLength / This->decimationFactor;
    }
    
    
    
    
    
    void BMDownsampler_process(BMDownsampler* This,
                               float* input,
                               float* output,
                               size_t numSamplesOut){
        
        // chunk processing loop
        while (numSamplesOut > 0){
            
            // get the write pointer for the buffer
            int bytesAvailable;
            float* head = TPCircularBufferHead(&This->buffer, &bytesAvailable);
            
            // convert units from bytes to samples to get the size of the
            // space available for writing
            int samplesInAvailable = bytesAvailable / sizeof(float);
            int sampleOutAvailable = (samplesInAvailable - (int)This->filterLength - 1) / (int)This->decimationFactor;
            
            // don't process more than the available space
            int samplesProcessingOut = numSamplesOut < sampleOutAvailable ? (int)numSamplesOut : sampleOutAvailable;
            int bytesProcessingIn = samplesProcessingOut * sizeof(float) * (int)This->decimationFactor;
            
            // copy the input into the buffer
            memcpy(head, input, bytesProcessingIn);
            
            // advance the write pointer
            TPCircularBufferProduce(&This->buffer, bytesProcessingIn);
            
            // get the read pointer for the buffer
            float* tail = TPCircularBufferTail(&This->buffer, &bytesAvailable);
            
            // do the decimation and filtering
            vDSP_desamp(tail, This->decimationFactor, This->AAFilter, output, samplesProcessingOut, This->filterLength);
            
            // advance the read pointer
            TPCircularBufferConsume(&This->buffer, bytesProcessingIn);
            
            // advance pointers
            input += samplesProcessingOut;
            output += samplesProcessingOut;
            
            // how many samples are left?
            numSamplesOut -= samplesProcessingOut;
        }
    }
    
    
    void BMDownsampler_free(BMDownsampler* This){
        free(This->AAFilter);
        This->AAFilter  = NULL;
        
        TPCircularBufferCleanup(&This->buffer);
    }
    
    
    /*
     * Calling function must ensure that the length of IR is at least
     * This->IRLength
     */
    void BMDownsampler_impulseResponse(BMDownsampler* This, float* IR){
        
        // set up the input long enough to flush the zeros out of the buffer
        // and produce the IR
        float* impulseIn = malloc(sizeof(float)*This->filterLength*2);
        memset(impulseIn,0,sizeof(float)*This->filterLength*2);
        impulseIn[0] = 1.0;
        
        // enter the impulse input and flush out the zeros
        BMDownsampler_process(This, impulseIn, IR, This->IRLength);
    }
    
    
#ifdef __cplusplus
}
#endif
