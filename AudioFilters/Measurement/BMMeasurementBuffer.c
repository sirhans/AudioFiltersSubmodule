//
//  BMMeasurementBuffer.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 11/3/20.
//  This file is public domain. No restrictions.
//

#include "BMMeasurementBuffer.h"
#include "Constants.h"
#include <stdlib.h>
#include <Accelerate/Accelerate.h>


void BMMeasurementBuffer_init(BMMeasurementBuffer *This, size_t lengthInSamples){
	TPCircularBufferInit(&This->buffer, (uint32_t)lengthInSamples*2);
	uint32_t bytesAvailable;
	float *head = TPCircularBufferHead(&This->buffer, &bytesAvailable);
	assert(bytesAvailable > lengthInSamples * sizeof(float));
	vDSP_vclr(head, 1, lengthInSamples);
	TPCircularBufferConsume(&This->buffer, (uint32_t)(lengthInSamples * sizeof(float)));
}

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
		uint32_t bytesWriting = (uint32_t)(numSamples * sizeof(float));
    
        // write into the buffer
        TPCircularBufferProduceBytes(&This->buffer, input, bytesWriting);
        
        // consume as many bytes as we just wrote
        TPCircularBufferConsume(&This->buffer, bytesWriting);
        
        numSamples -= samplesWriting;
    }
}
