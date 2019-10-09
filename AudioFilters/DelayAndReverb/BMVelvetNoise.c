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
        
        // allocate an array of tempoary storage
        float* t = malloc(sizeof(float)*numTaps);
        
        // generate times evenly spaced between startTime and endTime
        float increment = (endTimeMS - startTimeMS) / (float)(numTaps);
        vDSP_vramp(&startTimeMS, &increment, t, 1, numTaps);
        
        
        for(size_t i=0; i< numTaps; i++){
            // add a random float to each delay time to get uneven spacing
            t[i] += increment * (float)rand()/(float)RAND_MAX;
            
            // convert times from milliseconds to units of samples
            t[i] *= sampleRate / 1000.0f;
            
            // convert to unsigned long (size_t)
            indicesOut[i] = (size_t)t[i];
        }
        
        // free the temp storage
        free(t);
    }

	
    /*!
	 *BMVelvetNoise_setTapIndicesNA
     *
	 * @abstract Set tap indices using Velvet Noise method, with a temp array passed in to avoid memory allocation so that the function can be used in real time on the audio thread
     *
     * @notes (See: "Reverberation modeling using velvet noise" by M. Karjalainen and Hanna Järveläinen. https://www.researchgate.net/publication/283247924_Reverberation_modeling_using_velvet_noise
     *
     * @param startTimeMS   the first tap time will start soon after this
     * @param endTimeMS     the last tap will end before this time
     * @param indicesOut    array of indices for setting delay taps
	 * @param tempStorage   a temp array of length numTaps
     * @param sampleRate    sample rate of the audio system
     * @param numTaps       length of indicesOut
     */
    void BMVelvetNoise_setTapIndicesNA(float startTimeMS,
                                       float endTimeMS,
                                       size_t* indicesOut,
                                       float* tempStorage,
                                       float sampleRate,
                                       size_t numTaps){
        // generate times evenly spaced between startTime and endTime
        float increment = (endTimeMS - startTimeMS) / (float)(numTaps);
        vDSP_vramp(&startTimeMS, &increment, tempStorage, 1, numTaps);
        
        for(size_t i=0; i< numTaps; i++){
            // add a random float to each delay time to get uneven spacing
            tempStorage[i] += increment * (float)rand()/(float)RAND_MAX;
            // convert times from milliseconds to units of samples
            tempStorage[i] *= sampleRate / 1000.0f;
            
            // convert to unsigned long (size_t)
            indicesOut[i] = (size_t)tempStorage[i];
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
            size_t randomIndex = rand() % length;
            BMVelvetNoise_swapAt(A, i, randomIndex);
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
