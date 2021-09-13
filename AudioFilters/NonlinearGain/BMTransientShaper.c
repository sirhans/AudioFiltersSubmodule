//
//  BMTransientShaper.c
//  BMAUTransientShaper
//
//  Created by Nguyen Minh Tien on 7/9/21.
//

#include "BMTransientShaper.h"
#include "BMUnitConversion.h"

void BMTransientShaperSection_setAttackSlowFC(BMTransientShaperSection *This, float attackFc);
void BMTransientShaperSection_setAttackInstanceFC(BMTransientShaperSection *This, float releaseFc);
void BMTransientShaperSection_setDSMinFrequency(BMTransientShaperSection *This, float dsMinFc);

void BMTransientShaper_upperLimit(float limit, const float* input, float *output, size_t numSamples);
void BMTransientShaper_setSidechainNoiseGateThreshold(BMTransientShaper *This, float thresholdDb);
void BMTransientShaper_processStereo(BMTransientShaper *This,
                                           const float *inputL, const float *inputR,
                                           float *outputL, float *outputR,
                                     size_t numSamples);
void BMTransientShaper_processMono(BMTransientShaper *This,
                                         const float *input,
                                         float *output,
                                   size_t numSamples);

void BMTransientShaperSection_init(BMTransientShaperSection *This,
                                float releaseFilterFc,
                                float attackFc,
                                float dsfSensitivity,
                                float dsfFcMin, float dsfFcMax,
                                float exaggeration,
                                float sampleRate,
                                bool isStereo,
                               bool isTesting){
    This->sampleRate = sampleRate;
    This->attackExaggeration = exaggeration;
    This->sustainExaggeration = exaggeration;
    This->isStereo = isStereo;
    This->attackDepth = 1.0;
    This->releaseDepth = 1.0;
    This->isTesting = isTesting;
    
    This->inputBuffer = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->b1 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->b2 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->attackControlSignal = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->releaseControlSignal = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->attackInstanceFilter = malloc(sizeof(BMReleaseFilter)*BMTS_ARF_NUMLEVELS);
    This->attackSlowFilter = malloc(sizeof(BMAttackFilter)*BMTS_AF_NUMLEVELS);
    This->dsfAttack = malloc(sizeof(BMDynamicSmoothingFilter)*BMTS_DSF_NUMLEVELS);
    This->dsfSustain = malloc(sizeof(BMDynamicSmoothingFilter)*BMTS_DSF_NUMLEVELS);
    
    
    
    // the release filters generate the fast envelope across the peaks
    for(size_t i=0; i<BMTS_ARF_NUMLEVELS; i++)
        BMReleaseFilter_init(&This->attackInstanceFilter[i], releaseFilterFc, sampleRate);
    
    // the attack filter controls the attack time
    for(size_t i=0; i<BMTS_AF_NUMLEVELS; i++)
        BMAttackFilter_init(&This->attackSlowFilter[i], attackFc, sampleRate);
    
    BMTransientShaperSection_setAttackSlowFC(This, attackFc);
    
    //Sustain
    This->sustainFastReleaseFilter = malloc(sizeof(BMReleaseFilter)*BMTS_RRF_NUMLEVELS);
    This->sustainSlowReleaseFilter = malloc(sizeof(BMReleaseFilter)*BMTS_RRF_NUMLEVELS);
    for(size_t i=0; i<BMTS_RRF_NUMLEVELS; i++){
        BMReleaseFilter_init(&This->sustainFastReleaseFilter[i], releaseFilterFc, sampleRate);
        BMReleaseFilter_init(&This->sustainSlowReleaseFilter[i], releaseFilterFc, sampleRate);
    }
    
    float myFC = 0.05f;
    BMAttackFilter_init(&This->sustainFastAttackFilter, myFC, sampleRate);
    BMAttackFilter_init(&This->sustainSlowAttackFilter, myFC, sampleRate);
    
    // set the delay to 12 samples at 48 KHz sampleRate or
    // stretch appropriately for other sample rates
    size_t numChannels = isStereo ? 2 : 1;
    This->delaySamples = round(sampleRate * BMTS_DELAY_AT_48KHZ_SAMPLES / 48000.0f);
    BMShortSimpleDelay_init(&This->dly, numChannels, This->delaySamples);
    
    // the dynamic smoothing filter prevents clicks when changing gain
    for(size_t i=0; i<BMTS_DSF_NUMLEVELS; i++){
        BMDynamicSmoothingFilter_init(&This->dsfAttack[i], dsfSensitivity, dsfFcMin, dsfFcMax, This->sampleRate);
        BMDynamicSmoothingFilter_init(&This->dsfSustain[i], dsfSensitivity, dsfFcMin, dsfFcMax, This->sampleRate);
    }

}

void BMTransientShaperSection_free(BMTransientShaperSection *This){
    BMShortSimpleDelay_free(&This->dly);
    free(This->inputBuffer);
    This->inputBuffer = NULL;
    free(This->b1);
    This->b1 = NULL;
    free(This->b2);
    This->b2 = NULL;
    free(This->attackControlSignal);
    This->attackControlSignal = NULL;
    free(This->releaseControlSignal);
    This->releaseControlSignal = NULL;
    
    free(This->attackInstanceFilter);
    This->attackInstanceFilter = NULL;
    free(This->attackSlowFilter);
    This->attackSlowFilter = NULL;
    
    free(This->dsfAttack);
    This->dsfAttack = NULL;
    free(This->dsfSustain);
    This->dsfSustain = NULL;
}

void BMTransientShaperSection_setAttackSlowFC(BMTransientShaperSection *This, float attackFc){
    // set the attack filter
    for(size_t i=0; i<BMTS_AF_NUMLEVELS; i++)
    BMAttackFilter_setCutoff(&This->attackSlowFilter[i], attackFc);
}

void BMTransientShaperSection_setAttackInstanceFC(BMTransientShaperSection *This, float releaseFc){
    for(size_t i=0; i<BMTS_ARF_NUMLEVELS; i++)
        BMReleaseFilter_setCutoff(&This->attackInstanceFilter[i], releaseFc);
}

void BMTransientShaperSection_setSustainFastFC(BMTransientShaperSection *This, float releaseFc){
    for(size_t i=0; i<BMTS_RRF_NUMLEVELS; i++){
        BMReleaseFilter_setCutoff(&This->sustainFastReleaseFilter[i], releaseFc);
    }
}

void BMTransientShaperSection_setSustainSlowFC(BMTransientShaperSection *This, float releaseFc){
    for(size_t i=0; i<BMTS_RRF_NUMLEVELS; i++){
        BMReleaseFilter_setCutoff(&This->sustainSlowReleaseFilter[i], releaseFc);
    }
}

void BMTransientShaperSection_setDSMinFrequency(BMTransientShaperSection *This, float dsMinFc){
    for(size_t i=0; i<BMTS_DSF_NUMLEVELS; i++){
        BMDynamicSmoothingFilter_setMinFc(&This->dsfAttack[i], dsMinFc);
        BMDynamicSmoothingFilter_setMinFc(&This->dsfSustain[i], dsMinFc);
    }
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
void BMTransientShaperSection_simpleNoiseGate(BMTransientShaperSection *This,
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
void BMTransientShaperSection_generateControlSignal(BMTransientShaperSection *This,
                                                  float *input,
                                                  size_t numSamples){
    assert(numSamples <= BM_BUFFER_CHUNK_SIZE);
    
    
    /*************************
     * volume envelope in dB *
     *************************/
    
    // absolute value
    vDSP_vabs(input, 1, input, 1, numSamples);
    
    // apply a simple per-sample noise gate
    float noiseGateClosedValue = BM_DB_TO_GAIN(BMTS_NOISE_GATE_CLOSED_LEVEL);
    BMTransientShaperSection_simpleNoiseGate(This, input, This->noiseGateThreshold, noiseGateClosedValue, input, numSamples);
    
    // convert to decibels
    float one = 1.0f;
    vDSP_vdbcon(input, 1, &one, input, 1, numSamples, 0);
    
    /* ------------ ATTACK FILTER ---------*/
    // release filter to get instant attack envelope
    float *instantAttackEnvelope = This->b1;
    for(size_t i=0; i<BMTS_ARF_NUMLEVELS; i++)
        BMReleaseFilter_processBuffer(&This->attackInstanceFilter[i], input, instantAttackEnvelope, numSamples);

    // attack filter to get slow attack envelope
    float* slowAttackEnvelope = This->attackControlSignal;
    BMAttackFilter_processBuffer(&This->attackSlowFilter[0], instantAttackEnvelope, slowAttackEnvelope, numSamples);
    for(size_t i=1; i<BMTS_AF_NUMLEVELS; i++)
        BMAttackFilter_processBuffer(&This->attackSlowFilter[i], slowAttackEnvelope, slowAttackEnvelope, numSamples);
    
    
    
    /***************************************************************************
     * find gain reduction (dB) to have the envelope follow slowAttackEnvelope *
     ***************************************************************************/
    // controlSignal = slowAttackEnvelope - instantAttackEnvelope;
    vDSP_vsub(instantAttackEnvelope, 1, slowAttackEnvelope, 1, This->attackControlSignal, 1, numSamples);
    
    // limit the control signal slightly below 0 dB to prevent zippering during sustain sections
    float limit = -0.2f;
    BMTransientShaper_upperLimit(limit, This->attackControlSignal, This->attackControlSignal, numSamples);
    
    
    
    /************************************************
     * filter the control signal to reduce aliasing *
     ************************************************/
    //
    // smoothing filter to prevent clicks
    
    //Always make the attackControlSignal upside down
    for(size_t i=0; i < BMTS_DSF_NUMLEVELS; i++)
        BMDynamicSmoothingFilter_processBufferWithFastDescent2(&This->dsfAttack[i], This->attackControlSignal, This->attackControlSignal, numSamples);
    
    //Return sign
    float negOne = -1;
    vDSP_vsmul(This->attackControlSignal, 1, &negOne, This->attackControlSignal, 1, numSamples);
    
    
    /* ------------ SUSTAIN FILTER ---------*/
    /*--------------------------------------*/
    float* slowSustainEnvelope = This->b2;
    float* fastSustainEnvelope = This->releaseControlSignal;
    //Attack filter
    BMAttackFilter_processBufferBelowDb(&This->sustainSlowAttackFilter,-40, instantAttackEnvelope, instantAttackEnvelope, numSamples);
    
    for(size_t i=0; i<BMTS_ARF_NUMLEVELS; i++)
        BMReleaseFilter_processBuffer(&This->sustainSlowReleaseFilter[i], instantAttackEnvelope, slowSustainEnvelope, numSamples);
    
    
    for(size_t i=0; i<BMTS_ARF_NUMLEVELS; i++)
        BMReleaseFilter_processBuffer(&This->sustainFastReleaseFilter[i], instantAttackEnvelope, fastSustainEnvelope, numSamples);
    
    
//    if(This->isTesting)
//        memcpy(This->testBuffer1, slowSustainEnvelope, sizeof(float)*numSamples);
//    
//    if(This->isTesting)
//        memcpy(This->testBuffer2,fastSustainEnvelope, sizeof(float)*numSamples);
    
    //Get release control
    vDSP_vsub(fastSustainEnvelope, 1, slowSustainEnvelope, 1, This->releaseControlSignal, 1, numSamples);
    
    //Apply depth
    // exaggerate the control signal
    float adjustedExaggeration = This->attackDepth * This->attackExaggeration;
    vDSP_vsmul(This->attackControlSignal, 1, &adjustedExaggeration, This->attackControlSignal, 1, numSamples);
    
    adjustedExaggeration = This->releaseDepth * This->sustainExaggeration;
    vDSP_vsmul(This->releaseControlSignal, 1, &adjustedExaggeration, This->releaseControlSignal, 1, numSamples);
    
    
    
    //Mix attack & release control signal
    vDSP_vadd(This->attackControlSignal, 1, This->releaseControlSignal, 1, This->releaseControlSignal, 1, numSamples);
    
    if(This->isTesting)
        memcpy(This->testBuffer3, This->releaseControlSignal, sizeof(float)*numSamples);
    
    // convert back to linear scale
    BMConv_dBToGainV(This->releaseControlSignal, This->releaseControlSignal, numSamples);
   
}

void BMTransientShaperSection_processStereo(BMTransientShaperSection *This,
                                         const float *inputL, const float *inputR,
                                         float *outputL, float *outputR,
                                         size_t numSamples){
    
    size_t sampleProcessed = 0;
    
    while(sampleProcessed<numSamples){
        size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE,numSamples-sampleProcessed);
        
        // mix to mono
        float half = 0.5f;
        vDSP_vasm(inputL+sampleProcessed, 1, inputR+sampleProcessed, 1, &half, This->inputBuffer, 1, samplesProcessing);
        
        // generate a control signal to modulate the volume of the input
        BMTransientShaperSection_generateControlSignal(This, This->inputBuffer, samplesProcessing);
        
        // delay the input signal to enable faster response time without clicks
        const float *inputs [2] = {inputL+sampleProcessed, inputR+sampleProcessed};
        float* outputs [2] = {This->b1, This->b2};
        size_t numChannels = 2;
        BMShortSimpleDelay_process(&This->dly, inputs, outputs, numChannels, samplesProcessing);
        
        // apply the volume control signal to the delayed input
        //control signal -
        vDSP_vmul(This->b1, 1,  This->releaseControlSignal, 1, outputL+sampleProcessed, 1, samplesProcessing);
        vDSP_vmul(This->b2, 1,  This->releaseControlSignal, 1, outputR+sampleProcessed, 1, samplesProcessing);
        
        sampleProcessed += samplesProcessing;
    }
}

/*!
 *BMAttackShaperSection_processMono
 */
void BMTransientShaperSection_processMono(BMTransientShaperSection *This,
                                       float* input,
                                       float* output,
                                       size_t numSamples){
    
    size_t sampleProcessed = 0;
    while(sampleProcessed<numSamples){
        size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE,numSamples-sampleProcessed);

        // generate a control signal to modulate the volume of the input
        
        BMTransientShaperSection_generateControlSignal(This, input+sampleProcessed, samplesProcessing);
        
        // delay the input signal to enable faster response time without clicks
        const float *inputs [1] = {input+sampleProcessed};
        float *outputs [1] = {This->b1};
        BMShortSimpleDelay_process(&This->dly, inputs, outputs, 1, samplesProcessing);
        
        // apply the volume control signal to the delayed input
        vDSP_vmul(This->b1, 1, This->releaseControlSignal, 1, output+sampleProcessed, 1, samplesProcessing);
        
        sampleProcessed += samplesProcessing;
    }
}









bool BMTransientShaperSection_sidechainNoiseGateIsOpen(BMTransientShaperSection *This){
    return This->noiseGateIsOpen;
}


/*
 BMTransientShaper ------------------------------
 
 */
void BMTransientShaper_init(BMTransientShaper *This, bool isStereo, float sampleRate,bool isTesting){
    This->isStereo = isStereo;
    This->asSections = malloc(sizeof(BMTransientShaperSection)*BMTS_NUM_SECTIONS);
    
    // init the 2-way crossover
    float crossover2fc = 400.0;
    BMCrossover_init(&This->crossover2, crossover2fc, sampleRate, true, isStereo);
    
    float attackFC = 40.0f;
    float releaseFC = 20.0f;
    float dsfSensitivity = 1000.0f;
    float dsfFcMin = releaseFC;
    float dsfFcMax = 1000.0f;
    float exaggeration = 1.0f;
    BMTransientShaperSection_init(&This->asSections[0],
                               releaseFC,
                               attackFC,
                               dsfSensitivity,
                               dsfFcMin, dsfFcMax,
                               exaggeration,
                               sampleRate,
                               isStereo,false);
    
    dsfFcMax = 1000.0f;
    BMTransientShaperSection_init(&This->asSections[1],
                               releaseFC*BMTS_SECTION_2_RF_MULTIPLIER,
                               attackFC*BMTS_SECTION_2_AF_MULTIPLIER,
                               dsfSensitivity,
                               dsfFcMin, dsfFcMax,
                               exaggeration*BMTS_SECTION_2_EXAGGERATION_MULTIPLIER,
                               sampleRate,
                               isStereo,false);
    
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
    BMTransientShaper_setSidechainNoiseGateThreshold(This, -45.0f);
    
    // process a few buffers of silence to get the filters warmed up
    float* input = calloc(256,sizeof(float));
    float* outputL = calloc(256,sizeof(float));
    float* outputR = calloc(256,sizeof(float));
    
    This->asSections[0].isTesting = false;
    This->asSections[1].isTesting = false;
    if(isStereo)
        for(size_t i=0; i<10; i++)
            BMTransientShaper_processStereo(This, input, input, outputL, outputR, 256);
    else
        for(size_t i=0; i<10; i++)
            BMTransientShaper_processMono(This, input, outputL, 256);
    
    This->asSections[0].isTesting = isTesting;
    This->asSections[1].isTesting = isTesting;
    
    free(input);
    free(outputL);
    free(outputR);
}





void BMTransientShaper_free(BMTransientShaper *This){
    BMTransientShaperSection_free(&This->asSections[0]);
    BMTransientShaperSection_free(&This->asSections[1]);
    BMCrossover_free(&This->crossover2);
    free(This->b1L);
    This->b1L = NULL;
    
    free(This->asSections);
}





float BMTransientShaper_getLatencyInSeconds(BMTransientShaper *This){
    return (float)This->asSections->dly.delayLength / This->asSections[0].sampleRate;
}






void BMTransientShaper_processStereo(BMTransientShaper *This,
                                           const float *inputL, const float *inputR,
                                           float *outputL, float *outputR,
                                           size_t numSamples){
    assert(This->isStereo);
    
    size_t samplesProcessed = 0;
    size_t samplesProcessing;
    while(numSamples-samplesProcessed > 0){
        samplesProcessing = BM_MIN(numSamples-samplesProcessed, BM_BUFFER_CHUNK_SIZE);
        
//        // split the signal into two bands
//        BMCrossover_processStereo(&This->crossover2, inputL, inputR, This->b1L, This->b1R, This->b2L, This->b2R, samplesProcessing);
//
//
//        // process transient shapers on each band
//        BMTransientShaperSection_processStereo(&This->asSections[0], This->b1L, This->b1R, This->b1L, This->b1R, samplesProcessing);
//        BMTransientShaperSection_processStereo(&This->asSections[1], This->b2L, This->b2R, This->b2L, This->b2R, samplesProcessing);
//
//
//        // recombine the signal
//        vDSP_vadd(This->b1L, 1, This->b2L, 1, outputL, 1, samplesProcessing);
//        vDSP_vadd(This->b1R, 1, This->b2R, 1, outputR, 1, samplesProcessing);
        
        BMTransientShaperSection_processStereo(&This->asSections[0],inputL + samplesProcessed, inputR+ samplesProcessed, outputL+ samplesProcessed, outputR+ samplesProcessed, samplesProcessing);

        
        // advance pointers
        samplesProcessed += samplesProcessing;
    }
}

void BMTransientShaper_processMono(BMTransientShaper *This,
                                         const float *input,
                                         float *output,
                                         size_t numSamples){
    assert(!This->isStereo);
    
    while(numSamples > 0){
        size_t samplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
        
        // split the signal into two bands
        BMCrossover_processMono(&This->crossover2, input, This->b1L, This->b2L, samplesProcessing);

        // process transient shapers on each band
        BMTransientShaperSection_processMono(&This->asSections[0], This->b1L, This->b1L, samplesProcessing);
        BMTransientShaperSection_processMono(&This->asSections[1], This->b2L, This->b2L, samplesProcessing);

        // recombine the signal
        vDSP_vadd(This->b1L, 1, This->b2L, 1, output, 1, samplesProcessing);
        
        // advance pointers
        numSamples -= samplesProcessing;
        input += samplesProcessing;
        output += samplesProcessing;
    }
}

void BMTransientShaper_processStereoTest(BMTransientShaper *This,
                                           const float *inputL, const float *inputR,
                                           float *outputL, float *outputR,float* outCS1,float* outCS2,float* outCS3,
                                           size_t numSamples){
    assert(This->isStereo);
    
    size_t samplesProcessed = 0;
    size_t samplesProcessing;
    while(numSamples-samplesProcessed > 0){
        samplesProcessing = BM_MIN(numSamples-samplesProcessed, BM_BUFFER_CHUNK_SIZE);
        This->asSections[0].testBuffer1 = outCS1 + samplesProcessed;
        This->asSections[0].testBuffer2 = outCS2 + samplesProcessed;
        This->asSections[0].testBuffer3 = outCS3 + samplesProcessed;
        
        // process transient shapers on each band
        BMTransientShaperSection_processStereo(&This->asSections[0], inputL+ samplesProcessed, inputR+ samplesProcessed, outputL+ samplesProcessed, outputR+ samplesProcessed, samplesProcessing);
        

        
        // advance pointers
        samplesProcessed += samplesProcessing;
    }
}

void BMTransientShaper_setAttackTime(BMTransientShaper *This, float attackTimeInSeconds){
    // find the lpf cutoff frequency that corresponds to the specified attack time
    float attackFc = ARTimeToCutoffFrequency(attackTimeInSeconds, BMTS_AF_NUMLEVELS);
    
    BMTransientShaperSection_setAttackSlowFC(&This->asSections[0], attackFc);
    BMTransientShaperSection_setAttackSlowFC(&This->asSections[1], attackFc*BMTS_SECTION_2_AF_MULTIPLIER);
    
    float instanceFc = BM_MIN(attackFc * 0.6666f, 20.0f);
    BMTransientShaperSection_setAttackInstanceFC(&This->asSections[0], instanceFc);
    BMTransientShaperSection_setAttackInstanceFC(&This->asSections[1], instanceFc*BMTS_SECTION_2_RF_MULTIPLIER);
    
    float dsMinFc = instanceFc;
    BMTransientShaperSection_setDSMinFrequency(&This->asSections[0], dsMinFc);
    BMTransientShaperSection_setDSMinFrequency(&This->asSections[1], dsMinFc);
    
    
}
    


void BMTransientShaper_setAttackDepth(BMTransientShaper *This, float depth){
    for(size_t i=0; i<BMTS_NUM_SECTIONS; i++)
        This->asSections[i].attackDepth = depth;
}

void BMTransientShaper_setReleaseTime(BMTransientShaper *This, float releaseTimeInSeconds){
    // find the lpf cutoff frequency that corresponds to the specified attack time
    float slowReleaseFC = ARTimeToCutoffFrequency(releaseTimeInSeconds, BMTS_RRF_NUMLEVELS);
    
    slowReleaseFC *= 0.3f;//0.05
    BMTransientShaperSection_setSustainSlowFC(&This->asSections[0], slowReleaseFC);
    BMTransientShaperSection_setSustainSlowFC(&This->asSections[1], slowReleaseFC*BMTS_SECTION_2_RF_MULTIPLIER);
    
    float fastFC = slowReleaseFC*1.6f;
    BMTransientShaperSection_setSustainFastFC(&This->asSections[0], fastFC);
    BMTransientShaperSection_setSustainFastFC(&This->asSections[1], fastFC*BMTS_SECTION_2_RF_MULTIPLIER);
    
    This->asSections[0].sustainExaggeration = 1.0f;//(releaseTimeInSeconds-0.1f)/0.9f * 4.5f + 1.0f;
    This->asSections[1].sustainExaggeration = This->asSections[0].sustainExaggeration;
}
    


void BMTransientShaper_setReleaseDepth(BMTransientShaper *This, float depth){
    for(size_t i=0; i<BMTS_NUM_SECTIONS; i++)
        This->asSections[i].releaseDepth = depth;
}

void BMTransientShaper_setSidechainNoiseGateThreshold(BMTransientShaper *This, float thresholdDb){
    // Don't allow the noise gate threshold to be set lower than the
    // gain setting the gate takes when it's closed
    assert(thresholdDb > BMTS_NOISE_GATE_CLOSED_LEVEL);
    
    for(size_t i=0; i<BMTS_NUM_SECTIONS; i++)
        This->asSections[i].noiseGateThreshold = BM_DB_TO_GAIN(thresholdDb);
}



bool BMTransientShaper_sidechainNoiseGateIsOpen(BMTransientShaper *This){
    // return true if either one of the attack shaper sections has its gate open
    return This->asSections[0].noiseGateIsOpen || This->asSections[1].noiseGateIsOpen;
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
void BMTransientShaper_upperLimit(float limit, const float* input, float *output, size_t numSamples){
    vDSP_vneg(input, 1, output, 1, numSamples);
    limit = -limit;
    vDSP_vthr(output, 1, &limit, output, 1, numSamples);
    vDSP_vneg(output, 1, output, 1, numSamples);
}

