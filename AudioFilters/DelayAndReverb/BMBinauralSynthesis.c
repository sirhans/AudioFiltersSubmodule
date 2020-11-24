//
//  BMBinauralSynthesis.c
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 5/6/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#include "BMBinauralSynthesis.h"

#define numCoefficient 5
#define numLevel 2
#define BS_Level_Binaural 0
#define BS_Level_Highshelf 1
#define BS_Back_AngleRange 30
#define BS_HS_Gain_Min 0
#define BS_HS_Gain_Max -10
#define BS_HS_Freq 1500
#define RADIANS_TO_DEGREES(radians) ((radians) * (180.0 / M_PI))
#define DEGREES_TO_RADIANS(degrees) ((degrees) * (M_PI / 180.0))

static inline float calculateAlphaAngle(float degree);
void BMBinauralSynthesis_updateCoefficient(BMBinauralSynthesis *This);
void BMBinauralSynthesis_normalizeA0(BMBinauralSynthesis *This,size_t channel);

void BMBinauralSynthesis_init(BMBinauralSynthesis *This,bool isStereo,float sampleRate){
    This->numChannels = isStereo == true ? 2 : 1;
    This->coefficient_f = malloc(sizeof(float)*numCoefficient *This->numChannels);
    This->coefficient_d = malloc(sizeof(double)*numCoefficient *This->numChannels);
    This->coefficient_normD = malloc(sizeof(double)*numCoefficient *This->numChannels);
    This->degree = 0;
    This->needUpdate = false;
    This->sampleRate = sampleRate;
    This->hsGain = 0;
    This->hsFreq = BS_HS_Freq;
    
    BMMultiLevelBiquad_init(&This->filter, numLevel, sampleRate, isStereo, false, true);
    
    for(int i=0;i<numCoefficient  *This->numChannels;i++){
        This->coefficient_f[i] = This->coefficient_d[i] = This->coefficient_normD[i] = 0;
    }
    
    float c = 343; //m/s speed of sound
    float a = 0.09; //radius
    This->w0 = c/a;
    
    BMBinauralSynthesis_setAngleLeft(This, This->degree);
    BMBinauralSynthesis_setAngleRight(This, This->degree);
    
    BMBinauralSynthesis_updateCoefficient(This);
}

void BMBinauralSynthesis_destroy(BMBinauralSynthesis *This){
    free(This->coefficient_d);
    free(This->coefficient_normD);
    free(This->coefficient_f);
    
    BMMultiLevelBiquad_free(&This->filter);
}

void BMBinauralSynthesis_processMono(BMBinauralSynthesis *This,float* in,float* out, size_t numSamples){
    //update coefficient if need to
    BMBinauralSynthesis_updateCoefficient(This);
    
    //Process biquad
    BMMultiLevelBiquad_processBufferMono(&This->filter, in, out, numSamples);
}

void BMBinauralSynthesis_processStereo(BMBinauralSynthesis *This,float* inL,float* inR, float* outL, float* outR, size_t numSamples){
    //update coefficient if need to
    BMBinauralSynthesis_updateCoefficient(This);
    
    //Process biquad
    BMMultiLevelBiquad_processBufferStereo(&This->filter, inL, inR, outL, outR, numSamples);
}

void BMBinauralSynthesis_updateCoefficient(BMBinauralSynthesis *This){
    if(This->needUpdate){
        This->needUpdate = false;
        
        //Cast from float to double
        for(int i=0;i<numCoefficient  *This->numChannels;i++){
            This->coefficient_d[i] = This->coefficient_f[i];
        }
        //Normalize A0
        for(int i=0;i<This->numChannels;i++){
            BMBinauralSynthesis_normalizeA0(This, i);
        }
        
        //Set coefficient z
        BMMultiLevelBiquad_setCoefficientZ(&This->filter, BS_Level_Binaural, This->coefficient_normD);
        
        //Highshelf filter
        BMMultiLevelBiquad_setHighShelf(&This->filter, This->hsFreq, This->hsGain, BS_Level_Highshelf);
    }
}

#pragma mark - Set param

void BMBinauralSynthesis_setAngleLeft(BMBinauralSynthesis *This,float degree){
    float alpha = calculateAlphaAngle(degree);
    
    size_t channel = 0;
    
    This->coefficient_f[0 + channel*numCoefficient] = This->w0 + alpha  *This->sampleRate;//b0
    This->coefficient_f[1 + channel*numCoefficient] = This->w0 - alpha  *This->sampleRate;//b1
    This->coefficient_f[2 + channel*numCoefficient] = 0;//b2
    This->coefficient_f[3 + channel*numCoefficient] = This->w0 + This->sampleRate;//a0
    This->coefficient_f[4 + channel*numCoefficient] = This->w0 - This->sampleRate;//a1
    
    This->needUpdate = true;
}

void BMBinauralSynthesis_setAngleRight(BMBinauralSynthesis *This,float degree){
    float alpha = calculateAlphaAngle(degree);
    
    size_t channel = 1;
    
    This->coefficient_f[0 + channel*numCoefficient] = This->w0 + alpha  *This->sampleRate;//b0
    This->coefficient_f[1 + channel*numCoefficient] = This->w0 - alpha  *This->sampleRate;//b1
    This->coefficient_f[2 + channel*numCoefficient] = 0;//b2
    This->coefficient_f[3 + channel*numCoefficient] = This->w0 + This->sampleRate;//a0
    This->coefficient_f[4 + channel*numCoefficient] = This->w0 - This->sampleRate;//a1
    
    //Highshelf
    This->needUpdate = true;
    
    if(degree>=BS_Back_AngleRange&&degree<=180 - BS_Back_AngleRange){
        float scale = fabsf(degree-90)/ (90-BS_Back_AngleRange);
        float gain = (BS_HS_Gain_Max - BS_HS_Gain_Min) *  (1- scale) + BS_HS_Gain_Min;
        BMBinauralSynthesis_setHSGain(This, gain);
//        printf("right %f %f\n",degree,gain);
    }else{
        BMBinauralSynthesis_setHSGain(This,0);
    }
}

void BMBinauralSynthesis_setHSGain(BMBinauralSynthesis *This,float gainDb){
    This->hsGain = gainDb;
    This->needUpdate = true;
}

void BMBinauralSynthesis_normalizeA0(BMBinauralSynthesis *This,size_t channel){
    double b0 = This->coefficient_d[channel*numCoefficient];
    double b1 = This->coefficient_d[channel*numCoefficient + 1];
    double b2 = This->coefficient_d[channel*numCoefficient + 2];
    double a0 = This->coefficient_d[channel*numCoefficient + 3];
    double a1 = This->coefficient_d[channel*numCoefficient + 4];
    
    This->coefficient_normD[channel*numCoefficient] = b0/a0;//b0
    This->coefficient_normD[channel*numCoefficient + 1] = b1/a0;//b1
    This->coefficient_normD[ channel*numCoefficient + 2] = b2/a0;//b2
    This->coefficient_normD[channel*numCoefficient + 3] = a1/a0;//a1
    This->coefficient_normD[channel*numCoefficient + 4] = 0;//a2
    
}

inline float calculateAlphaAngle(float degree){
    //Confused about pi/2 = 90
    return 1.05 + 0.95* cosf(DEGREES_TO_RADIANS(((degree)/150.) * 180));
}

#pragma mark - Test
void BMBinauralSynthesis_getTFMagVectorData(BMBinauralSynthesis *This,float* freq, float* magnitude, size_t length , size_t level){
    BMMultiLevelBiquad_tfMagVectorAtLevel(&This->filter, freq, magnitude, length, level);
}
