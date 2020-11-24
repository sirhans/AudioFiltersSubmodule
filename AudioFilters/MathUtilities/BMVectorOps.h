//
//  BMVectorOps.h
//  BMAudioFilters
//
//  Created by hans anderson on 1/19/19.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMVectorOps_h
#define BMVectorOps_h

#include <MacTypes.h>

// 256 bit vectors
typedef unsigned vUint32_8 __attribute__((ext_vector_type(8),aligned(4)));
typedef float vFloat32_8 __attribute__((ext_vector_type(8),aligned(4)));
typedef int vSInt32_8 __attribute__((ext_vector_type(8),aligned(4)));

// larger vectors
typedef float vFloat32_32 __attribute__((ext_vector_type(32),aligned(4)));
typedef int vSInt32_32 __attribute__((ext_vector_type(32),aligned(4)));
typedef unsigned vUInt32_32 __attribute__((ext_vector_type(32),aligned(4)));



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
				 size_t length);

/*
 * returns a vector that contains max(a[i],b[i])
 */
vFloat32_8 vfmax_8(vFloat32_8 a, vFloat32_8 b);

/*
 * absolute value of each element in a
 */
vFloat32_8 vfabsf_8(vFloat32_8 a);

/*
 * absolute value of each element in a
 */
vFloat32_32 vfabsf_32(vFloat32_32 a);


/*
 * replace all negative values with zeros and return
 */
vFloat32_32 vfClipNeg32(vFloat32_32 A);


/*!
 *BMVectorNorm
 *
 * @abstract find the l2 norm of v
 *
 * @param v input vector
 * @param length length of v
 */
float BMVectorNorm(const float* v, size_t length);


/*!
 *BMVectorNormalise
 *
 * @abstract scale v so that its l2 norm is equal to one
 */
void BMVectorNormalise(float* v, size_t length);


#endif /* BMVectorOps_h */
