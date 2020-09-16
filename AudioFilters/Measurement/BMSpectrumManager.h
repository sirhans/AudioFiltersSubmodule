//
//  BMSpectrumManager.h
//  BMAUEqualizer
//
//  Created by Nguyen Minh Tien on 7/6/18.
//  Copyright © 2018 Nguyen Minh Tien. All rights reserved.
//

#ifndef BMSpectrumManager_h
#define BMSpectrumManager_h

#include <stdio.h>
#include "Constants.h"
#include "BMSpectrum.h"
#include "TPCircularBuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BMSpectrumManager {
        TPCircularBuffer dataBuffer;
        int storeSize;
        float refreshRate;
        float decay;
        float* magSpectrum;
        float* indices;
        float* yValue;
        
        BMSpectrum spectrum;
        
        
        bool audioReady;
    } BMSpectrumManager;
    
    void BMSpectrumManager_init(BMSpectrumManager* this,float decay,float rate);
    
    void BMSpectrumManager_destroy(BMSpectrumManager* this);
    
    void BMSpectrumManager_setAudioReady(BMSpectrumManager* this,bool ready);
    
    void BMSpectrumManager_storeData(BMSpectrumManager* this,float* outData ,UInt32 frameCount);
    
    bool BMSpectrumManager_processDataSpectrumY(BMSpectrumManager* this,float* spectrumDataY,size_t spectrumSize,float sHeight);
    
    void BMSpectrumManager_processPeakDataSpectrumY(BMSpectrumManager* this,float* spectrumDataY,float* spectrumPeakDataY,size_t spectrumSize,float sHeight);
    
    void BMSpectrumManager_prepareXSpectrumData(BMSpectrumManager* this,float* spectrumDataX,float* spectrumDataY,float* spectrumPeakDataY,size_t spectrumSize,float sWidth,float sHeight,float sampleRate);
    
//    void BMSpectrumManager_requestSwapBuffer(BMSpectrumManager* this);
#ifdef __cplusplus
}
#endif

#endif /* BMSpectrumManager_h */
