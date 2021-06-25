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
#define ReverbCount 3
#define ReverbVerifyChange 3
#define Reverb_MinDB -60 //dB
#define Reverb_DryWetDBRange 40
#define Reverb_ComplexcityAdjust 0.3f

#define AttackShaper_Time 0.080f
#define AttackShaper_Depth 2.0f
#define AttackShaper_Noisegate -45 //dB

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
// 0 is simplest spectrum and 1 is most complex
float spectralComplexity(float *A, size_t length);

void BMLongReverb_init(BMLongReverb* This,float sr){
    This->reverb = malloc(sizeof(BMLongReverbUnit)*ReverbCount);
    This->sampleRate = sr;
    This->fadeSamples = 1024;
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
        This->minSensitive = 0.1f;
        This->maxSensitive = 0.5f;
        This->minDecay = 0.5f;
        This->maxDecay = 40.0f;
        This->reverb[i].decayTime = This->minDecay;
        
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
        This->reverb[i].dryInput.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].dryInput.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        
        This->reverb[i].outputL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].outputR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        
        This->reverb[i].wetBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].wetBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].lastWetBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->reverb[i].lastWetBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        
        //Fade
        BMSmoothFade_init(&This->reverb[i].smoothFade, This->fadeSamples);
        This->reverb[i].state = RS_InActive;
        
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
        
        //Attack softener
        BMMultibandAttackShaper_init(&This->reverb[i].attackSoftener, true, sr);
        BMMultibandAttackShaper_setAttackTime(&This->reverb[i].attackSoftener, AttackShaper_Time);
        BMMultibandAttackShaper_setAttackDepth(&This->reverb[i].attackSoftener, AttackShaper_Depth);
        BMMultibandAttackShaper_setSidechainNoiseGateThreshold(&This->reverb[i].attackSoftener, AttackShaper_Noisegate);
        
        This->reverb[i].measureSamples = 4096;
        BMSpectrum_initWithLength(&This->reverb[i].measureSpectrum,This->reverb[i].measureSamples);
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
    This->updateReverbCount = 0;
    This->verifyReverbChangeCount = 0;
    
    for(int j=0;j<This->numVND;j++){
        This->vnd1BufferL[j] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->vnd1BufferR[j] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->vnd2BufferL[j] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        This->vnd2BufferR[j] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    }
    
    This->attackSoftenerBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->attackSoftenerBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    This->reverbActiveIdx = 0;
    //Active reverb
    This->reverb[This->reverbActiveIdx].state = RS_Active;
    BMSmoothFade_startFading(&This->reverb[This->reverbActiveIdx].smoothFade, FT_In);
    This->reverb[This->reverbActiveIdx].decayTime = This->maxDecay;
    //Fading reverb
    This->reverb[1].state = RS_Fading;
    This->reverb[1].decayTime = 0.5f;
    This->reverb[2].state = RS_InActive;
    This->reverb[2].decayTime = 0.5f;
    
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
        free(This->reverb[j].dryInput.bufferL);
        This->reverb[j].dryInput.bufferL = nil;
        free(This->reverb[j].dryInput.bufferR);
        This->reverb[j].dryInput.bufferR = nil;
        
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
    
    free(This->attackSoftenerBuffer.bufferL);
    This->attackSoftenerBuffer.bufferL = nil;
    free(This->attackSoftenerBuffer.bufferR);
    This->attackSoftenerBuffer.bufferR = nil;
    
    free(This->vnd1BufferL);
    free(This->vnd1BufferR);
    free(This->vnd2BufferL);
    free(This->vnd2BufferR);
    
    free(This->reverb);
    This->reverb = nil;
}

#pragma mark - Measurement
//3 Reverb : 1 active - 1 decay slowsly - 1 inactive for waiting new sound
void BMLongReverb_measureSpectrum(BMLongReverb* This,int reverbIdx,float* dryInputL,float* dryInputR,float* wetInputL,float* wetInputR,size_t numSamples){
    //Mix dry & wet to mono
    float mul = 0.5f;
    //check dry input vol
    float inputVol;
    vDSP_svesq(dryInputL, 1, &inputVol, numSamples);
    inputVol = sqrtf(inputVol);
    bool inputValidate = false;
    if(inputVol>=BM_DB_TO_GAIN(-60)){
        vDSP_vasm(dryInputL, 1, dryInputR, 1, &mul, This->vnd1BufferL[0], 1, numSamples);
        BMMeasurementBuffer_inputSamples(&This->reverb[reverbIdx].measureDryInput, This->vnd1BufferL[0], numSamples);
        inputValidate = true;
    }
    
    //Wet
    vDSP_vasm(wetInputL, 1, wetInputR, 1, &mul, This->vnd1BufferL[0], 1, numSamples);
    BMMeasurementBuffer_inputSamples(&This->reverb[reverbIdx].measureWetOutput, This->vnd1BufferL[0], numSamples);
        
    if(This->changeReverbCurrentSamples>=This->changeReverbDelaySamples&&
       inputValidate){
        float* dryInput = BMMeasurementBuffer_getCurrentPointer(&This->reverb[reverbIdx].measureDryInput);
        float* wetInput = BMMeasurementBuffer_getCurrentPointer(&This->reverb[reverbIdx].measureWetOutput);
        
        //Get spectrum of dry / wet input
        BMSpectrum_processDataBasic(&This->reverb[reverbIdx].measureSpectrum, dryInput, This->reverb[reverbIdx].measureSpectrumDryBuffer, true, This->reverb[reverbIdx].measureSamples);
        BMSpectrum_processDataBasic(&This->reverb[reverbIdx].measureSpectrum, wetInput, This->reverb[reverbIdx].measureSpectrumWetBuffer, true, This->reverb[reverbIdx].measureSamples);
        size_t outLength = This->reverb[reverbIdx].measureSamples/2;
        
        //Find normalize spectrum
//            printf("vol %f %f\n",normInputDry,BM_DB_TO_GAIN(-60));
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

        float result = This->minSensitive;
        if(normWet!=0)
            result = powf((normMix*normMix)/(normDry*normWet),1);
        

        float complexcity = spectralComplexity(This->reverb[reverbIdx].measureSpectrumDryBuffer, This->reverb[reverbIdx].measureSamples);
//        printf("result %f %f\n",result,complexcity);
        float factor = (1-Reverb_ComplexcityAdjust) + (Reverb_ComplexcityAdjust*complexcity);
        
        if(result<=This->maxSensitive*factor&&
           result>=This->minSensitive){
            if(This->verifyReverbChangeCount>=ReverbVerifyChange){
                //Shoud change active index
                This->verifyReverbChangeCount = 0;
                This->updateReverbCount = 0;
                
                This->changeReverbCurrentSamples = 0;
                
                //Reverb active idx
                This->reverbActiveIdx++;
                if(This->reverbActiveIdx>=ReverbCount)
                    This->reverbActiveIdx = 0;
                
                //Calculate active/ inactive decaytime
                float v = (This->maxSensitive-result)/(This->maxSensitive-This->minSensitive);
                float fadingDecay = (1-v) * (This->maxDecay - This->minDecay) + This->minDecay;
                printf("change %f %f %f\n",result,fadingDecay,This->maxSensitive*factor);
                
                for(int i=0;i<ReverbCount;i++){
                    if(i==This->reverbActiveIdx){
                        This->reverb[i].state=RS_Active;
                        This->reverb[i].decayTime = 40.0f;
                    }else{
                        if(This->reverb[i].state==RS_Active){
                            //Reverb currently active -> change it to fading state
                            This->reverb[i].state = RS_Fading;
                            This->reverb[i].decayTime = fadingDecay;
                        }else{
                            This->reverb[i].state = RS_InActive;
                            This->reverb[i].decayTime = 0.5f;
                        }
                    }
                }
            }else
                This->verifyReverbChangeCount++;
        }else{
            //Reset verify
            This->verifyReverbChangeCount = 0;
        }
    }else{
        This->changeReverbCurrentSamples += numSamples;
    }
}

void BMLongReverb_updateReverbInput(BMLongReverb* This,int reverbIdx,float* inputL,float* inputR,float* dryInputL,float* dryInputR,size_t numSamples){
    //Update 2 reverbs settings
    if(This->updateReverbCount<ReverbCount){
        if(This->reverb[reverbIdx].state==RS_Active){
            //fade in to avoid click
            BMSmoothFade_startFading(&This->reverb[reverbIdx].smoothFade, FT_In);
        }else if(This->reverb[reverbIdx].state==RS_Fading){
            //Fade out & silent the reverb
            BMSmoothFade_startFading(&This->reverb[reverbIdx].smoothFade, FT_Out);
        }else{
            //Inactive fading -> dont need to fade anymore
        }
        //Set decay
        BMLongLoopFDN_setRT60DecaySmooth(&This->reverb[reverbIdx].loopFDN, This->reverb[reverbIdx].decayTime,false);
        
        This->updateReverbCount++;
    }
    
    //Apply setting
    BMSmoothFade_processBufferStereo(&This->reverb[reverbIdx].smoothFade, inputL, inputR,dryInputL, dryInputR, numSamples);
}

void BMLongReverb_processStereo(BMLongReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t numSamples,bool offlineRendering){
    if(This->initNo==ReadyNo){
        assert(numSamples<=BM_BUFFER_CHUNK_SIZE);
        for(int reverbIdx=0;reverbIdx<ReverbCount;reverbIdx++){
            BMLongReverb_updateVND(This);
            BMLongReverb_updateDiffusion(This);
            
            //Process attack softener for wet input
            BMMultibandAttackShaper_processStereo(&This->reverb[reverbIdx].attackSoftener, inputL, inputR, This->attackSoftenerBuffer.bufferL, This->attackSoftenerBuffer.bufferR, numSamples);
            
            //Measurement
            if(reverbIdx==This->reverbActiveIdx){
                //Only measure on active reverb
                BMLongReverb_measureSpectrum(This,reverbIdx, This->attackSoftenerBuffer.bufferL, This->attackSoftenerBuffer.bufferR, This->reverb[reverbIdx].lastWetBuffer.bufferL, This->reverb[reverbIdx].lastWetBuffer.bufferR, numSamples);
            }
            
            //Update input
            BMLongReverb_updateReverbInput(This, reverbIdx, inputL, inputR, This->reverb[reverbIdx].dryInput.bufferL, This->reverb[reverbIdx].dryInput.bufferR, numSamples);
            
            //1st layer VND
            for(int i=0;i<This->numInput;i++){
                //PitchShifting delay into wetbuffer
                BMVelvetNoiseDecorrelator_processBufferStereo(&This->reverb[reverbIdx].vndArray[i],This->reverb[reverbIdx].dryInput.bufferL, This->reverb[reverbIdx].dryInput.bufferR, This->vnd1BufferL[i], This->vnd1BufferR[i], numSamples);
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
            if(reverbIdx==This->reverbActiveIdx){
                memcpy(This->reverb[reverbIdx].lastWetBuffer.bufferL, This->reverb[reverbIdx].wetBuffer.bufferL, numSamples*sizeof(float));
                memcpy(This->reverb[reverbIdx].lastWetBuffer.bufferR, This->reverb[reverbIdx].wetBuffer.bufferR, numSamples*sizeof(float));
            }
            
            
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
                vDSP_vsmsma(This->reverb[reverbIdx].wetBuffer.bufferL, 1, &This->reverb[reverbIdx].reverbMixer.mixTarget, This->reverb[reverbIdx].dryInput.bufferL, 1, &dryMix, This->reverb[reverbIdx].outputL, 1, numSamples);
                vDSP_vsmsma(This->reverb[reverbIdx].wetBuffer.bufferR, 1, &This->reverb[reverbIdx].reverbMixer.mixTarget, This->reverb[reverbIdx].dryInput.bufferR, 1, &dryMix, This->reverb[reverbIdx].outputR, 1, numSamples);
            }else{
                //Process reverb dry/wet mixer
                BMWetDryMixer_processBufferInPhase(&This->reverb[reverbIdx].reverbMixer, This->reverb[reverbIdx].wetBuffer.bufferL, This->reverb[reverbIdx].wetBuffer.bufferR, This->reverb[reverbIdx].dryInput.bufferL, This->reverb[reverbIdx].dryInput.bufferR, This->reverb[reverbIdx].outputL, This->reverb[reverbIdx].outputR, numSamples);
            }
        }
        
        //Mix reverb output to final output
        vDSP_vclr(outputL, 1, numSamples);
        vDSP_vclr(outputR, 1, numSamples);
        for(int reverbIdx = 0;reverbIdx<ReverbCount;reverbIdx++){
//            if(This->reverb[reverbIdx].state==RS_InActive){
                vDSP_vadd(This->reverb[reverbIdx].outputL, 1, outputL, 1, outputL, 1, numSamples);
                vDSP_vadd(This->reverb[reverbIdx].outputR, 1, outputR, 1, outputR, 1, numSamples);
//            }
        }
        //Normalize
        float norm = 1.0f/sqrtf(2.0f);
        vDSP_vsmul(outputL, 1, &norm, outputL, 1, numSamples);
        vDSP_vsmul(outputR, 1, &norm, outputR, 1, numSamples);
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

void BMLongReverb_setMinSensitive(BMLongReverb* This,float threshold){
    This->minSensitive = threshold;
}

void BMLongReverb_setMaxSensitive(BMLongReverb* This,float threshold){
    This->maxSensitive = threshold;
}

void BMLongReverb_setMinDecay(BMLongReverb* This,float decay){
    This->minDecay = decay;
}

void BMLongReverb_setMaxDecay(BMLongReverb* This,float decay){
    This->maxDecay = decay;
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

float l2Norm(float *A, size_t length){
    float sumOfSquares;
    vDSP_svesq(A,1,&sumOfSquares,length);
    return sqrtf(sumOfSquares);
}

float l1Norm(float *A, size_t length){
    float sumOfMag;
    vDSP_svemg(A,1,&sumOfMag,length);
    return sumOfMag;
}



// 0 is simplest spectrum and 1 is most complex
float spectralComplexity(float *A, size_t length){
    float l1n = l1Norm(A,length);
    float l2n = l2Norm(A,length);

    // Mathematica Prototype original
    // 1-(((Norm[x,2]/Norm[x,1])-(1/Sqrt[Length[x]]))/(1-1/Sqrt[Length[x]]))
    //
    // Mathematica Prototype simplified
    // -((Sqrt(length)*(-1+nDiv))/(-1+Sqrt(length)))

    float nDiv = l2n / l1n;
    float sqrtLn = sqrt((float)length);
    return -((sqrtLn * (nDiv - 1.0f)) / (sqrtLn - 1.0f));
//    return nDiv*1000;
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
