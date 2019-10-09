//
//  BMMonoToStereo.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/9/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMMonoToStereo_h
#define BMMonoToStereo_h

#include <stdio.h>
#include "BMVelvetNoiseDecorrelator.h"
#include "BMCrossover.h"


typedef struct BMMonoToStereo {
	BMCrossover lowCrossover, highCrossover;
    BMVelvetNoiseDecorrelator vnd;
	float *b1L, *b1R, *b2L, *b2R, *b3L, *b3R;
} BMMonoToStereo;


/*!
 *BMMonoToStereo_init
 */
void BMMonoToStereo_init(BMMonoToStereo *This, float sampleRate);


/*!
 *BMMonoToStereo_processBuffer
 */
void BMMonoToStereo_processBuffer(BMMonoToStereo *This,
								  const float* inputL, const float* inputR,
								  float* outputL, float* outputR,
								  size_t numSamples);

/*!
 *BMMonoToStereo_free
 */
void BMMonoToStereo_free(BMMonoToStereo *This);


#endif /* BMMonoToStereo_h */
