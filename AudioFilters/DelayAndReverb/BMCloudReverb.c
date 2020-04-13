//
//  BMCloudReverb.c
//  AUCloudReverb
//
//  Created by Nguyen Minh Tien on 2/27/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMCloudReverb.h"
#include "BMReverb.h"



#define Filter_Level_Lowshelf 0
#define Filter_Level_Tone 1
#define Filter_Level_Lowpass10k 2
#define Filter_Level_TotalLP 3

#define Filter_TotalLevel Filter_Level_Lowpass10k + Filter_Level_TotalLP

#define Filter_LS_FC 400

void BMCloudReverb_updateDiffusion(BMCloudReverb* This);
void BMCloudReverb_prepareLoopDelay(BMCloudReverb* This);
void BMCloudReverb_updateLoopGain(BMCloudReverb* This,size_t* delayTimeL,size_t* delayTimeR,float* gainL,float* gainR);
float calculateScaleVol(BMCloudReverb* This);
void BMCloudReverb_updateVND(BMCloudReverb* This);

float getVNDLength(float numTaps,float length){
    float vndLength = ((numTaps*numTaps)*length)/(1 + numTaps + numTaps*numTaps);
    return vndLength;
}

float randomAround1(int percent){
    int rdPercent = percent - 1 - (arc4random()%(percent*2 - 1)) + 100;
    float rand = rdPercent/100.0f;
    return rand;
}

void BMCloudReverb_init(BMCloudReverb* This,float sr){
    This->sampleRate = sr;
    //BIQUAD FILTER
    BMMultiLevelBiquad_init(&This->biquadFilter, Filter_TotalLevel, sr, true, false, true);
    //Tone control - use 6db
	BMCloudReverb_setHighCutFreq(This, 1200.0f);
    //lowpass 36db
	BMMultiLevelBiquad_setHighOrderBWLP(&This->biquadFilter, 9300, Filter_Level_Lowpass10k, Filter_Level_TotalLP);
    //Low shelf
    This->lsGain = 0;
	BMCloudReverb_setLSGain(This, This->lsGain);
    
    //VND
    This->updateVND = false;
    This->maxTapsEachVND = 24.0f;
    This->diffusion = 1.0f;
    This->vndLength = 0.25f;
    BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->vnd1, This->vndLength, This->maxTapsEachVND, 100, false, sr);
    BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->vnd2, This->vndLength, This->maxTapsEachVND, 100, false, sr);

    //Pitch shifting
    float delaySampleRange = 0.02f*sr;
    float duration = 20.0f;
    BMPitchShiftDelay_init(&This->pitchDelay, duration,delaySampleRange , delaySampleRange, sr);
    
    
    This->buffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->buffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->LFOBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->LFOBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->loopInput.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->loopInput.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->lastLoopBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->lastLoopBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    memset(This->lastLoopBuffer.bufferL, 0, sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    memset(This->lastLoopBuffer.bufferR, 0, sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    This->wetBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->wetBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    //Loop delay
    BMCloudReverb_prepareLoopDelay(This);
    
    BMCloudReverb_setLoopDecayTime(This, 10);
    BMCloudReverb_setDelayPitchMixer(This, 0.5f);
    BMWetDryMixer_init(&This->reverbMixer, sr);
    BMCloudReverb_setOutputMixer(This, 0.5f);
    
    //Pan
    BMPanLFO_init(&This->inputPan, 0.25f, 0.4f, sr);
    BMPanLFO_init(&This->outputPan, 0.25f, 0.4f, sr);
}

void BMCloudReverb_prepareLoopDelay(BMCloudReverb* This){
    size_t numDelays = 32;
    float maxDT = 0.20f;
    float minDT = 0.02f;
	bool zeroTaps = true;
    BMLongLoopFDN_init(&This->loopFND, numDelays, minDT, maxDT, zeroTaps, 4, 1, This->sampleRate);
}

void BMCloudReverb_destroy(BMCloudReverb* This){
    BMMultiLevelBiquad_free(&This->biquadFilter);
    
    BMVelvetNoiseDecorrelator_free(&This->vnd1);
    BMVelvetNoiseDecorrelator_free(&This->vnd2);
    BMVelvetNoiseDecorrelator_free(&This->vnd3);
    
    BMPitchShiftDelay_destroy(&This->pitchDelay);
    
    BMLongLoopFDN_free(&This->loopFND);
    
    
    free(This->buffer.bufferL);
    This->buffer.bufferL = nil;
    free(This->buffer.bufferR);
    This->buffer.bufferR = nil;
    free(This->LFOBuffer.bufferL);
    This->LFOBuffer.bufferL = nil;
    free(This->LFOBuffer.bufferR);
    This->LFOBuffer.bufferR = nil;
    free(This->loopInput.bufferL);
    This->loopInput.bufferL = nil;
    free(This->loopInput.bufferR);
    This->loopInput.bufferR = nil;
    free(This->lastLoopBuffer.bufferL);
    This->lastLoopBuffer.bufferL = nil;
    free(This->lastLoopBuffer.bufferR);
    This->lastLoopBuffer.bufferR = nil;
    free(This->wetBuffer.bufferL);
    This->wetBuffer.bufferL = nil;
    free(This->wetBuffer.bufferR);
    This->wetBuffer.bufferR = nil;
}

void BMCloudReverb_processStereo(BMCloudReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t numSamples){
    BMCloudReverb_updateDiffusion(This);
    BMCloudReverb_updateVND(This);
    
    //Quadrature oscilliscope
    BMPanLFO_process(&This->inputPan, This->LFOBuffer.bufferL, This->LFOBuffer.bufferR, numSamples);
    vDSP_vmul(inputL, 1, This->LFOBuffer.bufferL, 1, This->buffer.bufferL, 1, numSamples);
    vDSP_vmul(inputR, 1, This->LFOBuffer.bufferR, 1, This->buffer.bufferR, 1, numSamples);
    
//    memcpy(outputL, This->buffer.bufferL, sizeof(float)*numSamples);
//    memcpy(outputR, This->buffer.bufferR, sizeof(float)*numSamples);
//    return;
    
    //Filters
    BMMultiLevelBiquad_processBufferStereo(&This->biquadFilter, This->buffer.bufferL, This->buffer.bufferR, This->buffer.bufferL, This->buffer.bufferR, numSamples);
    
    //VND
    BMVelvetNoiseDecorrelator_processBufferStereo(&This->vnd1, This->buffer.bufferL, This->buffer.bufferR, This->buffer.bufferL, This->buffer.bufferR, numSamples);
    BMVelvetNoiseDecorrelator_processBufferStereo(&This->vnd2, This->buffer.bufferL, This->buffer.bufferR, This->buffer.bufferL, This->buffer.bufferR, numSamples);

    
    //PitchShifting delay into wetbuffer
    BMPitchShiftDelay_processStereoBuffer(&This->pitchDelay, This->buffer.bufferL, This->buffer.bufferR, This->wetBuffer.bufferL, This->wetBuffer.bufferR, numSamples);
    
    BMLongLoopFDN_process(&This->loopFND, This->wetBuffer.bufferL, This->wetBuffer.bufferR, This->wetBuffer.bufferL, This->wetBuffer.bufferR, numSamples);

    vDSP_vsmul(This->wetBuffer.bufferL, 1, &This->normallizeVol, This->wetBuffer.bufferL, 1, numSamples);
    vDSP_vsmul(This->wetBuffer.bufferR, 1, &This->normallizeVol, This->wetBuffer.bufferR, 1, numSamples);
    
    //Process reverb dry/wet mixer
    BMWetDryMixer_processBufferInPhase(&This->reverbMixer, This->wetBuffer.bufferL, This->wetBuffer.bufferR, inputL, inputR, outputL, outputR, numSamples);
    
    //Quadrature oscilliscope
    BMPanLFO_process(&This->outputPan, This->LFOBuffer.bufferL, This->LFOBuffer.bufferR, numSamples);
    vDSP_vmul(outputL, 1, This->LFOBuffer.bufferL, 1, outputL, 1, numSamples);
    vDSP_vmul(outputR, 1, This->LFOBuffer.bufferR, 1, outputR, 1, numSamples);
}

float calculateScaleVol(BMCloudReverb* This){
    float factor = log2f(This->decayTime);
    float scaleDB = -3 * (factor);
    return scaleDB;
}

#pragma mark - Set
void BMCloudReverb_setLoopDecayTime(BMCloudReverb* This,float decayTime){
    This->decayTime = decayTime;
    BMLongLoopFDN_setRT60Decay(&This->loopFND, decayTime);
    This->normallizeVol = BM_DB_TO_GAIN(calculateScaleVol(This));
}

void BMCloudReverb_setDelayPitchMixer(BMCloudReverb* This,float wetMix){
    BMPitchShiftDelay_setWetGain(&This->pitchDelay, wetMix);
    BMPanLFO_setDepth(&This->inputPan, wetMix);
    BMPanLFO_setDepth(&This->outputPan, wetMix);
}

void BMCloudReverb_setOutputMixer(BMCloudReverb* This,float wetMix){
    BMWetDryMixer_setMix(&This->reverbMixer, wetMix);
}

void BMCloudReverb_setDiffusion(BMCloudReverb* This,float diffusion){
    This->diffusion = diffusion;
    This->updateDiffusion = true;
}

void BMCloudReverb_updateDiffusion(BMCloudReverb* This){
    if(This->updateDiffusion){
        This->updateDiffusion = false;
        float numTaps = roundf(This->maxTapsEachVND * This->diffusion);
        BMVelvetNoiseDecorrelator_setNumTaps(&This->vnd1, numTaps);
        BMVelvetNoiseDecorrelator_setNumTaps(&This->vnd2, numTaps);
        BMVelvetNoiseDecorrelator_setNumTaps(&This->vnd3, numTaps);
    }
}

void BMCloudReverb_setLSGain(BMCloudReverb* This,float gainDb){
    BMMultiLevelBiquad_setLowShelfFirstOrder(&This->biquadFilter, Filter_LS_FC, gainDb, Filter_Level_Lowshelf);
}

void BMCloudReverb_setHighCutFreq(BMCloudReverb* This,float freq){
    BMMultiLevelBiquad_setLowPass6db(&This->biquadFilter, freq, Filter_Level_Tone);
}

#pragma mark - VND
void BMCloudReverb_setVNDLength(BMCloudReverb* This,float timeInS){
    This->vndLength = timeInS;
    This->updateVND = true;
}

void BMCloudReverb_updateVND(BMCloudReverb* This){
    if(This->updateVND){
        This->updateVND = false;
        BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->vnd1, This->vndLength, This->maxTapsEachVND, 100, false, This->sampleRate);
        BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->vnd2, This->vndLength, This->maxTapsEachVND, 100, false, This->sampleRate);
    }
}

#pragma mark - Test
void BMCloudReverb_impulseResponse(BMCloudReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t length){
    size_t sampleProcessed = 0;
    size_t sampleProcessing = 0;
    This->biquadFilter.useSmoothUpdate = false;
    while(sampleProcessed<length){
        sampleProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, length - sampleProcessed);
        
        BMCloudReverb_processStereo(This, inputL+sampleProcessed, inputR+sampleProcessed, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing);
        if(outputL[sampleProcessed]>1.0f)
            printf("error\n");
        sampleProcessed += sampleProcessing;
    }
    This->biquadFilter.useSmoothUpdate = true;
}
