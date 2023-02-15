//
//  BMSmoothFade.h
//  AudioMontage
//
//  Created by Nguyen Minh Tien on 10/1/20.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMSmoothFade_h
#define BMSmoothFade_h

#include <stdio.h>

typedef enum FadeType{
    FT_In,FT_Out,FT_Stop
}FadeType;

typedef struct BMSmoothFade{
    FadeType fadeType;
    size_t fadeLength;
    size_t fadeIdx;
    float lastVol;
    
    float* fadeBuffer;
}BMSmoothFade;

void BMSmoothFade_init(BMSmoothFade* This,size_t fadeLength);
void BMSmoothFade_free(BMSmoothFade* This);
void BMSmoothFade_processBufferMono(BMSmoothFade* This, float* inData,float* outData,size_t frameCount);
void BMSmoothFade_processBufferStereo(BMSmoothFade* This, float* inL,float* inR,float* outL,float* outR,size_t frameCount);
void BMSmoothFade_startFading(BMSmoothFade* This,FadeType type);
#endif /* BMSmoothFade_h */

#ifdef __cplusplus
}
#endif
