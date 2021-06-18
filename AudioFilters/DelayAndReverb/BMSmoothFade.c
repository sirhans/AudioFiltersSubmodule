//
//  BMSmoothFade.c
//  AudioMontage
//
//  Created by Nguyen Minh Tien on 10/1/20.
//

#include "BMSmoothFade.h"
#include <stdlib.h>
#include "Constants.h"
#include <Accelerate/Accelerate.h>

void BMSmoothFade_init(BMSmoothFade* This,size_t fadeLength){
    This->fadeType = FT_Stop;
    This->fadeIdx = 0;
    This->fadeLength = fadeLength;
    This->fadeBuffer = malloc(sizeof(float)*fadeLength);
    This->lastVol = 0.0f;
    vDSP_vfill(&This->lastVol, This->fadeBuffer, 1, fadeLength);
}

void BMSmoothFade_free(BMSmoothFade* This){
    free(This->fadeBuffer);
    This->fadeBuffer = NULL;
}

size_t BMSmoothFade_generateFadeInBuffer(BMSmoothFade* This,size_t frameCount){
    size_t fadeBufferLength = BM_MIN(frameCount, This->fadeLength-This->fadeIdx);
    float start = (float)This->fadeIdx/This->fadeLength;
    float increment = 1.0f/This->fadeLength;
    vDSP_vramp(&start, &increment, This->fadeBuffer, 1, fadeBufferLength);
    This->fadeIdx += fadeBufferLength;
    return fadeBufferLength;
}

size_t BMSmoothFade_generateFadeOutBuffer(BMSmoothFade* This,size_t frameCount){
    size_t fadeBufferLength = BM_MIN(frameCount, This->fadeLength-This->fadeIdx);
    float start = (float)(This->fadeLength-This->fadeIdx)/This->fadeLength;
    float increment = -1.0f/This->fadeLength;
    vDSP_vramp(&start, &increment, This->fadeBuffer, 1, fadeBufferLength);
    This->fadeIdx += fadeBufferLength;
    return fadeBufferLength;
}

void BMSmoothFade_startFading(BMSmoothFade* This,FadeType type){
    This->fadeType = type;
    This->fadeIdx = 0;
}

void BMSmoothFade_processBufferStereo(BMSmoothFade* This, float* inL,float* inR,float* outL,float* outR,size_t frameCount){
    if(This->fadeType!=FT_Stop){
        if(This->fadeType==FT_In){
            size_t fadeBufferLength = BMSmoothFade_generateFadeInBuffer(This, frameCount);
            //Go from 0 to 1
            if(This->fadeIdx==This->fadeLength){
                //Finish fade -> generate the rest fadebuffer to 1
                size_t remainSamples = frameCount - fadeBufferLength;
                This->lastVol = 1.0f;
                vDSP_vfill(&This->lastVol, This->fadeBuffer+fadeBufferLength, 1, remainSamples);
                This->fadeType = FT_Stop;
            }
            vDSP_vmul(inL, 1, This->fadeBuffer, 1, outL, 1, frameCount);
            vDSP_vmul(inR, 1, This->fadeBuffer, 1, outR, 1, frameCount);
            
            if(This->fadeType==FT_Stop){
                //Fill fade buffer with 1
                float num = 1.0f;
                vDSP_vfill(&num, This->fadeBuffer, 1, frameCount);
            }
            
        }else if(This->fadeType==FT_Out){
            size_t fadeBufferLength = BMSmoothFade_generateFadeOutBuffer(This, frameCount);
            //Go from 1 to 0
            if(This->fadeIdx==This->fadeLength){
                //Finish fade -> generate the rest fadebuffer to 1
                size_t remainSamples = frameCount - fadeBufferLength;
                float num = 0.0f;
                vDSP_vfill(&num, This->fadeBuffer+fadeBufferLength, 1, remainSamples);
                This->fadeType = FT_Stop;
            }
            
            vDSP_vmul(inL, 1, This->fadeBuffer, 1, outL, 1, frameCount);
            vDSP_vmul(inR, 1, This->fadeBuffer, 1, outR, 1, frameCount);
            
            if(This->fadeType==FT_Stop){
                //Fill fade buffer with 0
                float num = 0.0f;
                vDSP_vfill(&num, This->fadeBuffer, 1, frameCount);
            }
        }
    }else{
        vDSP_vmul(inL, 1, This->fadeBuffer, 1, outL, 1, frameCount);
        vDSP_vmul(inR, 1, This->fadeBuffer, 1, outR, 1, frameCount);
    }
    
}

void BMSmoothFade_processBufferMono(BMSmoothFade* This, float* inData,float* outData,size_t frameCount){
    if(This->fadeType==FT_In){
        size_t fadeBufferLength = BMSmoothFade_generateFadeInBuffer(This, frameCount);
        //Go from 0 to 1
        if(This->fadeIdx==This->fadeLength){
            //Finish fade -> generate the rest fadebuffer to 1
            size_t remainSamples = frameCount - fadeBufferLength;
            float num = 1.0f;
            vDSP_vfill(&num, This->fadeBuffer+fadeBufferLength, 1, remainSamples);
            This->fadeType = FT_Stop;
        }
        vDSP_vmul(inData, 1, This->fadeBuffer, 1, outData, 1, frameCount);
        
        if(This->fadeType==FT_Stop){
            //Fill fade buffer with 0
            float num = 1.0f;
            vDSP_vfill(&num, This->fadeBuffer, 1, frameCount);
        }
    }else if(This->fadeType==FT_Out){
        size_t fadeBufferLength = BMSmoothFade_generateFadeOutBuffer(This, frameCount);
        //Go from 1 to 0
        if(This->fadeIdx==This->fadeLength){
            //Finish fade -> generate the rest fadebuffer to 1
            size_t remainSamples = frameCount - fadeBufferLength;
            float num = 0.0f;
            vDSP_vfill(&num, This->fadeBuffer+fadeBufferLength, 1, remainSamples);
            This->fadeType = FT_Stop;
        }
        
        vDSP_vmul(inData, 1, This->fadeBuffer, 1, outData, 1, frameCount);
        
        if(This->fadeType==FT_Stop){
            //Fill fade buffer with 0
            float num = 0.0f;
            vDSP_vfill(&num, This->fadeBuffer, 1, frameCount);
        }
    }else{
        vDSP_vmul(inData, 1, This->fadeBuffer, 1, outData, 1, frameCount);
    }
    
    
}


