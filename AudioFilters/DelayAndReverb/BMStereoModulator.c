//
//  BMStereoModulator.c
//  AUStereoModulator
//
//  Created by Nguyen Minh Tien on 3/13/18.
//  Copyright © 2018 Nguyen Minh Tien. All rights reserved.
//

#include "BMStereoModulator.h"
#include "Constants.h"

#define BM_STEREO_MOD_LOW_CROSSOVER_FC 300
#define BM_STEREO_MOD_DIFFUSION_MINTAPSAMPLE 2.

void BMStereoModulator_UpdateVND(BMStereoModulator* This);
void BMStereoModulator_UpdateMidSide(BMStereoModulator* This);
void BMStereoModulator_updateQuadHz(BMStereoModulator* This);

void BMStereoModulator_init(BMStereoModulator* This,
                            double sRate,
                            float msDelayTime, float msMaxDelayTime,
                            int numTaps,int maxNumTaps,
                            float lfoFreqHz){
    This->sampleRate = sRate;
    
    //Malloc
    This->vndBuffer1L = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->vndBuffer1R = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->vndBuffer2L = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->vndBuffer2R = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->vndBuffer3L = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->vndBuffer3R = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    This->vol1 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->vol2 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->vol3 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    //Crossover
    BMCrossover_init(&This->crossLowFilter, BM_STEREO_MOD_LOW_CROSSOVER_FC, sRate, false, true);
    This->cbLowL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->cbLowR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->cbHighL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->cbHighR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    //Init filter
    This->numTaps = numTaps;
    This->msDelayTime = msDelayTime;
    This->vndUpdate = true;
    This->maxNumTaps = maxNumTaps;
    
    //Init 3 vnd filters
    BMVelvetNoiseDecorrelator_init(&This->vndFilterOffQuad, true, This->msDelayTime,msMaxDelayTime, This->numTaps,This->maxNumTaps, This->sampleRate,This->msDelayTime);
    BMVelvetNoiseDecorrelator_setDelayTimeNumTap(&This->vndFilterOffQuad, This->msDelayTime, This->numTaps, This->sampleRate,This->msDelayTime);
    
    BMVelvetNoiseDecorrelator_init(&This->vndFilter1, true, This->msDelayTime,msMaxDelayTime, This->numTaps,This->maxNumTaps, This->sampleRate,This->msDelayTime);
//    BMVelvetNoiseDecorrelator_setDelayTimeNumTap(&This->vndFilter1, This->msDelayTime, This->numTaps, This->sampleRate,This->msDelayTime);
    
    //Only using quadrature need 2 more vnd to update
    BMVelvetNoiseDecorrelator_init(&This->vndFilter2, true, This->msDelayTime,msMaxDelayTime, This->numTaps,This->maxNumTaps, This->sampleRate,This->msDelayTime);
    BMVelvetNoiseDecorrelator_init(&This->vndFilter3, true, This->msDelayTime,msMaxDelayTime, This->numTaps,This->maxNumTaps, This->sampleRate,This->msDelayTime);
    
    // Init VND filter for diffusion
    // the delay time should be the time between taps on the other VNDs above
    // let's assume the others have 16 taps at 100ms, then we need the diffuser
    // to have (100 / 16) ms of delay time
    float diffuserDelayTime = (This->msDelayTime/This->numTaps)*2.;
    float numTapAtMinSample = (diffuserDelayTime/1000.)*This->sampleRate/BM_STEREO_MOD_DIFFUSION_MINTAPSAMPLE;
    int diffNumtap = numTapAtMinSample> This->numTaps ? This->numTaps : numTapAtMinSample;
    BMVelvetNoiseDecorrelator_init(&This->vndFilterDiffusion, true, diffuserDelayTime,msMaxDelayTime, diffNumtap, This->maxNumTaps, This->sampleRate,This->msDelayTime);
    
    This->isInit = true;
    
    //QuadHz
    BMStereoModulator_setQuadHz(This, lfoFreqHz);
    
//    BMStereoModulator_impulseResponse(This, 2000);
}

void BMStereoModulator_impulseResponse(BMStereoModulator* this,size_t frameCount){
    float* irBuffer = malloc(sizeof(float)*frameCount);
    float* outBuffer = malloc(sizeof(float)*frameCount);
    for(int i=0;i<frameCount;i++){
        if(i==0)
            irBuffer[i] = 1;
        else
            irBuffer[i] = 0;
    }
    
    BMStereoModulator_processStereo(this, irBuffer, irBuffer, outBuffer, outBuffer, frameCount);
    
    printf("\[");
    for(size_t i=0; i<(frameCount-1); i++)
        printf("%f,", outBuffer[i]);
    printf("%f],",outBuffer[frameCount-1]);
    
}

void BMStereoModulator_destroy(BMStereoModulator* This){
    //Destroy right away
    free(This->vndBuffer1L);
    This->vndBuffer1L = NULL;
    free(This->vndBuffer1R);
    This->vndBuffer1R = NULL;
    
    free(This->vndBuffer2L);
    This->vndBuffer2L = NULL;
    free(This->vndBuffer2R);
    This->vndBuffer2R = NULL;
    
    free(This->vndBuffer3L);
    This->vndBuffer3L = NULL;
    free(This->vndBuffer3R);
    This->vndBuffer3R = NULL;
    
    free(This->vol1);
    This->vol1 = NULL;
    free(This->vol2);
    This->vol2 = NULL;
    free(This->vol3);
    This->vol3 = NULL;
    
    free(This->cbLowL);
    This->cbLowL = NULL;
    free(This->cbLowR);
    This->cbLowR = NULL;
    free(This->cbHighL);
    This->cbHighL = NULL;
    free(This->cbHighR);
    This->cbHighR = NULL;
    
    //Crossover
    BMCrossover_destroy(&This->crossLowFilter);
    
    BMVelvetNoiseDecorrelator_free(&This->vndFilterOffQuad);
    BMVelvetNoiseDecorrelator_free(&This->vndFilter1);
    BMVelvetNoiseDecorrelator_free(&This->vndFilter2);
    BMVelvetNoiseDecorrelator_free(&This->vndFilter3);
    BMVelvetNoiseDecorrelator_free(&This->vndFilterDiffusion);
    
    This->isInit = false;
}

void BMStereoModulator_processStereo(BMStereoModulator* This,float* inDataL,float* inDataR,float* outDataL,float* outDataR, size_t frameCount){
    if(This->isInit){
        //Use BMQuadratureOscillator to calculate the volume
        size_t processingFrame = frameCount;
        size_t processedFrame = 0;
        int standbyIndex;
        
        //Update vnd
        BMStereoModulator_UpdateVND(This);
        //Update midside
        BMStereoModulator_UpdateMidSide(This);
        //Update quadhz
        BMStereoModulator_updateQuadHz(This);
        
        while(processedFrame<frameCount){
            processingFrame = frameCount - processedFrame;
            //ProcessingFrame can not be larger than Buffer_chunk_size -> divide to process
            processingFrame = processingFrame<BM_BUFFER_CHUNK_SIZE ? processingFrame : BM_BUFFER_CHUNK_SIZE;
            //Standby -> need to process extra frame
            
            
            // create pointers to make the code readable when we reuse the output
            float* wetHighL = outDataL + processedFrame;
            float* wetHighR = outDataR + processedFrame;
            
            //Stereo modulator process only high freq
            if(This->turnOffQuad){
                //Cross over divide audio into high & low freq
                BMCrossover_processStereo(&This->crossLowFilter, inDataL+processedFrame, inDataR+processedFrame, This->cbLowL, This->cbLowR, This->cbHighL, This->cbHighR, processingFrame);
                
                // Diffusion done by VND
                BMVelvetNoiseDecorrelator_processBufferStereo(&This->vndFilterDiffusion, This->cbHighL, This->cbHighR, wetHighL, wetHighR, processingFrame);
                
                //Use only 1 vnd
                BMVelvetNoiseDecorrelator_processBufferStereo(&This->vndFilterOffQuad, wetHighL, wetHighR, This->vndBuffer1L, This->vndBuffer1R, processingFrame);
                //Copy from vndbuffer 1 to output
                memcpy(wetHighL, This->vndBuffer1L, sizeof(float)*processingFrame);
                memcpy(wetHighR, This->vndBuffer1R, sizeof(float)*processingFrame);
            }else{
                //Use quadrature to calculate the vol1,2,3
                //This will changed the processing frame that should be used 1st
                BMQuadratureOscillator_processQuadTriple(&This->quadratureOscil, This->vol1, This->vol2, This->vol3, &processingFrame,&standbyIndex);

                assert(processingFrame!=0);
                
                //Cross over divide audio into high & low freq
                BMCrossover_processStereo(&This->crossLowFilter, inDataL+processedFrame, inDataR+processedFrame, This->cbLowL, This->cbLowR, This->cbHighL, This->cbHighR, processingFrame);
                
                // Diffusion done by VND
                BMVelvetNoiseDecorrelator_processBufferStereo(&This->vndFilterDiffusion, This->cbHighL, This->cbHighR, wetHighL, wetHighR, processingFrame);
                
                //Vnd 1 & vnd 2, 3 run in parallel
                BMVelvetNoiseDecorrelator_processBufferStereo(&This->vndFilter1, wetHighL, wetHighR, This->vndBuffer1L, This->vndBuffer1R, processingFrame);
                BMVelvetNoiseDecorrelator_processBufferStereo(&This->vndFilter2, wetHighL, wetHighR, This->vndBuffer2L, This->vndBuffer2R, processingFrame);
                BMVelvetNoiseDecorrelator_processBufferStereo(&This->vndFilter3, wetHighL, wetHighR, This->vndBuffer3L, This->vndBuffer3R, processingFrame);
                
                //Mix output
                //1 + 2
                vDSP_vmma(This->vndBuffer1L, 1, This->vol1, 1, This->vndBuffer2L, 1, This->vol2, 1, wetHighL, 1, processingFrame);
                vDSP_vmma(This->vndBuffer1R, 1, This->vol1, 1, This->vndBuffer2R, 1, This->vol2, 1, wetHighR, 1, processingFrame);
                //OutData + 3
                vDSP_vma(This->vndBuffer3L, 1, This->vol3, 1, wetHighL, 1, wetHighL, 1, processingFrame);
                vDSP_vma(This->vndBuffer3R, 1, This->vol3, 1, wetHighR, 1, wetHighR, 1, processingFrame);
                
                //Reinit standby VND here
                BMStereoModulator_reInitStandBy(This,standbyIndex);
            }
            
            //Mix dry & wet high frequency
            //Dry is input -- Wet is output
            vDSP_vsmsma(This->cbHighL, 1, &This->dryVol, wetHighL, 1, &This->wetVol, wetHighL, 1, processingFrame);
            vDSP_vsmsma(This->cbHighR, 1, &This->dryVol, wetHighR, 1, &This->wetVol, wetHighR, 1, processingFrame);
            
            float* inMidSideL = This->vndBuffer1L;
            float* inMidSideR = This->vndBuffer1R;
            //Mix cross low & mid & high freq
            vDSP_vadd(wetHighL, 1, This->cbLowL, 1,inMidSideL , 1, processingFrame);
            vDSP_vadd(wetHighR, 1, This->cbLowR, 1,inMidSideR , 1, processingFrame);
            //        printf("process %zu %zu %u\n",processingFrame,processedFrame,(unsigned int)frameCount);
            
            //Use midside filter here
            BMStereoWidener_processAudio(&This->midSideFilter, inMidSideL, inMidSideR, outDataL+processedFrame, outDataR+processedFrame, processingFrame);
            
            processedFrame += processingFrame;
        }
    }
}


/*
 * In the set of three VNDs there are always two active and one on standby.
 * This function manages swapping the VNDs in and out of standby mode.
 */
void BMStereoModulator_reInitStandBy(BMStereoModulator* This, int standbyIndex){
    if(standbyIndex!=-1){
        //Update standby vnd delay time here
        if(standbyIndex==QIdx_Vol1&&!This->buffer1ReInit){
            //Vol1
            BMVelvetNoiseDecorrelator_setDelayTimeNumTap(&This->vndFilter1, This->msDelayTime, This->numTaps, This->sampleRate,This->msDelayTime);
            This->buffer1ReInit = true;
            This->buffer2ReInit = false;
            This->buffer3ReInit = false;
        }else if(standbyIndex==QIdx_Vol2&&!This->buffer2ReInit){
            //Vol2
            BMVelvetNoiseDecorrelator_setDelayTimeNumTap(&This->vndFilter2, This->msDelayTime, This->numTaps, This->sampleRate,This->msDelayTime);
            This->buffer1ReInit = false;
            This->buffer2ReInit = true;
            This->buffer3ReInit = false;
        }else if(standbyIndex==QIdx_Vol3&&!This->buffer3ReInit){
            //Vol3
            BMVelvetNoiseDecorrelator_setDelayTimeNumTap(&This->vndFilter3, This->msDelayTime, This->numTaps, This->sampleRate,This->msDelayTime);
            This->buffer1ReInit = false;
            This->buffer2ReInit = false;
            This->buffer3ReInit = true;
        }
    }
}

#pragma mark - Public methods
void BMStereoModulator_DrawIR(BMStereoModulator* This){
    float total = 20;
    float maxTightnessObs = 40;
    for(int i=0;i<total;i++){
        float delayTime = i* (maxTightnessObs - S_Tightness_Min) / total + S_Tightness_Min;
        BMStereoModulator_setDelayTime(This, delayTime);
        
        BMStereoModulator_impulseResponse(This, maxTightnessObs*This->sampleRate/1000.);
    }
}

void BMStereoModulator_setQuadHz(BMStereoModulator* This, float value){
    if(This->isInit){
        This->quadHz = value;
        if(This->quadHz<=S_Quad_Min){
            This->turnOffQuad = true;
        }else{
            This->turnOffQuad = false;
            This->updateQuad = true;
        }
    }
}

void BMStereoModulator_updateQuadHz(BMStereoModulator* This){
    if(This->isInit){
        if(This->updateQuad){
            This->updateQuad = false;
            BMQuadratureOscillator_reInit(&This->quadratureOscil, This->quadHz, This->sampleRate);
        }
    }
}

void BMStereoModulator_setDelayTime(BMStereoModulator* This,float msDelayTime){
    if(This->isInit){
        This->msDelayTime = msDelayTime;
        This->vndUpdate = true;
        
        
    }
}

void BMStereoModulator_setNumTap(BMStereoModulator* This,int numTap){
    if(This->isInit){
        This->numTaps = numTap;
        This->vndUpdate = true;
    }
}

void BMStereoModulator_setDryWet(BMStereoModulator* This,float wetVol){
    This->wetVol = sqrtf(wetVol);
    This->dryVol = sqrtf(1-wetVol);
//    printf("wet %f dry %f\n",This->wetVol,This->dryVol);
//    BMStereoModulator_DrawIR(This);
}

void BMStereoModulator_setMidSide(BMStereoModulator* This,float value){
    if(This->isInit){
        This->midSideV = value;
        This->midSideUpdate = true;
    }
}

void BMStereoModulator_UpdateVND(BMStereoModulator* This){
    if(This->vndUpdate){
        This->vndUpdate = false;
        BMVelvetNoiseDecorrelator_setDelayTimeNumTap(&This->vndFilterOffQuad, This->msDelayTime, This->numTaps, This->sampleRate,This->msDelayTime);
        BMVelvetNoiseDecorrelator_setDelayTimeNumTap(&This->vndFilter1, This->msDelayTime, This->numTaps, This->sampleRate,This->msDelayTime);
        BMVelvetNoiseDecorrelator_setDelayTimeNumTap(&This->vndFilter2, This->msDelayTime, This->numTaps, This->sampleRate,This->msDelayTime);
        BMVelvetNoiseDecorrelator_setDelayTimeNumTap(&This->vndFilter3, This->msDelayTime, This->numTaps, This->sampleRate,This->msDelayTime);
        //Update vnd diffusion
        float delayTime = (This->msDelayTime/This->numTaps)*2.;
        float numTapAtMinSample = (delayTime/1000.)*This->sampleRate/BM_STEREO_MOD_DIFFUSION_MINTAPSAMPLE;
        int diffNumtap = numTapAtMinSample> This->numTaps ? This->numTaps : numTapAtMinSample;
        BMVelvetNoiseDecorrelator_setDelayTimeNumTap(&This->vndFilterDiffusion, delayTime, diffNumtap, This->sampleRate,This->msDelayTime);
//        printf("update diff %f %d\n",delayTime,diffNumtap);
        
//        printf("%f %f\n",This->msDelayTime,This->numTap);
    }
}

void BMStereoModulator_UpdateMidSide(BMStereoModulator* This){
    if(This->midSideUpdate){
        This->midSideUpdate = false;
        BMStereoWidener_setWidth(&This->midSideFilter, This->midSideV);
    }
}
