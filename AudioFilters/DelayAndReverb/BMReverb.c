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
#include <mactypes.h>
#include "BMSorting.h"


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
    void BMReverbRandomiseOrderST(size_t* list, size_t seed, size_t stride, size_t length);
	void BMReverbRandomiseOrderF(float* list, size_t seed, size_t stride, size_t length);
    void BMReverbInitDelayOutputSigns(struct BMReverb *This);
    void BMReverbUpdateSettings(struct BMReverb *This);
	void BMReverbSetMidScoopGain(struct BMReverb *This, float gainDb);
    
    
    
    
    /*
     * Initialization: this MUST be called before running the reverb
     */
    void BMReverbInit(struct BMReverb *This, float sampleRate){
        // initialize all pointers to NULL
        BMReverbPointersToNull(This);
        
        // initialize the main filter setup
        BMMultiLevelBiquad_init(&This->mainFilter, 3, sampleRate, true, true, false);
		
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
		BMReverbSetMidScoopGain(This,BMREVERB_MID_SCOOP_GAIN);
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
        BMReverbRandomiseOrderF(((float*)&This->delayOutputSigns), seed, 2, This->halfNumDelays);
        //right
        BMReverbRandomiseOrderF(((float*)&This->delayOutputSigns)+1, seed+1, 2,  This->halfNumDelays);
    }
    
    
    
    
    void BMReverbUpdateRT60DecayTime(struct BMReverb *This){
        for (size_t i=0; i<This->numDelays; i++){
            // set gain for normal operation
            This->decayGainAttenuation[i/4][i%4] = BMReverbDelayGainFromRT60(This->rt60, (float)This->bufferLengths[i]/(float)This->sampleRate);
        }
    }
    
    
    
    
    
    void BMReverbSetSampleRate(struct BMReverb *This, float sampleRate){
        This->sampleRate = sampleRate;
        This->settingsQueuedForUpdate = true;
    }
    
    
    
	double BMReverbDelayGainFromRT60(double rt60, double delayTime){
		return pow(10.0, (-3.0 * delayTime) / rt60 );
	}
	
	
	
	double BMReverbDelayGainFromRT30(double rt30, double delayTime){
		return pow(10.0, (-2.0 * delayTime) / rt30 );
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
        BMFirstOrderArray4x4_setHighDecayFDN(&This->HSFArray, This->bufferLengths, This->highShelfFC, This->rt60, This->rt60/This->hfDecayMultiplier, This->numDelays);
    }
    
    
    
    
    // updates the low shelf filters after a change in FC or gain
    void BMReverbUpdateDecayLowShelfFilters(struct BMReverb *This){
        BMFirstOrderArray4x4_setLowDecayFDN(&This->LSFArray, This->bufferLengths, This->lowShelfFC, This->rt60, This->rt60/This->lfDecayMultiplier, This->numDelays);
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

	
	
	
	
		
	/*!
	 *BMReverbRandomInRangeUI
	 *
	 * @returns a random number in [min,max]
	 */
    size_t BMReverbRandomInRangeUI(size_t min, size_t max){
        size_t output = min + (size_t)arc4random_uniform(1+(UInt32)max-(UInt32)min);
		assert(output >= min && output <=max);
		return output;
    }
	
	
	
	
	/*!
	 *BMReverbContains
	 *
	 * @returns true if x is in the first length elements of A
	 */
	bool BMReverbContains(size_t *A, int64_t x, size_t length){
		for(size_t i=0; i<length; i++)
			if((int64_t)A[i]==x) return true;
		return false;
	}
    
	
	double BMReverbDelayGainFromRT30(double rt30, double delayTime){
		return pow(10.0, (-2.0 * delayTime) / rt30 );
	}
	
	
	double BMReverbDelayGainFromRT60(double rt60, double delayTime){
		return pow(10.0, (-3.0 * delayTime) / rt60 );
	}
	
    
	
	/*!
	 *BMReverbRandomsInRange
	 *
	 * @abstract generate length random floats between min and max, ensuring that the average is (max-min)/2 and that there will be values equal to min and max
	 */
	void BMReverbRandomsInRange(size_t min, size_t max, size_t *randomOutput, size_t length){
		// assert that it is possible to generate length unique values
		assert(max - min >= length);
		
		// assign the first value to min and asssign innerLength to reflect the
		// number of values still left to generate
		randomOutput[0] = min;
		size_t innerLength = length - 1;
		
		// if length is more than one, assign the last value to max and decrement innerLength
		if(length > 1){
			randomOutput[length-1] = max;
			innerLength--;
		}
		
		// we will not touch the first and last values from now on so we compute
		// new bounds to ensure that we don't modify the values already computed
		size_t innerMin = min+1;
		size_t innerMax = max-1;
		size_t* innerOutput = randomOutput + 1;
		
		// set the remaining values equal to the midpoint
		size_t midPoint = (innerMax + innerMin) / 2;
		for(size_t i=0; i<innerLength; i++)
			innerOutput[i] = midPoint;
		
		// Randomise the positions of the remaining values. The method below
		// maintains the mean delay time because it always shifts values in
		// pairs such that one value decreases by n samples while the other
		// increases by the same amount.
		// We start the count from i=1 so that no shifts occur when there are
		// only three delay lines because with three delays no randomisation is
		// possible without affecting the mean, since we have already fixed the
		// min and max values.
		size_t spread = innerMax - innerMin;
		for(size_t i=1; i<innerLength; i++){
			// continue attempting random shifts until we find a shift that
			// stays in bounds and doesn't duplicate any delay lengths
			bool didShift = false;
			while(!didShift){
				// select an element at random
				int64_t randomIndex = arc4random_uniform((uint32_t)innerLength);
				// generate a random shift in [-spread, spread]
				int randomShift = (int)arc4random_uniform((uint32_t)spread*2) - (int)spread;
				
				// check the validity of the proposed shift on the pair {i,randomIndex}
				//
				// can we shift the ith element like this?
				int64_t newLengthForI = innerOutput[i] + randomShift;
				int64_t newLengthForRandomIndex = innerOutput[randomIndex] - randomShift;
				bool canShift = true;
				// can't shift if randomIndex == i or randomShift == 0
				if(randomIndex == i || randomShift == 0)
					canShift = false;
				// can't shift the ith element out of bounds
				if(newLengthForI <= innerMin || newLengthForI >= innerMax)
					canShift = false;
				// can't shift the ith element if it would cause duplicate delay times
				if(BMReverbContains(innerOutput,newLengthForI,length))
					canShift = false;
				// can we shift the element at randomIndex like this?
				// can't shift it out of bounds
				if(newLengthForRandomIndex <= innerMin ||
				   newLengthForRandomIndex >= innerMax)
					canShift = false;
				// can't shift if it would cause duplicate delay times
				if(BMReverbContains(innerOutput,newLengthForRandomIndex,length))
					canShift = false;
				
				// if the pair of shifts is valid, do it
				if(canShift){
					innerOutput[i] = newLengthForI;
					innerOutput[randomIndex] = newLengthForRandomIndex;
					didShift = true;
				}
			}
		}
        
        // sort the output
        BMInsertionSort_size_t(randomOutput, length);
		
//		bool sorted = true;
//		for(size_t i=1; i<length; i++)
//			if(randomOutput[i]<randomOutput[i-1]) sorted = false;
//		if(!sorted)
//			printf("not sorted\n");
//		assert(sorted)
	}
    
	
	
    
    
    
    // generate a random list of times between min and max
    void BMReverbUpdateDelayTimes(struct BMReverb *This){
		
		// convert the min and max delay times to sample indices
		size_t minDelay = This->minDelay_seconds * This->sampleRate;
		size_t maxDelay = This->maxDelay_seconds * This->sampleRate;
        
		// generate random delay times int the specified range
        BMReverbRandomsInRange(minDelay, maxDelay,This->bufferLengths,This->numDelays);
        
		// sort so that we can ensure that the left and right channels get approximately equal average delay time
        BMInsertionSort_size_t(This->bufferLengths, This->numDelays);
        
        // randomise the order of the list of delay times
        // left channel
        BMReverbRandomiseOrderST(&This->bufferLengths[0], 1, 2, This->halfNumDelays);
        // right channel
        BMReverbRandomiseOrderST(&This->bufferLengths[1], 17, 2, This->halfNumDelays);
		
		// double-check the bounds
		for(size_t i=0; i<This->numDelays; i++)
			assert(This->bufferLengths[i] >= minDelay && This->bufferLengths[i] <= maxDelay);
		
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
    
    
    
    // randomise the order of a list of size_t uints
    void BMReverbRandomiseOrderST(size_t* list, size_t seed, size_t stride, size_t length){
        // seed the random number generator so we get the same result every time
        srand((int)seed);
        
        for (int i = 0; i<length; i++) {
            int j = (int)stride*(rand() % (int)(length/stride));
            
            // swap i with j
            size_t temp = list[i*stride];
            list[i*stride] = list[j];
            list[j] = temp;
        }
    }
	
	
	
	
	// randomise the order of a list of floats
	void BMReverbRandomiseOrderF(float* list, size_t seed, size_t stride, size_t length){
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
	
	
	
	void BMReverbSetMidScoopGain(struct BMReverb *This, float gainDb){
		BMMultiLevelBiquad_setBellQ(&This->mainFilter, BMREVERB_MID_SCOOP_FC, BMREVERB_MID_SCOOP_Q, gainDb, 2);
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
        BMMultiLevelBiquad_free(&This->mainFilter);
        
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
