//
//  BMDiffusion.c
//
//  Created by Nguyen Minh Tien on 3/29/18.
//  Anyone may use this file without restrictions
//

#include "BMDiffusion.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "BMIntegerMath.h"

#define dtFactor 2.85

float BMAllPassFilter_processSample(BMAllPassFilter *This,float x);
float BMNestedAllPass2_processSample(BMNestedAllPass2 *This,float x);
float BMNestedAllPass3_processSample(BMNestedAllPass3 *This,float x);
void calculateDelayTime(int dt1,int* dt2,int* dt3);



void BMAllPassFilter_init(BMAllPassFilter *This,size_t delaySamples,float decayAmount){
    This->delayLine = malloc(sizeof(float)*(delaySamples+1));
    memset(This->delayLine, 0, sizeof(float)*(delaySamples+1));
    This->coefficient = decayAmount;
    This->readIndex = 0;
    This->writeIndex = delaySamples;
    This->delayEndIndex = delaySamples;
}

void BMAllPassFilter_destroy(BMAllPassFilter *This){
    free(This->delayLine);
}

inline float BMAllPassFilter_processSample(BMAllPassFilter *This,float x){
    //Get delay data from delayline
    float dlOut = This->delayLine[This->readIndex];
    //Calculate the feedback signal
    float dlFeedback = dlOut*This->coefficient;
    //Calculate the delay input
    float dlIn = x - dlFeedback;
    //Write the dlIn to delayLine
    This->delayLine[This->writeIndex] = dlIn;
    //Calculate the bypass
    float dlBypass = dlIn*This->coefficient;
    
    //Advance next indices
    This->readIndex++;
    This->writeIndex++;
    if(This->readIndex>This->delayEndIndex)
        This->readIndex = 0;
    if(This->writeIndex>This->delayEndIndex)
        This->writeIndex = 0;
    
    //Filter output
    return dlOut + dlBypass;
}

void BMAllPassFilter_process(BMAllPassFilter *This,float* input,float* output,size_t frameCount){
    for(size_t i = 0;i<frameCount;i++){
        output[i] = BMAllPassFilter_processSample(This, input[i]);
    }
}

void BMAllPassFilter_impulseResponse(BMAllPassFilter *This,size_t frameCount){
    float* irBuffer = malloc(sizeof(float)*frameCount);
    for(int i=0;i<frameCount;i++){
        if(i==0)
            irBuffer[i] = 1;
        else
            irBuffer[i] = 0;
    }
    
    BMAllPassFilter_process(This, irBuffer,irBuffer, frameCount);
    
    printf("\n\\impulse response: {\n");
    for(size_t i=0; i<(frameCount-1); i++)
        printf("%f\n", irBuffer[i]);
    printf("%f}\n\n",irBuffer[frameCount-1]);
    
}


#pragma mark - Nested 2 AP
void BMNestedAllPass2_init(BMNestedAllPass2 *This, size_t delayTime,float coeff,size_t nestedDelayTime,float nestedCoeff,bool preNesting){
    This->delayLine = malloc(sizeof(float)*(delayTime+1));
    memset(This->delayLine, 0, sizeof(float)*(delayTime+1));
    This->coefficient = coeff;
    This->readIndex = 0;
    This->writeIndex = delayTime;
    This->delayEndIndex = delayTime;
    BMAllPassFilter_init(&This->nestedAP, nestedDelayTime, nestedCoeff);
    This->preNesting = preNesting;
}

void BMNestedAllPass2_destroy(BMNestedAllPass2 *This){
    free(This->delayLine);
    BMAllPassFilter_destroy(&This->nestedAP);
}

inline float BMNestedAllPass2_processSample(BMNestedAllPass2 *This,float x){
    //Read from delayline
    float dlOut = This->delayLine[This->readIndex];
    //process nested filter (if post nested)
    if(!This->preNesting){
        dlOut = BMAllPassFilter_processSample(&This->nestedAP, dlOut);
    }
    
    //Calculate feedback signal
    float dlFeedback = dlOut*This->coefficient;
    //Calculate delay input
    float dlIn = x - dlFeedback;
    
    //process nested filter (if pre-nested)
    if(This->preNesting){
        dlIn = BMAllPassFilter_processSample(&This->nestedAP, dlIn);
    }
    
    //Write to the delayline
    This->delayLine[This->writeIndex] = dlIn;
    
    //Advance next indices
    This->readIndex++;
    This->writeIndex++;
    if(This->readIndex>This->delayEndIndex)
        This->readIndex = 0;
    if(This->writeIndex>This->delayEndIndex)
        This->writeIndex = 0;
    
    //Calculate the bypass signal
    float dlBypass = dlIn  *This->coefficient;
    
    return dlOut + dlBypass;
}

void BMNestedAllPass2_process(BMNestedAllPass2 *This,float* input,float* output,size_t frameCount){
    for(size_t i = 0;i<frameCount;i++){
        output[i] = BMNestedAllPass2_processSample(This, input[i]);
    }
}

void BMNestedAllPass2_impulseResponse(BMNestedAllPass2 *This,size_t frameCount){
    float* irBuffer = malloc(sizeof(float)*frameCount);
    for(int i=0;i<frameCount;i++){
        if(i==0)
            irBuffer[i] = 1;
        else
            irBuffer[i] = 0;
    }
    
    BMNestedAllPass2_process(This, irBuffer,irBuffer, frameCount);
    
    printf("\n\\impulse response: {\n");
    for(size_t i=0; i<(frameCount-1); i++)
        printf("%f\n", irBuffer[i]);
    printf("%f}\n\n",irBuffer[frameCount-1]);
    
}

#pragma mark - Nested 3 AP
void BMNestedAllPass3_init(BMNestedAllPass3 *This, size_t delayTime,float coeff,size_t nestedDelayTime1,float nestedCoeff1,bool preNesting1,size_t nestedDelayTime2,float nestedCoeff2,bool preNesting2){
    This->delayLine = malloc(sizeof(float)*(delayTime+1));
    memset(This->delayLine, 0, sizeof(float)*(delayTime+1));
    This->coefficient = coeff;
    This->readIndex = 0;
    This->writeIndex = delayTime;
    This->delayEndIndex = delayTime;
    BMNestedAllPass2_init(&This->nestedAP2,nestedDelayTime1, nestedCoeff1, nestedDelayTime2, nestedCoeff2,preNesting2);
    This->preNesting = preNesting1;
}

void BMNestedAllPass3_destroy(BMNestedAllPass3 *This){
    free(This->delayLine);
    BMNestedAllPass2_destroy(&This->nestedAP2);
}

inline float BMNestedAllPass3_processSample(BMNestedAllPass3 *This,float x){
    //Read from delayline
    float dlOut = This->delayLine[This->readIndex];
    //process nested filter (if post nested)
    if(!This->preNesting){
        dlOut = BMNestedAllPass2_processSample(&This->nestedAP2, dlOut);
    }
    
    //Calculate feedback signal
    float dlFeedback = dlOut*This->coefficient;
    //Calculate delay input
    float dlIn = x - dlFeedback;
    
    //process nested filter (if pre-nested)
    if(This->preNesting){
        dlIn = BMNestedAllPass2_processSample(&This->nestedAP2, dlIn);
    }
    
    //Write to the delayline
    This->delayLine[This->writeIndex] = dlIn;
    
    //Advance next indices
    This->readIndex++;
    This->writeIndex++;
    if(This->readIndex>This->delayEndIndex)
        This->readIndex = 0;
    if(This->writeIndex>This->delayEndIndex)
        This->writeIndex = 0;
    
    //Calculate the bypass signal
    float dlBypass = dlIn  *This->coefficient;
    
    return dlOut + dlBypass;
}

void BMNestedAllPass3_process(BMNestedAllPass3 *This,float* input,float* output,size_t frameCount){
    for(size_t i = 0;i<frameCount;i++){
        output[i] = BMNestedAllPass3_processSample(This, input[i]);
    }
}

void BMNestedAllPass3_impulseResponse(BMNestedAllPass3 *This,size_t frameCount){
    float* irBuffer = malloc(sizeof(float)*frameCount);
    for(int i=0;i<frameCount;i++){
        if(i==0)
            irBuffer[i] = 1;
        else
            irBuffer[i] = 0;
    }
    
    BMNestedAllPass3_process(This, irBuffer,irBuffer, frameCount);
    
    printf("\n\\impulse response: {\n");
    for(size_t i=0; i<(frameCount-1); i++)
        printf("%f\n", irBuffer[i]);
    printf("%f}\n\n",irBuffer[frameCount-1]);
    
}

#pragma mark - Diffusion
void calculateDelayTime(int dt1,int* dt2,int* dt3){
    //Assume that dt1 = dt2 * dtFactor = dt3 * dtFactor * dtFactor
    //We know dt3
    int assumeDT2 = dt1 / dtFactor;
    int assumeDT3 = dt1 / (dtFactor * dtFactor);
    int fluct = 0;
    //Find dt2 so dt2 & dt1 are relatively prime
    while(true){
        //Check dt2 & dt1 relatively prime
        if(gcd_i(assumeDT2+fluct, dt1)==1){
            *dt2 = assumeDT2 + fluct;
            break;
        }
        if(gcd_i(assumeDT2-fluct, dt1)==1){
            *dt2 = assumeDT2 - fluct;
            break;
        }
        fluct++;
    }
    //Find dt3 so (dt1, dt3) & (dt2,dt3) relatively prime
    fluct = 0;
    while(true){
        //Check dt2 & dt3 relatively prime
        if(gcd_i(assumeDT3+fluct, dt1)==1&&gcd_i(assumeDT3+fluct, *dt2)==1){
            *dt3 = assumeDT3 + fluct;
            break;
        }
        if(gcd_i(assumeDT3-fluct, dt1)==1&&gcd_i(assumeDT3-fluct, *dt2)==1){
            *dt3 = assumeDT3 - fluct;
            break;
        }
        fluct++;
    }
}

void BMDiffusion_init(BMDiffusion *This,float totalDT){
    totalDT/=10;
    //Calculate dt1
    This->dt1 = roundf(totalDT/(1/dtFactor + 1/(dtFactor*dtFactor) + 1));
    //dt2 & dt3 base on dt1
    calculateDelayTime(This->dt1, &This->dt2, &This->dt3);
    printf("dt %d %d %d %f\n",This->dt1,This->dt2,This->dt3,totalDT);
    BMNestedAllPass3_init(&This->nestedAPFilterL, This->dt1, 0.2, This->dt2, 0.2, true,This->dt3,0.2,false);
    BMNestedAllPass3_init(&This->nestedAPFilterR, This->dt1, 0.2, This->dt2, 0.2, true,This->dt3,0.2,false);
    BMNestedAllPass3_impulseResponse(&This->nestedAPFilterL, 256);
//    BMNestedAllPass3_impulseResponse(&This->nestedAPFilterR, 256);
}

void BMDiffusion_destroy(BMDiffusion *This){
    BMNestedAllPass3_destroy(&This->nestedAPFilterL);
    BMNestedAllPass3_destroy(&This->nestedAPFilterR);
}

void BMDiffusion_process(BMDiffusion *This,float* inDataL,float* inDataR,float* outDataL,float* outDataR,size_t frameCount){
    BMNestedAllPass3_process(&This->nestedAPFilterL, inDataL, outDataL, frameCount);
    BMNestedAllPass3_process(&This->nestedAPFilterR, inDataR, outDataR, frameCount);
}



