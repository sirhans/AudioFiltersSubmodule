//
//  BMMultiLevelSVF.c
//  BMAudioFilters
//
//  Created by Nguyen Minh Tien on 1/9/19.
//  Copyright © 2019 Hans. All rights reserved.
//

#include "BMMultiLevelSVF.h"
#include <stdlib.h>
#include <assert.h>
#include "Constants.h"

#define SVF_Param_Count 3

static inline void BMMultiLevelSVF_processBufferAtLevel(BMMultiLevelSVF *This,
                                                        int level,int channel,
                                                        const float* input,
                                                        float* output,
                                                        size_t numSamples);
static inline void BMMultiLevelSVF_updateSVFParam(BMMultiLevelSVF *This);

void BMMultiLevelSVF_init(BMMultiLevelSVF *This,int numLevels,float sampleRate,
                          bool isStereo){
    This->sampleRate = sampleRate;
    This->numChannels = isStereo? 2 : 1;
    This->numLevels = numLevels;
    //If stereo -> we need totalnumlevel = numlevel *2
    int totalNumLevels = numLevels  *This->numChannels;
    This->a = (float**)malloc(sizeof(float*) * numLevels);
    This->m = (float**)malloc(sizeof(float*) * numLevels);
    This->tempA = (float**)malloc(sizeof(float*) * numLevels);
    This->tempM = (float**)malloc(sizeof(float*) * numLevels);
    for(int i=0;i<numLevels;i++){
        This->a[i] = malloc(sizeof(float) * SVF_Param_Count);
        This->m[i] = malloc(sizeof(float) * SVF_Param_Count);
        This->tempA[i] = malloc(sizeof(float) * SVF_Param_Count);
        This->tempM[i] = malloc(sizeof(float) * SVF_Param_Count);
    }
    
    This->ic1eq = malloc(sizeof(float)* totalNumLevels);
    This->ic2eq = malloc(sizeof(float)* totalNumLevels);
    for(int i=0;i<totalNumLevels;i++){
        This->ic1eq[i] = 0;
        This->ic2eq[i] = 0;
    }
}



void BMMultiLevelSVF_free(BMMultiLevelSVF *This){
    
    for(int i=0;i<This->numLevels;i++){
        free(This->a[i]);
        This->a[i] = NULL;
//        This->a[i] = malloc(sizeof(float) * SVF_Param_Count);
        
        free(This->m[i]);
        This->m[i] = NULL;
//        This->m[i] = malloc(sizeof(float) * SVF_Param_Count);
        
        free(This->tempA[i]);
        This->tempA[i] = NULL;
//        This->tempA[i] = malloc(sizeof(float) * SVF_Param_Count);
        
        free(This->tempM[i]);
        This->tempM[i] = NULL;
//        This->tempM[i] = malloc(sizeof(float) * SVF_Param_Count);
    }
    
    free(This->a);
    This->a = NULL;
    //This->a = (float**)malloc(sizeof(float*) * totalNumLevels);
    
    free(This->m);
    This->m = NULL;
//    This->m = (float**)malloc(sizeof(float*) * totalNumLevels);
    
    free(This->tempA);
    This->tempA = NULL;
//    This->tempA = (float**)malloc(sizeof(float*) * totalNumLevels);
    
    free(This->tempM);
    This->tempM = NULL;
//    This->tempM = (float**)malloc(sizeof(float*) * totalNumLevels);
    
    free(This->ic1eq);
    free(This->ic2eq);
    This->ic1eq = NULL;
    This->ic2eq = NULL;
//    This->ic1eq = malloc(sizeof(float)* totalNumLevels);
//    This->ic2eq = malloc(sizeof(float)* totalNumLevels);
    
}



#pragma mark - process
void BMMultiLevelSVF_processBufferMono(BMMultiLevelSVF *This,
                                       const float* input,
                                       float* output,
                                       size_t numSamples){
    assert(This->numChannels == 1);
    
    if(This->shouldUpdateParam)
        BMMultiLevelSVF_updateSVFParam(This);
    
    for(int level = 0;level<This->numLevels;level++){
        if(level==0){
            //Process into output
            BMMultiLevelSVF_processBufferAtLevel(This, level,0, input, output, numSamples);
        }else
            BMMultiLevelSVF_processBufferAtLevel(This, level,0, output, output, numSamples);
    }
}

void BMMultiLevelSVF_processBufferStereo(BMMultiLevelSVF *This,
                                         const float* inputL, const float* inputR,
                                         float* outputL, float* outputR, size_t numSamples){
    assert(This->numChannels == 2);
    
    if(This->shouldUpdateParam)
        BMMultiLevelSVF_updateSVFParam(This);
    
    for(int level = 0;level<This->numLevels;level++){
        if(level==0){
            //Process into output
            //Left channel
            BMMultiLevelSVF_processBufferAtLevel(This, level,0, inputL, outputL, numSamples);
            //Right channel
            BMMultiLevelSVF_processBufferAtLevel(This,level,1, inputR, outputR, numSamples);
            // + This->numLevels
        }else{
            //Left channel
            BMMultiLevelSVF_processBufferAtLevel(This,level,0, outputL, outputL, numSamples);
            //Right channel
            BMMultiLevelSVF_processBufferAtLevel(This,level,1, outputR,outputR, numSamples);
            // + This->numLevels
        }
    }
}

inline void BMMultiLevelSVF_processBufferAtLevel(BMMultiLevelSVF *This,
                                                 int level,int channel,
                                                 const float* input,
                                                 float* output,size_t numSamples){
    int icLvl = This->numLevels*channel + level;
    for(int i=0;i<numSamples;i++){
        float v3 = input[i] - This->ic2eq[icLvl];
        float v1 = This->a[level][0]*This->ic1eq[icLvl] + This->a[level][1]*v3;
        float v2 = This->ic2eq[icLvl] + This->a[level][1]*This->ic1eq[icLvl] + This->a[level][2] * v3;
        This->ic1eq[icLvl] = 2*v1 - This->ic1eq[icLvl];
        This->ic2eq[icLvl] = 2*v2 - This->ic2eq[icLvl];
        output[i] = This->m[level][0] * input[i] + This->m[level][1] * v1 + This->m[level][2]*v2;
    }
}

inline void BMMultiLevelSVF_updateSVFParam(BMMultiLevelSVF *This){
    if(This->shouldUpdateParam){
        This->shouldUpdateParam = false;
        //Update all param
        for(int i=0;i<This->numLevels;i++){
            for(int j=0;j<SVF_Param_Count;j++){
                This->a[i][j] = This->tempA[i][j];
                This->m[i][j] = This->tempM[i][j];
            }
            
        }
    }
}

#pragma mark - Filters
void BMMultiLevelSVF_setLowpass(BMMultiLevelSVF *This, double fc, size_t level){
    BMMultiLevelSVF_setLowpassQ(This, fc, 1./sqrtf(2.), level);
}

void BMMultiLevelSVF_setLowpassQ(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->tempA[level][0] = 1/(1 + g*(g+k));
    This->tempA[level][1] = g*This->tempA[level][0];
    This->tempA[level][2] = g*This->tempA[level][1];
    This->tempM[level][0] = 0;
    This->tempM[level][1] = 0;
    This->tempM[level][2] = 1;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setBandpass(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->tempA[level][0] = 1/(1 + g*(g+k));
    This->tempA[level][1] = g*This->tempA[level][0];
    This->tempA[level][2] = g*This->tempA[level][1];
    This->tempM[level][0] = 0;
    This->tempM[level][1] = 1;
    This->tempM[level][2] = 0;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setHighpass(BMMultiLevelSVF *This, double fc, size_t level){
    BMMultiLevelSVF_setHighpassQ(This, fc, 1./sqrtf(2.), level);
}

void BMMultiLevelSVF_setHighpassQ(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->tempA[level][0] = 1/(1 + g*(g+k));
    This->tempA[level][1] = g*This->tempA[level][0];
    This->tempA[level][2] = g*This->tempA[level][1];
    This->tempM[level][0] = 1;
    This->tempM[level][1] = -k;
    This->tempM[level][2] = -1;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setNotchpass(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->tempA[level][0] = 1/(1 + g*(g+k));
    This->tempA[level][1] = g*This->tempA[level][0];
    This->tempA[level][2] = g*This->tempA[level][1];
    This->tempM[level][0] = 1;
    This->tempM[level][1] = -k;
    This->tempM[level][2] = 0;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setPeak(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->tempA[level][0] = 1/(1 + g*(g+k));
    This->tempA[level][1] = g*This->tempA[level][0];
    This->tempA[level][2] = g*This->tempA[level][1];
    This->tempM[level][0] = 1;
    This->tempM[level][1] = -k;
    This->tempM[level][2] = -2;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setAllpass(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->tempA[level][0] = 1/(1 + g*(g+k));
    This->tempA[level][1] = g*This->tempA[level][0];
    This->tempA[level][2] = g*This->tempA[level][1];
    This->tempM[level][0] = 1;
    This->tempM[level][1] = -2*k;
    This->tempM[level][2] = 0;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setBell(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level){
    assert(level < This->numLevels);
    
    float A = powf(10, gain/40.);
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/(q*A);
    This->tempA[level][0] = 1/(1 + g*(g+k));
    This->tempA[level][1] = g*This->tempA[level][0];
    This->tempA[level][2] = g*This->tempA[level][1];
    This->tempM[level][0] = 1;
    This->tempM[level][1] = k*(A*A -1);
    This->tempM[level][2] = 0;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setLowShelf(BMMultiLevelSVF *This, double fc, double gain, size_t level){
    BMMultiLevelSVF_setLowShelfQ(This, fc, gain, 1./sqrtf(2.), level);
}

void BMMultiLevelSVF_setLowShelfQ(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level){
    assert(level < This->numLevels);
    
    float A = powf(10, gain/40.);
    float g = tanf(M_PI*fc/This->sampleRate)/sqrtf(A);
    float k = 1/q;
    This->tempA[level][0] = 1/(1 + g*(g+k));
    This->tempA[level][1] = g*This->tempA[level][0];
    This->tempA[level][2] = g*This->tempA[level][1];
    This->tempM[level][0] = 1;
    This->tempM[level][1] = k*(A -1);
    This->tempM[level][2] = A*A - 1;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setHighShelf(BMMultiLevelSVF *This, double fc, double gain, size_t level){
    BMMultiLevelSVF_setHighShelfQ(This, fc, gain, 1./sqrtf(2.), level);
}

void BMMultiLevelSVF_setHighShelfQ(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level){
    assert(level < This->numLevels);
    
    float A = powf(10, gain/40.);
    float g = tanf(M_PI*fc/This->sampleRate)*sqrtf(A);
    float k = 1/q;
    This->tempA[level][0] = 1/(1 + g*(g+k));
    This->tempA[level][1] = g*This->tempA[level][0];
    This->tempA[level][2] = g*This->tempA[level][1];
    This->tempM[level][0] = A*A;
    This->tempM[level][1] = k*(1 - A)*A;
    This->tempM[level][2] = 1 - A*A;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_impulseResponse(BMMultiLevelSVF *This,size_t frameCount){
    float* irBuffer = malloc(sizeof(float)*frameCount);
    float* irBufferR = malloc(sizeof(float)*frameCount);
    float* outBuffer = malloc(sizeof(float)*frameCount);
    float* outBufferR = malloc(sizeof(float)*frameCount);
    for(int i=0;i<frameCount;i++){
        if(i==0){
            irBuffer[i] = 1;
            irBufferR[i] = 1;
        }else{
            irBuffer[i] = 0;
            irBufferR[i] = 0;
        }
    }
    
    BMMultiLevelSVF_processBufferStereo(This, irBuffer, irBufferR, outBuffer, outBufferR, frameCount);
    
    printf("\[");
    for(size_t i=0; i<(frameCount-1); i++)
        printf("%f\n", outBufferR[i]);
    printf("%f],",outBufferR[frameCount-1]);
    
}

