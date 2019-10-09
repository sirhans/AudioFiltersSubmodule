//
//  BMStereoModulator.h
//  AUStereoModulator
//
//  Created by Nguyen Minh Tien on 3/13/18.
//  Copyright © 2018 Nguyen Minh Tien. All rights reserved.
//

#ifndef BMStereoModulator_h
#define BMStereoModulator_h

#include <stdio.h>
#import "BMVelvetNoiseDecorrelator.h"
#import <Accelerate/Accelerate.h>
#import "BMQuadratureOscillator.h"
#include "BMCrossover.h"
#include "BMStereoWidener.h"
#include "BMDiffusion.h"

#define S_Wet_Min 0.f
#define S_Wet_Max 1.f
#define S_Wet_DV 0.6f
#define S_Quad_Min 0.f
#define S_Quad_Max 7.f
#define S_Quad_DV 0.f
#define S_Tightness_Min 10.f
#define S_Tightness_Max 100.f
#define S_Tightness_DV 40.f
#define S_Density_Min 4.f
#define S_Density_Max 50.f
#define S_Density_DV 50.f
#define S_Midside_Min 0.f
#define S_Midside_Max 2.f

typedef struct BMStereoModulator {
    BMVelvetNoiseDecorrelator vndFilter1;
    BMVelvetNoiseDecorrelator vndFilter2;
    BMVelvetNoiseDecorrelator vndFilter3;
    BMVelvetNoiseDecorrelator vndFilterOffQuad;
    BMVelvetNoiseDecorrelator vndFilterDiffusion;
    
    BMQuadratureOscillator quadratureOscil;
    
    BMCrossover crossLowFilter;
//    BMCrossover crossHighFilter;
    
    BMStereoWidener midSideFilter;
    
//    BMDiffusion diffusionFilter;
    
    float quadHz;
    float msDelayTime;
    float numTaps;
    float sampleRate;
    
    bool vndUpdate;
    bool isInit;
    
    float* vndBuffer1L;
    float* vndBuffer1R;
    float* vndBuffer2L;
    float* vndBuffer2R;
    float* vndBuffer3L;
    float* vndBuffer3R;
    
    float* vol1;
    float* vol2;
    float* vol3;
    bool buffer1ReInit;
    bool buffer2ReInit;
    bool buffer3ReInit;
    
    bool turnOffQuad;
    bool updateQuad;
    
    float* cbLowL;
    float* cbLowR;
    float* cbHighL;
    float* cbHighR;
    
    float dryVol;
    float wetVol;
    
    float midSideV;
    bool midSideUpdate;
    
    int maxNumTaps;
} BMStereoModulator;

void BMStereoModulator_init(BMStereoModulator* This,
                            double sRate,
                            float msDelayTime, float msMaxDelayTime,
                            int numTaps,int maxNumTaps,
                            float lfoFreqHz);
void BMStereoModulator_destroy(BMStereoModulator* This);
void BMStereoModulator_impulseResponse(BMStereoModulator* this,size_t frameCount);

void BMStereoModulator_processStereo(BMStereoModulator* This,
                                     float* inDataL, float* inDataR,
                                     float* outDataL,float* outDataR,
                                     size_t frameCount);

void BMStereoModulator_reInitStandBy(BMStereoModulator* This, int standbyIndex);
void BMStereoModulator_setQuadHz(BMStereoModulator* This, float value);
void BMStereoModulator_setDelayTime(BMStereoModulator* This,float msDelayTime);
void BMStereoModulator_setNumTap(BMStereoModulator* This,int numTap);
void BMStereoModulator_setDryWet(BMStereoModulator* This,float wetVol);
void BMStereoModulator_setMidSide(BMStereoModulator* This,float value);
#endif /* BMStereoModulator_h */
