//
//  BMMultiLevelSVF.h
//  BMAudioFilters
//
//  Created by Nguyen Minh Tien on 1/9/19.
//  Copyright © 2019 Hans. All rights reserved.
//

#ifndef BMMultiLevelSVF_h
#define BMMultiLevelSVF_h

#include <stdio.h>
#include <Accelerate/Accelerate.h>

typedef struct BMMultiLevelSVF{
    float** a;
    float** m;
    float** tempA;
    float** tempM;
    float* ic1eq;
    float* ic2eq;
    int numLevels;
    int numChannels;
    double sampleRate;
    bool shouldUpdateParam;
}BMMultiLevelSVF;

/*init function
 */
void BMMultiLevelSVF_init(BMMultiLevelSVF *This,int numLevels,float sampleRate,
                          bool isStereo);

void BMMultiLevelSVF_free(BMMultiLevelSVF *This);


/*
 Process buffer
 */
void BMMultiLevelSVF_processBufferMono(BMMultiLevelSVF *This, const float* input, float* output, size_t numSamples);
void BMMultiLevelSVF_processBufferStereo(BMMultiLevelSVF *This, const float* inputL,const float* inputR, float* outputL, float* outputR, size_t numSamples);

/*
 Filters
 */
/* Lowpass */
void BMMultiLevelSVF_setLowpass(BMMultiLevelSVF *This, double fc, size_t level);
void BMMultiLevelSVF_setLowpassQ(BMMultiLevelSVF *This, double fc, double q, size_t level);

/* Bandpass */
void BMMultiLevelSVF_setBandpass(BMMultiLevelSVF *This, double fc, double q, size_t level);

/* Highpass */
void BMMultiLevelSVF_setHighpass(BMMultiLevelSVF *This, double fc, size_t level);
void BMMultiLevelSVF_setHighpassQ(BMMultiLevelSVF *This, double fc, double q, size_t level);

/* Notchpass / BandStop filter */
void BMMultiLevelSVF_setNotchpass(BMMultiLevelSVF *This, double fc, double q, size_t level);

/* Unknown */
void BMMultiLevelSVF_setPeak(BMMultiLevelSVF *This, double fc, double q, size_t level);

/* Allpass */
void BMMultiLevelSVF_setAllpass(BMMultiLevelSVF *This, double fc, double q, size_t level);

/* Bell */
void BMMultiLevelSVF_setBell(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level);

/* Lowshelf */
void BMMultiLevelSVF_setLowShelf(BMMultiLevelSVF *This, double fc, double gain, size_t level);
void BMMultiLevelSVF_setLowShelfQ(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level);

/* Highshelf */
void BMMultiLevelSVF_setHighShelf(BMMultiLevelSVF *This, double fc, double gain, size_t level);
void BMMultiLevelSVF_setHighShelfQ(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level);

/*impulse response*/
void BMMultiLevelSVF_impulseResponse(BMMultiLevelSVF *This,size_t frameCount);

#endif /* BMMultiLevelSVF_h */
