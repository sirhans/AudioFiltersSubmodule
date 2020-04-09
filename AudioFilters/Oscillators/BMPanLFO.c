//
//  BMPanLFO.c
//  AUCloudReverb
//
//  Created by Nguyen Minh Tien on 4/9/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMPanLFO.h"
#include <Accelerate/Accelerate.h>
#include "Constants.h"

void BMPanLFO_init(BMPanLFO *This,
                   float fHz,float depth,
                   float sampleRate){
    BMQuadratureOscillator_init(&This->oscil, fHz, sampleRate);
    This->depth = depth;
    This->base = 1.0f - depth;
    This->tempL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->tempR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
}

void BMPanLFO_destroy(BMPanLFO *This){
    free(This->tempL);
    This->tempL = NULL;
    
    free(This->tempR);
    This->tempR = NULL;
}


void BMPanLFO_process(BMPanLFO *This,
                      float* outL,
                      float* outR,
                      size_t numSamples){
    assert(numSamples<=BM_BUFFER_CHUNK_SIZE);
    
    BMQuadratureOscillator_process(&This->oscil, outL, outR, numSamples);
    //Get sign
    vDSP_vabs(outL, 1, outL, 1, numSamples);
    vDSP_vabs(outR, 1, outR, 1, numSamples);
    //Mul value to depth + base to sign
    vDSP_vsmsa(outL, 1, &This->depth, &This->base, outL, 1, numSamples);
    vDSP_vsmsa(outR, 1, &This->depth, &This->base, outR, 1, numSamples);
    
//    for(int i=0;i<numSamples;i++){
//
//        printf("%f %f\n",outL[i],outR[i]);
//    }
    
}
