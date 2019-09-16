//
//  BMDiffusion.c
//  AUStereoModulator
//
//  Created by Nguyen Minh Tien on 3/29/18.
//  Copyright © 2018 Nguyen Minh Tien. All rights reserved.
//

#include "BMDiffusion.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "BMIntegerMath.h"

#define dtFactor 2.85

float BMAllPassFilter_processSample(BMAllPassFilter* this,float x);
float BMNestedAllPass2_processSample(BMNestedAllPass2* this,float x);
float BMNestedAllPass3_processSample(BMNestedAllPass3* this,float x);
void calculateDelayTime(int dt1,int* dt2,int* dt3);



void BMAllPassFilter_init(BMAllPassFilter* this,size_t delaySamples,float decayAmount){
    this->delayLine = malloc(sizeof(float)*(delaySamples+1));
    memset(this->delayLine, 0, sizeof(float)*(delaySamples+1));
    this->coefficient = decayAmount;
    this->readIndex = 0;
    this->writeIndex = delaySamples;
    this->delayEndIndex = delaySamples;
}

void BMAllPassFilter_destroy(BMAllPassFilter* this){
    free(this->delayLine);
}

inline float BMAllPassFilter_processSample(BMAllPassFilter* this,float x){
    //Get delay data from delayline
    float dlOut = this->delayLine[this->readIndex];
    //Calculate the feedback signal
    float dlFeedback = dlOut*this->coefficient;
    //Calculate the delay input
    float dlIn = x - dlFeedback;
    //Write the dlIn to delayLine
    this->delayLine[this->writeIndex] = dlIn;
    //Calculate the bypass
    float dlBypass = dlIn*this->coefficient;
    
    //Advance next indices
    this->readIndex++;
    this->writeIndex++;
    if(this->readIndex>this->delayEndIndex)
        this->readIndex = 0;
    if(this->writeIndex>this->delayEndIndex)
        this->writeIndex = 0;
    
    //Filter output
    return dlOut + dlBypass;
}

void BMAllPassFilter_process(BMAllPassFilter* this,float* input,float* output,size_t frameCount){
    for(size_t i = 0;i<frameCount;i++){
        output[i] = BMAllPassFilter_processSample(this, input[i]);
    }
}

void BMAllPassFilter_impulseResponse(BMAllPassFilter* this,size_t frameCount){
    float* irBuffer = malloc(sizeof(float)*frameCount);
    for(int i=0;i<frameCount;i++){
        if(i==0)
            irBuffer[i] = 1;
        else
            irBuffer[i] = 0;
    }
    
    BMAllPassFilter_process(this, irBuffer,irBuffer, frameCount);
    
    printf("\n\\impulse response: {\n");
    for(size_t i=0; i<(frameCount-1); i++)
        printf("%f\n", irBuffer[i]);
    printf("%f}\n\n",irBuffer[frameCount-1]);
    
}


#pragma mark - Nested 2 AP
void BMNestedAllPass2_init(BMNestedAllPass2* this, size_t delayTime,float coeff,size_t nestedDelayTime,float nestedCoeff,bool preNesting){
    this->delayLine = malloc(sizeof(float)*(delayTime+1));
    memset(this->delayLine, 0, sizeof(float)*(delayTime+1));
    this->coefficient = coeff;
    this->readIndex = 0;
    this->writeIndex = delayTime;
    this->delayEndIndex = delayTime;
    BMAllPassFilter_init(&this->nestedAP, nestedDelayTime, nestedCoeff);
    this->preNesting = preNesting;
}

void BMNestedAllPass2_destroy(BMNestedAllPass2* this){
    free(this->delayLine);
    BMAllPassFilter_destroy(&this->nestedAP);
}

inline float BMNestedAllPass2_processSample(BMNestedAllPass2* this,float x){
    //Read from delayline
    float dlOut = this->delayLine[this->readIndex];
    //process nested filter (if post nested)
    if(!this->preNesting){
        dlOut = BMAllPassFilter_processSample(&this->nestedAP, dlOut);
    }
    
    //Calculate feedback signal
    float dlFeedback = dlOut*this->coefficient;
    //Calculate delay input
    float dlIn = x - dlFeedback;
    
    //process nested filter (if pre-nested)
    if(this->preNesting){
        dlIn = BMAllPassFilter_processSample(&this->nestedAP, dlIn);
    }
    
    //Write to the delayline
    this->delayLine[this->writeIndex] = dlIn;
    
    //Advance next indices
    this->readIndex++;
    this->writeIndex++;
    if(this->readIndex>this->delayEndIndex)
        this->readIndex = 0;
    if(this->writeIndex>this->delayEndIndex)
        this->writeIndex = 0;
    
    //Calculate the bypass signal
    float dlBypass = dlIn * this->coefficient;
    
    return dlOut + dlBypass;
}

void BMNestedAllPass2_process(BMNestedAllPass2* this,float* input,float* output,size_t frameCount){
    for(size_t i = 0;i<frameCount;i++){
        output[i] = BMNestedAllPass2_processSample(this, input[i]);
    }
}

void BMNestedAllPass2_impulseResponse(BMNestedAllPass2* this,size_t frameCount){
    float* irBuffer = malloc(sizeof(float)*frameCount);
    for(int i=0;i<frameCount;i++){
        if(i==0)
            irBuffer[i] = 1;
        else
            irBuffer[i] = 0;
    }
    
    BMNestedAllPass2_process(this, irBuffer,irBuffer, frameCount);
    
    printf("\n\\impulse response: {\n");
    for(size_t i=0; i<(frameCount-1); i++)
        printf("%f\n", irBuffer[i]);
    printf("%f}\n\n",irBuffer[frameCount-1]);
    
}

#pragma mark - Nested 3 AP
void BMNestedAllPass3_init(BMNestedAllPass3* this, size_t delayTime,float coeff,size_t nestedDelayTime1,float nestedCoeff1,bool preNesting1,size_t nestedDelayTime2,float nestedCoeff2,bool preNesting2){
    this->delayLine = malloc(sizeof(float)*(delayTime+1));
    memset(this->delayLine, 0, sizeof(float)*(delayTime+1));
    this->coefficient = coeff;
    this->readIndex = 0;
    this->writeIndex = delayTime;
    this->delayEndIndex = delayTime;
    BMNestedAllPass2_init(&this->nestedAP2,nestedDelayTime1, nestedCoeff1, nestedDelayTime2, nestedCoeff2,preNesting2);
    this->preNesting = preNesting1;
}

void BMNestedAllPass3_destroy(BMNestedAllPass3* this){
    free(this->delayLine);
    BMNestedAllPass2_destroy(&this->nestedAP2);
}

inline float BMNestedAllPass3_processSample(BMNestedAllPass3* this,float x){
    //Read from delayline
    float dlOut = this->delayLine[this->readIndex];
    //process nested filter (if post nested)
    if(!this->preNesting){
        dlOut = BMNestedAllPass2_processSample(&this->nestedAP2, dlOut);
    }
    
    //Calculate feedback signal
    float dlFeedback = dlOut*this->coefficient;
    //Calculate delay input
    float dlIn = x - dlFeedback;
    
    //process nested filter (if pre-nested)
    if(this->preNesting){
        dlIn = BMNestedAllPass2_processSample(&this->nestedAP2, dlIn);
    }
    
    //Write to the delayline
    this->delayLine[this->writeIndex] = dlIn;
    
    //Advance next indices
    this->readIndex++;
    this->writeIndex++;
    if(this->readIndex>this->delayEndIndex)
        this->readIndex = 0;
    if(this->writeIndex>this->delayEndIndex)
        this->writeIndex = 0;
    
    //Calculate the bypass signal
    float dlBypass = dlIn * this->coefficient;
    
    return dlOut + dlBypass;
}

void BMNestedAllPass3_process(BMNestedAllPass3* this,float* input,float* output,size_t frameCount){
    for(size_t i = 0;i<frameCount;i++){
        output[i] = BMNestedAllPass3_processSample(this, input[i]);
    }
}

void BMNestedAllPass3_impulseResponse(BMNestedAllPass3* this,size_t frameCount){
    float* irBuffer = malloc(sizeof(float)*frameCount);
    for(int i=0;i<frameCount;i++){
        if(i==0)
            irBuffer[i] = 1;
        else
            irBuffer[i] = 0;
    }
    
    BMNestedAllPass3_process(this, irBuffer,irBuffer, frameCount);
    
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

void BMDiffusion_init(BMDiffusion* this,float totalDT){
    totalDT/=10;
    //Calculate dt1
    this->dt1 = roundf(totalDT/(1/dtFactor + 1/(dtFactor*dtFactor) + 1));
    //dt2 & dt3 base on dt1
    calculateDelayTime(this->dt1, &this->dt2, &this->dt3);
    printf("dt %d %d %d %f\n",this->dt1,this->dt2,this->dt3,totalDT);
    BMNestedAllPass3_init(&this->nestedAPFilterL, this->dt1, 0.2, this->dt2, 0.2, true,this->dt3,0.2,false);
    BMNestedAllPass3_init(&this->nestedAPFilterR, this->dt1, 0.2, this->dt2, 0.2, true,this->dt3,0.2,false);
    BMNestedAllPass3_impulseResponse(&this->nestedAPFilterL, 256);
//    BMNestedAllPass3_impulseResponse(&this->nestedAPFilterR, 256);
}

void BMDiffusion_destroy(BMDiffusion* this){
    BMNestedAllPass3_destroy(&this->nestedAPFilterL);
    BMNestedAllPass3_destroy(&this->nestedAPFilterR);
}

void BMDiffusion_process(BMDiffusion* this,float* inDataL,float* inDataR,float* outDataL,float* outDataR,size_t frameCount){
    BMNestedAllPass3_process(&this->nestedAPFilterL, inDataL, outDataL, frameCount);
    BMNestedAllPass3_process(&this->nestedAPFilterR, inDataR, outDataR, frameCount);
}



