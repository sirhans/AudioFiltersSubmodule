//
//  BMTherapyFilter.h
//  BMAudioTherapy
//
//  Created by Nguyen Minh Tien on 07/04/2022.
//

#ifndef BMTherapyFilter_h
#define BMTherapyFilter_h

#include <stdio.h>
#include "BMMultiLevelBiquad.h"
#include "BMQuadratureOscillator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BMTherapyFilter{
    BMMultiLevelBiquad biquad;
    BMQuadratureOscillator lfo;
    float* lfoBuffer;
    
}BMTherapyFilter;

void BMTherapyFilter_init(BMTherapyFilter* This,bool stereo,float sampleRate);

void BMTherapyFilter_processBufferStereo(BMTherapyFilter* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t length);
void BMTherapyFilter_processBufferMono(BMTherapyFilter* This,float* input,float* output,size_t length);

#ifdef __cplusplus
}
#endif

#endif /* BMTherapyFilter_h */
