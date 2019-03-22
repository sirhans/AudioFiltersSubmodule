/*=====================================================================*
 *                   Copyright (C) 2011 Paul Mineiro                   *
 * All rights reserved.                                                *
 *                                                                     *
 * Redistribution and use in source and binary forms, with             *
 * or without modification, are permitted provided that the            *
 * following conditions are met:                                       *
 *                                                                     *
 *     * Redistributions of source code must retain the                *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer.                                       *
 *                                                                     *
 *     * Redistributions in binary form must reproduce the             *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer in the documentation and/or            *
 *     other materials provided with the distribution.                 *
 *                                                                     *
 *     * Neither the name of Paul Mineiro nor the names                *
 *     of other contributors may be used to endorse or promote         *
 *     products derived from this software without specific            *
 *     prior written permission.                                       *
 *                                                                     *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND              *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,         *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES               *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE             *
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER               *
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,                 *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES            *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE           *
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR                *
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF          *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY              *
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE             *
 * POSSIBILITY OF SUCH DAMAGE.                                         *
 *                                                                     *
 * Contact: Paul Mineiro <paul@mineiro.com>
 *=====================================================================*/

/*
 * This file has been modified by Hans Anderson in 2019 to add vectorised
 * functions on Apple platforms
 */

#ifndef __FAST_LOG_H_
#define __FAST_LOG_H_

#include <stdint.h>
#include "sse.h"
#include <Accelerate/Accelerate.h>
#include "BMVectorOps.h"

static inline float 
fastlog2 (float x)
{
  union { float f; uint32_t i; } vx = { x };
  union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
  float y = vx.i;
  y *= 1.1920928955078125e-7f;

  return y - 124.22551499f
           - 1.498030302f * mx.f 
           - 1.72587999f / (0.3520887068f + mx.f);
}



/*
 * vector log base 2
 * with ~0.04 % error
 *
 * @X         an input array of length length
 * @log2X     output array of length length
 * @length    the length of the arrays
 */
static void vector_fastlog2 (const float* X, float* log2X, size_t length)
{
    
    // process in chunks of 32 floats
    size_t n = length;
    while (n >= 32){
        union { vFloat32_32 f; vUInt32_32 i; } vX = { *(vFloat32_32 *)X };
        union { vUInt32_32 i; vFloat32_32 f; } mX = { (vX.i & 0x007FFFFF) | 0x3f000000 };
        
        vFloat32_32 y = __builtin_convertvector(vX.i,vFloat32_32);
        
        y *= 1.1920928955078125e-7f;
        
        *(vFloat32_32 *)log2X = y - 124.22551499f
                               - 1.498030302f * mX.f
                               - 1.72587999f / (0.3520887068f + mX.f);
        
        // advance the pointers
        X += 32;
        log2X += 32;
        n -= 32;
    }
    
    // process the remaining n < 32 values using the non-vectorised formula
    while (n > 0) {
        *log2X++ = fastlog2(*X++);
        n--;
    }
}


/*
 *! @abstract convert from linear gain to decibels fast
 * with ~0.04 % error
 *
 * @gain   an input array of length length
 * @dB     output array of length length
 */
// this function has been removed because it was slower than vDSP_vdbcon



static inline float
fastlog (float x)
{
  return 0.69314718f * fastlog2 (x);
}

static inline float 
fasterlog2 (float x)
{
  union { float f; uint32_t i; } vx = { x };
  float y = vx.i;
  y *= 1.1920928955078125e-7f;
  return y - 126.94269504f;
}



/*
 * This is the vectorized version of fasterlog2
 * with ~5% error
 *
 * @param X     input array of length "length"
 * @param log2X output array of length "length"
 */
static void vector_fasterLog2(const float* X, float* log2X, size_t length){
    // cast the pointer type of X from float to uint32;
    uint32_t* Xi = (uint32_t*) X;
    
    // convert from int32 in Xi to float in log2x;
    vDSP_vfltu32(Xi,1,log2X,1,length);
    
    // log2f *= 1.1920928955078125e-7f
    // log2f -= 126.94269504f;
    float B = 1.1920928955078125e-7f;
    float C = -126.94269504f;
    vDSP_vsmsa(log2X,1,&B,&C,log2X,1,length);
}


static inline float
fasterlog (float x)
{
//  return 0.69314718f * fasterlog2 (x);

  union { float f; uint32_t i; } vx = { x };
  float y = vx.i;
  y *= 8.2629582881927490e-8f;
  return y - 87.989971088f;
}

static inline float
fasterlog10 (float x){
    return 0.3010299957f * fasterlog2 (x);
}


/*
 * This is the vectorized version of fasterlog
 * with ~ 5% error
 *
 * @param X     input array of length "length"
 * @param log10X output array of length "length"
 */
static void vector_fasterLog10(const float* X, float* log10X, size_t length){
    // compute the log base 2
    vector_fasterLog2(X,log10X,length);
    
    // multiply by B to get the log base 10
    float B = 0.3010299957f;
    vDSP_vsmul(log10X, 1, &B, log10X, 1, length);
}



/*
 * fast convert from linear gain to decibels
 * with ~ 5% error
 *
 * @gain   an input array of length length
 * @dB     output array of length length
 */
static void vector_fasterGainToDb(const float* gain, float* dB, size_t length){
    vector_fasterLog2(gain, dB, length);
    float B = 20.0 * 0.3010299957f;
    vDSP_vsmul(dB, 1, &B, dB, 1, length);
}



#ifdef __SSE2__

static inline v4sf
vfastlog2 (v4sf x)
{
  union { v4sf f; v4si i; } vx = { x };
  union { v4si i; v4sf f; } mx; mx.i = (vx.i & v4sil (0x007FFFFF)) | v4sil (0x3f000000);
  v4sf y = v4si_to_v4sf (vx.i);
  y *= v4sfl (1.1920928955078125e-7f);

  const v4sf c_124_22551499 = v4sfl (124.22551499f);
  const v4sf c_1_498030302 = v4sfl (1.498030302f);
  const v4sf c_1_725877999 = v4sfl (1.72587999f);
  const v4sf c_0_3520087068 = v4sfl (0.3520887068f);

  return y - c_124_22551499
           - c_1_498030302 * mx.f 
           - c_1_725877999 / (c_0_3520087068 + mx.f);
}

static inline v4sf
vfastlog (v4sf x)
{
  const v4sf c_0_69314718 = v4sfl (0.69314718f);

  return c_0_69314718 * vfastlog2 (x);
}

static inline v4sf 
vfasterlog2 (v4sf x)
{
  union { v4sf f; v4si i; } vx = { x };
  v4sf y = v4si_to_v4sf (vx.i);
  y *= v4sfl (1.1920928955078125e-7f);

  const v4sf c_126_94269504 = v4sfl (126.94269504f);

  return y - c_126_94269504;
}

static inline v4sf
vfasterlog (v4sf x)
{
//  const v4sf c_0_69314718 = v4sfl (0.69314718f);
//
//  return c_0_69314718 * vfasterlog2 (x);

  union { v4sf f; v4si i; } vx = { x };
  v4sf y = v4si_to_v4sf (vx.i);
  y *= v4sfl (8.2629582881927490e-8f);

  const v4sf c_87_989971088 = v4sfl (87.989971088f);

  return y - c_87_989971088;
}

#endif // __SSE2__

#endif // __FAST_LOG_H_
