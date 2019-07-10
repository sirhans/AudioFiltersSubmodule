//
//  BMUpsampler2x.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#include "BMUpsampler2x.h"
#include <Accelerate/Accelerate.h>

void clearBuffers();

// from Upsampler2XNeon.hpp of Laurent de Soras HIIR library
//
//template <int NC>
//Upsampler2xNeon <NC>::Upsampler2xNeon ()
//:    _filter ()
//{
//    for (int i = 0; i < NBR_STAGES + 1; ++i)
//    {
//        _filter [i]._coef4 = vdupq_n_f32 (0);
//    }
//    if ((NBR_COEFS & 1) != 0)
//    {
//        const int      pos = (NBR_COEFS ^ 1) & (STAGE_WIDTH - 1);
//        _filter [NBR_STAGES]._coef [pos] = 1;
//    }
//
//    clear_buffers ();
//}
//
void BMUpsampler2x_init(BMUpsampler2x* This, size_t numCoefficients){
    This->numCoefficients = numCoefficients;
    This->stageWidth = 4;
    This->numStages = (This->numCoefficients + This->stageWidth - 1) / This->stageWidth;
    
    // set all the filter coefficients to zero
    for (size_t i = 0; i < This->numStages + 1; ++i)
    {
        This->filters[i] = 0.0f;
    }
    // if the number of coefficients is odd
    if ((This->numCoefficients & 1) != 0)
    {
        // set one of the coefficients to 1;
        const size_t pos = (numCoefficients ^ 1) & (This->stageWidth - 1);
        *(pos + (float*)(&This->filters[This->numStages])) = 1;
    }
    
    clearBuffers();
}

