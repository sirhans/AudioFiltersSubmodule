//
//  BMCloudReverb.c
//  AUCloudReverb
//
//  Created by Nguyen Minh Tien on 2/27/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMLongReverb.h"
#include "BMReverb.h"
#include "BMFastHadamard.h"


#define Filter_Level_Lowshelf 0
#define Filter_Level_Tone 1
#define Filter_Level_HP100Hz 3
#define Filter_Level_Lowpass10k 4
#define Filter_Level_TotalLP 3

#define Filter_TotalLevel Filter_Level_Lowpass10k + Filter_Level_TotalLP

#define Filter_LS_FC 400
#define PitchShift_BaseNote 0.14f
#define PitchShift_BaseDuration 10.0f
#define ReadyNo 98573
#define FDN_BaseMaxDelaySecond 0.800f
#define VND_BaseLength 0.2f
#define SFMCount 30
#define ReverbCount 2

void BMLongReverb_updateDiffusion(BMLongReverb* This);
void BMLongReverb_prepareLoopDelay(BMLongReverb* This,int reverbIdx);
void BMLongReverb_updateLoopGain(BMLongReverb* This,size_t* delayTimeL,size_t* delayTimeR,float* gainL,float* gainR);
float BMLongReverb_calculateScaleVol(BMLongReverb* This,int reverbIdx);
void BMLongReverb_updateVND(BMLongReverb* This);
void BMLongReverb_setHighCutFreqAtIdx(BMLongReverb* This,int reverbIdx,float freq);
void BMLongReverb_setLSGainAtIdx(BMLongReverb* This,int reverbIdx,float gainDb);
void BMLongReverb_setDiffusionAtIdx(BMLongReverb* This,int reverbIdx,float diffusion);
void BMLongReverb_setOutputMixerAtIdx(BMLongReverb* This,int reverbIdx,float wetMix);
void BMLongReverb_setDelayPitchMixerAtIdx(BMLongReverb* This,int reverbIdx,float wetMix);
void BMLongReverb_setLoopDecayTimeAtIdx(BMLongReverb* This,int reverbIdx,float decayTime);

void BMLongReverb_init(BMLongReverb* This,float sr){
    This->reverb = malloc(sizeof(BMLongReverbUnit)*ReverbCount);
    This->sampleRate = sr;
    for(int i=0;i<ReverbCount;i++){
        //BIQUAD FILTER
        BMMultiLevelBiquad_init(&This->reverb[i].biquadFilter, Filter_TotalLevel, sr, true, false, true);
        //Highpass 1st order 100Hz
        BMMultiLevelBiquad_setHighPass12db(&This->reverb[i].biquadFilter, 60, Filter_Level_HP100Hz);
        
        //lowpass 36db
        BMMultiLevelBiquad_setHighOrderBWLP(&This->reverb[i].biquadFilter, 9300, Filter_Level_Lowpass10k, Filter_Level_TotalLP);
        
        //Tone control - use 6db
        BMLongReverb_setHighCutFreqAtIdx(This,i, 1200.0f);
        //Low shelf
        This->lsGain = 0;
        BMLongReverb_setLSGainAtIdx(This,i, This->lsGain);
        
        //VND
        This->reverb[i].updateVND = false;
        This->maxTapsEachVND = 16;
        This->reverb[i].diffusion = 1.0f;
        This->reverb[i].vndLength = VND_BaseLength;
        This->reverb[i].fadeInS = 0;
        This->reverb[i].vndDryTap = false;
        
        This->numInput = 8;
        This->numVND = This->numInput;
        This->reverb[i].vndArray = malloc(sizeof(BMVelvetNoiseDecorrelator)*This->numVND);
        
        
        //Using vnd
        for(int j=0;j<This->numVND;j++){
            //First layer
            BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->reverb[i].vndArray[j], This->reverb[i].vndLength, This->maxTapsEachVND, 100, This->reverb[i].vndDryTap, sr);
            BMVelvetNoiseDecorrelator_setFadeIn(&This->reverb[i].vndArray[j], 0.0f);
            if(This->reverb[i].vndDryTap)
                BMVelvetNoiseDecorrelator_setWetMix(&This->reverb[i].vndArray[j], 1.0f);
        }
        
        //Pitch shifting
        size_t delayRange = (1000*sr)/48000.0f;
        size_t maxDelayRange = (20000*sr)/48000.0f;
        float duration = 3.5f;
        BMPitchShiftDelay_init(&This->reverb[i].pitchShiftDelay, duration,delayRange ,maxDelayRange , sr, false, true);
        
        BMPitchShiftDelay_setWetGain(&This->reverb[i].pitchShiftDelay, 1.0f);
        BMPitchShiftDelay_setDelayDuration(&This->reverb[i].pitchShiftDelay,duration);
        BMPitchShiftDelay_setDelayRange(&This->reverb[i].pitchShiftDelay, delayRange);
        BMPitchShiftDelay_setBandLowpass(&This->reverb[i].pitchShiftDelay, 4000);
        BMPitchShiftDelay_setBandHighpass(&This->reverb[i].pitchShiftDelay,120);
        BMPitchShiftDelay_setMixOtherChannel(&This->reverb[i].pitchShiftDelay, 1.0f);
        
        This->reverb[i].buffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].buffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].LFOBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].LFOBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].loopInput.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].loopInput.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].lastLoopBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].lastLoopBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        memset(This->reverb[i].lastLoopBuffer.bufferL, 0, sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        memset(This->reverb[i].lastLoopBuffer.bufferR, 0, sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        
        This->reverb[i].outputL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].outputR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        
        This->reverb[i].wetBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].wetBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].lastWetBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].lastWetBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        
        //Loop delay
        BMLongReverb_prepareLoopDelay(This,i);
        
        BMLongReverb_setLoopDecayTimeAtIdx(This,i, 10);
        BMLongReverb_setDelayPitchMixerAtIdx(This,i, 0.5f);
        BMWetDryMixer_init(&This->reverb[i].reverbMixer, sr);
        BMLongReverb_setOutputMixerAtIdx(This,i, 0.5f);
        
        BMSmoothGain_init(&This->reverb[i].smoothGain, sr);
        
        //Pan
        BMPanLFO_init(&This->reverb[i].inputPan, 0.412f, 0.6f, sr,true);
        BMPanLFO_init(&This->reverb[i].outputPan, 1.1f, 0.3f, sr,true);
        
        This->measureLength = 4096;
        BMHarmonicityMeasure_init(&This->reverb[i].chordMeasure, This->measureLength, sr);
        This->reverb[i].measureFillCount = 0;
//        This->reverb[i].measureInput = malloc(sizeof(float)*This->measureLength);
//        This->reverb[i].sfmBuffer = calloc(SFMCount, sizeof(float));
//        This->reverb[i].sfmIdx = 0;
        
        //Attack softener
        BMMultibandAttackShaper_init(&This->reverb[i].attackSoftener, true, sr);
        
        BMSpectrum_init(&This->reverb[i].measureSpectrum);
        This->reverb[i].measureSamples = 4096;
        BMMeasurementBuffer_init(&This->reverb[i].measureDryInput, This->reverb[i].measureSamples);
        BMMeasurementBuffer_init(&This->reverb[i].measureWetOutput, This->reverb[i].measureSamples);
        This->reverb[i].measureSpectrumDryBuffer = malloc(sizeof(float)*This->reverb[i].measureSamples);
        This->reverb[i].measureSpectrumWetBuffer = malloc(sizeof(float)*This->reverb[i].measureSamples);
        
    }
    
    This->vnd1BufferL = malloc(sizeof(float*)*This->numInput);
    This->vnd1BufferR = malloc(sizeof(float*)*This->numInput);
    This->vnd2BufferL = malloc(sizeof(float*)*This->numInput);
    This->vnd2BufferR = malloc(sizeof(float*)*This->numInput);
    This->changeReverbCurrentSamples = 0;
    This->changeReverbDelaySamples = 0.5f * This->sampleRate;
    This->changingReverbCount = 0;
    
    for(int j=0;j<This->numVND;j++){
        This->vnd1BufferL[j] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->vnd1BufferR[j] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->vnd2BufferL[j] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->vnd2BufferR[j] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    }
    This->dryInputL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->dryInputR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->reverbActiveIdx = 0;
    This->fadeSamples = 1024;
    BMSmoothFade_init(&This->fadeIn, This->fadeSamples);
    BMSmoothFade_init(&This->fadeOut, This->fadeSamples);
    
    This->initNo = ReadyNo;
}

void BMLongReverb_prepareLoopDelay(BMLongReverb* This,int reverbIdx){
    size_t numDelays = 24;
    float maxDT = FDN_BaseMaxDelaySecond;
    float minDT = 0.020f;
    bool zeroTaps = false;
    BMLongLoopFDN_init(&This->reverb[reverbIdx].loopFDN, numDelays, minDT, maxDT, zeroTaps, 8, 1,100,true, This->sampleRate);
}

void BMLongReverb_destroy(BMLongReverb* This){
    for(int j=0;j<ReverbCount;j++){
        BMMultiLevelBiquad_free(&This->reverb[j].biquadFilter);
        
        for(int i=0;i<This->numVND;i++){
            BMVelvetNoiseDecorrelator_free(&This->reverb[j].vndArray[i]);
        }
        
        
        BMPitchShiftDelay_destroy(&This->reverb[j].pitchShiftDelay);
        
        BMLongLoopFDN_free(&This->reverb[j].loopFDN);
        
        free(This->reverb[j].buffer.bufferL);
        This->reverb[j].buffer.bufferL = nil;
        free(This->reverb[j].buffer.bufferR);
        This->reverb[j].buffer.bufferR = nil;
        free(This->reverb[j].LFOBuffer.bufferL);
        This->reverb[j].LFOBuffer.bufferL = nil;
        free(This->reverb[j].LFOBuffer.bufferR);
        This->reverb[j].LFOBuffer.bufferR = nil;
        free(This->reverb[j].loopInput.bufferL);
        This->reverb[j].loopInput.bufferL = nil;
        free(This->reverb[j].loopInput.bufferR);
        This->reverb[j].loopInput.bufferR = nil;
        free(This->reverb[j].lastLoopBuffer.bufferL);
        This->reverb[j].lastLoopBuffer.bufferL = nil;
        free(This->reverb[j].lastLoopBuffer.bufferR);
        This->reverb[j].lastLoopBuffer.bufferR = nil;
        free(This->reverb[j].wetBuffer.bufferL);
        This->reverb[j].wetBuffer.bufferL = nil;
        free(This->reverb[j].wetBuffer.bufferR);
        This->reverb[j].wetBuffer.bufferR = nil;
        free(This->reverb[j].lastWetBuffer.bufferL);
        This->reverb[j].lastWetBuffer.bufferL = nil;
        free(This->reverb[j].lastWetBuffer.bufferR);
        This->reverb[j].lastWetBuffer.bufferR = nil;
        
        BMMultibandAttackShaper_free(&This->reverb[j].attackSoftener);
        BMSpectrum_free(&This->reverb[j].measureSpectrum);
        BMMeasurementBuffer_free(&This->reverb[j].measureDryInput);
        BMMeasurementBuffer_free(&This->reverb[j].measureWetOutput);
        free(This->reverb[j].measureSpectrumDryBuffer);
        This->reverb[j].measureSpectrumDryBuffer = nil;
        free(This->reverb[j].measureSpectrumWetBuffer);
        This->reverb[j].measureSpectrumWetBuffer = nil;
        
        
    }
    
    for(int i=0;i<This->numInput;i++){
        free(This->vnd1BufferL[i]);
        This->vnd1BufferL[i] = nil;
        free(This->vnd1BufferR[i]);
        This->vnd1BufferR[i] = nil;
        free(This->vnd2BufferL[i]);
        This->vnd2BufferL[i] = nil;
        free(This->vnd2BufferR[i]);
        This->vnd2BufferR[i] = nil;
    }
    
    free(This->dryInputL);
    This->dryInputL = nil;
    free(This->dryInputR);
    This->dryInputR = nil;
    
    free(This->vnd1BufferL);
    free(This->vnd1BufferR);
    free(This->vnd2BufferL);
    free(This->vnd2BufferR);
    
    free(This->reverb);
    This->reverb = nil;
}

#pragma mark - Measurement
void BMLongReverb_measureSpectrum(BMLongReverb* This,int reverbIdx,float* dryInputL,float* dryInputR,float* wetInputL,float* wetInputR,size_t numSamples){
    //Mix dry & wet to mono
    float mul = 0.5f;
    vDSP_vasm(dryInputL, 1, dryInputR, 1, &mul, This->vnd1BufferL[0], 1, numSamples);
    BMMeasurementBuffer_inputSamples(&This->reverb[reverbIdx].measureDryInput, This->vnd1BufferL[0], numSamples);
    float normInputDry;
    vDSP_svesq(This->vnd1BufferL[0], 1, &normInputDry, numSamples);
    
    //Wet
    vDSP_vasm(wetInputL, 1, wetInputR, 1, &mul, This->vnd1BufferL[0], 1, numSamples);
    BMMeasurementBuffer_inputSamples(&This->reverb[reverbIdx].measureWetOutput, This->vnd1BufferL[0], numSamples);
        
    if(This->changeReverbCurrentSamples>=This->changeReverbDelaySamples){
        float* dryInput = BMMeasurementBuffer_getCurrentPointer(&This->reverb[reverbIdx].measureDryInput);
        float* wetInput = BMMeasurementBuffer_getCurrentPointer(&This->reverb[reverbIdx].measureWetOutput);
        
        //Get spectrum of dry / wet input
        BMSpectrum_processDataBasic(&This->reverb[reverbIdx].measureSpectrum, dryInput, This->reverb[reverbIdx].measureSpectrumDryBuffer, true, This->reverb[reverbIdx].measureSamples);
        BMSpectrum_processDataBasic(&This->reverb[reverbIdx].measureSpectrum, wetInput, This->reverb[reverbIdx].measureSpectrumWetBuffer, true, This->reverb[reverbIdx].measureSamples);
        size_t outLength = This->reverb[reverbIdx].measureSamples/2;
        
        //Validate if we should use this dry input
        
        
        
        //Find normalize spectrum
        if(normInputDry!=0){
            float normDry;
            vDSP_svesq(This->reverb[reverbIdx].measureSpectrumDryBuffer, 1, &normDry, outLength);
            normDry = sqrtf(normDry);
            float normWet;
            vDSP_svesq(This->reverb[reverbIdx].measureSpectrumWetBuffer, 1, &normWet, outLength);
            normWet = sqrtf(normWet);
            
            //Multiple both spectrum together
            vDSP_vmul(This->reverb[reverbIdx].measureSpectrumDryBuffer, 1, This->reverb[reverbIdx].measureSpectrumWetBuffer, 1, This->reverb[reverbIdx].measureSpectrumDryBuffer, 1, outLength);
            float normMix;
            vDSP_sve(This->reverb[reverbIdx].measureSpectrumDryBuffer, 1, &normMix, outLength);
            normMix = sqrtf(normMix);
            if(normDry!=0&&normWet!=0){
                float result = powf((normMix*normMix)/(normDry*normWet),1);

                if(result<0.5f){
                    //Shoud change active index
                    This->changingReverbCount = 0;
                    This->reverbActiveIdx = This->reverbActiveIdx==0 ? 1 : 0;
                    This->changeReverbCurrentSamples = 0;
                    printf("change %f\n",result);
                }
                printf("%f\n",result);
            }
//            printf("norm %f %f\n",normWet,normDry);
        }
    }else{
        This->changeReverbCurrentSamples += numSamples;
    }
}

void BMLongReverb_updateDryInput(BMLongReverb* This,int reverbIdx,float* dryInputL,float* dryInputR,size_t numSamples){
    //Update 2 reverbs settings
    if(This->changingReverbCount<ReverbCount){
        if(reverbIdx==This->reverbActiveIdx){
            //fade in to avoid click
            BMSmoothFade_startFading(&This->fadeIn, FT_In);
            
            //Change reverb decay time
            This->reverb[reverbIdx].decayTime = 40;
            BMLongLoopFDN_setRT60DecaySmooth(&This->reverb[reverbIdx].loopFDN, This->reverb[reverbIdx].decayTime,false);
        }else{
            //Fade out & silent the reverb
            BMSmoothFade_startFading(&This->fadeOut, FT_Out);
            
            //Change reverb decay time
            This->reverb[reverbIdx].decayTime = 0.5f;
            BMLongLoopFDN_setRT60DecaySmooth(&This->reverb[reverbIdx].loopFDN, This->reverb[reverbIdx].decayTime,false);
        }
        This->changingReverbCount++;
    }
    
    //Apply setting
    if(reverbIdx==This->reverbActiveIdx){
        //Fade in only
        BMSmoothFade_processBufferStereo(&This->fadeIn, dryInputL, dryInputR,dryInputL, dryInputR, numSamples);
    }else{
        //Fade out only
        BMSmoothFade_processBufferStereo(&This->fadeOut, dryInputL, dryInputR,dryInputL, dryInputR, numSamples);
    }
}

void BMLongReverb_processStereo(BMLongReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t numSamples,bool offlineRendering){
    if(This->initNo==ReadyNo){
        assert(numSamples<=BM_BUFFER_CHUNK_SIZE);
        for(int reverbIdx=0;reverbIdx<ReverbCount;reverbIdx++){
            BMLongReverb_updateVND(This);
            BMLongReverb_updateDiffusion(This);
            
            //Process attack softener for wet input
            BMMultibandAttackShaper_processStereo(&This->reverb[reverbIdx].attackSoftener, inputL, inputR, This->dryInputL, This->dryInputR, numSamples);
            
            //Measurement
            if(reverbIdx==This->reverbActiveIdx){
                //Only measure on active reverb
                BMLongReverb_measureSpectrum(This,reverbIdx, This->dryInputL, This->dryInputR, This->reverb[reverbIdx].lastWetBuffer.bufferL, This->reverb[reverbIdx].lastWetBuffer.bufferR, numSamples);
            }
            
            //Update input
            BMLongReverb_updateDryInput(This, reverbIdx, This->dryInputL, This->dryInputR, numSamples);
            
            //1st layer VND
            for(int i=0;i<This->numInput;i++){
                //PitchShifting delay into wetbuffer
                BMVelvetNoiseDecorrelator_processBufferStereo(&This->reverb[reverbIdx].vndArray[i], This->dryInputL, This->dryInputR, This->vnd1BufferL[i], This->vnd1BufferR[i], numSamples);
            }
            
            if(This->numInput>4){
                BMFastHadamardTransformBuffer(This->vnd1BufferL, This->vnd2BufferL, This->numInput, numSamples);
                BMFastHadamardTransformBuffer(This->vnd1BufferR, This->vnd2BufferR, This->numInput, numSamples);
            }else{
                for(int j=0;j<This->numInput;j++){
                    memcpy(This->vnd2BufferL[j], This->vnd1BufferL[j], sizeof(float)*numSamples);
                    memcpy(This->vnd2BufferR[j], This->vnd1BufferR[j], sizeof(float)*numSamples);
                }
            }
            
            //Long FDN
            BMLongLoopFDN_processMultiChannelInput(&This->reverb[reverbIdx].loopFDN, This->vnd2BufferL, This->vnd2BufferR, This->numInput, This->reverb[reverbIdx].wetBuffer.bufferL, This->reverb[reverbIdx].wetBuffer.bufferR, numSamples);

            //Add vnd2Buffer[0] to the output of FDN to simulate the zero tap of fdn. Cant use zero tap setting
            // becauz of the Fast Hadamard Transform mix all the input.
            float mul = 1.0f;
            vDSP_vsma(This->vnd2BufferL[0], 1, &mul, This->reverb[reverbIdx].wetBuffer.bufferL, 1, This->reverb[reverbIdx].wetBuffer.bufferL, 1, numSamples);
            vDSP_vsma(This->vnd2BufferR[0], 1, &mul, This->reverb[reverbIdx].wetBuffer.bufferR, 1, This->reverb[reverbIdx].wetBuffer.bufferR, 1, numSamples);
            
            BMPitchShiftDelay_processStereoBuffer(&This->reverb[reverbIdx].pitchShiftDelay, This->reverb[reverbIdx].wetBuffer.bufferL, This->reverb[reverbIdx].wetBuffer.bufferR, This->reverb[reverbIdx].wetBuffer.bufferL, This->reverb[reverbIdx].wetBuffer.bufferR, numSamples);
            
            //Filters
            BMMultiLevelBiquad_processBufferStereo(&This->reverb[reverbIdx].biquadFilter, This->reverb[reverbIdx].wetBuffer.bufferL, This->reverb[reverbIdx].wetBuffer.bufferR, This->reverb[reverbIdx].wetBuffer.bufferL, This->reverb[reverbIdx].wetBuffer.bufferR, numSamples);
            
            //Normalize vol
            BMSmoothGain_processBuffer(&This->reverb[reverbIdx].smoothGain, This->reverb[reverbIdx].wetBuffer.bufferL, This->reverb[reverbIdx].wetBuffer.bufferR, This->reverb[reverbIdx].wetBuffer.bufferL, This->reverb[reverbIdx].wetBuffer.bufferR, numSamples);
            
            //Measurement
            memcpy(This->reverb[reverbIdx].lastWetBuffer.bufferL, This->reverb[reverbIdx].wetBuffer.bufferL, numSamples*sizeof(float));
            memcpy(This->reverb[reverbIdx].lastWetBuffer.bufferR, This->reverb[reverbIdx].wetBuffer.bufferR, numSamples*sizeof(float));
            
            
            //LFO pan
            // Input pan LFO
            BMPanLFO_process(&This->reverb[reverbIdx].inputPan, This->reverb[reverbIdx].LFOBuffer.bufferL, This->reverb[reverbIdx].LFOBuffer.bufferR, numSamples);
            vDSP_vmul(This->reverb[reverbIdx].wetBuffer.bufferL, 1, This->reverb[reverbIdx].LFOBuffer.bufferL, 1, This->reverb[reverbIdx].wetBuffer.bufferL, 1, numSamples);
            vDSP_vmul(This->reverb[reverbIdx].wetBuffer.bufferR, 1, This->reverb[reverbIdx].LFOBuffer.bufferR, 1, This->reverb[reverbIdx].wetBuffer.bufferR, 1, numSamples);
            BMPanLFO_process(&This->reverb[reverbIdx].outputPan, This->reverb[reverbIdx].LFOBuffer.bufferL, This->reverb[reverbIdx].LFOBuffer.bufferR, numSamples);
            vDSP_vmul(This->reverb[reverbIdx].wetBuffer.bufferL, 1, This->reverb[reverbIdx].LFOBuffer.bufferL, 1, This->reverb[reverbIdx].wetBuffer.bufferL, 1, numSamples);
            vDSP_vmul(This->reverb[reverbIdx].wetBuffer.bufferR, 1, This->reverb[reverbIdx].LFOBuffer.bufferR, 1, This->reverb[reverbIdx].wetBuffer.bufferR, 1, numSamples);
            
            //mix dry & wet reverb
            if(offlineRendering){
                float dryMix = 1 - This->reverb[reverbIdx].reverbMixer.mixTarget;
                vDSP_vsmsma(This->reverb[reverbIdx].wetBuffer.bufferL, 1, &This->reverb[reverbIdx].reverbMixer.mixTarget, inputL, 1, &dryMix, This->reverb[reverbIdx].outputL, 1, numSamples);
                vDSP_vsmsma(This->reverb[reverbIdx].wetBuffer.bufferR, 1, &This->reverb[reverbIdx].reverbMixer.mixTarget, inputR, 1, &dryMix, This->reverb[reverbIdx].outputR, 1, numSamples);
            }else{
                //Process reverb dry/wet mixer
                BMWetDryMixer_processBufferInPhase(&This->reverb[reverbIdx].reverbMixer, This->reverb[reverbIdx].wetBuffer.bufferL, This->reverb[reverbIdx].wetBuffer.bufferR, inputL, inputR, This->reverb[reverbIdx].outputL, This->reverb[reverbIdx].outputR, numSamples);
            }
        }
        
        //Mix reverb output to final output
        vDSP_vclr(outputL, 1, numSamples);
        vDSP_vclr(outputR, 1, numSamples);
        for(int reverbIdx = 0;reverbIdx<ReverbCount;reverbIdx++){
            vDSP_vadd(This->reverb[reverbIdx].outputL, 1, outputL, 1, outputL, 1, numSamples);
            vDSP_vadd(This->reverb[reverbIdx].outputR, 1, outputR, 1, outputR, 1, numSamples);
        }
    }
}



float BMLongReverb_calculateScaleVol(BMLongReverb* This,int reverbIdx){
    float factor = log2f(This->reverb[reverbIdx].decayTime);
    float scaleDB = (-3. * (factor)) + (1.2f-This->reverb[reverbIdx].diffusion)*10.0f;
//    printf("%f\n",scaleDB);
    return scaleDB;
}



#pragma mark - Set
void BMLongReverb_setLoopDecayTime(BMLongReverb* This,float decayTime){
    for(int reverbIdx=0;reverbIdx<ReverbCount;reverbIdx++){
        BMLongReverb_setLoopDecayTimeAtIdx(This, reverbIdx, decayTime);
    }
}

void BMLongReverb_setLoopDecayTimeAtIdx(BMLongReverb* This,int reverbIdx,float decayTime){
    This->reverb[reverbIdx].decayTime = decayTime;
    BMLongLoopFDN_setRT60DecaySmooth(&This->reverb[reverbIdx].loopFDN, decayTime,false);
    
    float gainDB = BMLongReverb_calculateScaleVol(This,reverbIdx);
    BMSmoothGain_setGainDb(&This->reverb[reverbIdx].smoothGain, gainDB);
    
}

#define DepthMax 0.90f
float BMLongReverb_getMixBaseOnMode(BMLongReverb* This,float mode){
    float mix = 0;
    if(mode==0){
        mix = 0.1f;
    }else if(mode==1){
        mix = 0.2f;
    }else if(mode==2){
        mix = 0.35f;
    }else if(mode==3){
        mix = 0.4f;
    }else if(mode==4){
        mix = 0.42f;
    }else if(mode==5){
        mix = 0.5f;
    }
    return mix;
}

float BMLongReverb_getDelayRangeBaseOnMode(BMLongReverb* This,float mode){
    float range = 0;
    if(mode==5){
        //Mode 6
        range = 800;
    }else if(mode==4){
        range = 600;
    }else{
        range = 500;
    }
    return (range/48000.0f)*This->sampleRate;
}

#define S_ChorusMode_Min 0
#define S_ChorusMode_Max 5
#define DC_Duration_Max 10.5f
#define DC_Duration_Min 3.5f

float BMLongReverb_getDurationBaseOnMode(BMLongReverb* This,float mode){
    float duration = (DC_Duration_Max-DC_Duration_Min)*(1 - mode/(S_ChorusMode_Max-S_ChorusMode_Min)) + DC_Duration_Min;
    if(mode==5){
        duration = duration * 1.6f;
    }else if(mode==4){
        duration = duration * 1.25f;
    }
    return duration;
}

void BMLongReverb_setDelayPitchMixer(BMLongReverb* This,float wetMix){
    for(int reverbIdx=0;reverbIdx<ReverbCount;reverbIdx++){
        BMLongReverb_setDelayPitchMixerAtIdx(This,reverbIdx, wetMix);
    }
}

void BMLongReverb_setDelayPitchMixerAtIdx(BMLongReverb* This,int reverbIdx,float wetMix){
    
    //Set delayrange of pitch shift to control speed of pitch shift
    //0 to 5
    float mode = BM_MIN(roundf((wetMix*5.0f)/0.75f),5);
    
    //mix from 0.1 to 0.5
    float mix = BMLongReverb_getMixBaseOnMode(This,mode);
    //Speed from 1/30 -> 1/10
    float duration = BMLongReverb_getDurationBaseOnMode(This,mode);
    
    float delayRange = BMLongReverb_getDelayRangeBaseOnMode(This,mode);
//    printf("pitch %f %f %f\n",mix,duration,delayRange);
    
    BMPitchShiftDelay_setWetGain(&This->reverb[reverbIdx].pitchShiftDelay, mix);
    BMPitchShiftDelay_setDelayDuration(&This->reverb[reverbIdx].pitchShiftDelay,duration);
    BMPitchShiftDelay_setDelayRange(&This->reverb[reverbIdx].pitchShiftDelay, delayRange);
    
    //Control pan
    BMPanLFO_setDepth(&This->reverb[reverbIdx].inputPan, wetMix*DepthMax);
    BMPanLFO_setDepth(&This->reverb[reverbIdx].outputPan, 0.5f*wetMix*DepthMax);
}

void BMLongReverb_setOutputMixer(BMLongReverb* This,float wetMix){
    for(int reverbIdx=0;reverbIdx<ReverbCount;reverbIdx++){
        BMLongReverb_setOutputMixerAtIdx(This, reverbIdx, wetMix);
    }
}

void BMLongReverb_setOutputMixerAtIdx(BMLongReverb* This,int reverbIdx,float wetMix){
    BMWetDryMixer_setMix(&This->reverb[reverbIdx].reverbMixer, wetMix*wetMix);
}

void BMLongReverb_setDiffusion(BMLongReverb* This,float diffusion){
    for(int reverbIdx=0;reverbIdx<ReverbCount;reverbIdx++){
        BMLongReverb_setDiffusionAtIdx(This, reverbIdx, diffusion);
    }
}

void BMLongReverb_setDiffusionAtIdx(BMLongReverb* This,int reverbIdx,float diffusion){
    if(This->reverb[reverbIdx].diffusion!=diffusion){
        This->reverb[reverbIdx].diffusion = diffusion;
        This->reverb[reverbIdx].updateDiffusion = true;
        This->numInput = BM_MAX((roundf(8 * diffusion)/2.0f)*2,2);
        
        //update wet vol
        float gainDb = BMLongReverb_calculateScaleVol(This,reverbIdx);
        BMSmoothGain_setGainDb(&This->reverb[reverbIdx].smoothGain, gainDb);
    }
}

void BMLongReverb_updateDiffusion(BMLongReverb* This){
    for(int reverbIdx=0;reverbIdx<ReverbCount;reverbIdx++){
        if(This->reverb[reverbIdx].updateDiffusion){
            This->reverb[reverbIdx].updateDiffusion = false;
            float numTaps = floorf(This->maxTapsEachVND * This->reverb[reverbIdx].diffusion);
            for(int i=0;i<This->numVND;i++){
                BMVelvetNoiseDecorrelator_setNumTaps(&This->reverb[reverbIdx].vndArray[i], numTaps);
            }
        }
    }
}

void BMLongReverb_setLSGain(BMLongReverb* This,float gainDb){
    for(int reverbIdx=0;reverbIdx<ReverbCount;reverbIdx++){
        BMLongReverb_setLSGainAtIdx(This, reverbIdx, gainDb);
    }
}

void BMLongReverb_setLSGainAtIdx(BMLongReverb* This,int reverbIdx,float gainDb){
    BMMultiLevelBiquad_setLowShelfFirstOrder(&This->reverb[reverbIdx].biquadFilter, Filter_LS_FC, gainDb, Filter_Level_Lowshelf);
}

void BMLongReverb_setHighCutFreq(BMLongReverb* This,float freq){
    for(int reverbIdx=0;reverbIdx<ReverbCount;reverbIdx++){
        BMLongReverb_setHighCutFreqAtIdx(This, reverbIdx, freq);
    }
}

void BMLongReverb_setHighCutFreqAtIdx(BMLongReverb* This,int reverbIdx,float freq){
    BMMultiLevelBiquad_setLowPass6db(&This->reverb[reverbIdx].biquadFilter, freq, Filter_Level_Tone);
}

#pragma mark - VND
void BMLongReverb_setFadeInVND(BMLongReverb* This,float timeInS){
    for(int reverbIdx=0;reverbIdx<ReverbCount;reverbIdx++){
        This->reverb[reverbIdx].desiredVNDLength = VND_BaseLength;
        if(timeInS>VND_BaseLength){
            //If fadein time is bigger than vndlength -> need to reset vnd length
            This->reverb[reverbIdx].desiredVNDLength = timeInS;
        }
        This->reverb[reverbIdx].fadeInS = timeInS;
        
        This->reverb[reverbIdx].updateVND = true;
    }
}

void BMLongReverb_updateVND(BMLongReverb* This){
    for(int reverbIdx=0;reverbIdx<ReverbCount;reverbIdx++){
        if(This->reverb[reverbIdx].updateVND){
            This->reverb[reverbIdx].updateVND = false;
            if(This->reverb[reverbIdx].desiredVNDLength!=This->reverb[reverbIdx].vndLength){
                This->reverb[reverbIdx].vndLength = This->reverb[reverbIdx].desiredVNDLength;
                //Free & reinit
                for(int i=0;i<This->numVND;i++){
                    //1st layer
                    BMVelvetNoiseDecorrelator_free(&This->reverb[reverbIdx].vndArray[i]);
                    BMVelvetNoiseDecorrelator_initWithEvenTapDensity(&This->reverb[reverbIdx].vndArray[i], This->reverb[reverbIdx].vndLength, This->maxTapsEachVND, 100, This->reverb[reverbIdx].vndDryTap, This->sampleRate);
                    if(This->reverb[reverbIdx].vndDryTap)
                        BMVelvetNoiseDecorrelator_setWetMix(&This->reverb[reverbIdx].vndArray[i], 1.0f);
                    //Update fade in
                    BMVelvetNoiseDecorrelator_setFadeIn(&This->reverb[reverbIdx].vndArray[i], This->reverb[reverbIdx].fadeInS);
                }

                BMLongReverb_setDiffusionAtIdx(This,reverbIdx, This->reverb[reverbIdx].diffusion);
            }else{
                for(int i=0;i<This->numVND;i++){
                    //Update fade in
                    BMVelvetNoiseDecorrelator_setFadeIn(&This->reverb[reverbIdx].vndArray[i], This->reverb[reverbIdx].fadeInS);
                }
            }
        }
    }
}

#pragma mark - Test
void BMLongReverb_impulseResponse(BMLongReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t length){
//    size_t sampleProcessed = 0;
//    size_t sampleProcessing = 0;
//    This->biquadFilter.useSmoothUpdate = false;
//    while(sampleProcessed<length){
//        sampleProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, length - sampleProcessed);
//
//        BMLongReverb_processStereo(This, inputL+sampleProcessed, inputR+sampleProcessed, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing,true);
//        if(outputL[sampleProcessed]>1.0f)
//            printf("error\n");
//        sampleProcessed += sampleProcessing;
//    }
//    This->biquadFilter.useSmoothUpdate = true;
}
