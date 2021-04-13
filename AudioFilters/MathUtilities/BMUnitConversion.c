//
//  BMUnitConversion.c
//  OscilloScope
//
//  Created by hans anderson on 9/8/20.
//  This file is public domain.
//

#include <math.h>
#include <Accelerate/Accelerate.h>

#define BM_DB_TO_GAIN(db) pow(10.0,db/20.0)
#define BM_GAIN_TO_DB(gain) log10f(gain)*20.0


float BMConv_dBToGain(float db){
	return powf(10.0f, db / 20.0f);
}


double BMConv_dBToGain_d(double db){
	return pow(10.0, db / 20.0);
}


float BMConv_gainToDb(float gain){
	return log10f(gain)*20.0f;
}


double BMConv_gainToDb_d(double gain){
	return log10(gain)*20.0;
}


float BMConv_hzToBark(float hz){
	// hzToBark[hz_] := 6 ArcSinh[hz/600]
	return 6.0f * asinhf(hz / 600.0f);
}


float BMConv_barkToHz(float bark){
	// barkToHz[bk_] := 600 Sinh[bk/6]
	return 600.0f * sinhf(bark / 6.0f);
}



float BMConv_hzToFFTBin(float hz, float fftSize, float sampleRate){
	// fftBinZeroDC[f_, fftLength_, sampleRate_] := f/(sampleRate/fftLength)
	return hz * (fftSize / sampleRate);
}


// how many fft bins are represented by a single pixel at freqHz?
float BMConv_fftBinsPerPixel(float freqHz,
					  float fftSize,
					  float pixelHeight,
					  float minFrequency,
					  float maxFrequency,
					  float sampleRate){
	
	// what bark frequency range does 1 pixel represent?
	float windowHeightInBarks = BMConv_hzToBark(maxFrequency) - BMConv_hzToBark(minFrequency);
	float pixelHeightInBarks = windowHeightInBarks / pixelHeight;
	
	// what frequency is one pixel above freqHz?
	float upperPixelFreq = BMConv_barkToHz(BMConv_hzToBark(freqHz)+pixelHeightInBarks);
	
	// return the height of one pixel in FFT bins
	return BMConv_hzToFFTBin(upperPixelFreq, fftSize, sampleRate) - BMConv_hzToFFTBin(freqHz, fftSize, sampleRate);
}

void BMConv_dBToGainV(const float *input, float *output, size_t numSamples){
    int numSamplesI = (int)numSamples;
    float magicNumber = 0.16609640474f;
    vDSP_vsmul(input, 1, &magicNumber, output, 1, numSamples);
    vvexp2f(output, output, &numSamplesI);
}

#include "BMUnitConversion.h"
