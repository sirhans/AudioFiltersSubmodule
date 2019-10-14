//
//  BM2x2Matrix.h
//  BMAudioFilters
//
//  Created by Hans on 6/7/16.
//
//  Most of this code should be replaced with simd_float 2x2 functions. We have
//  commented out the functions that should be replaced with simd function
//  calls. The only functions left are matrix rotation functions, which we have
//  updated to use the simd_float2x2 datatype.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BM2x2Matrix_h
#define BM2x2Matrix_h

// #define BM2x2Identity {1.0f, 0.0f, 0.0f, 1.0f}

#ifdef __cplusplus
extern "C" {
#endif
    
    
#include <math.h>
#include <simd/simd.h>



static inline simd_float2x2 BM2x2Matrix_rotationMatrix(float theta){
	simd_float2 column0 = {cosf(theta), sinf(theta)};
	simd_float2 column1 = {-sinf(theta),cosf(theta)};
	simd_float2x2 R = simd_matrix(column0, column1);
	return R;
}




/*!
 *BM2x2MatrixD_rotationMatrix
 *
 * @abstract returns a 2x2 rotation matrix with angle theta
 */
static inline simd_double2x2 BM2x2MatrixD_rotationMatrix(double theta){
	simd_double2 column0 = {cos(theta), sin(theta)};
	simd_double2 column1 = {-sin(theta),cos(theta)};
	simd_double2x2 R = simd_matrix(column0, column1);
	return R;
}




/*!
*BM2x2Matrix_rotate
*
* @abstract out = M.RotationMatrix(theta)
*/
static inline simd_float2x2 BM2x2Matrix_rotate(simd_float2x2 M, float theta){
	return simd_mul(M,BM2x2Matrix_rotationMatrix(theta));
}



    
//
//    /*
//     * |m11 m12|
//     * |m21 m22|
//     */
//    typedef struct {
//        float m11,m12,m21,m22;
//    } BM2x2Matrix;
//
//
//
//
//    /*
//     * |m11 m12|
//     * |m21 m22|
//     */
//    typedef struct {
//        double m11,m12,m21,m22;
//    } BM2x2MatrixD;
//
//
//
//
//    typedef struct {
//        float x,y;
//    } BMVector2;
//
//
//
//    typedef struct {
//        double x,y;
//    } BMVector2D;
//
//
//
//    // C = A.B
//    static inline BM2x2Matrix BM2x2Matrix_mmul(BM2x2Matrix A, BM2x2Matrix B){
//        BM2x2Matrix C = {A.m11*B.m11 + A.m12*B.m21, A.m11*B.m12 + A.m12*B.m22,
//                         A.m21*B.m11 + A.m22*B.m21, A.m21*B.m12 + A.m22*B.m22 };
//        return C;
//    }
//
//
//
//
//
//    // C = A.B
//    static inline BM2x2MatrixD BM2x2MatrixD_mmul(BM2x2MatrixD A, BM2x2MatrixD B){
//        BM2x2MatrixD C = {A.m11*B.m11 + A.m12*B.m21, A.m11*B.m12 + A.m12*B.m22,
//            A.m21*B.m11 + A.m22*B.m21, A.m21*B.m12 + A.m22*B.m22 };
//        return C;
//    }
//
//
//
//
//    // y = M.x
//    static inline BMVector2 BM2x2Matrix_mvmul(BM2x2Matrix M, BMVector2 x){
//        BMVector2 y = {x.x*M.m11 + x.y*M.m12, x.x*M.m21 + x.y*M.m22};
//        return y;
//    }
//
//
//
//
//
//    // y = M.x
//    static inline BMVector2D BM2x2MatrixD_mvmul(BM2x2MatrixD M, BMVector2D x){
//        BMVector2D y = {x.x*M.m11 + x.y*M.m12, x.x*M.m21 + x.y*M.m22};
//        return y;
//    }
//
//
//
//
//    // y = sM (scalar matrix multiplication)
//    static inline BM2x2Matrix BM2x2Matrix_smmul(float s, BM2x2Matrix M){
//        M.m11 *= s;
//        M.m12 *= s;
//        M.m21 *= s;
//        M.m22 *= s;
//        return M;
//    }
//
    
    
#ifdef __cplusplus
}
#endif

#endif /* BM2x2Matrix_h */
