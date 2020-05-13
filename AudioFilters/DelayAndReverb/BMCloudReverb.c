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
#define PitchShift_BaseNote 0.14f
#define PitchShift_BaseDuration 10.0f
#define ReadyNo 98573

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
    This->maxTapsEachVND = 16.0;
    This->diffusion = 1.0f;
    This->vndLength = 0.1f;
    
    This->numInput = 4;
    This->numVND = This->numInput*2;
    This->vndArray = malloc(sizeof(BMVelvetNoiseDecorrelator)*This->numVND);
    This->vndBufferL = malloc(sizeof(float*)*This->numInput);
    This->vndBufferR = malloc(sizeof(float*)*This->numInput);
    
    //Using 2 layer of vnd
    for(int i=0;i<This->numVND;i++){
        BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->vndArray[i], This->vndLength, This->maxTapsEachVND, 100, false, sr);
        BMVelvetNoiseDecorrelator_setFadeIn(&This->vndArray[i], 0.0f);
    }
    //Init temp buffer
    for(int i=0;i<This->numInput;i++){
        This->vndBufferL[i] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->vndBufferR[i] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    }
    
    //Pitch shifting
    This->numPitchShift = floorf(This->numInput * 0.5f);
    //Max delay range is when the pitchshift is equal baseNote
    float sampleToReachTarget = PitchShift_BaseDuration* This->sampleRate;
    Float64 pitchShift = powf(2, PitchShift_BaseNote/12.0f);
    size_t maxDelayRange = fabs(1.0f - pitchShift)*sampleToReachTarget;
    
    This->pitchShiftArray = malloc(sizeof(BMPitchShiftDelay)*This->numPitchShift);
    for(int i=0;i<This->numPitchShift;i++){
        BMPitchShiftDelay_init(&This->pitchShiftArray[i], PitchShift_BaseDuration,maxDelayRange , maxDelayRange, sr,fmodf(i, 2)==0);
        BMPitchShiftDelay_setWetGain(&This->pitchShiftArray[i], 1.0f);
    }
    
    
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
    BMPanLFO_init(&This->inputPan, 0.412f, 0.6f, sr);
    BMPanLFO_init(&This->outputPan, 0.5456f, 0.6f, sr);
    
    This->initNo = ReadyNo;
}

void BMCloudReverb_prepareLoopDelay(BMCloudReverb* This){
    size_t numDelays = 8;
    float maxDT = 0.20f;
    float minDT = 0.02f;
	bool zeroTaps = true;
    BMLongLoopFDN_init(&This->loopFND, numDelays, minDT, maxDT, zeroTaps, 4, 1, This->sampleRate);
}

void BMCloudReverb_destroy(BMCloudReverb* This){
    BMMultiLevelBiquad_free(&This->biquadFilter);
    
    for(int i=0;i<This->numVND;i++){
        BMVelvetNoiseDecorrelator_free(&This->vndArray[i]);
    }
    for(int i=0;i<This->numInput;i++){
        free(This->vndBufferL[i]);
        This->vndBufferL[i] = nil;
        free(This->vndBufferR[i]);
        This->vndBufferR[i] = nil;
    }
    
    free(This->vndBufferL);
    free(This->vndBufferR);
    
    for(int i =0;i<This->numPitchShift;i++)
        BMPitchShiftDelay_destroy(&This->pitchShiftArray[i]);
    
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

void BMCloudReverb_processStereo(BMCloudReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t numSamples,bool offlineRendering){
    if(This->initNo==ReadyNo){
        assert(numSamples<=BM_BUFFER_CHUNK_SIZE);
        
        BMCloudReverb_updateDiffusion(This);
        BMCloudReverb_updateVND(This);
        
        //Filters
        BMMultiLevelBiquad_processBufferStereo(&This->biquadFilter, inputL, inputR, This->buffer.bufferL, This->buffer.bufferR, numSamples);
        
        for(int i=0;i<This->numInput;i++){
            //PitchShifting delay into wetbuffer
            BMVelvetNoiseDecorrelator_processBufferStereo(&This->vndArray[i*2], This->buffer.bufferL, This->buffer.bufferR,This->buffer.bufferL, This->buffer.bufferR, numSamples);
            BMVelvetNoiseDecorrelator_processBufferStereo(&This->vndArray[i*2+1], This->buffer.bufferL, This->buffer.bufferR, This->vndBufferL[i], This->vndBufferR[i], numSamples);
            
            if(i<This->numPitchShift){
                //PitchShifting delay
                BMPitchShiftDelay_processStereoBuffer(&This->pitchShiftArray[i], This->vndBufferL[i], This->vndBufferR[i], This->vndBufferL[i], This->vndBufferR[i], numSamples);
            }
        }
        
        BMLongLoopFDN_processMultiChannelInput(&This->loopFND, This->vndBufferL, This->vndBufferR, This->numInput, This->wetBuffer.bufferL, This->wetBuffer.bufferR, numSamples);
		
        vDSP_vsmul(This->wetBuffer.bufferL, 1, &This->normallizeVol, This->wetBuffer.bufferL, 1, numSamples);
        vDSP_vsmul(This->wetBuffer.bufferR, 1, &This->normallizeVol, This->wetBuffer.bufferR, 1, numSamples);
        
        if(offlineRendering){
            float dryMix = 1 - This->reverbMixer.mixTarget;
            vDSP_vsmsma(This->wetBuffer.bufferL, 1, &This->reverbMixer.mixTarget, inputL, 1, &dryMix, outputL, 1, numSamples);
            vDSP_vsmsma(This->wetBuffer.bufferR, 1, &This->reverbMixer.mixTarget, inputR, 1, &dryMix, outputR, 1, numSamples);
        }else{
            //Process reverb dry/wet mixer
            BMWetDryMixer_processBufferInPhase(&This->reverbMixer, This->wetBuffer.bufferL, This->wetBuffer.bufferR, inputL, inputR, outputL, outputR, numSamples);
        }
        
//        memcpy(outputL,This->wetBuffer.bufferL, sizeof(float)*numSamples);
//        memcpy(outputR,This->wetBuffer.bufferL, sizeof(float)*numSamples);
//        return;
        
        //LFO pan
		// Input pan LFO
        BMPanLFO_process(&This->inputPan, This->LFOBuffer.bufferL, This->LFOBuffer.bufferR, numSamples);
        vDSP_vmul(outputL, 1, This->LFOBuffer.bufferL, 1, outputL, 1, numSamples);
        vDSP_vmul(outputR, 1, This->LFOBuffer.bufferR, 1, outputR, 1, numSamples);
        BMPanLFO_process(&This->outputPan, This->LFOBuffer.bufferL, This->LFOBuffer.bufferR, numSamples);
        vDSP_vmul(outputL, 1, This->LFOBuffer.bufferL, 1, outputL, 1, numSamples);
        vDSP_vmul(outputR, 1, This->LFOBuffer.bufferR, 1, outputR, 1, numSamples);
        
//        memcpy(outputL, This->wetBuffer.bufferL, sizeof(float)*numSamples);
//        memcpy(outputR, This->wetBuffer.bufferR, sizeof(float)*numSamples);
//        return;
    }
}

float calculateScaleVol(BMCloudReverb* This){
    float factor = log2f(This->decayTime);
    float scaleDB = (-3 * (factor)) + 14.0f;
    return scaleDB;
}

#pragma mark - Set
void BMCloudReverb_setLoopDecayTime(BMCloudReverb* This,float decayTime){
    This->decayTime = decayTime;
    BMLongLoopFDN_setRT60Decay(&This->loopFND, decayTime);
    This->normallizeVol = BM_DB_TO_GAIN(calculateScaleVol(This));
}

#define DepthMax 0.25f
void BMCloudReverb_setDelayPitchMixer(BMCloudReverb* This,float wetMix){
    //Set delayrange of pitch shift to control speed of pitch shift
    float baseNote = wetMix * PitchShift_BaseNote;
    
    float sampleToReachTarget = PitchShift_BaseDuration * This->sampleRate;
    for(int i=0;i<This->numPitchShift;i++){
         float newNote = baseNote/(i+1.0f);
        Float64 pitchShift = powf(2, newNote/12.0f);
        size_t delaySampleRange = fabs(1.0f - pitchShift)*sampleToReachTarget;
        BMPitchShiftDelay_setDelayRange(&This->pitchShiftArray[i], delaySampleRange);
		
		// reduce the sampes to reach target a little for the next pitch shifter to ensure that they do not all change direction at the same time.
		sampleToReachTarget *= 0.97f;
    }
    //Control pan
    BMPanLFO_setDepth(&This->inputPan, wetMix*DepthMax);
    BMPanLFO_setDepth(&This->outputPan, wetMix*DepthMax);
}

void BMCloudReverb_setOutputMixer(BMCloudReverb* This,float wetMix){
    BMWetDryMixer_setMix(&This->reverbMixer, wetMix*wetMix);
}

void BMCloudReverb_setDiffusion(BMCloudReverb* This,float diffusion){
    This->diffusion = diffusion;
    This->updateDiffusion = true;
}

void BMCloudReverb_updateDiffusion(BMCloudReverb* This){
    if(This->updateDiffusion){
        This->updateDiffusion = false;
        float numTaps = roundf(This->maxTapsEachVND * This->diffusion);
        for(int i=0;i<This->numVND;i++){
            BMVelvetNoiseDecorrelator_setNumTaps(&This->vndArray[i], numTaps);
        }
    }
}

void BMCloudReverb_setLSGain(BMCloudReverb* This,float gainDb){
    BMMultiLevelBiquad_setLowShelfFirstOrder(&This->biquadFilter, Filter_LS_FC, gainDb, Filter_Level_Lowshelf);
}

void BMCloudReverb_setHighCutFreq(BMCloudReverb* This,float freq){
    BMMultiLevelBiquad_setLowPass6db(&This->biquadFilter, freq, Filter_Level_Tone);
}

#pragma mark - VND
void BMCloudReverb_setFadeInVND(BMCloudReverb* This,float timeInS){
    for(int i=0;i<This->numVND;i++){
        BMVelvetNoiseDecorrelator_setFadeIn(&This->vndArray[i], timeInS);
    }
}

void BMCloudReverb_setVNDLength(BMCloudReverb* This,float timeInS){
    This->vndLength = timeInS;
    This->updateVND = true;
}

void BMCloudReverb_updateVND(BMCloudReverb* This){
    if(This->updateVND){
        This->updateVND = false;
        //Free & reinit
        for(int i=0;i<This->numVND;i++){
            BMVelvetNoiseDecorrelator_free(&This->vndArray[i]);
            BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->vndArray[i], This->vndLength, This->maxTapsEachVND, 100, false, This->sampleRate);
        }
    }
}

#pragma mark - Test
void BMCloudReverb_impulseResponse(BMCloudReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t length){
    size_t sampleProcessed = 0;
    size_t sampleProcessing = 0;
    This->biquadFilter.useSmoothUpdate = false;
    while(sampleProcessed<length){
        sampleProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, length - sampleProcessed);
        
        BMCloudReverb_processStereo(This, inputL+sampleProcessed, inputR+sampleProcessed, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing,true);
        if(outputL[sampleProcessed]>1.0f)
            printf("error\n");
        sampleProcessed += sampleProcessing;
    }
    This->biquadFilter.useSmoothUpdate = true;
}
