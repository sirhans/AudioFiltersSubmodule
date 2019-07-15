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
 * ported from:
 *    StageProc4Neon.hpp
 *    in the HIIR library by Laurent de Soras (http://ldesoras.free.fr/prod.html)
 */
static inline void BMHIIRStage_processSamplePos(size_t remaining, size_t numCoefficients, simd_float4* sample0, simd_float4* sample1, BMHIIRStage* filterStages){
    
    //    template <>
    //    inline void    StageProc4Neon <0>::process_sample_pos (const int nbr_coefs, float32x4_t &spl_0, float32x4_t &spl_1, StageDataNeon *stage_arr)
    //    {
    //        const int      cnt = nbr_coefs + 2;
    //
    //        stage_arr [cnt - 2]._mem4 = spl_0;
    //        stage_arr [cnt - 1]._mem4 = spl_1;
    //    }
    if(remaining == 0){
        const size_t cnt = numCoefficients + 2;
        
        filterStages[cnt - 2].mem = *sample0;
        filterStages[cnt - 1].mem = *sample1;
    }
    //    template <>
    //    inline void    StageProc4Neon <1>::process_sample_pos (const int nbr_coefs, float32x4_t &spl_0, float32x4_t &spl_1, StageDataNeon *stage_arr)
    //    {
    //        const int      cnt = nbr_coefs + 2 - 1;
    //
    //        const float32x4_t tmp_0 = vmlaq_f32 (
    //                                             stage_arr [cnt - 2]._mem4,
    //                                             spl_0 - stage_arr [cnt    ]._mem4,
    //                                             stage_arr [cnt    ]._coef4
    //                                             );
    //
    //        stage_arr [cnt - 2]._mem4 = spl_0;
    //        stage_arr [cnt - 1]._mem4 = spl_1;
    //        stage_arr [cnt    ]._mem4 = tmp_0;
    //
    //        spl_0 = tmp_0;
    //    }
    else if (remaining == 1){
        const size_t      cnt = numCoefficients + 2 - 1;
        
        const simd_float4 tmp_0 = simd_muladd(filterStages[cnt - 2].mem,
                                              *sample0 - filterStages[cnt].mem,
                                              filterStages[cnt].coef);
        
        filterStages[cnt-2].mem = *sample0;
        filterStages[cnt-1].mem = *sample1;
        filterStages[cnt].mem   = tmp_0;
        *sample0 = tmp_0;
    }
    //    template <int REMAINING>
    //    void    StageProc4Neon <REMAINING>::process_sample_pos (const int nbr_coefs, float32x4_t &spl_0, float32x4_t &spl_1, StageDataNeon *stage_arr)
    //    {
    //        const int      cnt = nbr_coefs + 2 - REMAINING;
    //
    //        const float32x4_t tmp_0 = vmlaq_f32 (
    //                                             stage_arr [cnt - 2]._mem4,
    //                                             spl_0 - stage_arr [cnt    ]._mem4,
    //                                             stage_arr [cnt    ]._coef4
    //                                             );
    //        const float32x4_t tmp_1 = vmlaq_f32 (
    //                                             stage_arr [cnt - 1]._mem4,
    //                                             spl_1 - stage_arr [cnt + 1]._mem4,
    //                                             stage_arr [cnt + 1]._coef4
    //                                             );
    //
    //        stage_arr [cnt - 2]._mem4 = spl_0;
    //        stage_arr [cnt - 1]._mem4 = spl_1;
    //
    //        spl_0 = tmp_0;
    //        spl_1 = tmp_1;
    //
    //        StageProc4Neon <REMAINING - 2>::process_sample_pos (
    //                                                            nbr_coefs,
    //                                                            spl_0,
    //                                                            spl_1,
    //                                                            stage_arr
    //                                                            );
    //    }
    
    else { // remaining > 1
        const size_t cnt = numCoefficients + 2 - remaining;
        const simd_float4 tmp_0 = simd_muladd(filterStages[cnt - 2].mem,
                                              *sample0 - filterStages[cnt].mem,
                                              filterStages[cnt].coef);
        const simd_float4 tmp_1 = simd_muladd(filterStages[cnt - 1].mem,
                                              *sample1 - filterStages[cnt+1].mem,
                                              filterStages[cnt+1].coef);
        filterStages[cnt-2].mem = *sample0;
        filterStages[cnt-1].mem = *sample1;
        
        *sample0 = tmp_0;
        *sample1 = tmp_1;
        
        BMHIIRStage_processSamplePos(remaining-2, numCoefficients, sample0, sample1, filterStages);
    }
}





#endif /* BMHIIRStage_h */
