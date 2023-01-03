//
//  BMVagusNerveTherapyFilter.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/13/22.
//  Copyright © 2022 BlueMangoo. All rights reserved.
//

#include "BMVagusNerveTherapyFilter.h"

#define BMVNTF_MIN_DB -16.0f
#define BMVNTF_MAX_SKIRT_DB_START -14.0f
#define BMVNTF_MAX_SKIRT_DB_END -3.5f
#define BMVNTF_END_TIME_HOURS 5.0
#define BMVNTF_FILTER_ONE_FC 858.854
#define BMVNTF_FILTER_TWO_FC 1408.85
#define BMVNTF_LFO_FC 1.0 / (6.0 * 60.0) // one cycle for every six minutes
#define BMVNTF_HIGHPASS_FC 350.0
#define BMVNTF_LOWPASS_FC 3000.0
#define BMVNTF_FIXED_FILTER_GAIN_COMPENSATION 3.5 // dB



void BMVagusNerveTherapyFilter_init(BMVagusNerveTherapyFilter *This, float sampleRate){
	bool stereo = true;
	size_t numSVFFilters = 4;
	size_t numFixedFilters = 2;
	float lfoFreq = BMVNTF_LFO_FC;
	float lfoMin = BMVNTF_MIN_DB;
	float lfoMax = BMVNTF_MAX_SKIRT_DB_START;
	This->getCoefficientsFromBiquadHelper = FALSE;
	This->filterWithBiquad = FALSE;
	
	BMMultiLevelSVF_init(&This->svf, numSVFFilters, sampleRate, stereo);
	BMMultiLevelBiquad_init(&This->fixedFilters, numFixedFilters, sampleRate, true, false, false);
	BMLFO_init(&This->lfo, lfoFreq, lfoMin, lfoMax, sampleRate);
	
	// set fixed filters to reduce low and high frequencies
	BMMultiLevelBiquad_setHighPass6db(&This->fixedFilters, BMVNTF_HIGHPASS_FC, 0);
	BMMultiLevelBiquad_setLowPass6db(&This->fixedFilters, BMVNTF_LOWPASS_FC, 1);
	
	// set the SVF to smoothly sweep the filter parameters
	BMMultiLevelSVF_enableFilterSweep(&This->svf, TRUE);
	
	This->timeSamples = 0;
	This->sampleRate = (size_t)sampleRate;
	
	// calculate how long the filter time will be in samples
	This->endTimeSamples = (size_t)round(BMVNTF_END_TIME_HOURS * 60.0 * 60.0 * (double)sampleRate);
	
	// initialise the LFO
	BMLFO_setMinMaxImmediately(&This->lfo, BMVNTF_MIN_DB, BMVNTF_MAX_SKIRT_DB_START);
}



void BMVagusNerveTherapyFilter_free(BMVagusNerveTherapyFilter *This){
	BMMultiLevelSVF_free(&This->svf);
	BMMultiLevelBiquad_free(&This->fixedFilters);
	BMLFO_free(&This->lfo);
}



void BMVagusNerveTherapyFilter_getCoefficientsFromBiquad(BMVagusNerveTherapyFilter *This, bool enabled){
	This->getCoefficientsFromBiquadHelper = enabled;
}



void BMVagusNerveTherapyFilter_filterWithBiquad(BMVagusNerveTherapyFilter *This, bool enabled){
	This->filterWithBiquad = enabled;
}



void BMVagusNerveTherapyFilter_setTimeHMS(BMVagusNerveTherapyFilter *This, size_t hours, size_t minutes, double seconds){
	// convert from H:M:S to samples
	size_t samplesPerSecond = (size_t)round(This->sampleRate);
	size_t samplesPerMinute = samplesPerSecond * 60;
	size_t samplesPerHour = samplesPerMinute * 60;
	size_t timeSamples = hours * samplesPerHour +
						 minutes * samplesPerMinute +
						 (size_t)round(seconds * (double)This->sampleRate);
	
	// set the time
	BMVagusNerveTherapyFilter_setTimeSamples(This, timeSamples);
}



size_t BMVagusNerveTherapyFilter_getTimeSamples(BMVagusNerveTherapyFilter *This){
	return This->timeSamples;
}



void BMVagusNerveTherapyFilter_getTimeHMS(BMVagusNerveTherapyFilter *This,
										  size_t *hours,
										  size_t *minutes,
										  double *seconds){
	double samplesPerSecond = This->sampleRate;
	size_t samplesPerMinute = (size_t)round(This->sampleRate * 60.0);
	size_t samplesPerHour = (size_t)round(This->sampleRate * 60.0 * 60.0);
	size_t remainder;
	
	*hours = This->timeSamples / samplesPerHour;
	*minutes = (This->timeSamples - (*hours * samplesPerHour)) / samplesPerMinute;
	remainder = This->timeSamples - (*hours * samplesPerHour) - (*minutes * samplesPerMinute);
	*seconds = (double)remainder / samplesPerSecond;
}




/*!
 *BMLerp
 *
 * @param a start value
 * @param b end value
 * @param t progress between start and end. in [0,1]. 0 returns a; 1 returns b.
 */
float BMLerp(float a, float b, float t){
	return a + t * (b-a);
}



float BMVagusNerveTherapyFilter_getFilterSkirtMaxGain(BMVagusNerveTherapyFilter *This){
	// measure the progress in the 5-hour course as a fraction in [0,1] with 0
	// meaning we just started and 1 meaning we completed the course
	float progress = (double)This->timeSamples / (double)This->endTimeSamples;
	
	// if we are past done, set the progress to 1
	if(This->timeSamples >= This->endTimeSamples) progress = 1.0;
	
	// compute the max gain at this point in the course
	float maxGain = BMLerp(BMVNTF_MAX_SKIRT_DB_START, BMVNTF_MAX_SKIRT_DB_END, progress);
	
	return maxGain;
}



/*!
 *BMVagusNerveTherapyFilter_updateLFOLimits
 */
void BMVagusNerveTherapyFilter_updateLFOLimits(BMVagusNerveTherapyFilter *This, bool updateSmoothly){
	float lfoMaxGain = BMVagusNerveTherapyFilter_getFilterSkirtMaxGain(This);
	
	if(updateSmoothly)
		BMLFO_setMinMaxSmoothly(&This->lfo, BMVNTF_MIN_DB, lfoMaxGain);
	else
		BMLFO_setMinMaxImmediately(&This->lfo, BMVNTF_MIN_DB, lfoMaxGain);
}



void BMVagusNerveTherapyFilter_setTimeSamples(BMVagusNerveTherapyFilter *This, size_t timeInSamples){
	This->timeSamples = timeInSamples;
	
	// update the LFO
	BMLFO_setTimeInSamples(&This->lfo, timeInSamples);
	BMVagusNerveTherapyFilter_updateLFOLimits(This,false);
	BMMUltiLevelSVF_forceImmediateUpdate(&This->svf);
}



void BMVagusNergeTherapyFilter_setFilters(BMVagusNerveTherapyFilter *This, float skirtGain, bool setSmoothly){
	
	// calculate the filter Q. The idea here is that we want the Q to be at its
	// minimum value when the skirt gain is 0dB, because that prevents a trough
	// from forming between the peaks of the two bell filters. The Q increases
	// as the skirt gain falls lower, with a maximum Q of 2.3.
	float minQ = 1.5f;
	float maxQ = 2.3f;
	float filterQ = minQ + ((maxQ - minQ) * (skirtGain / BMVNTF_MIN_DB));
	
	// the gain at the peak of filter one is affected by the skirt of filter 2
	// and vice versa. To keep the peaks at 0 dB we found the following function
	// in Mathematica. We found it by trial and error. It is not exact.
	// BellGain = 3.6 (skirtG/-30) + Sin[\[Pi] (skirtG/-30)^(2/3)]
	float skirtGainExcursion = (skirtGain / BMVNTF_MIN_DB);
	float bellGainDb = 3.6 * skirtGainExcursion;
	bellGainDb += sin(M_PI * pow(skirtGainExcursion,3.0/5.0));
	
	if(This->getCoefficientsFromBiquadHelper || This->filterWithBiquad){
		// set the filters in the biquad helper
		BMMultiLevelBiquad_setBellWithSkirt(&This->svf.biquadHelper, BMVNTF_FILTER_ONE_FC, filterQ, bellGainDb, skirtGain, 0);
		BMMultiLevelBiquad_setBellWithSkirt(&This->svf.biquadHelper, BMVNTF_FILTER_TWO_FC, filterQ, bellGainDb, skirtGain, 1);
		BMMultiLevelBiquad_setBellWithSkirt(&This->svf.biquadHelper, BMVNTF_FILTER_ONE_FC, filterQ, bellGainDb, skirtGain, 2);
		BMMultiLevelBiquad_setBellWithSkirt(&This->svf.biquadHelper, BMVNTF_FILTER_TWO_FC, filterQ, bellGainDb, skirtGain, 3);
		
		// copy the settings from the biquad to the svf
		BMMultiLevelSVF_copyStateFromBiquadHelper(&This->svf);
	}
	else {
		// set the filters directly using the svf formulae
		BMMultiLevelSVF_setBellWithSkirt(&This->svf, BMVNTF_FILTER_ONE_FC, bellGainDb, skirtGain, filterQ, 0);
		BMMultiLevelSVF_setBellWithSkirt(&This->svf, BMVNTF_FILTER_TWO_FC, bellGainDb, skirtGain, filterQ, 1);
		BMMultiLevelSVF_setBellWithSkirt(&This->svf, BMVNTF_FILTER_ONE_FC, bellGainDb, skirtGain, filterQ, 2);
		BMMultiLevelSVF_setBellWithSkirt(&This->svf, BMVNTF_FILTER_TWO_FC, bellGainDb, skirtGain, filterQ, 3);
	}
}



void BMVagusNerveTherapyFilter_process(BMVagusNerveTherapyFilter *This,
									   const float *inputL, const float *inputR,
									   float *outputL, float *outputR, size_t numSamples){

	// update the LFO and get the skirt gain
	bool setSmoothly = true;
	BMVagusNerveTherapyFilter_updateLFOLimits(This, setSmoothly);
	float skirtGain = BMLFO_advance(&This->lfo, numSamples);
	
	// update the filters
	BMVagusNergeTherapyFilter_setFilters(This, skirtGain, setSmoothly);
		
	// process audio
	if(This->filterWithBiquad)
		BMMultiLevelBiquad_processBufferStereo(&This->svf.biquadHelper, inputL, inputR, outputL, outputR, numSamples);
	else
		BMMultiLevelSVF_processBufferStereo(&This->svf, inputL, inputR, outputL, outputR, numSamples);
	
	// apply the fixed filters
	BMMultiLevelBiquad_processBufferStereo(&This->fixedFilters, outputL, outputR, outputL, outputR, numSamples);
	
        // compensate for gain reduction from fixed filters
        float g = BM_DB_TO_GAIN(BMVNTF_FIXED_FILTER_GAIN_COMPENSATION);
        vDSP_vsmul(outputL,1,&g,outputL,1,numSamples);
        vDSP_vsmul(outputR,1,&g,outputR,1,numSamples);

	// advance the timer
	This->timeSamples += numSamples;
}
