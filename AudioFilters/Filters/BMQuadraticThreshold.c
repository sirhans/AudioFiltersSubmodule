//
//  BMQuadraticThreshold.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/2/19.
//  Anyone may use this file without restrictions
//

#include "BMQuadraticThreshold.h"
#include <assert.h>

void BMQuadraticThreshold_initLower(BMQuadraticThreshold* This, float threshold, float width){
    This->isUpper = false;
    
    float l = threshold;
    float w = width;
    
    /*
     * we want to express (- l x + (l + w + x)^2/4)/w
     * as a polynomial A*x^2 * Bx + C
     *
     * Mathematica: (l^2 + 2 l w + w^2 + (-2 l + 2 w) x + x^2) / (4 w)
     */
    This->lPlusW = l + w;
    This->lMinusW = l - w;
    float fourWinv = 1.0 / (4.0f * w);
    // C = (l + w)^2 / (4 w)
    float C = This->lPlusW * This->lPlusW * fourWinv;
    // B = 2 (w - l) / (4 w)
    float B = 2.0f * (-This->lMinusW) * fourWinv;
    // A = 1 / (4 w)
    float A = fourWinv;
    This->coefficients[0] = A;
    This->coefficients[1] = B;
    This->coefficients[2] = C;
}





void BMQuadraticThreshold_initUpper(BMQuadraticThreshold* This, float threshold, float width){
    This->isUpper = true;
    
    float l = threshold;
    float w = width;
    
    /*
     * we want to express the curved part of the function
     * as a polynomial A*x^2 * Bx + C
     *
     * Mathematica: -(l^2 + (w - x)^2 - 2 l (w + x)) / (4 w)
     */
    This->lPlusW = l + w;
    This->lMinusW = l - w;
    float fourWinv = 1.0 / (4.0f * w);
    // C = (l + w)^2 / (4 w)
    float C = -This->lMinusW * This->lMinusW * fourWinv;
    // B = 2 (w - l) / (4 w)
    float B = 2.0f * This->lPlusW * fourWinv;
    // A = 1 / (4 w)
    float A = -fourWinv;
    This->coefficients[0] = A;
    This->coefficients[1] = B;
    This->coefficients[2] = C;
}




/*
 * Mathematica code:
 *
 * qThreshold[x_, l_, w_] :=
 *       If[x < l - w, l, If[x > l + w, x, (- l x + (l + w + x)^2/4)/w]]
 */
void BMQuadraticThreshold_lowerBuffer(BMQuadraticThreshold* This,
                              const float* input,
                              float* output,
                              size_t numFrames){
    assert(input != output && !This->isUpper);
    
    // clip values to [l-w,l+w];
    vDSP_vclip(input, 1, &This->lMinusW, &This->lPlusW, output, 1, numFrames);
    
    // process the polynomial
    vDSP_vpoly(This->coefficients, 1, output, 1, output, 1, numFrames, 2);
    
    // where the input is greater than the polynomial output, return the input
    // *** This works because the output has been clipped ***
    vDSP_vmax(input,1,output,1,output,1,numFrames);
}


/*
 * Mathematica code:
 *  qUpperThreshold[x_, l_,w_] := -If[w < -l + x, -l, If[-x > -l + w, -x,
                                            (l (-x) + 1/4 (-l + w - x)^2)/w]]
 */
void BMQuadraticThreshold_upperBuffer(BMQuadraticThreshold* This,
                                    const float* input,
                                    float* output,
                                    size_t numFrames){
    assert(input != output && This->isUpper);
    
    // clip values to [l-w,l+w];
    vDSP_vclip(input, 1, &This->lMinusW, &This->lPlusW, output, 1, numFrames);
    
    // process the polynomial
    vDSP_vpoly(This->coefficients, 1, output, 1, output, 1, numFrames, 2);
    
    // where the input is less than the polynomial output, return the input
    // *** this works because the output has been clipped ***
    vDSP_vmin(input,1,output,1,output,1,numFrames);
}
