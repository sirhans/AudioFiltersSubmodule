//
//  BMHIIRStageFPU.h
//  AudioFiltersXcodeProject
//
//  Ported from StageProcFpu.hpp from Laurent De Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by hans anderson on 7/17/19.
//  Anyone may use this file without restrictions of any kind.
//

#ifndef BMHIIRStageFPU_h
#define BMHIIRStageFPU_h

#include <mactypes.h>



static inline void BMHIIRStageFPU_processSamplePos(size_t remaining,
                                                   size_t numCoefficients,
                                                   float* sample0, float* sample1,
                                                   const float* coef,
                                                   float* x, float* y){

    while(remaining >= 2){
        const size_t cnt = numCoefficients - remaining;
        
        const float temp_0 = (*sample0 - y[cnt + 0]) * coef[cnt + 0] + x[cnt + 0];
        const float temp_1 = (*sample1 - y[cnt + 1]) * coef[cnt + 1] + x[cnt + 1];
        
        x [cnt + 0] = *sample0;
        x [cnt + 1] = *sample1;
        
        y [cnt + 0] = temp_0;
        y [cnt + 1] = temp_1;
        
        *sample0 = temp_0;
        *sample1 = temp_1;
        
        remaining -= 2;
    }
    
    if(remaining == 1){
        const size_t last = numCoefficients - 1;
        const float  temp = (*sample0 - y[last]) * coef[last] + x[last];
        x[last] = *sample0;
        y[last] = temp;
        *sample0 = temp;
    }
}



#endif /* BMHIIRStageFPU_h */
