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


#ifdef __cplusplus
extern "C" {
#endif
    
#define BMREVERB_MATRIXATTENUATION 0.5 // keep the mixing unitary
    
    /*
     * these functions should be called only from functions within this file
     */
    void BMReverbInitIndices(struct BMReverb* rv);
    void BMReverbIncrementIndices(struct BMReverb* rv);
    void BMReverbUpdateDelayTimes(struct BMReverb* rv);
    void BMReverbUpdateDecayHighShelfFilters(struct BMReverb* rv);
    void BMReverbUpdateDecayLowShelfFilters(struct BMReverb* rv);
    void BMReverbUpdateRT60DecayTime(struct BMReverb* rv);
    double BMReverbDelayGainFromRT60(double rt60, double delayTime);
    void BMReverbProcessWetSample(struct BMReverb* rv, float inputL, float inputR, float* outputL, float* outputR);
    void BMReverbUpdateNumDelayUnits(struct BMReverb* rv);
    void BMReverbPointersToNull(struct BMReverb* rv);
    void BMReverbRandomiseOrder(float* list, size_t seed, size_t length);
    void BMReverbInitDelayOutputSigns(struct BMReverb* rv);
    void BMReverbUpdateSettings(struct BMReverb* rv);
	void BlockCirculantMix4(simd_float4 *in,simd_float4 *out,simd_float1 matrixAttenuation,size_t numBlocks);
    
    
    
    
    
    
    /*
     * Initialization: this MUST be called before running the reverb
     */
    void BMReverbInit(struct BMReverb* rv, float sampleRate){
        // initialize all pointers to NULL
        BMReverbPointersToNull(rv);
        
        // initialize the main filter setup
        BMMultiLevelBiquad_init(&rv->mainFilter, 2, rv->sampleRate, true, true, false);
        
        // initialize default settings
        rv->sampleRate = sampleRate;
        rv->delayUnits = BMREVERB_NUMDELAYUNITS;
        rv->matrixAttenuation = BMREVERB_MATRIXATTENUATION;
        rv->highShelfFC = BMREVERB_HIGHSHELFFC;
        rv->hfDecayMultiplier = BMREVERB_HFDECAYMULTIPLIER;
        rv->lowShelfFC = BMREVERB_LOWSHELFFC;
        rv->lfDecayMultiplier = BMREVERB_LFDECAYMULTIPLIER;
        rv->minDelay_seconds = BMREVERB_PREDELAY;
        rv->maxDelay_seconds = BMREVERB_ROOMSIZE;
        rv->rt60 = BMREVERB_RT60;
        rv->delayUnits = BMREVERB_NUMDELAYUNITS;
        rv->settingsQueuedForUpdate = false;
        rv->slowDecayRT60 = BMREVERB_SLOWDECAYRT60;
        rv->newNumDelayUnits = BMREVERB_NUMDELAYUNITS;
        rv->preDelayUpdate = false;
        BMReverbSetHighPassFC(rv, BMREVERB_HIGHPASS_FC);
        BMReverbSetLowPassFC(rv, BMREVERB_LOWPASS_FC);
        BMReverbSetWetGain(rv, BMREVERB_WETMIX);
        BMReverbSetCrossStereoMix(rv, BMREVERB_CROSSSTEREOMIX);

        
        // initialize all the delays and delay-dependent settings
        BMReverbUpdateNumDelayUnits(rv);
    }
    
    
    
    
    /*
     * works in place and allows left and right inputs to point to
     * the same data for mono to stereo operation
     */
    void BMReverbProcessBuffer(struct BMReverb* rv, const float* inputL, const float* inputR, float* outputL, float* outputR, size_t numSamples){
        
        // don't process anything if there are nan values in the input
        if (isnan(inputL[0]) || isnan(inputR[0])) {
            memset(outputL, 0, sizeof(float)*numSamples);
            memset(outputR, 0, sizeof(float)*numSamples);
            return;
        }
        
        
        
        // this requires buffer memory so we do it in limited sized chunks to
        // avoid having to adjust the buffer length at runtime
        size_t samplesLeftToMix = numSamples;
        size_t samplesMixingNext = BM_MIN(BM_BUFFER_CHUNK_SIZE,samplesLeftToMix);
        size_t bufferedProcessingIndex = 0;
        while (samplesLeftToMix != 0) {
            
            // backup the input to allow in place processing
            memcpy(rv->dryL, inputL+bufferedProcessingIndex, sizeof(float)*samplesMixingNext);
            memcpy(rv->dryR, inputR+bufferedProcessingIndex, sizeof(float)*samplesMixingNext);
            
            
            
            // process the reverb to get the wet signal
            for (size_t i=bufferedProcessingIndex; i < bufferedProcessingIndex+samplesMixingNext; i++)
                BMReverbProcessWetSample(rv, inputL[i], inputR[i], &outputL[i], &outputR[i]);
            
            
            // mix R and L wet signals
            // mix left and right to left temp
            vDSP_vsmsma(outputL+bufferedProcessingIndex, 1, &rv->straightStereoMix, outputR+bufferedProcessingIndex, 1, &rv->crossStereoMix, rv->leftOutputTemp, 1, samplesMixingNext);
            // mix right and left to right
            vDSP_vsmsma(outputR+bufferedProcessingIndex, 1, &rv->straightStereoMix, outputL+bufferedProcessingIndex, 1, &rv->crossStereoMix, outputR+bufferedProcessingIndex, 1, samplesMixingNext);
            // copy left temp back to left output
            memcpy(outputL+bufferedProcessingIndex, rv->leftOutputTemp, sizeof(float)*samplesMixingNext);
            
            
            
            // mix dry and wet signals
            vDSP_vsmsma(rv->dryL, 1, &rv->dryGain, outputL+bufferedProcessingIndex, 1, &rv->wetGain, outputL+bufferedProcessingIndex, 1, samplesMixingNext);
            vDSP_vsmsma(rv->dryR, 1, &rv->dryGain, outputR+bufferedProcessingIndex, 1, &rv->wetGain, outputR+bufferedProcessingIndex, 1, samplesMixingNext);
            
            
            
            // update the number of samples left to process in the buffer
            samplesLeftToMix -= samplesMixingNext;
            bufferedProcessingIndex += samplesMixingNext;
            samplesMixingNext = BM_MIN(BM_BUFFER_CHUNK_SIZE,samplesLeftToMix);
        }
        
        
        
        // filter the wet output signal (highpass and lowpass)
        BMMultiLevelBiquad_processBufferStereo(&rv->mainFilter, outputL, outputR, outputL, outputR, numSamples);
        
        
        
        /*
         * if an update requiring memory allocation was requested, do it now.
         */
        if (rv->settingsQueuedForUpdate)
            BMReverbUpdateSettings(rv);
        
        //TODO -> cause the crash
        if(rv->preDelayUpdate){
            BMReverbUpdateDelayTimes(rv);
            rv->preDelayUpdate = false;
        }
    }
    
    
    
    void BMReverbUpdateSettings(struct BMReverb* rv){
        BMReverbUpdateNumDelayUnits(rv);
    }
    

    
    
    // this is the decay time of the reverb in normal operation
    void BMReverbSetRT60DecayTime(struct BMReverb* rv, float rt60){
        assert(rt60 >= 0.0);
        rv->rt60 = rt60;
        BMReverbUpdateRT60DecayTime(rv);
        BMReverbUpdateDecayHighShelfFilters(rv);
        BMReverbUpdateDecayLowShelfFilters(rv);
    }
    
    
    
    
    // this is the decay time for when the hold pedal is down
    void BMReverbSetSlowRT60DecayTime(struct BMReverb* rv, float slowRT60){
        assert(slowRT60 >= 0.0);
        rv->slowDecayRT60 = slowRT60;
        BMReverbUpdateRT60DecayTime(rv);
    }
    
    
    
    void BMReverbInitDelayOutputSigns(struct BMReverb* rv){
        // init delay output signs with an equal number of + and - for each channel
        float one = 1.0, negativeOne = -1.0;
        // left
        vDSP_vfill(&one, rv->delayOutputSigns+0*rv->fourthNumDelays, 1, rv->fourthNumDelays);
        vDSP_vfill(&negativeOne, rv->delayOutputSigns+1*rv->fourthNumDelays, 1, rv->fourthNumDelays);
        // right
        vDSP_vfill(&one, rv->delayOutputSigns+2*rv->fourthNumDelays, 1, rv->fourthNumDelays);
        vDSP_vfill(&negativeOne, rv->delayOutputSigns+3*rv->fourthNumDelays, 1, rv->fourthNumDelays);
        
        // randomise the order of the signs for each channel
        unsigned int seed = 1;
        // left
        BMReverbRandomiseOrder(rv->delayOutputSigns, seed, rv->halfNumDelays);
        //right
        BMReverbRandomiseOrder(rv->delayOutputSigns+rv->halfNumDelays, seed, rv->halfNumDelays);
    }
    
    
    
    
    void BMReverbUpdateRT60DecayTime(struct BMReverb* rv){
        for (size_t i=0; i<rv->numDelays; i++){
            
            // set gain for normal operation
            rv->decayGainAttenuation[i] = BMReverbDelayGainFromRT60(rv->rt60, rv->delayTimes[i]);
        }
    }
    
    
    
    
    
    void BMReverbSetSampleRate(struct BMReverb* rv, float sampleRate){
        rv->sampleRate = sampleRate;
        rv->settingsQueuedForUpdate = true;
    }
    
    
    
    

    
    
    // sets the cutoff frequency of the high shelf filters that increase
    // the decay rate of high frequencies in the reverb.
    void BMReverbSetHFDecayFC(struct BMReverb* rv, float fc){
        assert(fc <= 18000.0 && fc > 100.0f);
        rv->highShelfFC = fc;
        BMReverbUpdateDecayHighShelfFilters(rv);
    }
    //
    // same thing, for low frequencies
    void BMReverbSetLFDecayFC(struct BMReverb* rv, float fc){
        assert(fc <= 18000.0 && fc > 100.0f);
        rv->lowShelfFC = fc;
        BMReverbUpdateDecayHighShelfFilters(rv);
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
    void BMReverbSetHFDecayMultiplier(struct BMReverb* rv, float multiplier){
        rv->hfDecayMultiplier = multiplier;
        BMReverbUpdateDecayHighShelfFilters(rv);
    }
    //
    //
    // same thing, for low frequencies
    void BMReverbSetLFDecayMultiplier(struct BMReverb* rv, float multiplier){
        rv->lfDecayMultiplier = multiplier;
        BMReverbUpdateDecayLowShelfFilters(rv);
    }
    
    
    
    
    
    // updates the high shelf filters after a change in FC or gain
    void BMReverbUpdateDecayHighShelfFilters(struct BMReverb* rv){
        BMBiquadArray4_setHighDecayFDN(&rv->HSFArray, rv->delayTimes, rv->highShelfFC, rv->rt60, rv->rt60/rv->hfDecayMultiplier, rv->numDelays);
    }
    
    
    
    
    // updates the low shelf filters after a change in FC or gain
    void BMReverbUpdateDecayLowShelfFilters(struct BMReverb* rv){
        BMBiquadArray4_setLowDecayFDN(&rv->LSFArray, rv->delayTimes, rv->lowShelfFC, rv->rt60, rv->rt60/rv->lfDecayMultiplier, rv->numDelays);
    }
    
    
    
    
    
    // sets the length in seconds of the shortest and longest delay in the
    // network
    void BMReverbSetRoomSize(struct BMReverb* rv,
                             float preDelay_seconds,
                             float roomSize_seconds){
        assert(preDelay_seconds > 0.0);
        assert(roomSize_seconds > 2.0 * preDelay_seconds);
        
        rv->minDelay_seconds = preDelay_seconds;
        rv->maxDelay_seconds = roomSize_seconds;
        
        rv->preDelayUpdate = true;
    }
    
    
    
    
    // sets the amount of mixing between the two stereo channels
    void BMReverbSetCrossStereoMix(struct BMReverb* rv, float crossMix){
        assert(crossMix >= 0 && crossMix <=1);
        
        // maximum mix setting is equal amounts of L and R in both channels
        float maxMix = 1.0/sqrt(2.0);
        
        rv->crossStereoMix = crossMix*maxMix;
        rv->straightStereoMix = sqrtf(1.0f - crossMix*crossMix*maxMix*maxMix);
    }
    
    
    
    // wet and dry gain are balanced so that the total output level remains
    // constant as you as you adjust wet mix
    void BMReverbSetWetGain(struct BMReverb* rv, float wetGain){
        assert(wetGain >=0.0 && wetGain <=1.0);
        rv->wetGain = sinf(M_PI_2*wetGain);
        rv->dryGain = cosf(M_PI_2*wetGain);
    }
    
    
    
    __inline void BMReverbIncrementIndices(struct BMReverb* rv){
        // add one to each index
        for (size_t i=0; i<rv->numDelays; i++) rv->rwIndices[i]++;
        
        rv->samplesTillNextWrap--;
        
        // if any rwIndices need to wrap back to the beginning, wrap them and calculate the time until the next wrap is needed
        if (rv->samplesTillNextWrap==0) {
            rv->samplesTillNextWrap = SIZE_MAX;
            
            for (size_t i=0; i<rv->numDelays; i++) {
                // find the distance until we reach the end marker
                size_t distToEnd = rv->bufferEndIndices[i] - rv->rwIndices[i];
                
                // wrap if we reached the marker already
                if (distToEnd==0){
                    rv->rwIndices[i] = rv->bufferStartIndices[i];
                    distToEnd = rv->bufferLengths[i];
                }
                
                // find the shortest distance to the next wrap
                if (rv->samplesTillNextWrap > distToEnd) rv->samplesTillNextWrap=distToEnd;
            }
        }
    }
    
    
    
    
    
    // generate an evenly spaced but randomly jittered list of times between min and max
    void BMReverbUpdateDelayTimes(struct BMReverb* rv){
        
        float spacing = (rv->maxDelay_seconds - rv->minDelay_seconds) / (float)rv->halfNumDelays;
        
        // generate an evenly spaced list of times between min and max
        // left channel
        vDSP_vramp(&rv->minDelay_seconds, &spacing, rv->delayTimes, 1, rv->halfNumDelays);
        // right channel
        vDSP_vramp(&rv->minDelay_seconds, &spacing, rv->delayTimes+rv->halfNumDelays, 1, rv->halfNumDelays);
        
        // Seed the random number generator for consistency. Doing this ensures
        // that we get the same delay times every time we run the reverb.
        srand(111);
        
        // jitter the times so that the spacing is not perfectly even
        for (size_t i = 0; i < rv->numDelays; i++)
            rv->delayTimes[i] += spacing * ((float)rand() / (float) RAND_MAX);
        
        
        // randomise the order of the list of delay times
        // left channel
        BMReverbRandomiseOrder(rv->delayTimes, 17, rv->halfNumDelays);
        // right channel
        BMReverbRandomiseOrder(rv->delayTimes+rv->halfNumDelays, 4, rv->halfNumDelays);
        
        
        // convert times from milliseconds to samples and count the total
        rv->totalSamples = 0;
        for (size_t i = 0; i < rv->numDelays; i++) {
            rv->bufferLengths[i] = (size_t)round(rv->sampleRate*rv->delayTimes[i]);
            rv->totalSamples += rv->bufferLengths[i];
        }
        
        
        // allocate memory for the main delays in the network
        if (rv->delayLines) free(rv->delayLines);
        rv->delayLines = malloc(sizeof(float)*rv->totalSamples);
        vDSP_vclr(rv->delayLines,1, rv->totalSamples);
        
        // The following depend on delay time and have to be updated
        // whenever there is a change
        BMReverbUpdateRT60DecayTime(rv);
        BMReverbUpdateDecayHighShelfFilters(rv);
        BMReverbUpdateDecayLowShelfFilters(rv);
        BMReverbInitIndices(rv);
    }
    
    
    
    
    
    void BMReverbSetNumDelayUnits(struct BMReverb* rv, size_t delayUnits){
        rv->newNumDelayUnits = delayUnits;
        rv->settingsQueuedForUpdate = true;
    }
    
    
    
    
    
    void BMReverbUpdateNumDelayUnits(struct BMReverb* rv){
		assert(rv->newNumDelayUnits % 4 == 0);
		
        /*
         * before beginning, calculate some frequently reused values
         */
		rv->delayUnits = rv->newNumDelayUnits;
        rv->numDelays = rv->newNumDelayUnits*4;
        rv->halfNumDelays = rv->numDelays/2;
		rv->fourthNumDelays = rv->numDelays/4;
        // we compute attenuation on half delays because the reverb is stereo
        rv->inputAttenuation = 1.0f/sqrt((float)rv->halfNumDelays);
        
        // free old memory if necessary
        if (rv->bufferLengths) BMReverbFree(rv);
        
        
        /*
         * allocate memory for smaller buffers
         */
        rv->bufferLengths = malloc(sizeof(size_t)*rv->numDelays);
        rv->delayTimes = malloc(rv->numDelays*sizeof(float));
        rv->feedbackBuffers = malloc(rv->numDelays*sizeof(float));
        vDSP_vclr((float*)rv->feedbackBuffers, 1, rv->numDelays);
        rv->rwIndices = malloc(rv->numDelays*sizeof(size_t));
        rv->bufferStartIndices = malloc(rv->numDelays*sizeof(size_t));
        rv->bufferEndIndices = malloc(rv->numDelays*sizeof(size_t));
//        rv->mixingBuffers = malloc(rv->numDelays*sizeof(float));
        rv->decayGainAttenuation = malloc(rv->numDelays*sizeof(float));
        rv->leftOutputTemp = malloc(BM_BUFFER_CHUNK_SIZE*sizeof(float));
        rv->dryL = malloc(BM_BUFFER_CHUNK_SIZE*sizeof(float));
        rv->dryR = malloc(BM_BUFFER_CHUNK_SIZE*sizeof(float));
        rv->delayOutputSigns = malloc(rv->numDelays*sizeof(float));
        
        
        /*
         * set randomised signs for the output taps
         */
        BMReverbInitDelayOutputSigns(rv);
        
        
//        /*
//         * create pointers to facilitate mixing
//         */
//        rv->fb0 = (float*)rv->feedbackBuffers+(0*rv->fourthNumDelays);
//        rv->fb1 = (float*)rv->feedbackBuffers+(1*rv->fourthNumDelays);
//        rv->fb2 = (float*)rv->feedbackBuffers+(2*rv->fourthNumDelays);
//        rv->fb3 = (float*)rv->feedbackBuffers+(3*rv->fourthNumDelays);
//        rv->mb0 = rv->mixingBuffers+(0*rv->fourthNumDelays);
//        rv->mb1 = rv->mixingBuffers+(1*rv->fourthNumDelays);
//        rv->mb2 = rv->mixingBuffers+(2*rv->fourthNumDelays);
//        rv->mb3 = rv->mixingBuffers+(3*rv->fourthNumDelays);
        
        // init shelf filter arrays for high and low frequency decay
        BMBiquadArray4_init(&rv->HSFArray, rv->numDelays, rv->sampleRate);
        BMBiquadArray4_init(&rv->LSFArray, rv->numDelays, rv->sampleRate);
        
        /*
         * Allocate memory for the network delay buffers and update all the
         * delay-time-dependent parameters
         */
        BMReverbUpdateDelayTimes(rv);
        
        
        
        rv->settingsQueuedForUpdate = false;
    }
    
    
    
    // randomise the order of a list of floats
    void BMReverbRandomiseOrder(float* list, size_t seed, size_t length){
        // seed the random number generator so we get the same result every time
        srand((int)seed);
        
        for (int i = 0; i<length; i++) {
            int j = rand() % (int)length;
            
            // swap i with j
            float temp = list[i];
            list[i] = list[j];
            list[j] = temp;
        }
    }
    
    
    
    void BMReverbInitIndices(struct BMReverb* rv){
        size_t idx = 0;
        rv->samplesTillNextWrap = SIZE_MAX;
        for (size_t i = 0; i<rv->numDelays; i++) {
            // set the initial location of the rw pointer
            rv->rwIndices[i] = idx;
            
            // set a mark at the start location
            rv->bufferStartIndices[i] = idx;
            
            // set a mark at the end of the buffer (after the end)
            idx += rv->bufferLengths[i];
            rv->bufferEndIndices[i] = idx;
            
            // find the shortest distance until the next index wrap-around
            if (rv->bufferLengths[i] < rv->samplesTillNextWrap)
                rv->samplesTillNextWrap = rv->bufferLengths[i];
        }
    }
    
    
    // sets the cutoff frequency of a first order butterworth highpass
    // filter on the wet signal.  (that's 6db cutoff slope).  This does
    // not affect the dry signal at all.
    void BMReverbSetHighPassFC(struct BMReverb* rv, float fc){
        BMMultiLevelBiquad_setHighPass6db(&rv->mainFilter, fc, 0);
    }
    
    
    // sets the cutoff frequency of a first order butterworth lowpass
    // filter on the wet signal.  (that's 6db cutoff slope).  This does
    // not affect the dry signal at all.
    void BMReverbSetLowPassFC(struct BMReverb* rv, float fc){
        BMMultiLevelBiquad_setLowPass6db(&rv->mainFilter, fc, 0);
    }
    
    
    
    void BMReverbPointersToNull(struct BMReverb* rv){
        rv->delayLines = NULL;
        rv->bufferLengths = NULL;
        rv->feedbackBuffers = NULL;
        rv->bufferStartIndices = NULL;
        rv->bufferEndIndices = NULL;
        rv->rwIndices = NULL;
//        rv->mixingBuffers = NULL;
        rv->delayTimes = NULL;
        rv->decayGainAttenuation = NULL;
        rv->leftOutputTemp = NULL;
        rv->delayOutputSigns = NULL;
        rv->dryL = NULL;
        rv->dryR = NULL;
    }
    
    
    
    
    void BMReverbFree(struct BMReverb* rv){
        free(rv->delayLines);
        free(rv->bufferLengths);
        free(rv->feedbackBuffers);
        free(rv->bufferStartIndices);
        free(rv->bufferEndIndices);
        free(rv->rwIndices);
//        free(rv->mixingBuffers);
        free(rv->delayTimes);
        free(rv->decayGainAttenuation);
        free(rv->leftOutputTemp);
        free(rv->delayOutputSigns);
        free(rv->dryL);
        free(rv->dryR);
        BMMultiLevelBiquad_destroy(&rv->mainFilter);
        
        BMBiquadArray4_free(&rv->LSFArray);
        BMBiquadArray4_free(&rv->HSFArray);
        
        BMReverbPointersToNull(rv);
    }
    
	
	
	
	/*
	* Mix the feedback signal
	*
	* The code below does the first two stages of a fast hadamard transform,
	* followed by some swapping.
	* Leaving the transform incomplete is equivalent to using a
	* block-circulant mixing matrix. Typically, block circulant mixing is
	* done using the last two stages of the fast hadamard transform. Here
	* we use the first two stages instead because it permits us to do
	* vectorised additions and subtractions with a stride of 1.
	*
	* Regarding block-circulant mixing, see: https://www.researchgate.net/publication/282252790_Flatter_Frequency_Response_from_Feedback_Delay_Network_Reverbs
	*
	*
	* If you prefer a full fast Hadamard transform to this block-circulant
	* mixing, simply replace the code from here to the end of this function
	* with a call to BMFastHadamardTransform(...) as defined in
	* BMFastHadamard.h
	*
	*/
	__inline void BlockCirculantMix4(simd_float4 *in,
									 simd_float4 *out,
									 simd_float1 matrixAttenuation,
									 size_t numBlocks){
		size_t halfNumBlocks = numBlocks/2;
		size_t fourthNumBlocks = numBlocks/4;
		simd_float4 *inHalf = in + halfNumBlocks;
		simd_float4 *outHalf = out + halfNumBlocks;
		simd_float4 *outFourth = out + fourthNumBlocks;
		simd_float4 *outThreeFourths = outHalf + fourthNumBlocks;
		for(size_t i=0; i<halfNumBlocks; i++){
			// first half = first half plus second half
			out[i] = (in[i] + inHalf[i]) * matrixAttenuation;
			// second half = first half minus second half
			outHalf[i] = (in[i] - inHalf[i]) * matrixAttenuation;
		}
		for(size_t i=0; i<fourthNumBlocks; i++){
			// first fourth equals first forth + second fourth
			out[i] = out[i] + outFourth[i];
			// second fourth equals first forth - second fourth
			outFourth[i] = out[i] - outFourth[i];
			// third fourth equals third fourth + fourth fourth
			outHalf[i] = outHalf[i] + outThreeFourths[i];
			// fourth fourth equals third fourth - fourth fourth
			outThreeFourths[i] = outHalf[i] - outThreeFourths[i];
		}
		
		// rotate the output by one position so that the signal has to pass
		// numDelays/4 delays before completing the circuit
		simd_float1 lastElement = out[numBlocks-1].w;
		memmove(1 + (float*)out,out, numBlocks*sizeof(simd_float4));
		out[0].x = lastElement;
	}
	
	
	
    
    
    // process a single sample of input from right and left channels
    // the output is 100% wet
    __inline void BMReverbProcessWetSample(struct BMReverb* rv, float inputL, float inputR, float* outputL, float* outputR){
		
		// attenuate the input and set up a float4 vector to mix into the FDN
		simd_float4 attenuatedInput = simd_make_float4(inputL,inputR,inputL,inputR);
		attenuatedInput *= rv->inputAttenuation;
		
		// mix input and apply broadband decay
		for(size_t i=0; i<rv->fourthNumDelays; i++)
			rv->feedbackBuffers[i] = simd_muladd(rv->decayGainAttenuation[i], rv->feedbackBuffers[i], attenuatedInput);
        
        // high frequency decay
        BMBiquadArray4_processSample(&rv->HSFArray, rv->feedbackBuffers, rv->feedbackBuffers, rv->fourthNumDelays);
        
        // low frequency decay
        BMBiquadArray4_processSample(&rv->HSFArray, rv->feedbackBuffers, rv->feedbackBuffers, rv->fourthNumDelays);
        
        /*
         * write the mixture of input and feedback back into the delays
         */
		size_t j=0;
		for (size_t i=0; i < rv->fourthNumDelays; i++){
            rv->delayLines[rv->rwIndices[j++]] = rv->feedbackBuffers[i].x;
			rv->delayLines[rv->rwIndices[j++]] = rv->feedbackBuffers[i].y;
			rv->delayLines[rv->rwIndices[j++]] = rv->feedbackBuffers[i].z;
			rv->delayLines[rv->rwIndices[j++]] = rv->feedbackBuffers[i].w;
		}
        
        
        /*
         * increment delay indices
         */
        BMReverbIncrementIndices(rv);
        
        
        /*
         * read output from delays into the feedback buffers
         */
        // vDSP_vgathr indexes 1 as the first element of the array so we have to
        // add +1 to the reference to delayLines to compensate
        vDSP_vgathr(rv->delayLines+1, rv->rwIndices, 1, (float*)rv->feedbackBuffers, 1, rv->numDelays);
        
        
        /*
         * sum the delay line outputs to right and left channel outputs
         */
		*outputL = *outputR = 0.0f;
		for(size_t i=0; i<rv->fourthNumDelays; i++){
			// apply the signs of the output taps
			simd_float4 temp = rv->feedbackBuffers[i] * rv->delayOutputSigns[i];
			// sum two elements to left output
			*outputL += simd_reduce_add(temp.xz);
			// sum two elements to right output
			*outputR += simd_reduce_add(temp.yw);
		}
		
		// mix the feedback
		BlockCirculantMix4(rv->feedbackBuffers, rv->feedbackBuffers, rv->matrixAttenuation, rv->fourthNumDelays);
		for(size_t i=0; i<rv->numDelays; i++)
			assert(rv->rwIndices[i] < 138200);
	}
    
    
#ifdef __cplusplus
}
#endif
