//
//  BMHoldPeak.h
//  AudioFiltersXcodeProject
//
//  This class monitors the peak level of a number that updates frequently,
//  usually a level meter, for the purpose of displaying the recent peak value
//  as a number printed on the screen. It holds the peak value for a specified
//  time interval and updates it whenever the new value is larger than the
//  current value or whenever the time limit expires.
//
//  Created by hans anderson on 6/14/19.
//  This file may be used by anyone without restrictions of any kind.
//

#ifndef BMHoldPeak_h
#define BMHoldPeak_h

#include <stdio.h>


typedef struct BMHoldPeak {
    float maxValL, maxValR, peakHoldTimeLimit, peakHoldTimeL, peakHoldTimeR;
} BMHoldPeak;


/*!
 *BMHoldPeak_init
 *
 * @param holdTimeLimit_seconds  sets the amount of time a peak value holds before it can be updated with a lower value
 */
void BMHoldPeak_init(BMHoldPeak* This, float holdTimeLimit_seconds);



/*!
 *BMHoldPeak_updateStereo
 *
 * @param currentValL   the current value of the realtime indicator on Left channel
 * @param currentValR   the current value of the realtime indicator on Right channel
 * @param fixedPeakL    (output) the held peak value Left channel
 * @param fixedPeakR    (output) the held peak value Right channel
 * @param timeSinceLastUpdate_s   usually, this is equal to bufferSize / sampleRate
 */
void BMHoldPeak_updateStereo(BMHoldPeak* This,
                             float currentValL, float currentValR,
                             float* fixedPeakL, float* fixedPeakR,
                             float timeSinceLastUpdate_s);



void BMHoldPeak_updateMono(BMHoldPeak* This,
                           float currentVal,
                           float* fixedPeak,
                           float timeSinceLastUpdate_s);

#endif /* BMHoldPeak_h */
