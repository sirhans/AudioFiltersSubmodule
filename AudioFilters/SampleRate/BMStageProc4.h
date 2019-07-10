//
//  BMStageProc4.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/10/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifndef BMStageProc4_h
#define BMStageProc4_h

#include <MacTypes.h>
#include <simd/simd.h>


/*
 * ported from:
 *    StageProc4Neon.hpp
 *    in the HIIR library by Laurent de Soras (http://ldesoras.free.fr/prod.html)
 */
static inline void BMStageProc(size_t remaining, size_t numCoefficients, simd_float4* sample0, simd_float4* sample1, simd_float4* stageArr){
    
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
        
        stageArr[cnt - 2] = *sample0;
        stageArr[cnt - 1] = *sample1;
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
        
        const simd_float4 tmp_0 = simd_muladd(stageArr[cnt - 2],
                                              *sample0 - stageArr[cnt],
                                              stageArr[cnt]);
        
        stageArr[cnt-2] = *sample0;
        stageArr[cnt-1] = *sample1;
        stageArr[cnt]   = tmp_0;
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
        const simd_float4 tmp_0 = simd_muladd(stageArr [cnt - 2],
                                              *sample0 - stageArr[cnt],
                                              stageArr [cnt]);
        const simd_float4 tmp_1 = simd_muladd(stageArr [cnt - 1],
                                              *sample1 - stageArr[cnt+1],
                                              stageArr[cnt+1]);
        stageArr[cnt-2] = *sample0;
        stageArr[cnt-1] = *sample1;
        *sample0 = tmp_0;
        *sample1 = tmp_1;
        
        BMStageProc(remaining-2, numCoefficients, sample0, sample1, stageArr);
    }
}

#endif /* BMStageProc4_h */
