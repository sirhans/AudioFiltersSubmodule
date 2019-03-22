//
//  BMMidSide.h
//  BMAudioFilters
//
//  Convert from Stereo to Mid/Side and from Mid/Side to Stereo
//
//  Created by Hans on 15/2/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMMidSide_h
#define BMMidSide_h

#include <Accelerate/Accelerate.h>
    
    /*
     * convert 
     * stereo to mid / side
     *   or
     * mid / side to stereo
     * 
     * Does NOT process in place
     *
     * @param in1  - Left or Mid in
     * @param in2  - Right or Side in
     * @param out1 - Left or Mid out
     * @param out2 - Right or Side out
     */
    static __inline__ __attribute__((always_inline)) void
    BMMidSideMatrixConvert (float* in1, float* in2,
                            float* out1, float* out2,
                            size_t numSamples){
        float sqrt1_2 = M_SQRT1_2;
        
        // sum to mid and multiply by 1/sqrt(2)
        vDSP_vasm(in1, 1,
                  in2, 1,
                  &sqrt1_2,
                  out1, 1,
                  numSamples);
        
        // subtract to side and multiply by 1/sqrt(2)
        vDSP_vsbsm(in1, 1,
                   in2, 1,
                   &sqrt1_2,
                   out2, 1,
                   numSamples);
    }
    

    
    
#endif /* BMMidSide_h */

#ifdef __cplusplus
}
#endif
