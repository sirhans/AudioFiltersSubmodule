//
//  BMQuadratureOscillator.c
//  BMAudioFilters
//
//  A sinusoidal quadrature oscillator suitable for efficient LFO. However, it
//  is not restricted to low frequencies and may also be used for other
//  applications.
//
//  Created by Hans on 31/10/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#include <math.h>
#include "BMQuadratureOscillator.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
    

    
    void BMQuadratureOscillator_init(BMQuadratureOscillator* This,
                                     float fHz,
                                     float sampleRate){
        
        BMQuadratureOscillator_initMatrix(&This->m, fHz, sampleRate);
        
        // set the initial values for an oscillation amplitude of 1
        This->rq.x = 1.0f;
        This->rq.y = 0.0f;
        //Calculate oneSampleDistance
        BMVector2 oneSampleDistanceV = BM2x2Matrix_mvmul(This->m, This->rq);
        This->oneSampleDistance = oneSampleDistanceV.x>oneSampleDistanceV.y ?oneSampleDistanceV.y:oneSampleDistanceV.x;
        
        This->indexX = QIdx_Vol1;
        This->indexY = QIdx_Vol2;
        This->unUsedIndex = QIdx_Vol3;
        This->currentSample = 0;
        
        This->reachZeroSample = sampleRate/(fHz*4.);//each circle it reach 0 4 times
//        printf("estimate %f\n",This->reachZeroSample);
    }

    void BMQuadratureOscillator_reInit(BMQuadratureOscillator* This,
                                     float fHz,
                                     float sampleRate){
        This->fHz = fHz;
        This->sampleRate = sampleRate;
        This->needReInit = true;
    }
    
    
    
    void BMQuadratureOscillator_initMatrix(BM2x2Matrix* m,
                                           float frequency,
                                           float sampleRate){
        // we want a rotation matrix that completes a rotation of 2*M_PI in
        // (sampleRate / fHz) samples. That means that in 1 sample, we need to
        // rotate by an angle of fHz * 2 * M_PI / sampleRate.
        float oneSampleAngle = frequency * 2.0f * M_PI / sampleRate;
        *m = BM2x2Matrix_rotationMatrix(oneSampleAngle);
    }
    
    
    
    
    /*
     * // Mathematica Prototype code
     *
     * For[i = 1, i <= n, i++,
     *   out[[i]] = r[[1]];
     *
     *   r = m.r
     * ];
     *
     */
    void BMQuadratureOscillator_processQuad(BMQuadratureOscillator* This,
                                            float* r,
                                            float* q,
                                            size_t numSamples){
        for(size_t i=0; i<numSamples; i++){
            // copy the current sample value to output
            r[i] = This->rq.x;
            q[i] = This->rq.y;
            
            // multiply the vector rq by the rotation matrix m to generate the
            // next sample of output
            This->rq = BM2x2Matrix_mvmul(This->m, This->rq);
        }
    }
    
    void BMQuadratureOscillator_processQuadTriple(BMQuadratureOscillator* This,
                                                  float* vol1,
                                                  float* vol2,
                                                  float* vol3,
                                                  size_t* numSamples,int* standbyIndex){
        
        if(This->needReInit){
            This->needReInit = false;
            BMQuadratureOscillator_init(This, This->fHz, This->sampleRate);
        }
        
        if(This->oneSampleDistance!=0){
            float* arrayX = getArrayByIndex(This->indexX, vol1, vol2, vol3);
            float* arrayY = getArrayByIndex(This->indexY, vol1, vol2, vol3);
            float* unUsedArray = getArrayByIndex(This->unUsedIndex, vol1, vol2, vol3);
            *standbyIndex = This->unUsedIndex;
            float lastX = 0;
            float lastY = 0;
            for(size_t i=0; i<*numSamples; i++){
                // copy the current sample value to output
                arrayX[i] = This->rq.x;
                arrayY[i] = This->rq.y;
                unUsedArray[i] = 0;
    //            This->currentSample++;
                // multiply the vector rq by the rotation matrix m to generate the
                // next sample of output
                This->rq = BM2x2Matrix_mvmul(This->m, This->rq);

                if(fabs(arrayX[i])<=This->oneSampleDistance
                   &&lastX>=This->oneSampleDistance){
                    //Put indexX to standby - replace by unused index
                    int standByIdx = This->indexX;
                    This->indexX = This->unUsedIndex;
                    This->unUsedIndex = standByIdx;
                    *numSamples = i;
    //                printf("array %f %ld\n",arrayX[*numSamples-1],This->currentSample);
                    arrayX[*numSamples-1] = 0;
                    *standbyIndex = -1;
                    break;
                }else if(fabs(arrayY[i])<=This->oneSampleDistance
                         &&lastY>=This->oneSampleDistance){
                    int standByIdx = This->indexY;
                    This->indexY = This->unUsedIndex;
                    This->unUsedIndex = standByIdx;
                    *numSamples = i;
    //                printf("array %f %ld\n",arrayY[*numSamples-1],This->currentSample);
                    arrayY[*numSamples-1] = 0;
                    *standbyIndex = -1;
                    break;
                }
                lastX = fabs(arrayX[i]);
                lastY = fabs(arrayY[i]);
            }
        }else{
            memset(vol1,0,sizeof(float)*(*numSamples));
            memset(vol2,0,sizeof(float)*(*numSamples));
            memset(vol3,0,sizeof(float)*(*numSamples));
        }
    }
    
//    void BMQuadratureOscillator_processQuadTriple(BMQuadratureOscillator* This,
//                                                  float* vol1,
//                                                  float* vol2,
//                                                  float* vol3,
//                                                  size_t* numSamples,int* standbyIndex){
//        if(This->needReInit){
//            This->needReInit = false;
//            BMQuadratureOscillator_init(This, This->fHz, This->sampleRate);
//        }
//
//        float* arrayX = getArrayByIndex(This->indexX, vol1, vol2, vol3);
//        float* arrayY = getArrayByIndex(This->indexY, vol1, vol2, vol3);
//        float* unUsedArray = getArrayByIndex(This->unUsedIndex, vol1, vol2, vol3);
//        bool shouldUpdateArray = false;
//        *standbyIndex = This->unUsedIndex;
//        if(This->currentSample+*numSamples<This->reachZeroSample){
//            //No zero point in whole frame -> process as usual
//            This->currentSample += *numSamples;
//            //            printf("current %ld\n",This->currentSample);
//        }else{
//            //Standby -> recalculated *numsamples
//            //Check when reach 0, we will swap the buffers
//            *numSamples = floorf(This->reachZeroSample - This->currentSample);
//            //            printf("swap %ld\n",This->currentSample+*numSamples);
//            This->currentSample -= This->reachZeroSample - *numSamples;
//            shouldUpdateArray = true;
//            //Prevent standByIndex update
//            *standbyIndex = -1;
//        }
//
//        for(size_t i=0; i<*numSamples; i++){
//            // copy the current sample value to output
//            arrayX[i] = This->rq.x;
//            arrayY[i] = This->rq.y;
//            unUsedArray[i] = 0;
//
//            // multiply the vector rq by the rotation matrix m to generate the
//            // next sample of output
//            This->rq = BM2x2Matrix_mvmul(This->m, This->rq);
//        }
//
//        if(shouldUpdateArray){
//            shouldUpdateArray = false;
//            if(fabsf(arrayX[*numSamples-1])<fabsf(arrayY[*numSamples-1])){
//                //Put indexX to standby - replace by unused index
//                int standByIdx = This->indexX;
//                This->indexX = This->unUsedIndex;
//                This->unUsedIndex = standByIdx;
//                arrayX[*numSamples-1] = 0;
//            }else{
//                int standByIdx = This->indexY;
//                This->indexY = This->unUsedIndex;
//                This->unUsedIndex = standByIdx;
//                arrayY[*numSamples-1] = 0;
//            }
//        }
//    }
    
    float* getArrayByIndex(int index,
                           float* vol1,
                           float* vol2,
                           float* vol3){
        if(index==QIdx_Vol1)
            return vol1;
        else if(index==QIdx_Vol2)
            return vol2;
        else
            return vol3;
    }
    
    
    void BMQuadratureOscillator_processSingle(BMQuadratureOscillator* This,
                                              float* r,
                                              size_t numSamples){
        for(size_t i=0; i<numSamples; i++){
            // copy the current sample value to output
            r[i] = This->rq.x;
            
            // multiply the vector rq by the rotation matrix m to generate the
            // next sample of output
            This->rq = BM2x2Matrix_mvmul(This->m, This->rq);
        }
    }
    
#ifdef __cplusplus
}
#endif
