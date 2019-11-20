//
//  BMReverb.c
//  A feedback delay network reverb for Macintosh and iOS
//
//  Created by bluemangoo@bluemangoo.com on 7/2/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#include "BMReverb.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "Constants.h"
#include <simd/simd.h>
#include "BMFastHadamard.h"


#ifdef __cplusplus
extern "C" {
#endif
    
#define BMREVERB_MATRIX_ATTENUATION 0.5 // keep the mixing unitary
    
    /*
     * these functions should be called only from functions within this file
     */
    void BMReverbInitIndices(struct BMReverb *This);
    void BMReverbIncrementIndices(struct BMReverb *This);
    void BMReverbUpdateDelayTimes(struct BMReverb *This);
    void BMReverbUpdateDecayHighShelfFilters(struct BMReverb *This);
    void BMReverbUpdateDecayLowShelfFilters(struct BMReverb *This);
    void BMReverbUpdateRT60DecayTime(struct BMReverb *This);
    double BMReverbDelayGainFromRT60(double rt60, double delayTime);
    void BMReverbProcessWetSample(struct BMReverb *This, float inputL, float inputR, float* outputL, float* outputR);
    void BMReverbUpdateNumDelayUnits(struct BMReverb *This);
    void BMReverbPointersToNull(struct BMReverb *This);
    void BMReverbRandomiseOrder(float* list, size_t seed, size_t stride, size_t length);
    void BMReverbInitDelayOutputSigns(struct BMReverb *This);
    void BMReverbUpdateSettings(struct BMReverb *This);
    
    
    
    
    /*
     * Initialization: this MUST be called before running the reverb
     */
    void BMReverbInit(struct BMReverb *This, float sampleRate){
        // initialize all pointers to NULL
        BMReverbPointersToNull(This);
        
        // initialize the main filter setup
        BMMultiLevelBiquad_init(&This->mainFilter, 2, sampleRate, true, true, false);
		
		// initialize the wet/dry mixer
		BMWetDryMixer_init(&This->wetDryMixer,sampleRate);
        
        // initialize default settings
        This->sampleRate = sampleRate;
        This->delayUnits = BMREVERB_NUMDELAYUNITS;
        This->highShelfFC = BMREVERB_HIGHSHELFFC;
        This->hfDecayMultiplier = BMREVERB_HFDECAYMULTIPLIER;
        This->lowShelfFC = BMREVERB_LOWSHELFFC;
        This->lfDecayMultiplier = BMREVERB_LFDECAYMULTIPLIER;
        This->minDelay_seconds = BMREVERB_PREDELAY;
        This->maxDelay_seconds = BMREVERB_ROOMSIZE;
        This->rt60 = BMREVERB_RT60;
        This->delayUnits = BMREVERB_NUMDELAYUNITS;
        This->settingsQueuedForUpdate = false;
        This->slowDecayRT60 = BMREVERB_SLOWDECAYRT60;
        This->newNumDelayUnits = BMREVERB_NUMDELAYUNITS;
        This->preDelayUpdate = false;
        BMReverbSetHighPassFC(This, BMREVERB_HIGHPASS_FC);
        BMReverbSetLowPassFC(This, BMREVERB_LOWPASS_FC);
        BMReverbSetWetMix(This, BMREVERB_WETMIX);
        BMReverbSetStereoWidth(This, BMREVERB_STEREOWIDTH);
        
        // initialize all the delays and delay-dependent settings
        BMReverbUpdateNumDelayUnits(This);
    }
    
    
    
    
    /*
     * works in place and allows left and right inputs to point to
     * the same data for mono to stereo operation
     */
    void BMReverbProcessBuffer(struct BMReverb *This, const float* inputL, const float* inputR, float* outputL, float* outputR, size_t numSamples){
        
		
        // don't process anything if there are nan values in the input
        if (isnan(inputL[0]) || isnan(inputR[0])) {
            memset(outputL, 0, sizeof(float)*numSamples);
            memset(outputR, 0, sizeof(float)*numSamples);
            return;
        }
        
        
        // chunked processing
        while (numSamples > 0) {
			size_t numSamplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples);
            
            // backup the input to allow in place processing
            memcpy(This->dryL, inputL, sizeof(float)*numSamplesProcessing);
            memcpy(This->dryR, inputR, sizeof(float)*numSamplesProcessing);
            
            // process the reverb to get the wet signal
            for (size_t i=0; i < numSamplesProcessing; i++)
                BMReverbProcessWetSample(This, inputL[i], inputR[i], &outputL[i], &outputR[i]);
			
			// narrow the stereo width
			BMStereoWidener_processAudio(&This->stereoWidth,
										 outputL, outputR,
										 outputL, outputR,
										 numSamplesProcessing);

			// filter the wet signal
			BMMultiLevelBiquad_processBufferStereo(&This->mainFilter,
												   outputL, outputR,
												   outputL, outputR,
												   numSamplesProcessing);

			// mix wet and dry signals
			BMWetDryMixer_processBufferRandomPhase(&This->wetDryMixer,
												   outputL, outputR,
												   This->dryL, This->dryR,
												   outputL, outputR,
												   numSamplesProcessing);
            
			// advance pointers
			inputL += numSamplesProcessing;
			inputR += numSamplesProcessing;
			outputL += numSamplesProcessing;
			outputR += numSamplesProcessing;
			numSamples -= numSamplesProcessing;
        }
        
		
        /*
         * if an update requiring memory allocation was requested, do it now.
         */
        if (This->settingsQueuedForUpdate)
            BMReverbUpdateSettings(This);
        if(This->preDelayUpdate){
            BMReverbUpdateDelayTimes(This);
            This->preDelayUpdate = false;
        }
    }
    
    
    
    void BMReverbUpdateSettings(struct BMReverb *This){
        BMReverbUpdateNumDelayUnits(This);
    }
    
    
    
    
    // this is the decay time of the reverb in normal operation
    void BMReverbSetRT60DecayTime(struct BMReverb *This, float rt60){
        assert(rt60 >= 0.0);
        This->rt60 = rt60;
        BMReverbUpdateRT60DecayTime(This);
        BMReverbUpdateDecayHighShelfFilters(This);
        BMReverbUpdateDecayLowShelfFilters(This);
    }
    
    
    
    
    // this is the decay time for when the hold pedal is down
    void BMReverbSetSlowRT60DecayTime(struct BMReverb *This, float slowRT60){
        assert(slowRT60 >= 0.0);
        This->slowDecayRT60 = slowRT60;
        BMReverbUpdateRT60DecayTime(This);
    }
    
    
    
    void BMReverbInitDelayOutputSigns(struct BMReverb *This){
        // init delay output signs with an equal number of + and - for each channel
        for(size_t i=0; i<This->delayUnits; i++){
            // L and R positive
            This->delayOutputSigns[i].xy = 1.0f;
            // L and R negative
            This->delayOutputSigns[i].zw = -1.0f;
        }
        
        // randomise the order of the signs for each channel
        unsigned int seed = 17;
        // left
        BMReverbRandomiseOrder(((float*)&This->delayOutputSigns), seed, 2, This->halfNumDelays);
        //right
        BMReverbRandomiseOrder(((float*)&This->delayOutputSigns)+1, seed+1, 2,  This->halfNumDelays);
    }
    
    
    
    
    void BMReverbUpdateRT60DecayTime(struct BMReverb *This){
        for (size_t i=0; i<This->numDelays; i++){
            // set gain for normal operation
            This->decayGainAttenuation[i/4][i%4] = BMReverbDelayGainFromRT60(This->rt60, This->delayTimes[i]);
        }
    }
    
    
    
    
    
    void BMReverbSetSampleRate(struct BMReverb *This, float sampleRate){
        This->sampleRate = sampleRate;
        This->settingsQueuedForUpdate = true;
    }
    
    
    
    
    
    
    
    // sets the cutoff frequency of the high shelf filters that increase
    // the decay rate of high frequencies in the reverb.
    void BMReverbSetHFDecayFC(struct BMReverb *This, float fc){
        assert(fc <= 18000.0 && fc > 100.0f);
        This->highShelfFC = fc;
        BMReverbUpdateDecayHighShelfFilters(This);
    }
    //
    // same thing, for low frequencies
    void BMReverbSetLFDecayFC(struct BMReverb *This, float fc){
        assert(fc <= 18000.0 && fc > 100.0f);
        This->lowShelfFC = fc;
        BMReverbUpdateDecayHighShelfFilters(This);
    }
    
    
    
    
    
    // Multiplies the decay rate for high Frequencies
    //
    // RT60 decay time of frequencies above FC is
    //
    //     (RT60 time below fc)/multiplier
    //
    // So multiplier divides the RT60 time, thereby multiplying the decay rate
    //
    //
    // the formulae for calculating the filter coefficients are from Digital Filters for Everyone,
    // second ed., by Rusty Alred.  Section 2.3.10: Shelf Filters
    //
    void BMReverbSetHFDecayMultiplier(struct BMReverb *This, float multiplier){
        This->hfDecayMultiplier = multiplier;
        BMReverbUpdateDecayHighShelfFilters(This);
    }
    //
    //
    // same thing, for low frequencies
    void BMReverbSetLFDecayMultiplier(struct BMReverb *This, float multiplier){
        This->lfDecayMultiplier = multiplier;
        BMReverbUpdateDecayLowShelfFilters(This);
    }
    
    
    
    
    
    // updates the high shelf filters after a change in FC or gain
    void BMReverbUpdateDecayHighShelfFilters(struct BMReverb *This){
        BMFirstOrderArray4x4_setHighDecayFDN(&This->HSFArray, This->delayTimes, This->highShelfFC, This->rt60, This->rt60/This->hfDecayMultiplier, This->numDelays);
    }
    
    
    
    
    // updates the low shelf filters after a change in FC or gain
    void BMReverbUpdateDecayLowShelfFilters(struct BMReverb *This){
        BMFirstOrderArray4x4_setLowDecayFDN(&This->LSFArray, This->delayTimes, This->lowShelfFC, This->rt60, This->rt60/This->lfDecayMultiplier, This->numDelays);
    }
    
    
    
    
    
    // sets the length in seconds of the shortest and longest delay in the
    // network
    void BMReverbSetRoomSize(struct BMReverb *This,
                             float preDelay_seconds,
                             float roomSize_seconds){
        assert(preDelay_seconds > 0.0);
        assert(roomSize_seconds > 2.0 * preDelay_seconds);
        
        This->minDelay_seconds = preDelay_seconds;
        This->maxDelay_seconds = roomSize_seconds;
        
        This->preDelayUpdate = true;
    }
    
    
    
    
    // sets the amount of mixing between the two stereo channels
    void BMReverbSetStereoWidth(struct BMReverb *This, float width){
		BMStereoWidener_setWidth(&This->stereoWidth, width);
    }
    
    
    
    // wet and dry gain are balanced so that the total output level remains
    // constant as you as you adjust wet mix
    void BMReverbSetWetMix(struct BMReverb *This, float wetMix){
		BMWetDryMixer_setMix(&This->wetDryMixer, wetMix);
    }
    
    
    
    __inline void BMReverbIncrementIndices(struct BMReverb *This){
        // add one to each index
        for (size_t i=0; i<This->numDelays; i++) This->rwIndices[i]++;
        
        This->samplesTillNextWrap--;
        
        // if any rwIndices need to wrap back to the beginning, wrap them and calculate the time until the next wrap is needed
        if (This->samplesTillNextWrap==0) {
            This->samplesTillNextWrap = SIZE_MAX;
            
            for (size_t i=0; i<This->numDelays; i++) {
                // find the distance until we reach the end marker
                size_t distToEnd = This->bufferEndIndices[i] - This->rwIndices[i];
                
                // wrap if we reached the marker already
                if (distToEnd==0){
                    This->rwIndices[i] = This->bufferStartIndices[i];
                    distToEnd = This->bufferLengths[i];
                }
                
                // find the shortest distance to the next wrap
                if (This->samplesTillNextWrap > distToEnd) This->samplesTillNextWrap=distToEnd;
            }
        }
    }
    
    
    
    
    float BMReverbRandomInRange(float min, float max){
        double random = (double)arc4random() / (double)UINT32_MAX;
        return min + fmod(random, max - min);
    }
    
    
    
    
    
    /*!
     *BMReverbRandomsInRange
     *
     * @abstract generate length random floats between min and max, ensuring that the average is (max-min)/2 and that there will be a value equal to min
     */
    void BMReverbRandomsInRange(float min, float max, float* randomOutput, size_t length){
		// set the first value equal to min
		randomOutput[0] = min;
		
		// set the target mean half way between the two limits
        float targetMean = (max-min)/2.0f;
        float actualMean = min;
		
        size_t i;
        for(i=1; i<length-1; i++){
            float tempMin = min;
            float tempMax = max;
			
			// how much does the mean have to shift?
			float meanShift = targetMean - actualMean;
			// how much will the next random number influence the mean?
			float influence = 1.0f / ((float)i+1.0f);
			// how much does the mean of the next number have to shift, taking into account the influence of that number?
			meanShift /= influence;
			
            // if the actual mean is below the target mean, increase the min boundary
            if(actualMean < targetMean){
                // how much does the min have to shift in order to affect the desired mean shift on the next number?
                float minShift = meanShift * 2.0f;
                // set the min for the next random number
                tempMin = min + minShift;
            }
            else{
                // how much does the min have to shift in order to affect the desired mean shift on the next number?
                float maxShift = meanShift * 2.0f;
                // set the min for the next random number
                tempMax = max + maxShift;
				// prevent the max from exceeding the min
				if(tempMax < min)
					tempMax = min + 0.05 * (max - min);
            }
            
            // generate the next random number
            randomOutput[i] = BMReverbRandomInRange(tempMin, tempMax);
            
            // get the mean value of the numbers we have so far
            vDSP_meanv(randomOutput, 1, &actualMean, i+1);
        }
        
        // set the last number deterministically so that the actual mean is exactly equal to the target mean
        randomOutput[i] = targetMean + (targetMean - actualMean);
    }
    

    
    
    
    // https://rosettacode.org/wiki/Category:C
    void BMReverbInsertionSort(float* a, int n){
        for(size_t i = 1; i < n; ++i) {
            float tmp = a[i];
            size_t j = i;
            while(j > 0 && tmp < a[j - 1]) {
                a[j] = a[j - 1];
                --j;
            }
            a[j] = tmp;
        }
    }
    
	
	/*!
	 *BMReverbContains
	 *
	 * @returns true if x is found in the  first length elements of A
	 */
	bool BMReverbContains(size_t *A, size_t x, size_t length){
		for(size_t i=0; i<length; i++)
			if(A[i]==x) return true;
		return false;
	}
	
	
    
	
	/*!
	 *BMReverbEliminateDuplicates
	 *
	 * @abstract eliminates duplicates by incrementing until there are none left
	 */
	void BMReverbEliminateDuplicates(size_t *A, size_t length){
		for(size_t i=1; i<length; i++)
			while(BMReverbContains(A,A[i],i))
				A[i]++;
	}
	
	
    
    
    
    // generate an evenly spaced but randomly jittered list of times between min and max
    void BMReverbUpdateDelayTimes(struct BMReverb *This){
        
		// generate random delay times int the specified range
        BMReverbRandomsInRange(This->minDelay_seconds, This->maxDelay_seconds,This->delayTimes,This->numDelays);
        
		// sort so that we can ensure that the left and right channels get approximately equal average delay time
        BMReverbInsertionSort(This->delayTimes, (int)This->numDelays);
        
        // randomise the order of the list of delay times
        // left channel
        BMReverbRandomiseOrder(&This->delayTimes[0], 1, 2, This->halfNumDelays);
        // right channel
        BMReverbRandomiseOrder(&This->delayTimes[1], 17, 2, This->halfNumDelays);
        
        // convert times from milliseconds to samples
        for (size_t i = 0; i < This->numDelays; i++)
            This->bufferLengths[i] = (size_t)round(This->sampleRate*This->delayTimes[i]);
		
		// ensure that no two delays have the same length
		BMReverbEliminateDuplicates(This->bufferLengths,This->numDelays);
		
		// count the total number of samples in all delays
		This->totalSamples = 0;
		for(size_t i=0; i < This->numDelays; i++)
			This->totalSamples += This->bufferLengths[i];
        
        // allocate memory for the main delays in the network
        if (This->delayLines) free(This->delayLines);
        This->delayLines = calloc(This->totalSamples,sizeof(float));
        
        // The following depend on delay time and have to be updated
        // whenever there is a change
        BMReverbUpdateRT60DecayTime(This);
        BMReverbUpdateDecayHighShelfFilters(This);
        BMReverbUpdateDecayLowShelfFilters(This);
        BMReverbInitIndices(This);
    }
    
    
    
    
    
    void BMReverbSetNumDelayUnits(struct BMReverb *This, size_t delayUnits){
        This->newNumDelayUnits = delayUnits;
        This->settingsQueuedForUpdate = true;
    }
    
    
    
    
    
    void BMReverbUpdateNumDelayUnits(struct BMReverb *This){
        
        // before beginning, calculate some frequently reused values
        This->delayUnits = This->newNumDelayUnits;
        This->numDelays = This->newNumDelayUnits*4;
        This->halfNumDelays = This->numDelays/2;
        This->fourthNumDelays = This->numDelays/4;
		
        // we compute attenuation on half delays because the reverb is stereo
        This->inputAttenuation = 1.0f/sqrtf((float)This->halfNumDelays);
        
        // free old memory if necessary
        if (This->leftOutputTemp) BMReverbFree(This);
        
        // allocate memory for buffers
        This->leftOutputTemp = malloc(BM_BUFFER_CHUNK_SIZE*sizeof(float));
        This->dryL = malloc(BM_BUFFER_CHUNK_SIZE*sizeof(float));
        This->dryR = malloc(BM_BUFFER_CHUNK_SIZE*sizeof(float));
		
		// clear the feedback buffers
		memset((float*)This->feedbackBuffers,0,sizeof(float)*This->numDelays);
        
        // set randomised signs for the output taps
        BMReverbInitDelayOutputSigns(This);
        
        // init shelf filter arrays for high and low frequency decay
        BMFirstOrderArray4x4_init(&This->HSFArray, This->numDelays, This->sampleRate);
        BMFirstOrderArray4x4_init(&This->LSFArray, This->numDelays, This->sampleRate);
        
        // Allocate memory for the network delay buffers and update all the
		// delay-time-dependent parameters
        BMReverbUpdateDelayTimes(This);
        
        This->settingsQueuedForUpdate = false;
    }
    
    
    
    // randomise the order of a list of floats
    void BMReverbRandomiseOrder(float* list, size_t seed, size_t stride, size_t length){
        // seed the random number generator so we get the same result every time
        srand((int)seed);
        
        for (int i = 0; i<length; i++) {
            int j = (int)stride*(rand() % (int)(length/stride));
            
            // swap i with j
            float temp = list[i*stride];
            list[i*stride] = list[j];
            list[j] = temp;
        }
    }
    
    
    
    void BMReverbInitIndices(struct BMReverb *This){
        size_t idx = 0;
        This->samplesTillNextWrap = SIZE_MAX;
        for (size_t i = 0; i<This->numDelays; i++) {
            // set the initial location of the rw pointer
            This->rwIndices[i] = idx;
            
            // set a mark at the start location
            This->bufferStartIndices[i] = idx;
            
            // set a mark at the end of the buffer (after the end)
            idx += This->bufferLengths[i];
            This->bufferEndIndices[i] = idx;
            
            // find the shortest distance until the next index wrap-around
            if (This->bufferLengths[i] < This->samplesTillNextWrap)
                This->samplesTillNextWrap = This->bufferLengths[i];
        }
    }
    
    
    // sets the cutoff frequency of a first order butterworth highpass
    // filter on the wet signal.  (that's 6db cutoff slope).  This does
    // not affect the dry signal at all.
    void BMReverbSetHighPassFC(struct BMReverb *This, float fc){
        BMMultiLevelBiquad_setHighPass6db(&This->mainFilter, fc, 0);
    }
    
    
    // sets the cutoff frequency of a first order butterworth lowpass
    // filter on the wet signal.  (that's 6db cutoff slope).  This does
    // not affect the dry signal at all.
    void BMReverbSetLowPassFC(struct BMReverb *This, float fc){
        BMMultiLevelBiquad_setLowPass6db(&This->mainFilter, fc, 1);
    }
    
    
    
    void BMReverbPointersToNull(struct BMReverb *This){
        This->delayLines = NULL;
        This->leftOutputTemp = NULL;
        This->dryL = NULL;
        This->dryR = NULL;
    }
    
    
    
    
    void BMReverbFree(struct BMReverb *This){
        free(This->delayLines);
        free(This->leftOutputTemp);
        free(This->dryL);
        free(This->dryR);
        BMMultiLevelBiquad_destroy(&This->mainFilter);
        
        BMReverbPointersToNull(This);
    }
    
    
    
    
    
    // process a single sample of input from right and left channels
    // the output is 100% wet
    __inline void BMReverbProcessWetSample(struct BMReverb *This, float inputL, float inputR, float* outputL, float* outputR){
        
        // attenuate the input and set up a float4 vector to mix into the FDN
        simd_float4 attenuatedInput = simd_make_float4(inputL,inputR,inputL,inputR);
        attenuatedInput *= This->inputAttenuation;
        
        // mix input and apply broadband decay
        for(size_t i=0; i<This->fourthNumDelays; i++)
            This->feedbackBuffers[i] = (attenuatedInput + This->feedbackBuffers[i]) * This->decayGainAttenuation[i];
        
        // high frequency decay
		BMFirstOrderArray4x4_processSample(&This->HSFArray, This->feedbackBuffers, This->feedbackBuffers, This->fourthNumDelays);
        
        /*
         * write the mixture of input and feedback back into the delays
         */
        size_t j=0;
        for (size_t i=0; i < This->fourthNumDelays; i++){
            This->delayLines[This->rwIndices[j++]] = This->feedbackBuffers[i].x;
            This->delayLines[This->rwIndices[j++]] = This->feedbackBuffers[i].y;
            This->delayLines[This->rwIndices[j++]] = This->feedbackBuffers[i].z;
            This->delayLines[This->rwIndices[j++]] = This->feedbackBuffers[i].w;
        }
        
        
        /*
         * increment delay indices
         */
        BMReverbIncrementIndices(This);
        
        
        /*
         * read output from delays into the feedback buffers
         */
        // vDSP_vgathr indexes 1 as the first element of the array so we have to
        // add +1 to the reference to delayLines to compensate
        vDSP_vgathr(This->delayLines+1, This->rwIndices, 1, (float*)This->feedbackBuffers, 1, This->numDelays);
        
        
        /*
         * sum the delay line outputs to right and left channel outputs
         */
        *outputL = *outputR = 0.0f;
        for(size_t i=0; i<This->fourthNumDelays; i++){
            // apply the signs of the output taps
            simd_float4 temp = This->feedbackBuffers[i] * This->delayOutputSigns[i];
            // sum two elements to left output
            *outputL += temp.x + temp.z;
            // sum two elements to right output
            *outputR += temp.y + temp.w;
        }
        
        // mix the feedback
        BMBlockCirculantMix4x4(This->feedbackBuffers, This->feedbackBuffers);
		
		// attenuate to keep the feedback unitary
        for(size_t i=0; i<This->fourthNumDelays; i++)
            This->feedbackBuffers[i] *= BMREVERB_MATRIX_ATTENUATION;
    }
    
    
#ifdef __cplusplus
}
#endif
