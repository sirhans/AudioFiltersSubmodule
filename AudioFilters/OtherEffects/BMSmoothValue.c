//
//  BMSmoothValue.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/13/22.
//  I release this file into the public domain with no restrictions.
//

#include "BMSmoothValue.h"
#include <Accelerate/Accelerate.h>



void BMSmoothValue_init(BMSmoothValue *This, float updateTimeSeconds, float sampleRate){
	This->sampleRate = sampleRate;
	This->updateTimeInSamples = (size_t)round(sampleRate * updateTimeSeconds);
	This->currentValue = 0.0f;
	This->targetValue = 0.0f;
	This->pendingTargetValue = 0.0f;
	This->updateProgress = This->updateTimeInSamples;
	This->pendingUpdateTimeInSamples = This->updateTimeInSamples;
	This->incrementSet = FALSE;
	This->immediateUpdate = FALSE;
}


// this function exists to prevent errors when setValue is called from any thread
// other than the audio thread
void BMSmoothValue_setIncrement(BMSmoothValue *This){
	This->targetValue = This->pendingTargetValue;
	This->updateTimeInSamples = This->pendingUpdateTimeInSamples;
	
	// if the update mode is immediate, update the current value directly
	if(This->immediateUpdate){
		This->currentValue = This->targetValue;
		This->updateProgress = This->updateTimeInSamples;
	}
	// if the update is gradual, start at 0
	else{
		This->updateProgress = 0;
	}
	
	// compute the per-sample increment to reach the target on time
	This->increment = (This->targetValue - This->currentValue) / (float)This->updateTimeInSamples;
	This->incrementSet = TRUE;
}



void BMSmoothValue_setUpdateTime(BMSmoothValue *This, float updateTimeSeconds){
	This->pendingUpdateTimeInSamples = (size_t)round(This->sampleRate * updateTimeSeconds);
}



void BMSmoothValue_setValueSmoothly(BMSmoothValue *This, float value){
	This->pendingTargetValue = value;
	This->immediateUpdate = FALSE;
	This->incrementSet = FALSE; // flag the increment as unset, but let the audio thread do the computation
}



void BMSmoothValue_setValueImmediately(BMSmoothValue *This, float value){
	This->pendingTargetValue = value;
	This->immediateUpdate = TRUE;
	This->incrementSet = FALSE;
}




void BMSmoothValue_process(BMSmoothValue *This, float *output, size_t numSamples){
	
	// Compute the increment if it hasn't been set yet. We do this here
	// to ensure that the increment can't be changed by another thread while
	// this function is executing on the audio thread.
	if(!This->incrementSet) BMSmoothValue_setIncrement(This);
	
	
	// if we aren't currently doing an update, fill the output with a constant value
	if(This->updateProgress == This->updateTimeInSamples){
		vDSP_vfill(&This->currentValue, output, 1, numSamples);
	}
	//
	// else: if the value needs to update
	else {
		
		size_t timeTillUpdateComplete = This->updateTimeInSamples - This->updateProgress;
		
		// if the update will finish before the end of the buffer
		if(timeTillUpdateComplete < numSamples){
			// complete the interpolation, stopping one sample before the target
			vDSP_vramp(&This->currentValue, &This->increment, output, 1, timeTillUpdateComplete);
			
			// update the progress
			This->updateProgress = This->updateTimeInSamples;
			
			// set the current value exactly equal to the target in case the
			// incrementation didn't give a precise result.
			This->currentValue = This->targetValue;
		
			// fill the remaining part of the buffer with the current value
			vDSP_vfill(&This->currentValue, output + timeTillUpdateComplete, 1, numSamples - timeTillUpdateComplete);
		}
		// if the update will not finish before the end of the buffer
		else {
			// interpolate towards the target till the end of the buffer
			vDSP_vramp(&This->currentValue, &This->increment, output, 1, numSamples);
			
			// update the current value
			This->currentValue = output[numSamples-1] + This->increment;
			This->updateProgress += numSamples;
			
			// if the update is finished, set the current value equal to the
			// target in case there were numerical errors with the
			// incrementation process.
			if (This->updateProgress == This->updateTimeInSamples)
				This->currentValue = This->targetValue;
		}
	}
}



float BMSmoothValue_advance(BMSmoothValue *This, size_t numSamples){
	// Compute the increment if it hasn't been set yet. We do this here
	// to ensure that the increment can't be changed by another thread while
	// this function is executing on the audio thread.
	if(!This->incrementSet) BMSmoothValue_setIncrement(This);
	
	
	// if we aren't currently doing an update, return the current value
	if(This->updateProgress == This->updateTimeInSamples){
		return This->currentValue;
	}
	//
	// else: if the value needs to update
	else {
		
		size_t timeTillUpdateComplete = This->updateTimeInSamples - This->updateProgress;
		
		// if the update will finish before numSamples
		if(timeTillUpdateComplete < numSamples){
			// cache the current value
			float currentValueCached = This->currentValue;
			
			// update the progress to indicate completion
			This->updateProgress = This->updateTimeInSamples;
			
			// set the current value exactly equal to the target
			This->currentValue = This->targetValue;
		
			// return the previously cached current value
			return currentValueCached;
		}
		// if the update will not finish before the end of the buffer
		else {
			// cache the current value
			float currentValueCached = This->currentValue;
			
			// advance the current value
			This->currentValue += This->increment * (float)numSamples;
			
			// update the progress
			This->updateProgress += numSamples;
			
			// if the update is finished, set the current value equal to the
			// target in case there were numerical errors with the
			// incrementation process.
			if (This->updateProgress == This->updateTimeInSamples)
				This->currentValue = This->targetValue;
			
			// return the cached current value
			return currentValueCached;
		}
	}
}
