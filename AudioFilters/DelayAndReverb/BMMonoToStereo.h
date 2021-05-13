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
    BMCrossover3way crossover;
    BMVelvetNoiseDecorrelator vnd;
	float *lowL, *lowR, *midL, *midR, *highL, *highR;
	bool stereoInput;
} BMMonoToStereo;


/*!
 *BMMonoToStereo_init
 */
void BMMonoToStereo_init(BMMonoToStereo *This, float sampleRate, bool stereoInput);

/*!
 *BMMonoToStereo_initBigger
 */
void BMMonoToStereo_initBigger(BMMonoToStereo *This, float sampleRate, bool stereoInput);

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



/*!
 *BMMonoToStereo_setWetMix
 *
 * @param This pointer to an initialised struct
 * @param wetMix01 linear scale mix in [0,1]
 */
void BMMonoToStereo_setWetMix(BMMonoToStereo *This, float wetMix01);


#endif /* BMMonoToStereo_h */
