//
//  BMSaturation.c
//  BMAudioFilters
//
//  Created by Hans on 31/7/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#include "BMSaturation.h"
#include <stdlib.h>

    void BMSaturation_processBufferMono(BMSaturation* This,
                                        float* input,
                                        float* output,
                                        size_t numSamples){
        
        // We need unique buffers because this doesn't work in place
        assert(input != output);
        
        
        while (numSamples > 0){
        
            // what is the largest chunk we can process this time through
            // the loop?
            size_t samplesProcessing = MIN(numSamples, BM_BUFFER_CHUNK_SIZE);
            
            /*
             * We need to process through four gainstages.  It's a little messy
             * because they don't process in-place so we have to swap back and forth
             * between the output and the temporary buffer.
             */
            
            // input => gs0 => tBuffer
            BMGainStage_processBufferMono(&This->gainStages[0],
                                          input, This->tBuffer, numSamples);
            // tBuffer => gs1 => output
            BMGainStage_processBufferMono(&This->gainStages[1],
                                          This->tBuffer, output, numSamples);
            
            for(size_t i=2; i<BMSAT_NUMSTAGES;i+=2){
                // output => tBuffer
                BMGainStage_processBufferMono(&This->gainStages[i], output,
                                              This->tBuffer, numSamples);
                // tBuffer => output
                BMGainStage_processBufferMono(&This->gainStages[i+1], output,
                                              This->tBuffer, numSamples);
            }
            
            // update the number of samples remaining to process
            numSamples -= samplesProcessing;
            
            // advance the input and output pointers beyond the part
            // we have finished processing
            input += samplesProcessing;
            output += samplesProcessing;
        }
    }
    
    
    
    
    
    
    void BMSaturation_init(BMSaturation* This, float sampleRate, float AAFilterFc){
        // number of gain stages must be divisible by 2
        assert(BMSAT_NUMSTAGES % 2 == 0);
        
        for(size_t i=0; i<BMSAT_NUMSTAGES; i++){
            BMGainStage_init(&This->gainStages[i], sampleRate, AAFilterFc);
        }
        
        This->tBuffer = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE);
    }
    
    
    
    
    
    
    void BMSaturation_destroy(BMSaturation* This){
        for(size_t i=0; i<BMSAT_NUMSTAGES; i++){
            BMGainStage_destroy(&This->gainStages[i]);
        }
        
        free(This->tBuffer);
    }
    
    
    
    void BMSaturation_setGain(BMSaturation* This, float gain){
        for(size_t i=0; i<BMSAT_NUMSTAGES; i++){
            BMGainStage_setGain(&This->gainStages[i],gain);
        }
    }
    
    
    
    void BMSaturation_setAAFC(BMSaturation* This, float fc){
        for(size_t i=0; i<BMSAT_NUMSTAGES; i++){
            BMGainStage_setAACutoff(&This->gainStages[i],fc);
        }
    }
    
    
    
    void BMSaturation_setSampleRate(BMSaturation* This, float sampleRate ){
        for(size_t i=0; i<BMSAT_NUMSTAGES; i++){
            BMGainStage_setSampleRate(&This->gainStages[i],sampleRate);
        }
    }
    
    
    
    void BMSaturation_setTightness(BMSaturation* This, float tightness ){
        for(size_t i=0; i<BMSAT_NUMSTAGES; i++){
            BMGainStage_setTightness(&This->gainStages[i], tightness);
        }
    }
    
    
#ifdef __cplusplus
}
#endif
