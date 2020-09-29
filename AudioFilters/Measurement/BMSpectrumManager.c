//
//  BMSpectrumManager.c
//  BMAUEqualizer
//
//  Created by Nguyen Minh Tien on 7/6/18.
//  Copyright © 2018 Nguyen Minh Tien. All rights reserved.
//

#include "BMSpectrumManager.h"
#include <Accelerate/Accelerate.h>
#import "BMRMSPower.h"
#import "MyConstants.h"
#import "BMSpectrum.h"

#define spectrumMinDB 25.0
#define spectrumMaxDB spectrumMinDB + (maxDB - minDB)*1.2
#define StoreSize 4096
#define GRAP_NUMPOINT 500

void BMSpectrumManager_prepareIndices(BMSpectrumManager* this, size_t n);

void BMSpectrumManager_init(BMSpectrumManager* this,float decay,float rate){
    this->storeSize = StoreSize;
    this->refreshRate = rate;
    this->decay = decay;
    BMSpectrum_initWithLength(&this->spectrum,StoreSize);
    this->magSpectrum = malloc(sizeof(float)*StoreSize);
    this->indices = malloc(sizeof(float)*GRAP_NUMPOINT);
    this->yValue = malloc(sizeof(float)*GRAP_NUMPOINT);
    //Buffer
    BMSpectrumManager_prepareIndices(this, StoreSize);
    //Circular buffer
    TPCircularBufferInit(&this->dataBuffer, sizeof(float)* (this->storeSize + BM_BUFFER_CHUNK_SIZE));
    TPCircularBufferClear(&this->dataBuffer);
    // get the head pointer
    uint32_t availableBytes;
    float* head = TPCircularBufferHead(&this->dataBuffer, &availableBytes);
    
    // confirm that the buffer has space as request ed
    assert(availableBytes > sizeof(float)*(this->storeSize + BM_BUFFER_CHUNK_SIZE));
    
    // write zeros
    memset(head, 0, sizeof(float)*(this->storeSize));
    
    // mark the bytes as written
    TPCircularBufferProduce(&this->dataBuffer,
                            sizeof(float)*(this->storeSize));
 
    this->isInit = true;
}

void BMSpectrumManager_prepareIndices(BMSpectrumManager* this, size_t n){
    //Generate indices
    float magSpectrumSize = n;
    float maxVal = powf(1.01,GRAP_NUMPOINT)-1.0;
    for(int t =0; t < GRAP_NUMPOINT; t++){
        this->indices[t] = (magSpectrumSize-1.0)*((powf(1.01,(float)t+1.0)-1.0)/maxVal);
//        this->indices[t] = (float)(t * magSpectrumSize)/GRAP_NUMPOINT;
//        printf("%f\n",this->indices[t]);
    }
}



void BMSpectrumManager_destroy(BMSpectrumManager* this){
    this->isInit = false;
    TPCircularBufferCleanup(&this->dataBuffer);
    
    free(this->magSpectrum);
    this->magSpectrum = NULL;
    
    free(this->indices);
    this->indices = NULL;
    
    free(this->yValue);
    this->yValue = NULL;
}

#pragma mark - store audio data
#pragma mark - Store data
void BMSpectrumManager_storeData(BMSpectrumManager* this,float* inData ,UInt32 frameCount){
    while (frameCount > 0){
        
        // get the write pointer for the buffer
        int bytesAvailable;
        float* head = TPCircularBufferHead(&this->dataBuffer, &bytesAvailable);
        
        // convert units from bytes to samples to get the size of the
        // space available for writing
        int samplesAvailable = bytesAvailable / sizeof(float);
        
        // don't process more than the available write space in the input buffer
        int samplesProcessing = frameCount < samplesAvailable ? (int)frameCount : samplesAvailable;
        
        // don't process more than the available space in the interleaving buffers
        samplesProcessing = samplesProcessing < BM_BUFFER_CHUNK_SIZE ? samplesProcessing : BM_BUFFER_CHUNK_SIZE;
        
        // convert samples to bytes for memcpy
        int bytesProcessing = samplesProcessing * sizeof(float);
        
        // copy the input into the buffer
        memcpy(head, inData, bytesProcessing);
        
        // advance the write pointer
        TPCircularBufferProduce(&this->dataBuffer, bytesProcessing);
        
        // advance the read pointer
        //Consume amount = produce amount to keep data newest
        TPCircularBufferConsume(&this->dataBuffer, bytesProcessing);
        
        // advance pointers
        inData += samplesProcessing;
        
        // how many samples are left?
        frameCount -= samplesProcessing;
    }
}

#pragma mark - Data Delegate
bool BMSpectrumManager_processDataSpectrumY(BMSpectrumManager* this,float* spectrumDataY,size_t spectrumSize,float sHeight){
    if(this->isInit){
        //get data
        uint32_t bytesAvailable;
        float* tail = TPCircularBufferTail(&this->dataBuffer, &bytesAvailable);
        
        int samplesAvailable = bytesAvailable / sizeof(float);
        
        // don't process more than the available write space in the input buffer
        int inputSize = this->storeSize < samplesAvailable ? (int)this->storeSize : samplesAvailable;

        float inRMS = BMRMSPower_process(tail, inputSize);
        if(inRMS>0){
            size_t outSize;
//            float nq;
//            BMSpectrum_processData(&this->spectrum, tail,this->magSpectrum, inputSize, &outSize, &nq);
            outSize = (inputSize / 2);
            this->magSpectrum[outSize] = BMSpectrum_processDataBasic(&this->spectrum, tail, this->magSpectrum, true, inputSize);
            
            if(inputSize>0){
                
                // Limit the range of output values
                float minValue = BM_DB_TO_GAIN(-160.0f);
                float maxValue = BM_DB_TO_GAIN(60.0f);
                vDSP_vclip(this->magSpectrum, 1, &minValue, &maxValue, this->magSpectrum, 1, outSize);
                float b = 0.01;
                vDSP_vdbcon(this->magSpectrum, 1, &b, this->magSpectrum, 1, outSize, 1);
                
                //Calculate spectrum Data y value
                vDSP_vqint(this->magSpectrum, this->indices, 1, this->yValue, 1, spectrumSize, outSize);
                
                //Adjust yValue from minDb to max Db to 0 -> 1
                //((yValue[i] - spectrumMinDB)/(spectrumMaxDB - spectrumMinDB)-.5)*sHeight;
                float a1 = (-spectrumMinDB);
                float a2 = -1./(spectrumMaxDB - spectrumMinDB);
                float a3 = 1;
                vDSP_vsadd(this->yValue, 1, &a1, this->yValue, 1, spectrumSize);
                vDSP_vsmsa(this->yValue, 1, &a2, &a3, this->yValue, 1, spectrumSize);
                vDSP_vsmul(this->yValue, 1, &sHeight, spectrumDataY, 1, spectrumSize);

                return true;
            }
        }
    }
    return false;
}

void BMSpectrumManager_processPeakDataSpectrumY(BMSpectrumManager* this,float* spectrumDataY,float* spectrumPeakDataY,size_t spectrumSize,float sHeight){
    //Peak line
    //spectrumPeakData = MAX(-halfHeight,spectrumPeakData - sHeight * 0.03);
    //Decay spectrum peak data overtime
//    float halfHeight = sHeight* .5;
    float decayV = (sHeight * this->decay)/this->refreshRate;//0.015
    vDSP_vsadd(spectrumPeakDataY, 1, &decayV, spectrumPeakDataY, 1, spectrumSize);
    //Clip it to -Halfheight
    float clipMin = 0;
    float clipMax = FLT_MAX;
    vDSP_vclip(spectrumPeakDataY, 1, &clipMin, &clipMax, spectrumPeakDataY, 1, spectrumSize);
    //Get new peak value
    vDSP_vmin(spectrumPeakDataY, 1, spectrumDataY, 1, spectrumPeakDataY, 1, spectrumSize);
}

void BMSpectrumManager_prepareXSpectrumData(BMSpectrumManager* this,float* spectrumDataX,float* spectrumDataY,float* spectrumPeakDataY,size_t spectrumSize,float sWidth,float sHeight,float sampleRate){
    if(this->isInit){
        float defaultV = sHeight;
        vDSP_vfill(&defaultV, spectrumDataY, 1, spectrumSize);
        vDSP_vfill(&defaultV, spectrumPeakDataY, 1, spectrumSize);
        
        int inputSize = this->storeSize;
        for(int i =0;i<inputSize;i++){
            this->magSpectrum[i] = i * sampleRate/this->storeSize;
    //        printf("mag %f\n",this->magSpectrum[i]);
        }
        vDSP_vqint(this->magSpectrum, this->indices, 1, spectrumDataX, 1, spectrumSize, inputSize);
    }
}
