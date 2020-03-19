//
//  BMMeasurementBuffer.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 11/3/20.
//  This file is public domain. No restrictions.
//

#include "BMMeasurementBuffer.h"
#include "Constants.h"


void BMMeasurementBuffer_inputSamples(BMMeasurementBuffer *This,
                                      const float* input,
                                      size_t numSamples){
    
    // We set the desired buffer length at init time and then we always consume
    // as many bytes as we produce after that.
    
    while(numSamples > 0){
        // check how much space is available
        uint32_t spaceAvailable;
        TPCircularBufferHead(&This->buffer, &spaceAvailable);
        
        // how many samples can we write?
        size_t samplesWriting = BM_MIN(spaceAvailable,numSamples);
    
        // write into the buffer
        TPCircularBufferProduceBytes(&This->buffer, input, (uint32_t)(sizeof(float)*samplesWriting));
        
        // consume as many bytes as we just wrote
        TPCircularBufferConsume(&This->buffer, (uint32samplesWriting);
        
        numSamples -= samplesWriting;
    }
}
