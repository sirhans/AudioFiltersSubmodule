//
//  BMSimpleFDN.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 9/14/19.
//  Anyone may use this file. No restritions.
//

#include "BMSimpleFDN.h"
#include <stdlib.h>
#include <assert.h>
#include <Accelerate/Accelerate.h>
#include "BMFastHadamard.h"
#include "BMIntegerMath.h"


#define UNIQUESUMS_ATTEMPTS_LIMIT 5000


// forward declarations
float BMSimpleFDN_gainFromRT60(float rt60, float delayTime);
void BMSimpleFDN_randomShuffle(float* A, size_t length);
void BMSimpleFDN_randomSigns(float* A, size_t length);
inline float BMSimpleFDN_processSample(BMSimpleFDN *This, float input);





void BMSimpleFDN_velvetNoiseDelayTimes(BMSimpleFDN *This){
    
    // find out what spacing the delays should have to put them in the range
    // between minDelay and maxDelay. The last should never exceed max delay and
    // the first can not be before min delay.
    float delayTimeRange = (This->maxDelayS - This->minDelayS);
    float delaySpacing = delayTimeRange / (float)(This->numDelays);
    size_t minDelaySamples = This->minDelayS  *This->sampleRate;
    
    // ensure that delay times can be assigned without duplication
    assert(delayTimeRange  *This->sampleRate >= This->numDelays);
    
    // assign evenly spaced delay times
    for(size_t i=0; i<This->numDelays; i++)
        This->delayLengths[i] = minDelaySamples + (size_t)(delaySpacing*This->sampleRate * (float)i);

    // randomly jitter the delay times
    for(size_t i=0; i<This->numDelays-1; i++){
        // find the distance between the ith delay and its next neighbor
        size_t range = This->delayLengths[i+1] - This->delayLengths[i];
        
        // generate a random number in range
        size_t jitter = arc4random() % range;
        
        This->delayLengths[i] += jitter;
    }
    // randomly jitter the last delay time
    size_t maxDelayLength = This->maxDelayS  *This->sampleRate;
    size_t jitter = arc4random() % (maxDelayLength - This->delayLengths[This->numDelays-2]);
    This->delayLengths[This->numDelays-1] += jitter;
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
}




void BMSimpleFDN_logVelvetNoiseDelayTimes(BMSimpleFDN *This){
    
    // find out what spacing the delays should have to put them in the range
    // between minDelay and maxDelay. The last should never exceed max delay and
    // the first can not be before min delay.
    float delayTimeRangeRatio = This->maxDelayS/This->minDelayS;
    double delayCommonRatio = pow(delayTimeRangeRatio, 1.0/(double)This->numDelays);
    
    // ensure that delay times can be assigned without duplication
    assert((This->maxDelayS - This->minDelayS)  *This->sampleRate >= This->numDelays);
    
    // assign logarithmically spaced delay times
    for(size_t i=0; i<This->numDelays; i++){
        double delayTimeSeconds = This->minDelayS * pow(delayCommonRatio,(double)i);
        This->delayLengths[i] = delayTimeSeconds  *This->sampleRate;
    }
    
    // ensure that no two delays have the same length
    for(size_t i=0; i<This->numDelays; i++)
        for(size_t j=0; j<i; j++)
            if(This->delayLengths[j] == This->delayLengths[i])
                This->delayLengths[i] = This->delayLengths[j] + 1;
    
    // randomly jitter the delay times
    for(size_t i=0; i<This->numDelays-1; i++){
        // find the distance between the ith delay and its next neighbor
        size_t range = This->delayLengths[i+1] - This->delayLengths[i];
        
        // generate a random number in range
        size_t jitter = arc4random() % range;
        
        This->delayLengths[i] += jitter;
    }
    // randomly jitter the last delay time
    size_t maxDelayLength = This->maxDelayS  *This->sampleRate;
    size_t range = maxDelayLength - This->delayLengths[This->numDelays - 2];
    size_t jitter = arc4random() % range;
    This->delayLengths[This->numDelays - 1] += jitter;
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
}



/*!
 *countPrimesInRange
 */
size_t countPrimesInRange(size_t min, size_t max){
    size_t count = 0;
    for(size_t i=min; i<=max; i++)
        if(isPrimeMr(i))
            count++;
    
    return count;
}


/*!
 *getScaleForNPrimesInRange
 *
 * @return a value of scale for which there are there N primes in [min*scale,max*scale]
 */
float getScaleForNPrimesInRange(float min, float max, size_t N){
    size_t numPrimesInRange = countPrimesInRange(min, max);
    float scale = 1.0f;
    float increment = powf(2.0,1.0/12.0); // an arbitraty number to use for increasing the scale geometrically
    
    while(numPrimesInRange < N){
        scale *= increment; // increase the scale
        numPrimesInRange = countPrimesInRange(min * scale, max * scale);
    }
    
    return scale;
}




size_t nextPrime(size_t n){
    n++;
    while(!isPrimeMr(n))
        n++;
    return n;
}





void BMSimpleFDN_scaledPrimeDelayTimes(BMSimpleFDN *This){
    
    // what is the smallest scale for which [scale*This->minDelayS, scale*maxDelayS]
    // contains This->numDelays primes
    float scale = getScaleForNPrimesInRange(This->minDelayS, This->maxDelayS, This->numDelays);
    
    // set the first delay time, downscaled
    float downscaledFirstDelay = scale  *This->minDelayS;
    
    // what scale factor converts the downscaled version to the full size version?
    float fullSizeFirstDelay = This->minDelayS  *This->sampleRate;
    float fullSizeScale = fullSizeFirstDelay/downscaledFirstDelay;
    
    // set the first delay
    This->delayLengths[0] = (size_t)fullSizeFirstDelay;
    
    // assign delay times according to a scaled up array of consecutive primes
    size_t previousDelayDownscaled = downscaledFirstDelay;
    for(size_t i=1; i<This->numDelays; i++){
        size_t currentDelayDownscaled = nextPrime(previousDelayDownscaled);
        This->delayLengths[i] = (float)currentDelayDownscaled * fullSizeScale;
        previousDelayDownscaled = currentDelayDownscaled;
    }
    
    // randomly jitter the delay times
    for(size_t i=0; i<This->numDelays-1; i++){
        // find the distance between the ith delay and its next neighbor
        size_t range = This->delayLengths[i+1] - This->delayLengths[i];

        // generate a random number in range
        size_t jitter = arc4random() % range;

        This->delayLengths[i] += jitter;
    }
    // randomly jitter the last delay time
    size_t maxDelayLength = This->maxDelayS  *This->sampleRate;
    size_t range = maxDelayLength - This->delayLengths[This->numDelays - 2];
    size_t jitter = arc4random() % range;
    This->delayLengths[This->numDelays - 1] += jitter;
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
}




void pseudoRandomInRange(size_t* delayTimes, float min, float max, size_t numDelays){
    // randomly choose to skew left (-1) or right (1)
    float skew = (arc4random() % 2 == 0) ? -1.0f : 1.0f;
    // force the skew toward the positive side
    //skew = 1.0;
    
    if(numDelays == 1){
        delayTimes[0] = (size_t)round(min + (max - min) * (0.5f + skew*0.25f));
        return;
    }
    if(numDelays == 2){
        float pivot = min + (max-min) * (0.5f + skew*(1.0f / 6.0f));
        pseudoRandomInRange(delayTimes, min, pivot, numDelays/2);
        pseudoRandomInRange(delayTimes+(numDelays/2), pivot, max, numDelays/2);
        return;
    }
    if(numDelays == 4){
        float pivot = min + (max-min) * (0.5f + skew*(3.0f / 20.0f));
        pseudoRandomInRange(delayTimes, min, pivot, numDelays/2);
        pseudoRandomInRange(delayTimes+(numDelays/2), pivot, max, numDelays/2);
        return;
    }
    if(numDelays == 8){
        float pivot = min + (max-min) * (0.5f + skew*(0.121567f));
        pseudoRandomInRange(delayTimes, min, pivot, numDelays/2);
        pseudoRandomInRange(delayTimes+(numDelays/2), pivot, max, numDelays/2);
        return;
    }
    if(numDelays == 16){
        float pivot = min + (max-min) * (0.5f + skew*(0.0923872f));
        pseudoRandomInRange(delayTimes, min, pivot, numDelays/2);
        pseudoRandomInRange(delayTimes+(numDelays/2), pivot, max, numDelays/2);
        return;
    }
    if(numDelays == 32){
        float pivot = min + (max-min) * (0.5f + skew*(0.0678465f));
        pseudoRandomInRange(delayTimes, min, pivot, numDelays/2);
        pseudoRandomInRange(delayTimes+(numDelays/2), pivot, max, numDelays/2);
        return;
    }
    if(numDelays == 64){
        float pivot = min + (max-min) * (0.5f + skew*(0.0488923f));
        pseudoRandomInRange(delayTimes, min, pivot, numDelays/2);
        pseudoRandomInRange(delayTimes+(numDelays/2), pivot, max, numDelays/2);
        return;
    }
    assert(numDelays <= 64);
}





void BMSimpleFDN_pseudoRandomDelayTimes(BMSimpleFDN *This){
    pseudoRandomInRange(This->delayLengths,
                        This->minDelayS*This->sampleRate,
                        This->maxDelayS*This->sampleRate,
                        This->numDelays);
    
    // jitter the delay times like velvet noise
    for(size_t i=0; i<This->numDelays-1; i++){
        // find the distance between the ith delay and its next neighbor
        size_t range = This->delayLengths[i+1] - This->delayLengths[i];

        // generate a random number in range
        size_t jitter = arc4random() % range;

        This->delayLengths[i] += jitter;
    }
    // randomly jitter the last delay time
    size_t maxDelayLength = This->maxDelayS  *This->sampleRate;
    size_t jitter = arc4random() % (maxDelayLength - This->delayLengths[This->numDelays-2]);
    This->delayLengths[This->numDelays-1] += jitter;
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
}





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
    
    // return true if x shares a common factor with any of the first numElements
    for(size_t i=0; i<numElements; i++)
        if(gcd_ui(A[i],x) != 1)
            return true;
    
    // return false if not found
    return false;
}




void BMSimpleFDN_randomDelayTimes(BMSimpleFDN *This){
    
    // convert min and max delay from seconds to samples
    size_t minDelay = This->minDelayS  *This->sampleRate;
    size_t maxDelay = This->maxDelayS  *This->sampleRate;
    
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
            if(!containsX(This->delayLengths,This->delayLengths[i],i))
                generatedUniqueNewEntry = true;
        }
    }
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
}





void BMSimpleFDN_randomDelayTimesFixedTotal(BMSimpleFDN *This){
    
    // convert min and max delay from seconds to samples
    size_t minDelay = This->minDelayS  *This->sampleRate;
    size_t maxDelay = This->maxDelayS  *This->sampleRate;
    
    // find the range of delay times in samples
    size_t delayRange = maxDelay - minDelay;
    
    // assert that there are enough integers between min and max to give each
    // delay a unique length
    assert(delayRange >= This->numDelays);
    
    // set the total randomisation, which is the total amount by which the whole
    // set of delays can exceed minDelay
    float averageRandomisation = (maxDelay - minDelay)/2.0f;
    float totalRandomisation = This->numDelays * averageRandomisation;
    
    // set the first delay time
    size_t randomisation = (arc4random() % (delayRange + 1));
    This->delayLengths[0] = minDelay + randomisation;
    totalRandomisation -= randomisation;
    
    // generate the remaining random delay lengths
    for(size_t i=1; i<This->numDelays; i++){
        bool generatedUniqueNewEntry = false;
        while(!generatedUniqueNewEntry){
            // generate a new random delay in the specified range
            size_t randomisationRange = totalRandomisation / (This->numDelays - i);
            size_t randomisation = (arc4random() % (randomisationRange + 1));
            This->delayLengths[i] = minDelay + randomisation;
            // if the entry we just generated is unique, stop searching
            if(!containsX(This->delayLengths,This->delayLengths[i],i)){
                generatedUniqueNewEntry = true;
                totalRandomisation -= randomisation;
            }
        }
    }
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
}





size_t randomPrimeInRange(size_t min, size_t max){
    size_t random = 6;
    while(!isPrimeMr(random))
        random = min + arc4random() % (max-min + 1);
    return random;
}





void BMSimpleFDN_randomPrimesDelayTimes(BMSimpleFDN *This){
    
    // convert min and max delay from seconds to samples
    size_t minDelay = This->minDelayS  *This->sampleRate;
    size_t maxDelay = This->maxDelayS  *This->sampleRate;
    
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
            This->delayLengths[i] = randomPrimeInRange(minDelay,maxDelay);
            // if the entry we just generated is unique, stop searching
            if(!containsX(This->delayLengths,This->delayLengths[i],i))
                generatedUniqueNewEntry = true;
        }
    }
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
}





void BMSimpleFDN_relativelyPrimeDelayTimes(BMSimpleFDN *This){
    
    // convert min and max delay from seconds to samples
    size_t minDelay = This->minDelayS  *This->sampleRate;
    size_t maxDelay = This->maxDelayS  *This->sampleRate;
    
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
            if(!containsCommonFactor(This->delayLengths,This->delayLengths[i],i))
                generatedUniqueNewEntry = true;
        }
    }
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
}






/*!
 *containsRedundantSum
 *
 * @returns true if x + A[i] = A[j] + A[k], for some i,j,k
 */
bool containsRedundantSum(size_t* A, size_t x, size_t numElements){
    
    // allocate memory to store the sum of x with all other elements
    size_t *xSums = malloc(sizeof(size_t) * numElements);
    
    // compute the sum of x with all other elements
    for(size_t i=0; i<numElements; i++)
        xSums[i] = x + A[i];
    
    // compute the sum of all other elements with themselves
    for(size_t i=0; i<numElements; i++)
        for(size_t j=0; j<=i; j++)
            if(containsX(xSums, A[i] + A[j], numElements)){
                free(xSums);
                return true;
            }
    
    
    // return false if not found
    free(xSums);
    return false;
}




/*!
 *containsRedundantCFSum
 *
 * @returns true if gcd(x + A[i], A[j] + A[k]) > 1, for some i,j,k
 */
bool containsRedundantCFSum(size_t* A, size_t x, size_t numElements){
    
    // allocate memory to store the sum of x with all other elements
    size_t *xSums = malloc(sizeof(size_t) * numElements);
    
    // compute the sum of x with all other elements
    for(size_t i=0; i<numElements; i++)
        xSums[i] = x + A[i];
    
    // check for common factors between xSums ans all other existing delayTimes
    for(size_t i=0; i<numElements; i++)
        for(size_t j=0; j<=i; j++)
            if(containsCommonFactor(xSums, A[i] + A[j], numElements)){
                free(xSums);
                return true;
            }
    
    
    // return false if not found
    free(xSums);
    return false;
}




void BMSimpleFDN_uniqueSumsDelayTimes(BMSimpleFDN *This){
    
    // convert min and max delay from seconds to samples
    size_t minDelay = This->minDelayS  *This->sampleRate;
    size_t maxDelay = This->maxDelayS  *This->sampleRate;
    
    // find the range of delay times in samples
    size_t delayRange = maxDelay - minDelay;
    
    // assert that there are enough integers between min and max to give each
    // delay a unique length
    assert(delayRange >= This->numDelays);
    
    // generate the first delay time randomly
    This->delayLengths[0] = minDelay + (arc4random() % (delayRange + 1));
    
    // generate random delay lengths
    bool done = false;
    while(!done){
        bool stuck = false;
        size_t attempts = 0;
        for(size_t i=1; i<This->numDelays && !stuck; i++){
            bool generatedUniqueNewEntry = false;
            while(!generatedUniqueNewEntry & !stuck){
                // generate a new random delay in the specified range
                This->delayLengths[i] = minDelay + (arc4random() % (delayRange + 1));
                // if the entry we just generated is relatively prime to all the
                // previous ones, stop
                if(!containsRedundantSum(This->delayLengths,This->delayLengths[i],i))
                    generatedUniqueNewEntry = true;
                else
                    attempts++;
                if(attempts > UNIQUESUMS_ATTEMPTS_LIMIT)
                    stuck = true;
            }
        }
        if(!stuck)
            done = true;
    }
    
    // randomly assign delay tap signs
    BMSimpleFDN_randomSigns(This->outputTapSigns, This->numDelays);
}






void BMSimpleFDN_init(BMSimpleFDN *This,
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
    if(method == DTM_LOGVELVETNOISE)
        BMSimpleFDN_logVelvetNoiseDelayTimes(This);
    if(method == DTM_UNIQUESUMS)
        BMSimpleFDN_uniqueSumsDelayTimes(This);
    if(method == DTM_RANDOMPRIMES)
        BMSimpleFDN_randomPrimesDelayTimes(This);
    if(method == DTM_SCALEDPRIMES)
        BMSimpleFDN_scaledPrimeDelayTimes(This);
    if(method == DTM_PSEUDORANDOM)
        BMSimpleFDN_pseudoRandomDelayTimes(This);
    if(method == DTM_RANDOMFIXEDTOTAL)
        BMSimpleFDN_randomDelayTimesFixedTotal(This);

    
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






float BMSimpleFDN_processSample(BMSimpleFDN *This, float input){
    float output = 0;
    
    // read from the delays
    for(size_t i=0; i<This->numDelays; i++){
        // read output from each delay into a temp buffer
        float readSample = This->delays[i][This->rwIndices[i]];
        
        // attenuate and write to buffer1
        This->buffer1[i] = readSample  *This->attenuationCoefficients[i];
        
        // sum to output
        output += This->buffer1[i]  *This->outputTapSigns[i];
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







void BMSimpleFDN_processBuffer(BMSimpleFDN *This,
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






void BMSimpleFDN_free(BMSimpleFDN *This){
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






void BMSimpleFDN_impulseResponse(BMSimpleFDN *This, float* IR, size_t numSamples){
    // process the initial impulse
    IR[0] = BMSimpleFDN_processSample(This, 1.0f);
    
    // process the remaining zeros
    for(size_t i=1; i<numSamples; i++)
        IR[i] = BMSimpleFDN_processSample(This, 0.0f);
}
