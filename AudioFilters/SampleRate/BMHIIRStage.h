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

#ifndef BMHIIRStageData_h
#define BMHIIRStageData_h

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

typedef struct BMHIIRStageData {
    simd_float4 coef;
    simd_float4 mem;
} BMHIIRStageData;

#endif /* BMHIIRStageData_h */
