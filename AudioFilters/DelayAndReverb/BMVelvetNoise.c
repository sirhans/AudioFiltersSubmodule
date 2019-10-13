//
//  BMVelvetNoise.c
//  BMAudioFilters
//
//  Created by Hans on 1/6/17.
//
//  The author releases this file into the public domain with no restrictions
//  of any kind.
//

#ifdef __cplusplus
extern "C" {
#endif

#include "BMVelvetNoise.h"
#include <stdlib.h>
	
    /*!
	 *BMVelvetNoise_setTapIndices
     *
	 * @abstract Set tap indices using Velvet Noise method
     *
     * @notes (See: "Reverberation modeling using velvet noise" by M. Karjalainen and Hanna Järveläinen. https://www.researchgate.net/publication/283247924_Reverberation_modeling_using_velvet_noise
     *
     * @param startTimeMS   the first tap time will start soon after this
     * @param endTimeMS     the last tap will end before this time
     * @param indicesOut    array of indices for setting delay taps
     * @param sampleRate    sample rate of the audio system
     * @param numTaps       length of indicesOut
     */
    void BMVelvetNoise_setTapIndices(float startTimeMS,
                                     float endTimeMS,
                                     size_t* indicesOut,
                                     float sampleRate,
                                     size_t numTaps){
        
        // compute the spacing for evenly spaced between startTime and endTime
        float incrementMS = (endTimeMS - startTimeMS) / (float)numTaps;
		float incrementSamples = sampleRate * incrementMS / 1000.0f;
		float startTimeSamples = sampleRate * startTimeMS / 1000.0f;
        
        for(size_t i=0; i< numTaps; i++){
			// generate an evenly spaced tap time
			float tapTime = (float)i * incrementSamples + startTimeSamples;
			
            // add a random float to each delay time to get uneven spacing
			float randomJitter = incrementSamples * (float)arc4random()/(float)UINT32_MAX;
            tapTime += randomJitter;
            
            // convert to unsigned long (size_t)
            indicesOut[i] = (size_t)tapTime;
        }
    }
    
	
	
	
    /*
     * Swap the values at positions i and j in the array A
     */
    void BMVelvetNoise_swapAt(float* A, size_t i, size_t j){
        float t = A[i];
        A[i] = A[j];
        A[j] = t;
    }
    
    
    
    
    /*
     * randomly shuffle the order of elements in A
     */
    void BMVelvetNoise_randomShuffle(float* A, size_t length){
        for(size_t i=0; i<length; i++){
            // swap the ith element in A with a randomly selected element
            BMVelvetNoise_swapAt(A, i, arc4random_uniform((uint32_t)length));
        }
    }
    
    
    
    /*!
	 *BMVelvetNoise_setTapSigns
	 *
     * @abstract Set the values in tapSigns randomly to -1 and 1, with an equal number of + and - values.
     *
     * @param tapSigns   input array
     * @param numTaps    length of tapSigns
     */
    void BMVelvetNoise_setTapSigns(float* tapSigns,
                                   size_t numTaps){
        // set half the signs negative and the other half positive
        size_t i=0;
        for(; i<numTaps/2; i++)
            tapSigns[i] = 1.0f;
        for(; i<numTaps; i++)
            tapSigns[i] = -1.0f;
        
        // shuffle the order of the signs randomly
        BMVelvetNoise_randomShuffle(tapSigns,numTaps);
    }
    

#ifdef __cplusplus
}
#endif
