//
//  BMSimpleFDN.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 9/14/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#include "BMSimpleFDN.h"
#include <stdlib.h>
#include <assert.h>
#include <Accelerate/Accelerate.h>
#include "BMFastHadamard.h"
#include "BMIntegerMath.h"



// forward declarations
float BMSimpleFDN_gainFromRT60(float rt60, float delayTime);
void BMSimpleFDN_randomShuffle(float* A, size_t length);
void BMSimpleFDN_randomSigns(float* A, size_t length);
inline float BMSimpleFDN_processSample(BMSimpleFDN* This, float input);





void BMSimpleFDN_velvetNoiseDelayTimes(BMSimpleFDN* This){
    
    // allocate an array to compute delay times in seconds as floating point numbers
    float* delayLengthsFloat = malloc(sizeof(float) * This->numDelays);
    
    // find out what spacing the delays should have to put them in the range
    // between minDelay and maxDelay. The last should never exceed max delay and
    // the first can not be before min delay.
    float delayTimeRange = (This->maxDelayS - This->minDelayS);
    float delaySpacing = delayTimeRange / (float)(This->numDelays);
    
    // ensure that delay times can be assigned without duplication
    assert(delaySpacing * This->sampleRate > This->numDelays);

    for(size_t i=0; i<This->numDelays; i++){
        // place the delays at even intervals between min and max
        delayLengthsFloat[i] = (float)i * delaySpacing;
        
        // move each delay back by a random number between 0 and delaySpacing
        float jitter = delaySpacing * (float)arc4random() / (float)UINT32_MAX;
        delayLengthsFloat[i] += jitter;
        
        // convert from float to uint
        This->delayLengths[i] = (size_t)(delayLengthsFloat[i] * This->sampleRate);
    }
    
    // ensure that no two delays have been rounded to the same number
    for(size_t i=0; i<This->numDelays-1; i++)
        if(This->delayLengths[i+1] == This->delayLengths[i])
            This->delayLengths[i+1] += 1;
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
    
    // free temporary memory
    free(delayLengthsFloat);
};


/*!
 *containsX
 *
 * @returns true if X is found in the first numElements of A
 */
bool containsX(size_t* A, size_t x, size_t numElements){
    
    // return true if x is equal to any of the first numElements
    for(size_t i=0; i<numElements; i++)
        if(A[i]==x)
            return true;
    
    // return false if not found
    return false;
}






/*!
 *containsCommonFactor
 *
 * @returns true if X has a non-trivial common factor with any of the first numElements of A
 */
bool containsCommonFactor(size_t* A, size_t x, size_t numElements){
    
    // return true if x is equal to any of the first numElements
    for(size_t i=0; i<numElements; i++)
        if(gcd_ui(A[i],x) != 1)
            return true;
    
    // return false if not found
    return false;
}




void BMSimpleFDN_randomDelayTimes(BMSimpleFDN* This){
    
    // convert min and max delay from seconds to samples
    size_t minDelay = This->minDelayS * This->sampleRate;
    size_t maxDelay = This->maxDelayS * This->sampleRate;
    
    // find the range of delay times in samples
    size_t delayRange = maxDelay - minDelay;
    
    // assert that there are enough integers between min and max to give each
    // delay a unique length
    assert(delayRange >= This->numDelays);
    
    // generate the first delay time randomly
    This->delayLengths[0] = minDelay + (arc4random() % (delayRange + 1));
    
    // generate the remaining random delay lengths
    for(size_t i=1; i<This->numDelays; i++){
        bool generatedUniqueNewEntry = false;
        while(!generatedUniqueNewEntry){
            // generate a new random delay in the specified range
            This->delayLengths[i] = minDelay + (arc4random() % (delayRange + 1));
            // if the entry we just generated is unique, stop searching
            if(!containsX(This->delayLengths,This->delayLengths[i],i-1))
                generatedUniqueNewEntry = true;
        }
    }
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
}






void BMSimpleFDN_relativelyPrimeDelayTimes(BMSimpleFDN* This){
    
    // convert min and max delay from seconds to samples
    size_t minDelay = This->minDelayS * This->sampleRate;
    size_t maxDelay = This->maxDelayS * This->sampleRate;
    
    // find the range of delay times in samples
    size_t delayRange = maxDelay - minDelay;
    
    // assert that there are enough integers between min and max to give each
    // delay a unique length
    assert(delayRange >= This->numDelays);
    
    // generate the first delay time randomly
    This->delayLengths[0] = minDelay + (arc4random() % (delayRange + 1));
    
    // generate random delay lengths
    for(size_t i=1; i<This->numDelays; i++){
        bool generatedUniqueNewEntry = false;
        while(!generatedUniqueNewEntry){
            // generate a new random delay in the specified range
            This->delayLengths[i] = minDelay + (arc4random() % (delayRange + 1));
            // if the entry we just generated is relatively prime to all the
            // previous ones, stop
            if(!containsCommonFactor(This->delayLengths,This->delayLengths[i],i-1))
                generatedUniqueNewEntry = true;
        }
    }
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
}







void BMSimpleFDN_init(BMSimpleFDN* This,
                      float sampleRate,
                      size_t numDelays,
                      enum delayTimeMethod method,
                      float minDelayTimeSeconds,
                      float maxDelayTimeSeconds,
                      float RT60DecayTimeSeconds){
    // number of delays must be a power of two
    assert(BMPowerOfTwoQ(numDelays));
    
    This->numDelays = numDelays;
    This->sampleRate = sampleRate;
    This->RT60DecayTime = RT60DecayTimeSeconds;
    This->minDelayS = minDelayTimeSeconds;
    This->maxDelayS = maxDelayTimeSeconds;
    
    // allocate memory for some arrays
    This->delayLengths = malloc(sizeof(size_t) * numDelays);
    This->delays = malloc(sizeof(float*) * numDelays);
    This->attenuationCoefficients = malloc(sizeof(float) * numDelays);
    This->rwIndices = calloc(numDelays, sizeof(size_t));
    This->outputTapSigns = malloc(sizeof(float) * numDelays);
    This->buffer1 = malloc(sizeof(float) * 3 * numDelays);
    This->buffer2 = This->buffer1 + numDelays;
    This->buffer3 = This->buffer2 + numDelays;

    // compute delay times according to the specified method
    if(method == DTM_VELVETNOISE)
        BMSimpleFDN_velvetNoiseDelayTimes(This);
    if(method == DTM_RANDOM)
        BMSimpleFDN_randomDelayTimes(This);
    if(method == DTM_RELATIVEPRIME)
        BMSimpleFDN_relativelyPrimeDelayTimes(This);
    
    // count the total delay memory
    size_t totalDelay=0;
    for(size_t i=0; i<This->numDelays; i++)
        totalDelay += This->delayLengths[i];
    
    // allocate delay memory and set to zero
    This->delays[0] = calloc(totalDelay, sizeof(float));
    
    // set pointers to each delay
    for(size_t i=1; i<This->numDelays; i++)
        This->delays[i] = This->delays[i-1] + This->delayLengths[i-1];
    
    // set the attenuation coefficients
    float matrixAttenuation = sqrt(1.0 / (double) This->numDelays);
    for(size_t i=0; i<This->numDelays; i++){
        float delayTime = (float)This->delayLengths[i] / This->sampleRate;
        float rt60attenuation = BMSimpleFDN_gainFromRT60(This->RT60DecayTime, delayTime);
        This->attenuationCoefficients[i] = matrixAttenuation * rt60attenuation;
    }
}






float BMSimpleFDN_processSample(BMSimpleFDN* This, float input){
    float output = 0;
    
    // read from the delays
    for(size_t i=0; i<This->numDelays; i++){
        // read output from each delay into a temp buffer
        float readSample = This->delays[i][This->rwIndices[i]];
        
        // attenuate and write to buffer1
        This->buffer1[i] = readSample * This->attenuationCoefficients[i];
        
        // sum to output
        output += This->buffer1[i] * This->outputTapSigns[i];
    }
    
    
    // mix the feedback
    BMFastHadamardTransform(This->buffer1, This->buffer1, This->buffer2, This->buffer3, This->numDelays);
    
    
    // write back to the delays
    for(size_t i=0; i<This->numDelays; i++){
        // mix input with feedback and write into the delays
        This->delays[i][This->rwIndices[i]] = input + This->buffer1[i];
        
        // advance the delay indices and wrap to zero if at the end
        This->rwIndices[i]++;
        if(This->rwIndices[i] >= This->delayLengths[i])
            This->rwIndices[i] = 0;
    }
    
    
    return output;
}







void BMSimpleFDN_processBuffer(BMSimpleFDN* This,
                               const float* input,
                               float* output,
                               size_t numSamples){
    
    for(size_t i=0; i<numSamples; i++)
        output[i] = BMSimpleFDN_processSample(This, input[i]);
}






// computes the appropriate feedback gain attenuation
// to get an exponential decay envelope with the specified RT60 time
// (in seconds) from a delay line of the specified length.
//
// This formula comes from solving EQ 11.33 in DESIGNING AUDIO EFFECT PLUG-INS IN C++ by Will Pirkle
// which is attributed to Jot, originally.
float BMSimpleFDN_gainFromRT60(float rt60, float delayTime){
    if(rt60 == FLT_MAX)
        return 1.0f;
    else
        return powf(10.0f, (-3.0f * delayTime) / rt60 );
}






void BMSimpleFDN_free(BMSimpleFDN* This){
    // we only need to free one delay because we allocated with a single call
    // to malloc
    free(This->delays[0]);
    This->delays[0] = NULL;
    
    free(This->delays);
    This->delays = NULL;
    
    free(This->delayLengths);
    This->delayLengths = NULL;
    
    free(This->attenuationCoefficients);
    This->attenuationCoefficients = NULL;
    
    free(This->rwIndices);
    This->rwIndices = NULL;
    
    free(This->outputTapSigns);
    This->outputTapSigns = NULL;
    
    free(This->buffer1);
    This->buffer1 = NULL;
}






void BMSimpleFDN_randomShuffle(float* A, size_t length){
    // swap every element in A with another element in a random position
    for(size_t i=0; i<length; i++){
        size_t randomIndex = arc4random() % length;
        float temp = A[i];
        A[i] = A[randomIndex];
        A[randomIndex] = temp;
    }
}





void BMSimpleFDN_randomSigns(float* A, size_t length){
    
    // half even, half odd
    size_t i=0;
    for(; i<length/2; i++)
        A[i] = 1.0;
    for(; i<length; i++)
        A[i] = -1.0;
    
    // randomise the order
    BMSimpleFDN_randomShuffle(A, length);
}






void BMSimpleFDN_impulseResponse(BMSimpleFDN* This, float* IR, size_t numSamples){
    // process the initial impulse
    IR[0] = BMSimpleFDN_processSample(This, 1.0f);
    
    // process the remaining zeros
    for(size_t i=1; i<numSamples; i++)
        IR[i] = BMSimpleFDN_processSample(This, 0.0f);
}
