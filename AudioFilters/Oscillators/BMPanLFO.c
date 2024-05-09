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
                   float sampleRate, bool randomStart){
	// the oscillator frequency is half the output frequency because the square
	// rectifier in the process function doubles the frequency
	float oscilatorFrequency = fHz * 0.5f;
	
	BMQuadratureOscillator_init(&This->oscil, oscilatorFrequency, sampleRate);
    This->depth = depth;
    This->base = 1.0f - depth;
    This->tempL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->tempR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    if(randomStart){
        //Randomize starting phase
        int cycleRange = sampleRate/fHz;
        float randomStart = arc4random()%cycleRange;
        float output;
        for(int i=0;i<randomStart;i++){
            BMQuadratureOscillator_process(&This->oscil, &output, &output, 1);
        }
    }
}

void BMPanLFO_destroy(BMPanLFO *This){
    free(This->tempL);
    This->tempL = NULL;
    
    free(This->tempR);
    This->tempR = NULL;
}

void BMPanLFO_setDepth(BMPanLFO *This,float depth){
    This->depth = depth;
    This->base = 1.0f - depth * 0.5f;
}


void BMPanLFO_process(BMPanLFO *This,
                      float* outL,
                      float* outR,
                      size_t numSamples){
    assert(numSamples<=BM_BUFFER_CHUNK_SIZE);
    
    BMQuadratureOscillator_process(&This->oscil, outL, outR, numSamples);
    
    //Remove sign
    vDSP_vsq(outL, 1, outL, 1, numSamples);
    vDSP_vsq(outR, 1, outR, 1, numSamples);
    
    //Mul value to depth + base to sign
    vDSP_vsmsa(outL, 1, &This->depth, &This->base, outL, 1, numSamples);
    vDSP_vsmsa(outR, 1, &This->depth, &This->base, outR, 1, numSamples);
}
