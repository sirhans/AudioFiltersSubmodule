//
//  BMVocalRemover.h
//  AudioFiltersXcodeProject
//
//  This class strips lead vocals and anything else on the centre channel, but
//  leaves the bass frequencies intact.
//
//  Created by hans anderson on 10/2/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMVocalRemover_h
#define BMVocalRemover_h

#include <stdio.h>
#include "BMCrossover.h"
#include "Constants.h"

typedef struct BMVocalRemover {
    BMCrossover crossover;
    float highLeft [BM_BUFFER_CHUNK_SIZE];
    float highRight [BM_BUFFER_CHUNK_SIZE];
    float lowLeft [BM_BUFFER_CHUNK_SIZE];
    float lowRight [BM_BUFFER_CHUNK_SIZE];
} BMVocalRemover;


/*!
 *BMVocalRemover_init
 */
void BMVocalRemover_init(BMVocalRemover *This, float sampleRate);


/*!
 *BMVocalRemover_free
 */
void BMVocalRemover_free(BMVocalRemover *This);



/*!
*BMVocalRemover_process
*/
void BMVocalRemover_process(BMVocalRemover *This,
                             const float *inL, const float *inR,
                             float *outL, float *outR,
                             size_t numSamples);



#endif /* BMVocalRemover_h */
