//
//  BMVectorOps.c
//  GainStageVintageCleanAU3
//
//  Created by hans anderson on 3/14/20.
//  This file is public domain with no restrictions.
//

#include "BMVectorOps.h"
#include <Accelerate/Accelerate.h>


/*!
 * BMvInRangef
 *
 * @param X input array
 * @param lowerLimit lower limit
 * @param upperLimit upper limit
 * @param result output array
 * @param length length of X and result
 * @brief test whether each element of X is in the specified range, vector float output
 * @notes result[i] is 1.0 where X[i] is within limits, 0.0 otherwise. Returns floating point output for use in vectorised code without conditional branching
 * @code result[i] = -1.0f * (X[i] >= lowerLimit && X[i] <= upperLimit);
 */
void BMvInRangef(const float* X,
                           float lowerLimit,
                           float upperLimit,
                           float* result,
                           size_t length){
    size_t n = length;
    
    // process in chunks of 32
    while(n >= 32){
        vFloat32_32 Xi = *(vFloat32_32 *)X;
        vSInt32_32 r = (Xi >= lowerLimit && Xi <= upperLimit);
        vFloat32_32 rf = __builtin_convertvector(r,vFloat32_32);
        *(vFloat32_32 *)result = rf;
        
        X += 32;
        result += 32;
        n -= 32;
    }
    
    // finish up the last values
    while (n > 0){
        *result++ = (*X >= lowerLimit && *X <= upperLimit);
        X++;
        n--;
    }
}


/*
 * returns a vector that contains max(a[i],b[i])
 */
vFloat32_8 vfmax_8(vFloat32_8 a, vFloat32_8 b){
    vSInt32_8 ci = a > b;
    vFloat32_8 cf = __builtin_convertvector(ci,vFloat32_8);
    return a*cf + b*(1.0 - cf);
}


/*
 * absolute value of each element in a
 */
vFloat32_8 vfabsf_8(vFloat32_8 a){
    vFloat32_8 t;
    *(vFloat *)&t = vfabsf(*(vFloat *)(&a));
    *((vFloat *)&t+1) = vfabsf(*((vFloat *)&a+1));
    return t;
}


/*
 * absolute value of each element in a
 */
vFloat32_32 vfabsf_32(vFloat32_32 a){
    vFloat32_32 t;
    
    *(vFloat *)&t = vfabsf(*(vFloat *)(&a));
    *((vFloat *)&t + 1) = vfabsf(*((vFloat *)&a + 1));
    *((vFloat *)&t + 2) = vfabsf(*((vFloat *)&a + 2));
    *((vFloat *)&t + 3) = vfabsf(*((vFloat *)&a + 3));
    *((vFloat *)&t + 4) = vfabsf(*((vFloat *)&a + 4));
    *((vFloat *)&t + 5) = vfabsf(*((vFloat *)&a + 5));
    *((vFloat *)&t + 6) = vfabsf(*((vFloat *)&a + 6));
    *((vFloat *)&t + 7) = vfabsf(*((vFloat *)&a + 7));
    
    return t;
}


/*
 * replace all negative values with zeros and return
 */
vFloat32_32 vfClipNeg32(vFloat32_32 A){
    vFloat32_32 t = vfabsf_32(A);
    return (t + A) * 0.5;
}


/*!
 *BMVectorNorm
 *
 * @abstract find the l2 norm of v
 *
 * @param v input vector
 * @param length length of v
 */
float BMVectorNorm(const float* v, size_t length){
	float sumsq;
	vDSP_svesq(v, 1, &sumsq, length);
	return sqrtf(sumsq);
}



/*!
 *BMVectorNormSplitComplex
 *
 * @abstract find the l2 norm of v
 *
 * @param v input vector
 * @param length length of v
 */
float BMVectorNormSplitComplex(const DSPSplitComplex v, size_t length){
	float sumsqi, sumsqr;
	vDSP_svesq(v.imagp, 1, &sumsqi, length);
	vDSP_svesq(v.realp, 1, &sumsqr, length);
	return sqrtf(sumsqi + sumsqr);
}


/*!
 *BMVectorNormalise
 *
 * @abstract scale v so that its l2 norm is equal to one
 */
void BMVectorNormalise(float* v, size_t length){
	float norm = BMVectorNorm(v, length);
	if(norm > 0){
		float scale = 1.0 / norm;
		vDSP_vsmul(v,1,&scale,v,1,length);
	}
}

