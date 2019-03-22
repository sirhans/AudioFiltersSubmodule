//
//  ComplexMath.h
//  TheEngineSample
//
//  Created by Hans on 2/11/15.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef ComplexMath_h
#define ComplexMath_h

#include <Accelerate/Accelerate.h>
#include "Constants.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline DSPDoubleComplex DSPDoubleComplex_cmul(DSPDoubleComplex a, DSPDoubleComplex b){
    DSPDoubleComplex result;
    result.real = (a.real * b.real) - (a.imag * b.imag);
    result.imag = (a.real * b.imag) + (a.imag * b.real);
    return result;
}

static inline DSPDoubleComplex DSPDoubleComplex_init(double real, double imag){
    DSPDoubleComplex result;
    result.real = real;
    result.imag = imag;
    return result;
}

static inline DSPDoubleComplex DSPDoubleComplex_divide(DSPDoubleComplex numerator, DSPDoubleComplex denominator){
    // see: http://mathworld.wolfram.com/ComplexDivision.html
    // numerator = a + bi
    // denominator = c + di
    
    // ac + bd
    double nReal = numerator.real*denominator.real + numerator.imag*denominator.imag;
    
    // bc - ad
    double nImag = numerator.imag * denominator.real - numerator.real*denominator.imag;
    
    // c^2 + d^2;
    double dReal = denominator.real*denominator.real + denominator.imag*denominator.imag;
    
    return DSPDoubleComplex_init(nReal / dReal, nImag / dReal);
}

static inline DSPDoubleComplex DSPDoubleComplex_add3(DSPDoubleComplex a,DSPDoubleComplex b, DSPDoubleComplex c){
    DSPDoubleComplex result;
    result.real = a.real + b.real + c.real;
    result.imag = a.imag + b.imag + c.imag;
    return result;
}

static inline DSPDoubleComplex DSPDoubleComplex_smul(double r, DSPDoubleComplex c){
    DSPDoubleComplex result;
    result.real = r*c.real;
    result.imag = r*c.imag;
    return result;
}

static inline double DSPDoubleComplex_abs(DSPDoubleComplex a){
    return sqrt(a.real*a.real + a.imag*a.imag);
}

/*
 * returns the complex number z that represents the per sample
 * phase shift at frequency f at sample rate fs.
 *
 * f: frequency
 * 
 * fs: sampling rate
 *
 */
static inline DSPDoubleComplex DSPDoubleComplex_z(double f, double fs){
    double perSamplePhaseShift = 2.0 * M_PI * (f / fs);
    
    // This comes directly from Euler's formula
    // see: http://mathworld.wolfram.com/EulerFormula.html
    DSPDoubleComplex z = DSPDoubleComplex_init(cos(perSamplePhaseShift), -sin(perSamplePhaseShift));
    
    return z;
}

#ifdef __cplusplus
}
#endif

#endif /* ComplexMath_h */
