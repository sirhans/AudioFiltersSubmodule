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
#define Therapy_Bell1_FC 900
#define Therapy_Bell2_FC 1300
#define Therapy_Bell_Gain 0
#define Therapy_Bell_Q 2.0

#define Therapy_Bell_Skirt_Min -40.0

#define Therapy_Bell_Skirt_Slow_Max -10.0
#define Therapy_Bell_Skirt_Fast_Max -20.0

#define Therapy_LFO_Slow_FC (5.0/1800.0)
#define Therapy_LFO_Fast_FC (13.0/1800.0)

void BMTherapyFilter_updateBellSkirt(BMTherapyFilter* This);

void BMTherapyFilter_init(BMTherapyFilter* This,bool stereo,float sampleRate){
	BMMultiLevelBiquad_init(&This->biquad, Therapy_Level, sampleRate, stereo, true, true);
	BMMultiLevelBiquad_setBellWithSkirt(&This->biquad, Therapy_Bell1_FC, Therapy_Bell_Q, Therapy_Bell_Gain, Therapy_Bell_Skirt_Min, Therapy_Bell1_Level);
	BMMultiLevelBiquad_setBellWithSkirt(&This->biquad, Therapy_Bell2_FC, Therapy_Bell_Q, Therapy_Bell_Gain, Therapy_Bell_Skirt_Min, Therapy_Bell2_Level);
	This->skirtSlowMaxDB = Therapy_Bell_Skirt_Slow_Max;
	This->skirtFastMaxDB = Therapy_Bell_Skirt_Fast_Max;
	This->skirtMinDB     = Therapy_Bell_Skirt_Min;
	
	BMQuadratureOscillator_init(&This->lfo, Therapy_LFO_Fast_FC, sampleRate);
	This->lfoBuffer = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
}

void BMTherapyFilter_destroy(BMTherapyFilter* This){
	free(This->lfoBuffer);
	This->lfoBuffer = nil;
}

void BMTherapyFilter_processBufferStereo(BMTherapyFilter* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t length){
	size_t samplesProcessed = 0;
	while (samplesProcessed<length) {
		size_t samplesProcessing = BM_MIN(length-samplesProcessed, BM_BUFFER_CHUNK_SIZE);
		BMMultiLevelBiquad_processBufferStereo(&This->biquad, inputL+samplesProcessed, inputR+samplesProcessed, outputL+samplesProcessed, outputR+samplesProcessed, samplesProcessing);
		
		//LFO
		BMQuadratureOscillator_process(&This->lfo, This->lfoBuffer, This->lfoBuffer, samplesProcessing);
		
        //Update Bell skirt
        BMTherapyFilter_updateBellSkirt(This);
		
		samplesProcessed += samplesProcessing;
	}
}

void BMTherapyFilter_processBufferMono(BMTherapyFilter* This,float* input,float* output,size_t length){
	size_t samplesProcessed = 0;
	while (samplesProcessed<length) {
		size_t samplesProcessing = BM_MIN(length-samplesProcessed, BM_BUFFER_CHUNK_SIZE);
		BMMultiLevelBiquad_processBufferMono(&This->biquad, input+samplesProcessed, output+samplesProcessed, samplesProcessing);
		
        //LFO
        BMQuadratureOscillator_process(&This->lfo, This->lfoBuffer, This->lfoBuffer, samplesProcessing);
        
        //Update Bell skirt
        BMTherapyFilter_updateBellSkirt(This);
		
		samplesProcessed += samplesProcessing;
	}
}

inline void BMTherapyFilter_updateBellSkirt(BMTherapyFilter* This){
    // calculate the skirt level at the first sample of this buffer
    float lfo0 = This->lfoBuffer[0];
    // lfo0 is in [-1,1]. We need to scale and shift to [skirtMin,skirtMax]
    //
    // scale lfo0 to [0,1]
    lfo0 = (lfo0 + 1.0f) * 0.5f;
    //
    // scale lfo0 to [0,skirtRangeDb]
    float skirtRangeDB = (This->skirtFastMaxDB - This->skirtMinDB);
    lfo0 *= skirtRangeDB;
    //
    // shift to get skirtLevel in [skirtMinDB, skirtMaxDB]
    float skirtLevel = lfo0 + This->skirtMinDB;
    
    // update the Biquad filter coefficients
    BMMultiLevelBiquad_setBellWithSkirt(&This->biquad, Therapy_Bell1_FC, Therapy_Bell_Q, Therapy_Bell_Gain, skirtLevel, Therapy_Bell1_Level);
    BMMultiLevelBiquad_setBellWithSkirt(&This->biquad, Therapy_Bell2_FC, Therapy_Bell_Q, Therapy_Bell_Gain, skirtLevel, Therapy_Bell2_Level);
}

