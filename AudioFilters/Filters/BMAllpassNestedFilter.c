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
    This->delaySamples = delaySamples * fmodl(random(), 100.0f)/100.0f;
    This->decay1 = dc1;
    This->decay2 = dc2;
    
    SetupCircularBuffer(&This->circularBufferL, This->delaySamples);
    SetupCircularBuffer(&This->circularBufferR, This->delaySamples);
    
    This->tempL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->tempR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->tempOutL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->tempOutR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
}

void BMAllpassFilterData_destroy(BMAllpassFilterData* This){
    TPCircularBufferCleanup(&This->circularBufferL);
    TPCircularBufferCleanup(&This->circularBufferR);
    
    free(This->tempL);
    This->tempL = NULL;
    free(This->tempR);
    This->tempR = NULL;
    
    free(This->tempOutL);
    This->tempOutL = NULL;
    free(This->tempOutR);
    This->tempOutR = NULL;
}


void BMAllpassNestedFilter_processBufferStereo(BMAllpassNestedFilter* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t level,size_t numSamples){
    
    assert(numSamples<=BM_BUFFER_CHUNK_SIZE);
    
    BMAllpassFilterData* filterData = &This->nestedData[level];
    
    size_t sampleProcessed = 0;
    size_t sampleProcessing = 0;
    
    while(sampleProcessed<numSamples){
        sampleProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples - sampleProcessed);
        sampleProcessing = BM_MIN(sampleProcessing, filterData->delaySamples);
    
        //Produce buffer - write
        uint32_t bytesAvailableForWrite;
        float* headL = TPCircularBufferHead(&filterData->circularBufferL, &bytesAvailableForWrite);
        float* headR = TPCircularBufferHead(&filterData->circularBufferR, &bytesAvailableForWrite);

        //Tail for read
        uint32_t bytesAvailableForRead;
        float* tailL = TPCircularBufferTail(&filterData->circularBufferL, &bytesAvailableForRead);
        float* tailR = TPCircularBufferTail(&filterData->circularBufferR, &bytesAvailableForRead);
        
        //Add delay output to the input
        vDSP_vsma(tailL, 1, &filterData->decay1, inputL+sampleProcessed, 1, filterData->tempL+sampleProcessed, 1, sampleProcessing);
        vDSP_vsma(tailR, 1, &filterData->decay1, inputR+sampleProcessed, 1, filterData->tempR+sampleProcessed, 1, sampleProcessing);
        
        //Save temp to output
        float dcNeg = -filterData->decay1;
        vDSP_vsmul(filterData->tempL+sampleProcessed, 1, &dcNeg, filterData->tempOutL+sampleProcessed, 1, sampleProcessing);
        vDSP_vsmul(filterData->tempR+sampleProcessed, 1, &dcNeg, filterData->tempOutL+sampleProcessed, 1, sampleProcessing);
        
        //Nested here
        if(level<This->numLevel-1){
            BMAllpassNestedFilter_processBufferStereo(This, filterData->tempL+sampleProcessed, filterData->tempR+sampleProcessed, filterData->tempL+sampleProcessed, filterData->tempR+sampleProcessed, level+1, sampleProcessing);
        }
        
        //Feed temp to the circular buffer
        uint32_t byteProcessing = (uint32_t)(sampleProcessing * sizeof(float));
        memcpy(headL, filterData->tempL+sampleProcessed, byteProcessing);
        // mark the written region of the buffer as written
        TPCircularBufferProduce(&filterData->circularBufferL, byteProcessing);
        memcpy(headR, filterData->tempR+sampleProcessed, byteProcessing);
        // mark the written region of the buffer as written
        TPCircularBufferProduce(&filterData->circularBufferR, byteProcessing);
        
        vDSP_vsma(tailL, 1, &filterData->decay2, filterData->tempOutL+sampleProcessed, 1, outputL+sampleProcessed, 1, sampleProcessing);
        vDSP_vsma(tailR, 1, &filterData->decay2, filterData->tempOutR+sampleProcessed, 1, outputR+sampleProcessed, 1, sampleProcessing);
        
        TPCircularBufferConsume(&filterData->circularBufferL, byteProcessing);
        TPCircularBufferConsume(&filterData->circularBufferR, byteProcessing);
        
        sampleProcessed += sampleProcessing;
    }
}


