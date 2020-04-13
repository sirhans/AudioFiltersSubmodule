//
//  BMAllpassNestedFilter.c
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 4/10/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMAllpassNestedFilter.h"
#include "Constants.h"
#include <Accelerate/Accelerate.h>

void BMAllpassFilterData_init(BMAllpassFilterData* This,size_t delaySamples,float dc1, float dc2);
void BMAllpassFilterData_destroy(BMAllpassFilterData* This);

void SetupCircularBuffer(TPCircularBuffer* buffer,size_t delaySamples){
    //Init delay buffer
    TPCircularBufferInit(buffer, (uint32_t)(delaySamples + BM_BUFFER_CHUNK_SIZE)*sizeof(float));
    TPCircularBufferClear(buffer);
    TPCircularBufferProduce(buffer, (uint32_t)delaySamples*sizeof(float));
    //Clear buffer
    uint32_t availableByte;
    void* tail = TPCircularBufferTail(buffer, &availableByte);
    memset(tail, 0, availableByte);
}

void BMAllpassNestedFilter_init(BMAllpassNestedFilter* This,size_t delaySamples,float dc1, float dc2,size_t nestedLevel){
    This->numLevel = nestedLevel + 1;
    This->nestedData = malloc(sizeof(BMAllpassFilterData)*This->numLevel);
    for(int i=0;i<This->numLevel;i++){
        BMAllpassFilterData_init(&This->nestedData[i], delaySamples, dc1, dc2);
    }
}

void BMAllpassNestedFilter_destroy(BMAllpassNestedFilter* This){
    for(int i=0;i<This->numLevel;i++){
        BMAllpassFilterData_destroy(&This->nestedData[i]);
    }
}

void BMAllpassFilterData_init(BMAllpassFilterData* This,size_t delaySamples,float dc1, float dc2){
    This->delaySamples = delaySamples;
    This->decay1 = dc1;
    This->decay2 = dc2;
    
    SetupCircularBuffer(&This->circularBufferL, delaySamples);
    SetupCircularBuffer(&This->circularBufferR, delaySamples);
    
    This->tempL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->tempR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
}

void BMAllpassFilterData_destroy(BMAllpassFilterData* This){
    TPCircularBufferCleanup(&This->circularBufferL);
    TPCircularBufferCleanup(&This->circularBufferR);
    
    free(This->tempL);
    This->tempL = NULL;
    free(This->tempR);
    This->tempR = NULL;
}


void BMAllpassNestedFilter_processBufferStereo(BMAllpassNestedFilter* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t level,size_t numSamples){
    
    assert(numSamples<=BM_BUFFER_CHUNK_SIZE);
    
    BMAllpassFilterData* filterData = &This->nestedData[level];
    
    //Produce buffer - write
    uint32_t bytesAvailableForWrite;
    void* headL = TPCircularBufferHead(&filterData->circularBufferL, &bytesAvailableForWrite);
    void* headR = TPCircularBufferHead(&filterData->circularBufferL, &bytesAvailableForWrite);
    
    //Tail for read
    uint32_t bytesAvailableForRead;
    float* tailL = TPCircularBufferTail(&filterData->circularBufferL, &bytesAvailableForRead);
    float* tailR = TPCircularBufferTail(&filterData->circularBufferR, &bytesAvailableForRead);
    
    //Add delay output to the input
    vDSP_vsma(tailL, 1, &filterData->decay1, inputL, 1, filterData->tempL, 1, numSamples);
    vDSP_vsma(tailR, 1, &filterData->decay1, inputR, 1, filterData->tempR, 1, numSamples);
    
    //Save temp to output
    float dcNeg = - filterData->decay1;
    vDSP_vsmul(filterData->tempL, 1, &dcNeg, outputL, 1, numSamples);
    vDSP_vsmul(filterData->tempR, 1, &dcNeg, outputR, 1, numSamples);
    
    //Nested here
    if(level<This->numLevel-1){
        BMAllpassNestedFilter_processBufferStereo(This, filterData->tempL, filterData->tempR, filterData->tempL, filterData->tempR, level+1, numSamples);
    }
    
    //Feed temp to the circular buffer
    uint32_t byteProcessing = (uint32_t)(numSamples * sizeof(float));
    memcpy(headL, filterData->tempL, byteProcessing);
    // mark the written region of the buffer as written
    TPCircularBufferProduce(&filterData->circularBufferL, byteProcessing);
    memcpy(headR, filterData->tempR, byteProcessing);
    // mark the written region of the buffer as written
    TPCircularBufferProduce(&filterData->circularBufferR, byteProcessing);
    
    //Output
    vDSP_vsma(tailL, 1, &filterData->decay2, outputL, 1, outputL, 1, numSamples);
    vDSP_vsma(tailR, 1, &filterData->decay2, outputR, 1, outputR, 1, numSamples);
    
    TPCircularBufferConsume(&filterData->circularBufferL, byteProcessing);
    TPCircularBufferConsume(&filterData->circularBufferR, byteProcessing);
}


