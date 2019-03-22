//
//  BMVelvetNoise.h
//  BMAudioFilters
//
// This class sets delay tap times for "Velvet Noise", used in reverb
// and multi-tap delays.
//
// See: "Reverberation modeling using velvet noise" by M. Karjalainen
//    and Hanna Järveläinen.
//    https://www.researchgate.net/publication/283247924_Reverberation_modeling_using_velvet_noise
//
//  Created by Hans on 1/6/17.
//
//  The author releases this file into the public domain with no restrictions
//  of any kind.
//

#ifdef __cplusplus
extern "C" {
#endif


#ifndef BMVelvetNoise_h
#define BMVelvetNoise_h
    
#include <Accelerate/Accelerate.h>

    
/*
 * Set tap indices using Velvet Noise method
 *
 * (See: "Reverberation modeling using velvet noise" by M. Karjalainen
 * and Hanna Järveläinen.
 * https://www.researchgate.net/publication/283247924_Reverberation_modeling_using_velvet_noise
 *
 * @param startTimeMS   the first tap time will start soon after this
 * @param endTimeMS     the last tap will end before this time
 * @param indicesOut    array of indices for setting delay taps
 * @param sampleRate    sample rate of the audio system
 * @param numTaps       length of indicesOut
 */
void BMVelvetNoise_setTapIndices(float startTimeMS,
                                 float endTimeMS,
                                 size_t* indicesOut,
                                 float sampleRate,
                                 size_t numTaps);

//No allocate version
void BMVelvetNoise_setTapIndicesNA(float startTimeMS,
                                   float endTimeMS,
                                   size_t* indicesOut,
                                   float* tempStorage,
                                   float sampleRate,
                                   size_t numTaps);
    
    /*
     * Set the values in tapSigns randomly to -1 and 1, with an equal
     * number of + and - values.
     *
     * @param tapSigns   input array
     * @param numTaps    length of tapSigns
     */
void BMVelvetNoise_setTapSigns(float* tapSigns,
                               size_t numTaps);
    

#endif /* BMVelvetNoise_h */



#ifdef __cplusplus
}
#endif
