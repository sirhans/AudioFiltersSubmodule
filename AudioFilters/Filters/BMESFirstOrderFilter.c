//
//  BMESFirstOrderFilter.c
//  BMAUAmpVelocityCatch
//
//  Created by Nguyen Minh Tien on 04/01/2022.
//

#include "BMESFirstOrderFilter.h"
#include "Constants.h"

#define ES_Parameter_TableSize 1000.0f
#define ES_Parameter_Count 3

void BMESFirstOrderFilter_calculateHighShelfParameter(BMESFirstOrderFilter* This,float fc,float gainDb,float* b0,float* b1,float* a1);
void BMESFirstOrderFilter_updateTableParamters(BMESFirstOrderFilter* This);

void BMESFirstOrderFilter_init(BMESFirstOrderFilter* This,float minDb, float maxDb, float fc,float sampleRate){
    This->parametersTable = malloc(sizeof(float*)*ES_Parameter_Count);
    This->parameterBuffers = malloc(sizeof(float*)*ES_Parameter_Count);
    for(int i=0;i<ES_Parameter_Count;i++){
        This->parametersTable[i] = malloc(sizeof(float)*ES_Parameter_TableSize);
        This->parameterBuffers[i] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    }
    This->buffer = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->sampleRate = sampleRate;
    This->oldInputL = 0;
    This->oldInputR = 0;
    This->oldOutputL = 0;
    This->oldOutputR = 0;
    
    BMESFirstOrderFilter_setMinDb(This, minDb);
    BMESFirstOrderFilter_setMaxDb(This, maxDb);
    BMESFirstOrderFilter_setHighShelfFC(This, fc);
}

void BMESFirstOrderFilter_destroy(BMESFirstOrderFilter* This){
    for(int i=0;i<ES_Parameter_Count;i++){
        free(This->parametersTable[i]);
    }
    free(This->parametersTable);
    This->parametersTable = nil;
    
    free(This->buffer);
    This->buffer = nil;
}

#pragma mark - Set function

void BMESFirstOrderFilter_setMinDb(BMESFirstOrderFilter* This, float minDb){
    This->minDb = minDb;
    BMESFirstOrderFilter_updateTableParamters(This);
}

void BMESFirstOrderFilter_setMaxDb(BMESFirstOrderFilter* This, float maxDb){
    This->maxDb = maxDb;
    BMESFirstOrderFilter_updateTableParamters(This);
}

void BMESFirstOrderFilter_setOutMinDb(BMESFirstOrderFilter* This, float minDb){
    This->outMinDb = minDb;
    BMESFirstOrderFilter_updateTableParamters(This);
}

void BMESFirstOrderFilter_setOutMaxDb(BMESFirstOrderFilter* This, float maxDb){
    This->outMaxDb = maxDb;
    BMESFirstOrderFilter_updateTableParamters(This);
}

void BMESFirstOrderFilter_setHighShelfFC(BMESFirstOrderFilter* This, float fc){
    This->fc = fc;
    BMESFirstOrderFilter_updateTableParamters(This);
}

void BMESFirstOrderFilter_updateTableParamters(BMESFirstOrderFilter* This){
    //Calculate the parameter table every time the fc changed.
    for(int i=0;i<ES_Parameter_TableSize;i++){
        float gainDb = This->outMinDb + (This->outMaxDb-This->outMinDb) * ((float)i/(ES_Parameter_TableSize-1));
        BMESFirstOrderFilter_calculateHighShelfParameter(This, This->fc, gainDb, &This->parametersTable[0][i],&This->parametersTable[1][i],&This->parametersTable[2][i]);
    }
}

void BMESFirstOrderFilter_calculateHighShelfParameter(BMESFirstOrderFilter* This,float fc,float gainDb,float* b0,float* b1,float* a1){
    float gainV = BM_DB_TO_GAIN(gainDb);
    double gamma = tanf(M_PI * fc / This->sampleRate);
    double one_over_denominator;
    if(gainV>1.0f){
        one_over_denominator = 1.0f / (gamma + 1.0f);
        *b0 = (gamma + gainV) * one_over_denominator;
        *b1 = (gamma - gainV) * one_over_denominator;
        *a1 = (gamma - 1.0f) * one_over_denominator;
    }else{
        one_over_denominator = 1.0f / (gamma*gainV + 1.0f);
        *b0 = gainV*(gamma + 1.0f) * one_over_denominator;
        *b1 = gainV*(gamma - 1.0f) * one_over_denominator;
        *a1 = (gainV*gamma - 1.0f) * one_over_denominator;
    }
}

#pragma mark - Process function
void BMESFirstOrderFilter_processStereoBuffer(BMESFirstOrderFilter* This,float* inputL, float* inputR, float* outputL,float* outputR,float* gainArray,float numSamples){
    //Clip the gain Array inside the gainMin/MaxDb
    vDSP_vclip(gainArray, 1, &This->minDb, &This->maxDb, gainArray, 1, numSamples);
    
    //Convert gainArray to the index for the linear computing
    float dbRange = (ES_Parameter_TableSize-1.0f)/(This->maxDb-This->minDb);
    float negMinDb = -This->minDb;
    
    int sampleProcessed = 0;
    int sampleProcessing = 0;
    while(sampleProcessed<numSamples){
        sampleProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples - sampleProcessed);
        
        // 1000 * (gain - minDb)/(maxDb-minDb)
        vDSP_vsadd(gainArray+sampleProcessed, 1, &negMinDb, This->buffer, 1, sampleProcessing);
        vDSP_vsmul(This->buffer, 1, &dbRange, This->buffer, 1, sampleProcessing);
        
        //Lookup & calculate parameter value
        //the value of the table scale from outMinDb -> outMaxDb so look up this will automatically scale the array
        //from minDb-maxDb to outMinDb-outMaxDb
        for(int i = 0;i<ES_Parameter_Count;i++)
            vDSP_vlint(This->parametersTable[i], This->buffer, 1, This->parameterBuffers[i], 1, sampleProcessing, ES_Parameter_TableSize);
        
        //Apply parameter to filters for everysamples
        for(int i=sampleProcessed;i<sampleProcessed +sampleProcessing;i++){
            outputL[i] = This->parameterBuffers[0][i]*inputL[i] + This->parameterBuffers[1][i]*This->oldInputL - This->parameterBuffers[2][i]*This->oldOutputL;
            outputR[i] = This->parameterBuffers[0][i]*inputR[i] + This->parameterBuffers[1][i]*This->oldInputR - This->parameterBuffers[2][i]*This->oldOutputR;
            
            This->oldInputL = inputL[i];
            This->oldOutputL = outputL[i];
            This->oldInputR = inputR[i];
            This->oldOutputR = outputR[i];
        }
        
        sampleProcessed += sampleProcessing;
    }
    
}
