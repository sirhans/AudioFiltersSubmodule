//
//  BMAttackShaper.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 18/2/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include <Accelerate/Accelerate.h>
#include "BMAttackShaper.h"
#include "Constants.h"
#include "fastpow.h"

#define BMAS_NOISE_GATE_CLOSED_LEVEL -63.0f



/*!
 *BMAttackShaperSection_init
 */
void BMAttackShaperSection_init(BMAttackShaperSection *This,
								float releaseFilterFc,
								float attackFc,
								float dsfSensitivity,
								float dsfFcMin, float dsfFcMax,
								float exaggeration,
								float sampleRate,
								bool isStereo);



/*!
*BMAttackShaperSection_setAttackTime
*
* @param attackTime in seconds
*/
void BMAttackShaperSection_setAttackTime(BMAttackShaperSection *This, float attackTime);




/*!
*BMAttackShaperSection_process
*/
void BMAttackShaperSection_processMono(BMAttackShaperSection *This,
							const float* input,
							float* output,
							size_t numSamples);


/*!
*BMAttackShaperSection_processStereo
*/
void BMAttackShaperSection_processStereo(BMAttackShaperSection *This,
										 const float* inputL, const float* inputR,
										 float* outputL, float* outputR,
										 size_t numSamples);


/*!
 *BMAttackShaperSection_free
 */
void BMAttackShaperSection_free(BMAttackShaperSection *This);





void BMMultibandAttackShaper_init(BMMultibandAttackShaper *This, bool isStereo, float sampleRate){
	This->isStereo = isStereo;
	
	// init the 2-way crossover
	float crossover2fc = 400.0;
	BMCrossover_init(&This->crossover2, crossover2fc, sampleRate, true, isStereo);
	
	float attackFC = 40.0f;
	float releaseFC = 20.0f;
	float dsfSensitivity = 1000.0f;
	float dsfFcMin = releaseFC;
	float dsfFcMax = 1000.0f;
	float exaggeration = 2.0f;
	BMAttackShaperSection_init(&This->asSections[0],
							   releaseFC,
							   attackFC,
							   dsfSensitivity,
							   dsfFcMin, dsfFcMax,
							   exaggeration,
							   sampleRate,
							   isStereo);
	
	dsfFcMax = 1000.0f;
	BMAttackShaperSection_init(&This->asSections[1],
							   releaseFC*BMAS_SECTION_2_RF_MULTIPLIER,
							   attackFC*BMAS_SECTION_2_AF_MULTIPLIER,
							   dsfSensitivity,
							   dsfFcMin, dsfFcMax,
							   exaggeration*BMAS_SECTION_2_EXAGGERATION_MULTIPLIER,
							   sampleRate,
							   isStereo);
	
	// allocate buffer memory
	size_t bufferSize = sizeof(float) * BM_BUFFER_CHUNK_SIZE;
	if(isStereo){
		This->b1L = malloc(4 * bufferSize);
        This->b2L = This->b1L + BM_BUFFER_CHUNK_SIZE;
        This->b1R = This->b2L + BM_BUFFER_CHUNK_SIZE;
        This->b2R = This->b1R + BM_BUFFER_CHUNK_SIZE;
	} else {
		This->b1L = malloc(2 * bufferSize);
		This->b2L = This->b1L + BM_BUFFER_CHUNK_SIZE;
	}
	
	// set default noise gate threshold
	BMMultibandAttackShaper_setSidechainNoiseGateThreshold(This, -45.0f);
	
	// process a few buffers of silence to get the filters warmed up
	float* input = calloc(256,sizeof(float));
	float* outputL = calloc(256,sizeof(float));
	float* outputR = calloc(256,sizeof(float));
	//
	if(isStereo)
		for(size_t i=0; i<10; i++)
			BMMultibandAttackShaper_processStereo(This, input, input, outputL, outputR, 256);
	else
		for(size_t i=0; i<10; i++)
			BMMultibandAttackShaper_processMono(This, input, outputL, 256);
	//
	free(input);
	free(outputL);
	free(outputR);
}





void BMMultibandAttackShaper_free(BMMultibandAttackShaper *This){
	BMAttackShaperSection_free(&This->asSections[0]);
	BMAttackShaperSection_free(&This->asSections[1]);
	BMCrossover_free(&This->crossover2);
	free(This->b1L);
	This->b1L = NULL;
}





void BMMultibandAttackShaper_processStereo(BMMultibandAttackShaper *This,
										   const float *inputL, const float *inputR,
										   float *outputL, float *outputR,
										   size_t numSamples){
	assert(This->isStereo);
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
		
		// split the signal into two bands
		BMCrossover_processStereo(&This->crossover2, inputL, inputR, This->b1L, This->b1R, This->b2L, This->b2R, samplesProcessing);

		
		// process transient shapers on each band
		BMAttackShaperSection_processStereo(&This->asSections[0], This->b1L, This->b1R, This->b1L, This->b1R, samplesProcessing);
		BMAttackShaperSection_processStereo(&This->asSections[1], This->b2L, This->b2R, This->b2L, This->b2R, samplesProcessing);
		
		
		// recombine the signal
		vDSP_vadd(This->b1L, 1, This->b2L, 1, outputL, 1, samplesProcessing);
		vDSP_vadd(This->b1R, 1, This->b2R, 1, outputR, 1, samplesProcessing);

		
		// advance pointers
		numSamples -= samplesProcessing;
		inputL += samplesProcessing;
		outputL += samplesProcessing;
		inputR += samplesProcessing;
		outputR += samplesProcessing;
	}
}





void BMMultibandAttackShaper_processMono(BMMultibandAttackShaper *This,
										 const float *input,
										 float *output,
										 size_t numSamples){
	assert(!This->isStereo);
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
		
		// split the signal into two bands
		BMCrossover_processMono(&This->crossover2, input, This->b1L, This->b2L, samplesProcessing);

		// process transient shapers on each band
		BMAttackShaperSection_processMono(&This->asSections[0], This->b1L, This->b1L, samplesProcessing);
		BMAttackShaperSection_processMono(&This->asSections[1], This->b2L, This->b2L, samplesProcessing);

		// silence the channels we aren't processing
//				memset(This->b1L, 0, sizeof(float)*samplesProcessing);
//				memset(This->b2L, 0, sizeof(float)*samplesProcessing);

		// recombine the signal
		vDSP_vadd(This->b1L, 1, This->b2L, 1, output, 1, samplesProcessing);
		
		// advance pointers
		numSamples -= samplesProcessing;
		input += samplesProcessing;
		output += samplesProcessing;
	}
}



void BMAttackShaperSection_setAttackFrequency(BMAttackShaperSection *This, float attackFc){
	// set the attack filter
	for(size_t i=0; i<BMAS_AF_NUMLEVELS; i++)
	BMAttackFilter_setCutoff(&This->af[i], attackFc);
}



void BMAttackShaperSection_setReleaseFrequency(BMAttackShaperSection *This, float releaseFc){
	for(size_t i=0; i<BMAS_RF_NUMLEVELS; i++)
		BMReleaseFilter_setCutoff(&This->rf[i], releaseFc);
}


void BMAttackShaperSection_setDSMinFrequency(BMAttackShaperSection *This, float dsMinFc){
	for(size_t i=0; i<BMAS_DSF_NUMLEVELS; i++)
		BMDynamicSmoothingFilter_setMinFc(&This->dsf[i], dsMinFc);
}





void BMAttackShaperSection_init(BMAttackShaperSection *This,
								float releaseFilterFc,
								float attackFc,
								float dsfSensitivity,
								float dsfFcMin, float dsfFcMax,
								float exaggeration,
								float sampleRate,
								bool isStereo){
	This->sampleRate = sampleRate;
	This->exaggeration = exaggeration;
    This->isStereo = isStereo;
	This->depth = 1.0;
	
	// the release filters generate the fast envelope across the peaks
	for(size_t i=0; i<BMAS_RF_NUMLEVELS; i++)
		BMReleaseFilter_init(&This->rf[i], releaseFilterFc, sampleRate);
	
	// the attack filter controls the attack time
	for(size_t i=0; i<BMAS_AF_NUMLEVELS; i++)
		BMAttackFilter_init(&This->af[i], attackFc, sampleRate);
	BMAttackShaperSection_setAttackFrequency(This, attackFc);
    
    // set the delay to 12 samples at 48 KHz sampleRate or
    // stretch appropriately for other sample rates
    size_t numChannels = isStereo ? 2 : 1;
    This->delaySamples = round(sampleRate * BMAS_DELAY_AT_48KHZ_SAMPLES / 48000.0f);
    BMShortSimpleDelay_init(&This->dly, numChannels, This->delaySamples);
	
	// the dynamic smoothing filter prevents clicks when changing gain
	for(size_t i=0; i<BMAS_DSF_NUMLEVELS; i++)
		BMDynamicSmoothingFilter_init(&This->dsf[i], dsfSensitivity, dsfFcMin, dsfFcMax, This->sampleRate);
}




void BMAttackShaperSection_setAttackTime(BMAttackShaperSection *This, float attackTime){
	// find the lpf cutoff frequency that corresponds to the specified attack time
	float fc = ARTimeToCutoffFrequency(attackTime, BMAS_AF_NUMLEVELS);
	
	// set the attack filter
	BMAttackShaperSection_setAttackFrequency(This, fc);
}



/*!
 *BMAttackShaper_upperLimit
 *
 * level off the signal above a specified limit
 *
 *    output[i] = input[i] < limit ? input[i] : limit
 *
 * @param limit do not let the output value exceed this
 * @param input input array of length numSamples
 * @param output ouput array of length numSamples
 * @param numSamples length of input and output arrays
 */
void BMAttackShaper_upperLimit(float limit, const float* input, float *output, size_t numSamples){
	vDSP_vneg(input, 1, output, 1, numSamples);
	limit = -limit;
	vDSP_vthr(output, 1, &limit, output, 1, numSamples);
	vDSP_vneg(output, 1, output, 1, numSamples);
}




/*!
 *BMAttackShaper_simpleNoiseGate
 *
 * for (size_t i=0; i<numSamples; i++){
 *    if(input[i] < threshold) output[i] = closedValue;
 *    else output[i] = input[i];
 * }
 *
 * @param This pointer to an initialised struct
 * @param input input array
 * @param threshold below this value, the gate closes
 * @param closedValue when the gate is closed, this is the output value
 * @param output output array
 * @param numSamples length of input and output arrays
 */
void BMAttackShaperSection_simpleNoiseGate(BMAttackShaperSection *This,
									const float* input,
									float threshold,
									float closedValue,
									float* output,
									size_t numSamples){
	
	// (input[i] < threshold) ? output[i] = 0;
	if(threshold > closedValue)
		vDSP_vthres(input, 1, &threshold, output, 1, numSamples);
	// (output[i] < closedValue) ? output[i] = closedValue;
	vDSP_vthr(output, 1, &closedValue, output, 1, numSamples);
	
	// get the max value to find out if the gate was open at any point during the buffer
	float maxValue;
	vDSP_maxv(output, 1, &maxValue, numSamples);
	This->noiseGateIsOpen = maxValue > This->noiseGateThreshold;
	if(This->noiseGateThreshold < closedValue) This->noiseGateIsOpen = false;
}





/*!
 *BMAttackShaperSection_generateControlSignal
 */
void BMAttackShaperSection_generateControlSignal(BMAttackShaperSection *This,
												  const float *input,
												  float *buffer,
												  float *controlSignal,
												  size_t numSamples){
	assert(numSamples <= BM_BUFFER_CHUNK_SIZE);
	assert(buffer != controlSignal);
	
	
	/*************************
	 * volume envelope in dB *
	 *************************/
	float *instantAttackEnvelope = buffer;
	// absolute value
	vDSP_vabs(input, 1, instantAttackEnvelope, 1, numSamples);
	
	// apply a simple per-sample noise gate
	float noiseGateClosedValue = BM_DB_TO_GAIN(BMAS_NOISE_GATE_CLOSED_LEVEL);
	BMAttackShaperSection_simpleNoiseGate(This, instantAttackEnvelope, This->noiseGateThreshold, noiseGateClosedValue, instantAttackEnvelope, numSamples);
	
	// convert to decibels
	float one = 1.0f;
	vDSP_vdbcon(instantAttackEnvelope, 1, &one, instantAttackEnvelope, 1, numSamples, 0);
	
	// release filter to get instant attack envelope
	for(size_t i=0; i<BMAS_RF_NUMLEVELS; i++)
		BMReleaseFilter_processBuffer(&This->rf[i], instantAttackEnvelope, instantAttackEnvelope, numSamples);
	
	// attack filter to get slow attack envelope
	float* slowAttackEnvelope = controlSignal;
	BMAttackFilter_processBuffer(&This->af[0], instantAttackEnvelope, slowAttackEnvelope, numSamples);
	for(size_t i=1; i<BMAS_AF_NUMLEVELS; i++)
		BMAttackFilter_processBuffer(&This->af[i], slowAttackEnvelope, slowAttackEnvelope, numSamples);
	

	
	
	/***************************************************************************
	 * find gain reduction (dB) to have the envelope follow slowAttackEnvelope *
	 ***************************************************************************/
	// controlSignal = slowAttackEnvelope - instantAttackEnvelope;
	vDSP_vsub(instantAttackEnvelope, 1, slowAttackEnvelope, 1, controlSignal, 1, numSamples);
	
	// limit the control signal slightly below 0 dB to prevent zippering during sustain sections
	float limit = -0.2f;
	BMAttackShaper_upperLimit(limit, controlSignal, controlSignal, numSamples);
	
	
	
	/************************************************
	 * filter the control signal to reduce aliasing *
	 ************************************************/
	//
	// smoothing filter to prevent clicks
	for(size_t i=0; i < BMAS_DSF_NUMLEVELS; i++)
		BMDynamicSmoothingFilter_processBufferWithFastDescent2(&This->dsf[i], controlSignal, controlSignal, numSamples);

	// exaggerate the control signal
	float adjustedExaggeration = This->depth * This->exaggeration;
	vDSP_vsmul(controlSignal, 1, &adjustedExaggeration, controlSignal, 1, numSamples);
	
	// convert back to linear scale
	vector_fastDbToGain(controlSignal, controlSignal, numSamples);
}





void BMAttackShaperSection_processStereo(BMAttackShaperSection *This,
										 const float *inputL, const float *inputR,
										 float *outputL, float *outputR,
										 size_t numSamples){
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE,numSamples);
		
		// mix to mono
		float half = 0.5;
		vDSP_vasm(inputL, 1, inputR, 1, &half, This->b1, 1, samplesProcessing);
		
		// generate a control signal to modulate the volume of the input
        float *controlSignal = This->b1;
		BMAttackShaperSection_generateControlSignal(This, This->b1, This->b2, controlSignal, samplesProcessing);
		
		// delay the input signal to enable faster response time without clicks
		const float *inputs [2] = {inputL, inputR};
		float *delayedInputL = This->b0;
		float *delayedInputR = This->b2;
		float* outputs [2] = {delayedInputL, delayedInputR};
		size_t numChannels = 2;
		BMShortSimpleDelay_process(&This->dly, inputs, outputs, numChannels, samplesProcessing);
//		memcpy(delayedInputL,inputL,sizeof(float)*samplesProcessing);
//		memcpy(delayedInputR,inputR,sizeof(float)*samplesProcessing);
		
		// apply the volume control signal to the delayed input
		vDSP_vmul(delayedInputL, 1, controlSignal, 1, outputL, 1, samplesProcessing);
		vDSP_vmul(delayedInputR, 1, controlSignal, 1, outputR, 1, samplesProcessing);
		
		// advance pointers
		inputL += samplesProcessing;
		outputL += samplesProcessing;
		inputR += samplesProcessing;
		outputR += samplesProcessing;
		numSamples -= samplesProcessing;
	}
}





/*!
 *BMAttackShaperSection_processMono
 */
void BMAttackShaperSection_processMono(BMAttackShaperSection *This,
									   const float* input,
									   float* output,
									   size_t numSamples){
    
    while(numSamples > 0){
        size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE,numSamples);

		// generate a control signal to modulate the volume of the input
		float *controlSignal = This->b1;
		BMAttackShaperSection_generateControlSignal(This, input, This->b2, controlSignal, samplesProcessing);
		
        // delay the input signal to enable faster response time without clicks
		const float *inputs [1] = {input};
        float *delayedInput = This->b0;
		float *outputs [1] = {delayedInput};
        BMShortSimpleDelay_process(&This->dly, inputs, outputs, 1, samplesProcessing);
        
        // apply the volume control signal to the delayed input
        vDSP_vmul(delayedInput, 1, controlSignal, 1, output, 1, samplesProcessing);
        
        // advance pointers
        input += samplesProcessing;
        output += samplesProcessing;
        numSamples -= samplesProcessing;
    }
}





void BMAttackShaperSection_free(BMAttackShaperSection *This){
	BMShortSimpleDelay_free(&This->dly);
	// BMMultiLevelBiquad_free(&This->hpf);
}



bool BMAttackShaperSection_sidechainNoiseGateIsOpen(BMAttackShaperSection *This){
	return This->noiseGateIsOpen;
}




void BMMultibandAttackShaper_setAttackTime(BMMultibandAttackShaper *This, float attackTimeInSeconds){
	// find the lpf cutoff frequency that corresponds to the specified attack time
	float attackFc = ARTimeToCutoffFrequency(attackTimeInSeconds, BMAS_AF_NUMLEVELS);
	
	BMAttackShaperSection_setAttackFrequency(&This->asSections[0], attackFc);
	BMAttackShaperSection_setAttackFrequency(&This->asSections[1], attackFc*BMAS_SECTION_2_AF_MULTIPLIER);
	
	float releaseFc = BM_MIN(attackFc * 0.6666f, 20.0f);
	BMAttackShaperSection_setReleaseFrequency(&This->asSections[0], releaseFc);
	BMAttackShaperSection_setReleaseFrequency(&This->asSections[1], releaseFc*BMAS_SECTION_2_RF_MULTIPLIER);
	
	float dsMinFc = releaseFc;
	BMAttackShaperSection_setDSMinFrequency(&This->asSections[0], dsMinFc);
	BMAttackShaperSection_setDSMinFrequency(&This->asSections[1], dsMinFc);
}
	


void BMMultibandAttackShaper_setAttackDepth(BMMultibandAttackShaper *This, float depth){
	for(size_t i=0; i<BMAS_NUM_SECTIONS; i++)
		This->asSections[i].depth = depth;
}



void BMMultibandAttackShaper_setSidechainNoiseGateThreshold(BMMultibandAttackShaper *This, float thresholdDb){
	// Don't allow the noise gate threshold to be set lower than the
	// gain setting the gate takes when it's closed
	assert(thresholdDb > BMAS_NOISE_GATE_CLOSED_LEVEL);
	
	for(size_t i=0; i<BMAS_NUM_SECTIONS; i++)
		This->asSections[i].noiseGateThreshold = BM_DB_TO_GAIN(thresholdDb);
}



bool BMMultibandAttackShaper_sidechainNoiseGateIsOpen(BMMultibandAttackShaper *This){
	// return true if either one of the attack shaper sections has its gate open
	return This->asSections[0].noiseGateIsOpen || This->asSections[1].noiseGateIsOpen;
}
