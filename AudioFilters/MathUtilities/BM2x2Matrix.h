//
//  BM2x2Matrix.h
//  BMAudioFilters
//
//  Created by Hans on 6/7/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BM2x2Matrix_h
#define BM2x2Matrix_h

#define BM2x2Identity {1.0f, 0.0f, 0.0f, 1.0f}

#ifdef __cplusplus
extern "C" {
#endif
    
    
#include <math.h>
    
    
    /*
     * |m11 m12|
     * |m21 m22|
     */
    typedef struct {
        float m11,m12,m21,m22;
    } BM2x2Matrix;

    
    
    
    /*
     * |m11 m12|
     * |m21 m22|
     */
    typedef struct {
        double m11,m12,m21,m22;
    } BM2x2MatrixD;
    
    
    
    
    typedef struct {
        float x,y;
    } BMVector2;
    
    
    
    typedef struct {
        double x,y;
    } BMVector2D;
    
    
    
    // C = A.B
    static inline BM2x2Matrix BM2x2Matrix_mmul(BM2x2Matrix A, BM2x2Matrix B){
        BM2x2Matrix C = {A.m11*B.m11 + A.m12*B.m21, A.m11*B.m12 + A.m12*B.m22,
                         A.m21*B.m11 + A.m22*B.m21, A.m21*B.m12 + A.m22*B.m22 };
        return C;
    }
    
    
    
    
    
    // C = A.B
    static inline BM2x2MatrixD BM2x2MatrixD_mmul(BM2x2MatrixD A, BM2x2MatrixD B){
        BM2x2MatrixD C = {A.m11*B.m11 + A.m12*B.m21, A.m11*B.m12 + A.m12*B.m22,
            A.m21*B.m11 + A.m22*B.m21, A.m21*B.m12 + A.m22*B.m22 };
        return C;
    }
    
    
    
    
    
    static inline BM2x2Matrix BM2x2Matrix_rotationMatrix(float theta){
        BM2x2Matrix R = {cosf(theta), -sinf(theta),
                         sinf(theta), cosf(theta)};
        return R;
    }
    
    
    
    
    
    static inline BM2x2MatrixD BM2x2MatrixD_rotationMatrix(double theta){
        BM2x2MatrixD R = {cos(theta), -sin(theta),
            sin(theta), cos(theta)};
        return R;
    }
    
    
    
    
    
    // C = M.R(theta)
    static inline BM2x2Matrix BM2x2Matrix_rotate(BM2x2Matrix M, float theta){
        return BM2x2Matrix_mmul(M,BM2x2Matrix_rotationMatrix(theta));
    }
    
    
    
    
    
    // y = M.x
    static inline BMVector2 BM2x2Matrix_mvmul(BM2x2Matrix M, BMVector2 x){
        BMVector2 y = {x.x*M.m11 + x.y*M.m12, x.x*M.m21 + x.y*M.m22};
        return y;
    }
    
    
    
    
    
    // y = M.x
    static inline BMVector2D BM2x2MatrixD_mvmul(BM2x2MatrixD M, BMVector2D x){
        BMVector2D y = {x.x*M.m11 + x.y*M.m12, x.x*M.m21 + x.y*M.m22};
        return y;
    }
    
    
    
    
    // y = sM (scalar matrix multiplication)
    static inline BM2x2Matrix BM2x2Matrix_smmul(float s, BM2x2Matrix M){
        M.m11 *= s;
        M.m12 *= s;
        M.m21 *= s;
        M.m22 *= s;
        return M;
    }
    
    
    
#ifdef __cplusplus
}
#endif

#endif /* BM2x2Matrix_h */
