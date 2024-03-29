//
//  BMTransientShaper.h
//  BMAUTransientShaper
//
//  Created by Nguyen Minh Tien on 7/9/21.
//

#ifndef BMTransientShaper_h
#define BMTransientShaper_h

#include <stdio.h>
#include "Constants.h"
#include "BMEnvelopeFollower.h"
#include "BMMultiLevelBiquad.h"
#include "BMDynamicSmoothingFilter.h"
#include "BMShortSimpleDelay.h"
#include "BMQuadraticLimiter.h"
#include "BMCrossover.h"
#include "BMQuadraticThreshold.h"


#define BMTS_DELAY_AT_48KHZ_SAMPLES 30
#define BMTS_AF_NUMLEVELS 1
#define BMTS_ARF_NUMLEVELS 1
#define BMTS_RRF1_NUMLEVELS 1
#define BMTS_RRF2_NUMLEVELS 1
#define BMTS_DSF_NUMLEVELS 1
#define BMTS_NUM_SECTIONS 2
#define BMTS_SECTION_2_AF_MULTIPLIER 1.25f
#define BMTS_SECTION_2_RF_MULTIPLIER 2.0f
#define BMTS_SECTION_2_EXAGGERATION_MULTIPLIER 1.1f
#define BMTS_NOISE_GATE_CLOSED_LEVEL -100.0f

#define BMTS_BAND_1_FC 300.0f
#define BMTS_BAND_2_FC 750.0f
#define BMTS_BAND_3_FC 1600.0f

typedef struct BMTransientShaperSection {
    float* b1;
    float* b2;
    float* attackControlSignal;
    float* releaseControlSignal;
    BMMultiReleaseFilter attackReduceInstantFilter;
    BMMultiAttackFilter attackReduceSlowFilter;
    BMMultiReleaseFilter attackBoostInstantFilter;
    BMMultiAttackFilter attackBoostSlowFilter;
    BMAttackFilter attackBoostSmoothFilter;
    
    BMMultiReleaseFilter sustainFastReleaseFilter;
    BMMultiReleaseFilter sustainSlowReleaseFilter;

    BMMultiReleaseFilter sustainSmoothReleaseFilter;
    BMMultiAttackFilter sustainSmoothAttackFilter;
    
    BMMultiReleaseFilter sustainStandardReleaseFilter;
    BMMultiAttackFilter sustainStandardAttackFilter;
    
    BMMultiReleaseFilter sustainInputReleaseFilter;
    //Delay smooth
    BMMultiAttackFilter sustainDelayAttackFilter;
    BMDynamicSmoothingFilter* sustainDSFFilter;
    
    float* standard;
    
    BMDynamicSmoothingFilter* dsfAttack;
    BMDynamicSmoothingFilter* dsfSustain;
    
    
    BMShortSimpleDelay dly;
    size_t delaySamples;
    float attackExaggeration,sustainExaggeration, attackDepth, releaseDepth, sampleRate, noiseGateThreshold;
    bool isStereo;
    bool isTesting;
    bool noiseGateIsOpen;
    float* testBuffer1;
    float* testBuffer2;
    float* testBuffer3;
    float releaseSamples;
    float currentReleaseSample;
    float lastOutput;
    
    //Limiter
    BMQuadraticThreshold lowerLimiter;
} BMTransientShaperSection;


typedef struct BMTransientShaper {
    BMTransientShaperSection* asSections;// [BMTS_NUM_SECTIONS];
    BMCrossover crossover2;
    float *b1L, *b2L, *b1R, *b2R;
    float* inputBuffer;
    bool isStereo;
    bool isInit;
} BMTransientShaper;

void BMTransientShaper_init(BMTransientShaper *This, bool isStereo, float sampleRate,bool isTesting);
void BMTransientShaper_free(BMTransientShaper *This);

void BMTransientShaper_processStereo(BMTransientShaper *This,
                                           const float *inputL, const float *inputR,
                                           float *outputL, float *outputR,
                                     size_t numSamples);
void BMTransientShaper_processMono(BMTransientShaper *This,
                                         const float *input,
                                         float *output,
                                   size_t numSamples);

void BMTransientShaper_processStereoTest(BMTransientShaper *This,
                                           const float *inputL, const float *inputR,
                                           float *outputL, float *outputR,float* outCS1,float* outCS2,float* outCS3,
                                         size_t numSamples);

void BMTransientShaper_setAttackTime(BMTransientShaper *This, float attackTimeInSeconds);
void BMTransientShaper_setAttackDepth(BMTransientShaper *This, float depth);
void BMTransientShaper_setReleaseTime(BMTransientShaper *This, float releaseTimeInSeconds);
void BMTransientShaper_setReleaseDepth(BMTransientShaper *This, float depth);

#endif /* BMTransientShaper_h */
