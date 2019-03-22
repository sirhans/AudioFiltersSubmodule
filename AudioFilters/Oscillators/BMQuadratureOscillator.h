//
//  BMQuadratureOscillator.h
//  BMAudioFilters
//
//  A sinusoidal quadrature oscillator suitable for efficient LFO. However, it
//  is not restricted to low frequencies and may also be used for other
//  applications.
//
//  This oscillator works by multiplying a 2x2 matrix by a vector of length 2
//  once per sample. The idea comes from a Mathematica model of an exchange of
//  kinetic and potential energy.  The code for that model appears in the
//  comment at the bottom of this file.
//
//  The output is a quadrature phase wave signal in two arrays of floats. Values
//  are in the range [-1,1].  The first sample of output is 1 in the array r and
//  0 in q. Other initial values can be acheived by directly editing the value
//  of the struct variable rq after initialisation.
//
//
//  USAGE EXAMPLE
//
//  Initialisation
//
//  BMQuadratureOscillator osc;
//  // 2 hz oscillation at sample rate of 48hkz
//  BMQuadratureOscillator_init(osc, 2.0f, 48000.0f);
//
//
//  oscillation
//
//  float* r = malloc(sizeof(float)*100);
//  float* q = malloc(sizeof(float)*100);
//
//  // fill buffers r and q with quadrature wave signal 100 samples long
//  BMQuadratureOscillator_processQuad(osc, r, q, 100);
//
//  Created by Hans on 31/10/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMQuadratureOscillator_h
#define BMQuadratureOscillator_h

#include "BM2x2Matrix.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum{
        QIdx_Vol1 = 0,
        QIdx_Vol2 = 1,
        QIdx_Vol3 = 2,
    } QuadIdx;
    
    typedef struct BMQuadratureOscillator {
        BM2x2Matrix m;
        BMVector2 rq;
        float oneSampleDistance;
        int indexX;
        int indexY;
        int unUsedIndex;
        long currentSample;
        float reachZeroSample;
        bool needReInit;
        float fHz;
        float sampleRate;
    } BMQuadratureOscillator;
    
    
    /*
     * Initialize the rotation speed and set initial values
     * (no memory is allocated for this)
     *
     * @param This       pointer to an oscillator struct
     * @param fHz        frequency in Hz
     * @param sampleRate sample rate in Hz
     *
     */
    void BMQuadratureOscillator_init(BMQuadratureOscillator* This,
                                     float fHz,
                                     float sampleRate);
    
    void BMQuadratureOscillator_reInit(BMQuadratureOscillator* This,
                                       float fHz,
                                       float sampleRate);
    
    
    
    void BMQuadratureOscillator_initMatrix(BM2x2Matrix* m,
                                           float frequency,
                                           float sampleRate);
    
    
    
    
    
    /*
     * Generate numSamples of oscillation in quadrature phase output into the 
     * arrays q and r.  Continues smoothly from the previous function call.
     *
     * @param r          an array for output in phase
     * @param q          an array for output in quadrature with r
     * @param numSamples length of q and r
     *
     */
    void BMQuadratureOscillator_processQuad(BMQuadratureOscillator* This,
                                            float* r,
                                            float* q,
                                            size_t numSamples);
    
    void BMQuadratureOscillator_processQuadTriple(BMQuadratureOscillator* This,
                                                  float* r,
                                                  float* q,
                                                  float* s,
                                                  size_t *numSamples,
                                                  int* standbyIndex);
    
    /*
     * Generate numSamples of oscillation into the array r.  Continues smoothly 
     * from the previous function call.
     *
     * @param r          an array for output
     * @param numSamples length of r
     *

     */
    void BMQuadratureOscillator_processSingle(BMQuadratureOscillator* This,
                                            float* r,
                                            size_t numSamples);
    
    float* getArrayByIndex(int index,
                           float* r,
                           float* q,
                           float* s);
    
#ifdef __cplusplus
}
#endif

#endif /* BMQuadratureOscillator_h */


// Mathematica code for the energy exchange model
//
/*
 Module[{q, r, qr, rq, qq, rr,  crossRate = 0.2, fbRate, i, n = 100
 , out},
 out = 0* Range[n];
 q = 1;
 r = 0;
 
 fbRate = Sqrt[1.0 - crossRate^2];
 
 For[i = 1, i <= n, i++,
 out[[i]] = q;
 
 (* crossed output energy *)
 qr = crossRate*q;
 rq = crossRate*r;
 
 (* feedback energy *)
 qq = q*fbRate;
 rr = r*fbRate;
 
 q = qq + rq;
 r = rr - qr;
 ];
 
 Show[
 ListLinePlot[out, PlotRange -> All, DataRange -> {0, n - 1}](*,
 Plot[Cos[x crossRate],{x,0,n}, PlotStyle\[Rule]Directive[{Black,
 Dashed}]]*)
 ]
 ]
 
 */



// mathematica code for the matrix version that is implemented in this file
//
/*
 
 Module[{r, m,  crossRate = 0.2, i, n = 100
 , out},
 out = 0* Range[n];
 
 r = {1, 0};
 m = RotationMatrix[crossRate];
 
 For[i = 1, i <= n, i++,
 out[[i]] = r[[1]];
 
 r = m.r
 ];
 
 Show[
 ListLinePlot[out, PlotRange -> All, DataRange -> {0, n - 1}]
 ]
 ]
 
 */
