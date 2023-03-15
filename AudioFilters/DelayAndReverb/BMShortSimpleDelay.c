//
//  BMShortSimpleDelay.c
//  SaturatorAU
//
//  This is a static delay class for efficiently implementing delay times that
//  are short relative to the audio buffer length.
//
//  Created by hans anderson on 9/27/19.
//  Anyone may use this file without restrictions of any kind.
//

#include "BMShortSimpleDelay.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "Constants.h"
#include <Accelerate/Accelerate.h>


void BMShortSimpleDelay_process(BMShortSimpleDelay *This,
                                const float **inputs,
                                float **outputs,
                                size_t numChannels,
                                size_t numSamples){
    assert(This->numChannels == numChannels);
    
    // does not work in place
    assert(inputs[0] != outputs[0]);
    
	// bypass if the delay is zero
	if (This->delayLength == 0) {
		for(size_t i=0; i<numChannels; i++){
			memcpy(outputs[i], inputs[i], numSamples*sizeof(float));
		}
	}
	else {
		for(size_t i=0; i<numChannels; i++){
			// when the audio buffer length is longer than the delay we can copy
			// part of the audio directly from input to output. The remaining part
			// waits in the buffer until the next call.
			if(numSamples >= This->delayLength){
				// copy from delay buffer to output
				memcpy(outputs[i],
					   This->delayPtrs[i],
					   sizeof(float)*This->delayLength);
				
				// copy directly from input to output
				memcpy(outputs[i] + This->delayLength,
					   inputs[i],
					   sizeof(float)*(numSamples-This->delayLength));
				
				// copy from input to delay buffer
				memcpy(This->delayPtrs[i],
					   inputs[i] + numSamples - This->delayLength,
					   sizeof(float)*This->delayLength);
			}
			
			// when the delay time is longer than the audio buffer then we have to
			// shift the contents of the delay in addition to copying in and out
			// of buffers. This is much slower and should be avoided by using
			// the BMSimpleDelay class, which uses a circular buffer to eliminate
			// the need for shifting the contents of the delay.
			else {
				// copy from the delay buffer to the output
				memcpy(outputs[i],
					   This->delayPtrs[i],
					   sizeof(float)*numSamples);
				
				// move samples within the delay buffer (this is the slow part)
				memmove(This->delayPtrs[i],
						This->delayPtrs[i]+numSamples,
						sizeof(float)*(This->delayLength - numSamples));
				
				// copy into the delay buffer
				memcpy(This->delayPtrs[i] + This->delayLength - numSamples,
					   inputs[i],
					   sizeof(float)*numSamples);
			}
		}
	}
    
    // if a new targetLength has been set, update the delay time
    if(This->delayLength != This->targetDelayLength){
        BMShortSimpleDelay_free(This);
        BMShortSimpleDelay_init(This, This->numChannels, This->targetDelayLength);
        This->delayLength = This->targetDelayLength;
    }
}




/*!
 *BMShortSimpleDelay_init
 */
void BMShortSimpleDelay_init(BMShortSimpleDelay* This, size_t numChannels, size_t lengthSamples){
    This->numChannels = numChannels;
    This->delayLength = lengthSamples;
    This->targetDelayLength = lengthSamples;
	
	// in case of zero delay time, allocate one sample of memory so that we
	// don't get an error when we free the memory later
	if (lengthSamples==0) lengthSamples = 1;
    
    This->delayPtrs = malloc(sizeof(float*)*numChannels);
    
    // allocate memory for all delays with a single call
    This->delayMemory = calloc(numChannels*lengthSamples,sizeof(float));
    
    for(size_t i=0; i<numChannels; i++){
        This->delayPtrs[i] = This->delayMemory + (i*lengthSamples);
    }
}




/*!
 *BMShortSimpleDelay_free
 */
void BMShortSimpleDelay_free(BMShortSimpleDelay* This){
    free(This->delayMemory);
    This->delayMemory = NULL;
    free(This->delayPtrs);
    This->delayPtrs = NULL;
}




/*!
 *BMShortSimpleDelay_changeLength
 */
void BMShortSimpleDelay_changeLength(BMShortSimpleDelay* This, size_t lengthSamples){
    This->targetDelayLength = lengthSamples;
}



void BMShortSimpleDelay_clearBuffers(BMShortSimpleDelay *This){
	vDSP_vclr(This->delayMemory, 1, This->delayLength * This->numChannels);
}
