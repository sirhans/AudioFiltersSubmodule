//
//  BMHysteresis.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 29/10/19.
//  Anyone may use this file without restrictions
//

#include "BMHysteresis.h"
#include <math.h>

void BMHysteresis_processMono(BMHysteresis *This, const float *input, float* output, size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
        float o = input[i]*This->cL;
        This->cL += This->r - fabsf(o);
        output[i] = o;
    }
    
    // output = input[i]*(This->cL - output + r);
    // o = i*(c + r - o);
    // o = i*(c+r) - i*o;
    // o (1+i) = i*(c+r);
    // o = (c+r) * i/(1+i)
}

void BMHysteresis_processMonoTwoSided(BMHysteresis *This,
                                      const float *inputPos, const float *inputNeg,
                                      float* outputPos, float* outputNeg,
                                      size_t numSamples){
    for(size_t i=0; i<numSamples; i++){
        float oPos = inputPos[i]*This->cL;
        float oNeg = inputNeg[i]*This->cL;
        This->cL += This->r - oPos + oNeg;
        outputPos[i] = oPos;
        outputNeg[i] = oNeg;
    }
    
    // output = input[i]*(This->cL - output + r);
    // oPos = iPos*(c + r - oPos + oNeg);
    // oNeg = iNeg*(c + r - oPos + oNeg);
    // oPos + iPos*oPos = iPos*(c + r + oNeg);
    // oPos = (iPos / (1 + iPos)) * (c + r + oNeg);
    // oNeg = (iNeg / (1 - iNeg)) * (c + r + oPos);
    //
    //   * iPA = (iPos / (1 + iPos))  && * iNA = (iNeg / (1 - iNeg))
    // oPos = iPA * (c + r + iNA * (c + r + oPos));
    // oPos - iPA*iNA*oPos = iPA *(c + r + iNA(c+r));
    // oPos * (1-iPA*iNA) = iPA * (c+r)*(1+iNA);
    // oPos = iPA * (c+r) * (1+iNA) / (1 - iPA*iNA);
    //
    // oNeg = iNA * (c + r + iPA * (c + r + oNeg))
    // oNeg - iNA*iPA*oNeg = iNA * ( c + r + iPA*(c+r))
    // oNeg = iNA * (c+r) * (1+iPA) / (1 - iPA*iNA);
}
