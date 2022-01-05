//
//  BMESFirstOrderFilter.h
//  BMAUAmpVelocityCatch
//
//  Created by Nguyen Minh Tien on 04/01/2022.
//
//This filter is updated every samples when changing the gain

#ifndef BMESFirstOrderFilter_h
#define BMESFirstOrderFilter_h

#include <stdio.h>
#include <Accelerate/Accelerate.h>

typedef struct BMESFirstOrderFilter{
    float** parametersTable;
    float minDb,maxDb,fc;
    float sampleRate;
    float** parameterBuffers;
    float* buffer;
    float oldInputL,oldInputR,oldOutputL,oldOutputR;
}BMESFirstOrderFilter;

void BMESFirstOrderFilter_init(BMESFirstOrderFilter* This,float minDb, float maxDb, float fc,float sampleRate);
void BMESFirstOrderFilter_destroy(BMESFirstOrderFilter* This);

void BMESFirstOrderFilter_setMinDb(BMESFirstOrderFilter* This, float minDb);
void BMESFirstOrderFilter_setMaxDb(BMESFirstOrderFilter* This, float maxDb);
void BMESFirstOrderFilter_setHighShelfFC(BMESFirstOrderFilter* This, float fc);

void BMESFirstOrderFilter_processStereoBuffer(BMESFirstOrderFilter* This,float* inputL, float* inputR, float* outputL,float* outputR,float* gainArray,float numSamples);
#endif /* BMESFirstOrderFilter_h */
