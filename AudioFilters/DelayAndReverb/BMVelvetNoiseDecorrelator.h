//
//  BMVelvetNoiseDecorrelator.h
//  Saturator
//
//  Created by TienNM on 12/4/17
//  Rewritten by Hans on 9 October 2019
//  Anyone may use this file without restrictions
//

#ifndef BMVelvetNoiseDecorrelator_h
#define BMVelvetNoiseDecorrelator_h

#include <stdio.h>
#include "BMMultiTapDelay.h"


typedef struct BMVelvetNoiseDecorrelator {
    BMMultiTapDelay multiTapDelay;
    float sampleRate;
} BMVelvetNoiseDecorrelator;




/*!
 *BMVelvetNoiseDecorrelator_init
 */
void BMVelvetNoiseDecorrelator_init(BMVelvetNoiseDecorrelator* This,
                                    float maxDelaySeconds,
                                    size_t maxTapsPerChannel,
                                    float rt60DecayTimeSeconds,
									float wetMix,
                                    float sampleRate);




/*!
 *BMVelvetNoiseDecorrelator_free
 */
void BMVelvetNoiseDecorrelator_free(BMVelvetNoiseDecorrelator* This);





/*!
 *BMVelvetNoiseDecorrelator_update
 */
void BMVelvetNoiseDecorrelator_update(BMVelvetNoiseDecorrelator* This,
									  float diffusionSeconds,
									  size_t tapsPerChannel,
									  float rt60DecayTimeSeconds,
									  float wetMix);



/*!
 *BMVelvetNoiseDecorrelator_processBufferStereo
 */
void BMVelvetNoiseDecorrelator_processBufferStereo(BMVelvetNoiseDecorrelator* This,
                                                   float* inputL,
                                                   float* inputR,
                                                   float* outputL,
                                                   float* outputR,
                                                   size_t length);


#endif /* BMVelvetNoiseDecorrelator_h */
