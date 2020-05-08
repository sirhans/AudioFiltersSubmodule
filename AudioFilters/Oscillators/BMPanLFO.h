//
//  BMPanLFO.h
//  AUCloudReverb
//
//  Created by Nguyen Minh Tien on 4/9/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMPanLFO_h
#define BMPanLFO_h

#include <stdio.h>
#include "BMQuadratureOscillator.h"

typedef struct BMPanLFO {
    BMQuadratureOscillator oscil;
    float depth;
    float base;
    float* tempL;
    float* tempR;
} BMPanLFO;

/*!
 *BMPanLFO_init
 */
void BMPanLFO_init(BMPanLFO *This,
                   float fHz,
                   float depth,
                   float sampleRate);

void BMPanLFO_destroy(BMPanLFO *This);

void BMPanLFO_process(BMPanLFO *This,
                      float* outL,
                      float* outR,
                      size_t numSamples);

void BMPanLFO_setDepth(BMPanLFO *This,float depth);

#endif /* BMPanLFO_h */
