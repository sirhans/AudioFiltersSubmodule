//
//  BMHIIRStageData.h
//  AudioFiltersXcodeProject
//
//  Ported from StageDataNeon.h from Laurent De Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 11/7/19.
//  Anyone may use this file without restrictions
//

#ifndef BMHIIRStage_h
#define BMHIIRStage_h

#include <simd/simd.h>

#define BM_HIIR_STAGE_WIDTH 4

/*
 * Given an integer i, we want to select a filter coefficient index such that
 * when i % 4 == 0, the selected index is 1,
 * when i % 4 == 1, the selected index is 0,
 * when i % 4 == 2, the selected index is 3,
 * when i % 4 == 3, the selected index is 2.
 *
 * De Soras computes this using the following formula:
 */
#define BM_HIIR_GET_INDEX(i) (i ^ 1) & (BM_HIIR_STAGE_WIDTH - 1)

typedef struct BMHIIRStage {
    simd_float4 coef;
    simd_float4 mem;
} BMHIIRStage;

/*
 * Based on StageProcNeon.hpp
 */
void BMHIIRStage_processSamplePos(BMHIIRStage* filterStages, size_t currentStage, simd_float4* y, simd_float4* mem){
    
    // base case of recursion; do nothing.
    if (currentStage == 0) return;
    
    // main recursive function
    //
    //    template <int CUR>
    //    void    StageProcNeon <CUR>::process_sample_pos (StageDataNeon *stage_ptr, float32x4_t &y, float32x4_t &mem)
    //    {
    //        StageProcNeon <CUR - 1>::process_sample_pos (stage_ptr, y, mem);
    //
    //        const float32x4_t x    = mem;
    //        stage_ptr [PREV]._mem4 = y;
    //
    //        mem = stage_ptr [CUR]._mem4;
    //        y   = vmlaq_f32 (x, y - mem, stage_ptr [CUR]._coef4);
    //    }
    
    // recursion
    BMHIIRStage_processSamplePos(filterStages, currentStage-1, y, mem);
    
    const simd_float4 x = *mem;
    filterStages[currentStage-1].mem = *y;
    
    *mem = filterStages[currentStage].mem;
    *y = simd_muladd(x, *y - *mem, filterStages[currentStage].coef);
}


/*
 * Based on StageProcNeon.hpp
 */
void BMHIIRStage_processSampleNeg(BMHIIRStage* filterStages, size_t currentStage, simd_float4* y, simd_float4* mem){
    
    // base case of recursion; do nothing.
    if (currentStage == 0) return;
    
//    template <int CUR>
//    void    StageProcNeon <CUR>::process_sample_neg (StageDataNeon *stage_ptr, float32x4_t &y, float32x4_t &mem)
//    {
//        StageProcNeon <CUR - 1>::process_sample_neg (stage_ptr, y, mem);
//
//        const float32x4_t x = mem;
//        stage_ptr [PREV]._mem4 = y;
//
//        mem = stage_ptr [CUR]._mem4;
//        y  += mem;
//        y  *= stage_ptr [CUR]._coef4;
//        y  -= x;
//    }
    BMHIIRStage_processSampleNeg(filterStages, currentStage-1, y, mem);
    
TODO: finish this
}




#endif /* BMHIIRStage_h */
