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


typedef struct BMHarmonicityMeasure {
    BMCepstrum cepstrum;
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




#endif /* BMHarmonicityMeasure_h */
