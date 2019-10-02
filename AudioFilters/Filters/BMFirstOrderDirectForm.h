//
//  BMFirstOrderDirectForm.h
//  SaturatorAU
//
//  Created by hans anderson on 9/29/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMFirstOrderDirectForm_h
#define BMFirstOrderDirectForm_h

#include <stdio.h>
#include <simd/simd.h>

typedef struct BMFirstOrderDirectForm {
    float z1_f, b0_f, b1_f, a1_f, az_f;
    simd_float2 z1_f2, b0_f2, b1_f2, a1_f2, az_f2;
    simd_float4 z1_f4, b0_f4, b1_f4, a1_f4, az_f4;
    float sampleRate;
} BMFirstOrderDirectForm;

/*!
 *BMFirstOrderDirectForm_processM
 */
void BMFirstOrderDirectForm_processM(BMFirstOrderDirectForm* This,
                                           const float** input,
                                           float** output,
                                           size_t numChannels,
                                           size_t numSamples);


/*!
 *BMFirstOrderDirectForm_process1
 */
void BMFirstOrderDirectForm_process1(BMFirstOrderDirectForm* This,
                                           const float* input,
                                           float* output,
                                           size_t numSamples);


/*!
 *BMFirstOrderDirectForm_init
 *
 * @abstract this only sets the delays to zero and sets the sample rate. after calling it you still need to set a filter type.
 */
void BMFirstOrderDirectForm_init(BMFirstOrderDirectForm* This, float sampleRate);



/*!
 *BMFirstOrderDirectForm_setLowpass
 */
void BMFirstOrderDirectForm_setLowpass(BMFirstOrderDirectForm* This, float fc);




#endif /* BMFirstOrderLowpassDirectForm_h */
