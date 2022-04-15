//
//  BMTherapyFilter.c
//  BMAudioTherapy
//
//  Created by Nguyen Minh Tien on 07/04/2022.
//

#include "BMTherapyFilter.h"
#include "Constants.h"

//2 Bell filter Biquad
//Default : filter 1 : fc 850hz
//Filter 2 : fc 1500Hz
//    Q=1.8
//    Fc1=850Hz
//    Fc2=1500Hz
//    Gain=0dB
//    Skirt gain moves between -20dB and 0 dB
//    I recommend using BMLFO freq : LFO does over a time period of 1800 seconds
//    In figure 5 I count about 13 oscillations of the LFO in 1800 seconds
//    So (13/1800) Hz
//    Slowest LFO setting: 5/1800 Hz. Range: [-20,0]dB
//    Fastest LFO Setting:
//    13/1800 Hz. Range: [-20,-12] dB
//    When it’s fast, the range is not so wide. The reason is to limit the amount of dB it can move in one minute. So it moves at almost the same speed most of the time.

#define Therapy_Level 2
#define Therapy_Bell1_Level 0
#define Therapy_Bell2_Level 1
#define Therapy_Bell1_FC 850
#define Therapy_Bell2_FC 1500
#define Therapy_Bell_Gain 0
#define Therapy_Bell_Q 1.8

#define Therapy_LFO_Slow_Min -20
#define Therapy_LFO_Slow_Max 0
#define Therapy_LFO_Slow_FC (5.0/1800)

#define Therapy_LFO_Fast_Min -20
#define Therapy_LFO_Fast_Max -12
#define Therapy_LFO_Fast_FC (13.0/1800)

void BMTherapyFilter_init(BMTherapyFilter* This,bool stereo,float sampleRate){
    BMMultiLevelBiquad_init(&This->biquad, Therapy_Level, sampleRate, stereo, true, true);
    BMMultiLevelBiquad_setBellWithSkirt(&This->biquad, Therapy_Bell1_FC, Therapy_Bell_Q, Therapy_Bell_Gain, Therapy_LFO_Slow_Min, Therapy_Bell1_Level);
    BMMultiLevelBiquad_setBellWithSkirt(&This->biquad, Therapy_Bell2_FC, Therapy_Bell_Q, Therapy_Bell_Gain, Therapy_LFO_Slow_Min, Therapy_Bell2_Level);
    
    BMQuadratureOscillator_init(&This->lfo, Therapy_LFO_Slow_FC, sampleRate);
    This->lfoBuffer = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
}

void BMTherapyFilter_processBufferStereo(BMTherapyFilter* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t length){
    size_t samplesProcessed = 0;
    while (samplesProcessed<length) {
        size_t samplesProcessing = BM_MIN(length-samplesProcessed, BM_BUFFER_CHUNK_SIZE);
        BMMultiLevelBiquad_processBufferStereo(&This->biquad, inputL+samplesProcessed, inputR+samplesProcessed, outputL+samplesProcessed, outputR+samplesProcessed, samplesProcessing);
        
        BMQuadratureOscillator_process(&This->lfo, This->lfoBuffer, This->lfoBuffer, samplesProcessing);
        
        vDSP_vmul(outputL+samplesProcessed, 1, This->lfoBuffer, 1, outputL+samplesProcessed, 1, samplesProcessing);
        vDSP_vmul(outputR+samplesProcessed, 1, This->lfoBuffer, 1, outputR+samplesProcessed, 1, samplesProcessing);
        
        samplesProcessed += samplesProcessing;
    }
}

void BMTherapyFilter_processBufferMono(BMTherapyFilter* This,float* input,float* output,size_t length){
    size_t samplesProcessed = 0;
    while (samplesProcessed<length) {
        size_t samplesProcessing = BM_MIN(length-samplesProcessed, BM_BUFFER_CHUNK_SIZE);
        BMMultiLevelBiquad_processBufferMono(&This->biquad, input+samplesProcessed, output+samplesProcessed, samplesProcessing);
        
        BMQuadratureOscillator_process(&This->lfo, This->lfoBuffer, This->lfoBuffer, samplesProcessing);
        
        vDSP_vmul(output+samplesProcessed, 1, This->lfoBuffer, 1, output+samplesProcessed, 1, samplesProcessing);
        
        samplesProcessed += samplesProcessing;
    }
}

