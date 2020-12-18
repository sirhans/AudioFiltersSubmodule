//
//  BMReleaseShaper.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 11/12/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMReleaseShaper.h"
#include <Accelerate/Accelerate.h>
#include "Constants.h"
#include "BMUnitConversion.h"

void BMReleaseShaper_processStereo(BMReleaseShaper *This,
                                   const float *inputL, const float *inputR,
                                   float *outputL, float *outputR, size_t length){
    // chunked processing
    size_t i=0;
    size_t numSamplesRemaining = length;
    while(numSamplesRemaining > 0){
        size_t samplesProcessing = BM_MIN(numSamplesRemaining, length);
        
        // mix to mono
        float half = 0.5f;
        vDSP_vasm(inputL + i, 1, inputR + i, 1, &half, This->b1, 1, samplesProcessing);
        
        // rectify
        vDSP_vabs(This->b1, 1, This->b1, 1, samplesProcessing);
        
        // threshold to remove zeros
        float minValue = BM_DB_TO_GAIN(-100.0f);
        vDSP_vthr(This->b1, 1, &minValue, This->b1, 1, samplesProcessing);
        
        // dB conversion
        float zeroDb = 1.0f;
        bool use20dB = true;
        vDSP_vdbcon(This->b1, 1, &zeroDb, This->b1, 1, samplesProcessing, use20dB);
        
        // release filter to get fast decay envelope -> buffer1
        for(size_t i=0; i<BMRS_NUM_RELEASE_FILTERS; i++)
        BMReleaseFilter_processBuffer(&This->fastRelease[i], This->b1, This->b1, samplesProcessing);
        
        // release filter again to get slow decay envelope -> buffer2
        BMReleaseFilter_processBuffer(&This->slowRelease, This->b1, This->b2, samplesProcessing);
        
        // subtract buffer2 - buffer1 -> releaseDetection
        vDSP_vsub(This->b1, 1, This->b2, 1, This->b1, 1, samplesProcessing);
        
        // multiply by release control value
        vDSP_vsmul(This->b1, 1, &This->releaseAmount, This->b1, 1, samplesProcessing);
        
        // dB to gain -> control signal buffer
        BMConv_dBToGainV(This->b1, This->b1, samplesProcessing);
        
        // multiply control signal * input
        vDSP_vmul(This->b1, 1, inputL + i, 1, outputL + i, 1, samplesProcessing);
        vDSP_vmul(This->b1, 1, inputR + i, 1, outputR + i, 1, samplesProcessing);
        
        // advance index
        i += samplesProcessing;
        numSamplesRemaining -= samplesProcessing;
    }
}
