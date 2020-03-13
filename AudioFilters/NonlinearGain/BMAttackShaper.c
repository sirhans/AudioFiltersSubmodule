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




/*!
 *BMMultibandAttackShaper_init
 */
void BMMultibandAttackShaper_init(BMMultibandAttackShaper *This, bool isStereo, float sampleRate){
	This->isStereo = isStereo;
	
	// init the 4-way crossover
	BMCrossover4way_init(&This->crossover4,
						 BMAS_BAND_1_FC,
						 BMAS_BAND_2_FC,
						 BMAS_BAND_3_FC,
						 sampleRate,
						 true,
						 isStereo);
	
	// init the 2-way crossover
	float crossover2fc = 300.0;
	BMCrossover_init(&This->crossover2, crossover2fc, sampleRate, true, isStereo);
	
	
	float attackFC = 40.0f;
	float releaseFC = 20.0f;
	float dsfSensitivity = 1000.0f;
	float dsfFcMin = releaseFC;
	float dsfFcMax = 1000.0f;
	float exaggeration = 1.5f;
	BMAttackShaperSection_init(&This->asSections[0],
							   releaseFC,
							   attackFC,
							   dsfSensitivity,
							   dsfFcMin, dsfFcMax,
							   exaggeration,
							   sampleRate,
							   isStereo);
	
	
	// dsfFcMax = 1000.0f;
	// highpassFrequency = 1000.0f;
	exaggeration = 1.65f;
	BMAttackShaperSection_init(&This->asSections[1],
							   releaseFC*2.0f,
							   attackFC*1.25f,
							   dsfSensitivity,
							   dsfFcMin, dsfFcMax,
							   exaggeration,
							   sampleRate,
							   isStereo);
	
	
	
	// init the attack shaper sections
//	float releaseFC = 20.0f;
//	float attackFC = 10.0f;
//	float dsfSensitivity = 200.0f;
//	float dsfFcMin = releaseFC;
//	float dsfFcMax = 3.0f * BMAS_BAND_1_FC;
//	float highpassFrequency = BMAS_BAND_1_FC;
//	BMAttackShaperSection_init(&This->asSections[0],
//							   releaseFC,
//							   highpassFrequency,
//							   attackFC,
//							   dsfSensitivity,
//							   dsfFcMin, dsfFcMax,
//							   sampleRate,
//							   isStereo);
//
//
//	dsfFcMax = BMAS_BAND_2_FC * 3.0f;
//	highpassFrequency = BMAS_BAND_2_FC * 1.5f;
//	dsfSensitivity = 200.0f;
//	BMAttackShaperSection_init(&This->asSections[1],
//							   releaseFC,
//							   highpassFrequency,
//							   attackFC,
//							   dsfSensitivity,
//							   dsfFcMin, dsfFcMax,
//							   sampleRate,
//							   isStereo);
//
//	dsfFcMax = BMAS_BAND_3_FC * 3.0f;
//	highpassFrequency = BMAS_BAND_2_FC * 1.5f;
//	dsfSensitivity = 600.0f;
//	BMAttackShaperSection_init(&This->asSections[2],
//							   releaseFC,
//							   highpassFrequency,
//							   attackFC,
//							   dsfSensitivity,
//							   dsfFcMin, dsfFcMax,
//							   sampleRate,
//							   isStereo);
//
//	dsfFcMax = 0.5 * sampleRate / 2.0f;
//	highpassFrequency = BMAS_BAND_2_FC * 1.5f;
//	dsfSensitivity = 600.0f;
//	BMAttackShaperSection_init(&This->asSections[3],
//							   releaseFC,
//							   highpassFrequency,
//							   attackFC,
//							   dsfSensitivity,
//							   dsfFcMin, dsfFcMax,
//							   sampleRate,
//							   isStereo);
}


/*!
 *BMMultibandAttackShaper_free
 */
void BMMultibandAttackShaper_free(BMMultibandAttackShaper *This){
	BMAttackShaperSection_free(&This->asSections[0]);
	BMAttackShaperSection_free(&This->asSections[1]);
	BMCrossover4way_free(&This->crossover4);
}


/*!
 *BMMultibandAttackShaper_processStereo
 */
void BMMultibandAttackShaper_processStereo(BMMultibandAttackShaper *This,
									 const float *inputL, const float *inputR,
									 float *outputL, float *outputR,
									 size_t numSamples){
	assert(This->isStereo);
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
		
		// split the signal into four bands
		BMCrossover4way_processStereo(&This->crossover4,
									  inputL, inputR,
									  This->b1L, This->b1R,
									  This->b2L, This->b2R,
									  This->b3L, This->b3R,
									  This->b4L, This->b4R,
									  samplesProcessing);
		
		// process transient shapers on each band
		BMAttackShaperSection_processStereo(&This->asSections[0],
											This->b1L, This->b1R,
											This->b1L, This->b1R,
											samplesProcessing);
		BMAttackShaperSection_processStereo(&This->asSections[1],
											This->b2L, This->b2R,
											This->b2L, This->b2R,
											samplesProcessing);
		BMAttackShaperSection_processStereo(&This->asSections[2],
											This->b3L, This->b3R,
											This->b3L, This->b3R,
											samplesProcessing);
		BMAttackShaperSection_processStereo(&This->asSections[3],
											This->b4L, This->b4R,
											This->b4L, This->b4R,
											samplesProcessing);
		
		// recombine the four bands back into one signal
		BMCrossover4way_recombine(This->b1L, This->b1R,
								  This->b2L, This->b2R,
								  This->b3L, This->b3R,
								  This->b4L, This->b4R,
								  outputL, outputR,
								  samplesProcessing);
		
		// advance pointers
		numSamples -= samplesProcessing;
		inputL += samplesProcessing;
		outputL += samplesProcessing;
		inputR += samplesProcessing;
		outputR += samplesProcessing;
	}
}




/*!
 *BMMultibandAttackShaper_processMono
 */
void BMMultibandAttackShaper_processMono(BMMultibandAttackShaper *This,
										 const float *input,
										 float *output,
										 size_t numSamples){
	assert(!This->isStereo);
	
	while(numSamples > 0){
		size_t samplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
		
//		BMAttackShaperSection_processMono(&This->asSections[0], input, output, samplesProcessing);
		
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
		
		
//		// split the signal into four bands
//		BMCrossover4way_processMono(&This->crossover4,
//									input,
//									This->b1L,
//									This->b2L,
//									This->b3L,
//									This->b4L,
//									samplesProcessing);
//
//
//		// process transient shapers on each band
//		BMAttackShaperSection_processMono(&This->asSections[0], This->b1L, This->b1L, samplesProcessing);
//		BMAttackShaperSection_processMono(&This->asSections[1], This->b2L, This->b2L, samplesProcessing);
//		BMAttackShaperSection_processMono(&This->asSections[2], This->b3L, This->b3L, samplesProcessing);
//		BMAttackShaperSection_processMono(&This->asSections[3], This->b4L, This->b4L, samplesProcessing);
//
//		// silence the channels we aren't processing
////		memset(This->b1L, 0, sizeof(float)*samplesProcessing);
////		memset(This->b2L, 0, sizeof(float)*samplesProcessing);
////		memset(This->b3L, 0, sizeof(float)*samplesProcessing);
//		memset(This->b4L, 0, sizeof(float)*samplesProcessing);
//
//		// recombine the four bands back into one signal
//		BMCrossover4way_recombineMono(This->b1L,
//									  This->b2L,
//									  This->b3L,
//									  This->b4L,
//									  output,
//									  samplesProcessing);
		
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
	
	// the release filters generate the fast envelope across the peaks
	for(size_t i=0; i<BMAS_RF_NUMLEVELS; i++)
		BMReleaseFilter_init(&This->rf[i], releaseFilterFc, sampleRate);
	
	// the attack filter controls the attack time
	for(size_t i=0; i<BMAS_AF_NUMLEVELS; i++)
		BMAttackFilter_init(&This->af[i], attackFc, sampleRate);
	BMAttackShaperSection_setAttackFrequency(This, attackFc);
    
    // set the delay to 12 samples at 48 KHz sampleRate or
    // stretch appropriately for other sample rates
    size_t numChannels = 1;
    This->delaySamples = round(sampleRate * BMAS_DELAY_AT_48KHZ_SAMPLES / 48000.0f);
    BMShortSimpleDelay_init(&This->dly, numChannels, This->delaySamples);
	
	// the dynamic smoothing filter prevents clicks when changing gain
	for(size_t i=0; i<BMAS_DSF_NUMLEVELS; i++)
		BMDynamicSmoothingFilter_init(&This->dsf[i], dsfSensitivity, dsfFcMin, dsfFcMax, This->sampleRate);
}




void BMAttackShaperSection_setAttackTime(BMAttackShaperSection *This, float attackTime){
	// find the lpf cutoff frequency that corresponds to the specified attack time
	size_t attackFilterNumLevels = 1;
	float fc = ARTimeToCutoffFrequency(attackTime, attackFilterNumLevels);
	
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
 * @param input input array
 * @param threshold below this value, the gate closes
 * @param closedValue when the gate is closed, this is the output value
 * @param output output array
 * @param numSamples length of input and output arrays
 */
void BMAttackShaper_simpleNoiseGate(const float* input, float threshold, float closedValue, float* output, size_t numSamples){
	// (input[i] < threshold) ? output[i] = 0;
	vDSP_vthres(input, 1, &threshold, output, 1, numSamples);
	// (output[i] < closedValue) ? output[i] = closedValue;
	vDSP_vthr(output, 1, &closedValue, output, 1, numSamples);
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
	float nearZero = BM_DB_TO_GAIN(-60.0f);
	float noiseFloor = BM_DB_TO_GAIN(-45.0f);
	BMAttackShaper_simpleNoiseGate(instantAttackEnvelope, noiseFloor, nearZero, instantAttackEnvelope, numSamples);
	
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
	vDSP_vsmul(controlSignal, 1, &This->exaggeration, controlSignal, 1, numSamples);
	
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
		BMAttackShaperSection_generateControlSignal(This, This->b1, This->b2, This->b1, samplesProcessing);
		
		// delay the input signal to enable faster response time without clicks
		const float *inputs [2];
		float *outputs [2];
		inputs[0] = inputL; inputs[1] = inputR;
		float* delayedInputL = This->b0;
		float* delayedInputR = This->b2;
		outputs[0] = delayedInputL; outputs[1] = delayedInputR;
		BMShortSimpleDelay_process(&This->dly, inputs, outputs, 1, samplesProcessing);
		
		// apply the volume control signal to the delayed input
		vDSP_vmul(delayedInputL, 1, This->b1, 1, outputL, 1, samplesProcessing);
		vDSP_vmul(delayedInputR, 1, This->b1, 1, outputR, 1, samplesProcessing);
		
		// advance pointers
		inputL += samplesProcessing;
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
		const float *t1 = input;
		const float **inputs = &t1;
		float *delayedInput = This->b2;
        float **outputs = &delayedInput;
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
