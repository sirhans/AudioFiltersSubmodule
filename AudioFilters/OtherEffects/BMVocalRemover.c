//
//  BMVocalRemover.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/2/19.
//  Anyone may use this file without restrictions of any kind
//

#include "BMVocalRemover.h"

#define BM_VOCAL_REMOVER_CROSSOVER_FC 300.0f
#define BM_VOCAL_REMOVER_FOURTH_ORDER_CROSSOVER true

/*!
 *BMVocalRemover_init
 */
void BMVocalRemover_init(BMVocalRemover *This, float sampleRate){
    BMCrossover_init(&This->crossover,
                     BM_VOCAL_REMOVER_CROSSOVER_FC,
                     sampleRate,
                     BM_VOCAL_REMOVER_FOURTH_ORDER_CROSSOVER,
                     true);
}


/*!
 *BMVocalRemover_free
 */
void BMVocalRemover_free(BMVocalRemover *This){
    BMCrossover_destroy(&This->crossover);
}



/*!
*BMVocalRevmover_process
*/
void BMVocalRevmover_process(BMVocalRemover *This,
                             const float *inL, const float *inR,
                             float *outL, float *outR,
                             size_t numSamples){
    
    while(numSamples > 0){
        // limit the number of samples to the size of the buffers
        size_t samplesProcessing = BM_MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
        
        // apply a crossover to separate high and low frequencies
        BMCrossover_processStereo(&This->crossover, inL, inR, This->lowLeft, This->lowRight, This->highLeft, This->highRight, samplesProcessing);
        
        // subtract the left and right channels of the high frequencies to
        // remove the audio in the centre channel
        float* highNoVoice = This->highLeft;
        vDSP_vsub(This->highLeft, 1, This->highRight, 1, highNoVoice, 1, samplesProcessing);
        
        // add the bass from the left channel to the voice-removed signal
        vDSP_vadd(This->lowLeft, 1, highNoVoice, 1, outL, 1, samplesProcessing);
        
        // copy from the left channel output to the right channel
        memcpy(outR, outL, sizeof(float)*samplesProcessing);
        
        // advance pointers for the next chunk of audio
        numSamples -= samplesProcessing;
        inL += samplesProcessing;
        inR += samplesProcessing;
        outL += samplesProcessing;
        outR += samplesProcessing;
    }
}


