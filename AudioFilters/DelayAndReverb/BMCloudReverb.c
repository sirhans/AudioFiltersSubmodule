//
//  BMCloudReverb.c
//  AUCloudReverb
//
//  Created by Nguyen Minh Tien on 2/27/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMCloudReverb.h"
#include "BMReverb.h"

#define Filter_Level_Lowpass 0
#define Filter_Level_Bell 1
#define Filter_Level_Highpass 2
#define Filter_Level_Lowshelf 3

#define Filter_LS_FC 400

void BMCloudReverb_updateDiffusion(BMCloudReverb* This);
void BMCloudReverb_prepareLoopDelay(BMCloudReverb* This);
void BMCloudReverb_updateLoopDelayTime(BMCloudReverb* This,size_t* delayTimeL,size_t* delayTimeR,float* gainL,float* gainR,float baseS);
void BMCloudReverb_updateLoopGain(BMCloudReverb* This,size_t* delayTimeL,size_t* delayTimeR,float* gainL,float* gainR);


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
    BMMultiLevelBiquad_init(&This->biquadFilter, 4, sr, true, false, true);
    //Tone control - use 12db
    BMMultiLevelBiquad_setLowPass12db(&This->biquadFilter, 4000, Filter_Level_Lowpass);
    //Bell
    BMMultiLevelBiquad_setBellQ(&This->biquadFilter, 1300, sqrtf(0.5f), -5, Filter_Level_Bell);
    //High passs
    BMMultiLevelBiquad_setHighPass6db(&This->biquadFilter, 100, Filter_Level_Highpass);
    //Low shelf
    BMMultiLevelBiquad_setLowShelf(&This->biquadFilter, Filter_LS_FC, 2, Filter_Level_Lowshelf);
    
    //VND
    float totalS = 0.5f;
    This->maxTapsEachVND = 16.0f;
    This->diffusion = 1.0f;
    float vnd1Length = totalS/3.0f;//getVNDLength(numTaps, totalS);
    BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->vnd1, vnd1Length, This->maxTapsEachVND, 100, false, sr);
    BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->vnd2, vnd1Length, This->maxTapsEachVND, 100, false, sr);
    //Last vnd dont have dry tap -> always wet 100%
    BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->vnd3,vnd1Length , This->maxTapsEachVND, 100, false, sr);

    //Pitch shifting
    float delaySampleRange = 0.02f*sr;
    float duration = 20.0f;
    BMPitchShiftDelay_init(&This->pitchDelay, duration,delaySampleRange , delaySampleRange, sr);
    
    
    This->buffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->buffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->loopInput.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->loopInput.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->lastLoopBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->lastLoopBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    memset(This->lastLoopBuffer.bufferL, 0, sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    memset(This->lastLoopBuffer.bufferR, 0, sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    This->wetBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->wetBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    BMCloudReverb_setLoopDecayTime(This, 10);
    BMCloudReverb_setDelayPitchMixer(This, 0.5f);
    BMWetDryMixer_init(&This->reverbMixer, sr);
    BMCloudReverb_setOutputMixer(This, 0.5f);
    
    //Loop delay
    BMCloudReverb_prepareLoopDelay(This);
}

void BMCloudReverb_prepareLoopDelay(BMCloudReverb* This){
    float totalTime = 2.0f;
    BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->loopVND, totalTime, 16, This->decayTime, true, This->sampleRate);
    BMVelvetNoiseDecorrelator_setWetMix(&This->loopVND, 1.0f);
    
//    BMCloudReverb_updateLoopDelayTime(This, This->loopDT.bufferL, This->loopDT.bufferR, This->loopGain.bufferL, This->loopGain.bufferR,baseS);
    
}

void BMCloudReverb_destroy(BMCloudReverb* This){
    BMMultiLevelBiquad_free(&This->biquadFilter);
    
    BMVelvetNoiseDecorrelator_free(&This->vnd1);
    BMVelvetNoiseDecorrelator_free(&This->vnd2);
    BMVelvetNoiseDecorrelator_free(&This->vnd3);
    
    BMPitchShiftDelay_destroy(&This->pitchDelay);
    
    BMVelvetNoiseDecorrelator_free(&This->loopVND);
    
    free(This->buffer.bufferL);
    This->buffer.bufferL = nil;
    free(This->buffer.bufferR);
    This->buffer.bufferR = nil;
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
    //Filters
    BMMultiLevelBiquad_processBufferStereo(&This->biquadFilter, inputL, inputR, This->buffer.bufferL, This->buffer.bufferR, numSamples);
    
    //VND
    BMVelvetNoiseDecorrelator_processBufferStereo(&This->vnd1, This->buffer.bufferL, This->buffer.bufferR, This->buffer.bufferL, This->buffer.bufferR, numSamples);
    BMVelvetNoiseDecorrelator_processBufferStereo(&This->vnd2, This->buffer.bufferL, This->buffer.bufferR, This->buffer.bufferL, This->buffer.bufferR, numSamples);
    BMVelvetNoiseDecorrelator_processBufferStereo(&This->vnd3, This->buffer.bufferL, This->buffer.bufferR, This->buffer.bufferL, This->buffer.bufferR, numSamples);
    
//    memcpy(outputL, This->buffer.bufferL, sizeof(float)*numSamples);
//    memcpy(outputR, This->buffer.bufferR, sizeof(float)*numSamples);
//    return;
    
    //PitchShifting delay into wetbuffer
    BMPitchShiftDelay_processStereoBuffer(&This->pitchDelay, This->buffer.bufferL, This->buffer.bufferR, This->wetBuffer.bufferL, This->wetBuffer.bufferR, numSamples);
    
    //START
    //Delay loop - add lastloop buffer to current wetbuffer
    vDSP_vadd(This->wetBuffer.bufferL, 1, This->lastLoopBuffer.bufferL, 1, This->loopInput.bufferL, 1, numSamples);
    vDSP_vadd(This->wetBuffer.bufferR, 1, This->lastLoopBuffer.bufferR, 1, This->loopInput.bufferR, 1, numSamples);
    
    BMVelvetNoiseDecorrelator_processBufferStereoWithFinalOutput(&This->loopVND, This->loopInput.bufferL, This->loopInput.bufferR, This->wetBuffer.bufferL, This->wetBuffer.bufferR, This->lastLoopBuffer.bufferR, This->lastLoopBuffer.bufferL, numSamples);
    
    
    //Process reverb dry/wet mixer
    BMWetDryMixer_processBufferInPhase(&This->reverbMixer, This->wetBuffer.bufferL, This->wetBuffer.bufferR, inputL, inputR, outputL, outputR, numSamples);
}

#pragma mark - Set
void BMCloudReverb_setLoopDecayTime(BMCloudReverb* This,float decayTime){
    This->decayTime = decayTime;
    BMVelvetNoiseDecorrelator_setRT60DecayTime(&This->loopVND, decayTime);
}

void BMCloudReverb_setDelayPitchMixer(BMCloudReverb* This,float wetMix){
    BMPitchShiftDelay_setWetGain(&This->pitchDelay, wetMix);
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
    BMMultiLevelBiquad_setLowShelf(&This->biquadFilter, Filter_LS_FC, gainDb, Filter_Level_Lowshelf);
}

void BMCloudReverb_setHighCutFreq(BMCloudReverb* This,float freq){
    BMMultiLevelBiquad_setLowPass12db(&This->biquadFilter, freq, Filter_Level_Lowpass);
}

#pragma mark - Test
void BMCloudReverb_impulseResponse(BMCloudReverb* This,float* outputL,float* outputR,size_t length){
    float* data = malloc(sizeof(float)*length);
    memset(data, 0, sizeof(float)*length);
    data[0] = 1.0f;
    size_t sampleProcessed = 0;
    size_t sampleProcessing = 0;
    while(sampleProcessed<length){
        sampleProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, length - sampleProcessed);
        
        BMCloudReverb_processStereo(This, data+sampleProcessed, data+sampleProcessed, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing);
        
        sampleProcessed += sampleProcessing;
    }
    
}
