//
//  BMHarmonicityMeasure.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 11/3/20.
//  This file is public domain. No restrictions.
//

#ifndef BMHarmonicityMeasure_h
#define BMHarmonicityMeasure_h

#include <stdio.h>
#include "BMCepstrum.h"
#include "BMSFM.h"

typedef struct BMHarmonicityMeasure {
    BMCepstrum cepstrum;
    BMSFM sfm;
    float* input;
    float* output;
    size_t length;
    float sampleRate;
} BMHarmonicityMeasure;


/*!
 *BMHarmonicityMeasure_init
 */
void BMHarmonicityMeasure_init(BMHarmonicityMeasure *This, size_t bufferLength, float sampleRate);

/*!
 *BMHarmonicityMeasure_free
 */
void BMHarmonicityMeasure_free(BMHarmonicityMeasure *This);

/*!
*BMHarmonicityMeasure_processStereoBuffer
*/
float BMHarmonicityMeasure_processStereoBuffer(BMHarmonicityMeasure *This,float* inputL,float* inputR,size_t length);

float BMHarmonicityMeasure_processMonoBuffer(BMHarmonicityMeasure *This,float* input,size_t length);

#endif /* BMHarmonicityMeasure_h */
