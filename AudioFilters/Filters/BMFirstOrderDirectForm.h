//
//  BMFirstOrderLowpassDirectForm.h
//  SaturatorAU
//
//  Created by hans anderson on 9/29/19.
//  Copyright © 2019 bluemangoo. All rights reserved.
//

#ifndef BMFirstOrderLowpassDirectForm_h
#define BMFirstOrderLowpassDirectForm_h

#include <stdio.h>
#include <simd/simd.h>

typedef struct BMFirstOrderLowpassDirectForm {
    float z1_f, b0_f, b1_f, a1_f;
    simd_float2 z1_f2, b0_f2, b1_f2, a1_f2;
    simd_float4 z1_f4, b0_f4, b1_f4, a1_f4;
    float sampleRate;
} BMFirstOrderLowpassDirectForm;


void BMFirstOrderLowpassDirectForm_process(BMFirstOrderLowpassDirectForm* This,
                                           )

#endif /* BMFirstOrderLowpassDirectForm_h */
