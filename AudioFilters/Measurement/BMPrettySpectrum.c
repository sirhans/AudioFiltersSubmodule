//
//  BMPrettySpectrum.c
//  OscilloScope
//
//  Created by hans anderson on 8/19/20.
//  This file is public domain with no restrictions
//

#include "BMPrettySpectrum.h"
#include "Constants.h"
#include "BMSpectrogram.h"

#define BMPS_FFT_INPUT_LENGTH_48KHZ 8192
#define BMPS_DECAY_RATE_DB_PER_SECOND 24




/*!
 *BMPrettySpectrum_init
 */
void BMPrettySpectrum_init(BMPrettySpectrum *This, size_t maxOutputLength, float sampleRate){
	This->sampleRate = sampleRate;
	This->fftInputLength = BMPS_FFT_INPUT_LENGTH_48KHZ;
	This->timeSinceLastUpdate = 0.0f;
	
	// double the fft length if the sample rate is greater than 48 Khz
	if(sampleRate > 50000)
		This->fftInputLength *= 2;

	// add extra space in the circular buffer to avoid writing and reading the
	// same data at the same time
	This->bufferLength = 4 * This->fftInputLength;
	
	// init the circular buffer
	TPCircularBufferInit(&This->buffer, sizeof(float)*(uint32_t)This->bufferLength);
	
	// init the spectrum
	BMSpectrum_initWithLength(&This->spectrum, This->fftInputLength);
	
	// set the fftOutputLength
	This->fftOutputLength = 1 + This->fftInputLength / 2;
	
	// allocate space for the fft output buffers
	This->fftb1 = malloc(sizeof(float)*This->fftOutputLength);
	This->fftb2 = malloc(sizeof(float)*This->fftOutputLength);
	
	// allocate space for the final output buffers
	This->ob1 = malloc(sizeof(float)*maxOutputLength);
	This->ob2 = malloc(sizeof(float)*maxOutputLength);
	
	// set the decay rate for the spectrum graph to fall
	This->decayRateDbPerSecond = BMPS_DECAY_RATE_DB_PER_SECOND;
}




/*!
 *BMPrettySpectrum_free
 */
void BMPrettySpectrum_free(BMPrettySpectrum *This){
	TPCircularBufferCleanup(&This->buffer);
	BMSpectrum_free(&This->spectrum);
	
	free(This->fftb1);
	This->fftb1 = NULL;
	free(This->fftb2);
	This->fftb2 = NULL;
	free(This->ob1);
	This->ob1 = NULL;
	free(This->ob2);
	This->ob2 = NULL;
}




/*!
 *BMPrettySpectrum_inputBuffer
 *
 * This inserts samples into the buffer. It does not trigger any processing at
 * all. All processing is done from the graphics thread through
 */
void BMPrettySpectrum_inputBuffer(BMPrettySpectrum *This, const float *input, size_t length){
	
	// what is the size of the input in bytes?
	uint32_t bytesToInsert = (uint32_t)(sizeof(float)*length);
	
	// assert that we are not writing more data than the buffer can hold.
	// the danger of doing so is that the graphics thread may be reading while
	// this audio thread is writing and we want to ensure that they don't access
	// the same data simultaneously.
	assert(length < This->bufferLength - This->fftInputLength);
	
	// how many bytes are now available for reading?
	uint32_t bytesAvailableForReading;
	TPCircularBufferTail(&This->buffer, &bytesAvailableForReading);
	
	// if there are not enough readable bytes available to generate an output we
	// insert zeros into the buffer to make up the difference
	uint32_t bytesRequiredForSpectrum = (uint32_t)(sizeof(float)*This->fftInputLength);
	if(bytesAvailableForReading + bytesToInsert < bytesRequiredForSpectrum){
		
		// find out how many bytes we have to write
		uint32_t bytesWriting = bytesRequiredForSpectrum - (bytesAvailableForReading + bytesToInsert);
		
		// confirm that there is sufficient space to write that number of bytes
		uint32_t bytesAvailableForWriting;
		float *writePointer = TPCircularBufferHead(&This->buffer, &bytesAvailableForWriting);
		assert(bytesAvailableForWriting >= bytesWriting);
		
		// write zeros to the buffer
		memset(writePointer, 0, bytesWriting);
		
		// mark the bytes written
		TPCircularBufferProduce(&This->buffer, bytesWriting);
	}
	
	// if there are more than enough readable bytes available then we mark some
	// data as used to reduce the delay between the read and write pointers
	else if(bytesAvailableForReading + bytesToInsert > bytesRequiredForSpectrum) {
		TPCircularBufferConsume(&This->buffer, (bytesAvailableForReading + bytesToInsert) - bytesRequiredForSpectrum);
	}
	
	// confirm that there is space available for writing
	uint32_t bytesAvailableForWriting;
	TPCircularBufferHead(&This->buffer, &bytesAvailableForWriting);
	assert(bytesAvailableForReading >= bytesToInsert);
		
	// insert new data to the circular buffer
	TPCircularBufferProduceBytes(&This->buffer, (void*)input, bytesToInsert);
	
	// count time represented by the samples we have inserted
	This->timeSinceLastUpdate += (float)length / This->sampleRate;
}




/*!
 * BMPrettySpectrum_getOutput
 */
void BMPrettySpectrum_getOutput(BMPrettySpectrum *This,
								float *output,
								float minFreq, float maxFreq,
								enum BMPSScale scale,
								size_t outputLength){
	// how many bytes to compute the fft?
	uint32_t bytesRequiredForSpectrum = (uint32_t)This->fftInputLength*sizeof(float);
	
	// how much data is available for reading?
	uint32_t bytesAvailableForReading;
	float *readPointer = TPCircularBufferTail(&This->buffer, &bytesAvailableForReading);
	
	// if there is enough audio data in the buffer
	if(bytesAvailableForReading >= bytesRequiredForSpectrum){
		// compute the spectrum. Buffer into b1
		float nyquist;
		bool applyWindow = true;
		nyquist = BMSpectrum_processDataBasic(&This->spectrum, readPointer, This->fftb1, applyWindow, This->fftInputLength);
		This->fftb1[This->fftOutputLength - 1] = nyquist;
	}
	// if there is not enough data in the buffer, just write zeros to the output
	else
		memset(&This->fftb1,0,sizeof(float) * This->fftOutputLength);
	
	// find the decay rate for the FFT output
	float decay = BM_DB_TO_GAIN(This->decayRateDbPerSecond * This->timeSinceLastUpdate);
	
	// reset the decay timer
	This->timeSinceLastUpdate = 0.0f;
	
	// allow the old buffered data to decay in volume
	vDSP_vsmul(This->fftb2, 1, &decay, This->fftb2, 1, This->fftOutputLength);
	
	// if the new data is greater than the old data, replace the old with the new
	vDSP_vmax(This->fftb2, 1, This->fftb1, 1, This->fftb2, 1, This->fftOutputLength);
	
	// apply a threshold to the data so that zeros will not come out as -inf when we
	// convert to dB
	float minValue = BM_DB_TO_GAIN(-170.0f);
	vDSP_vthr(This->fftb2, 1, &minValue, This->fftb2, 1, This->fftOutputLength);
	
	// convert to decibels. buffer to b1
	float one = 1.0f;
	vDSP_vdbcon(This->fftb2, 1, &one, This->fftb1, 1, This->fftOutputLength, 0);
	
	// convert to bark scale and interpolate into the output buffer
//	BMSpectrogram_fftBinsToBarkScale(This->fftb1, This->ob1, This->fftInputLength, outputLength, This->minFreq, This->maxFreq, <#size_t fftBinInterpolationPadding#>, <#size_t upsampledPixels#>, <#const float *b3#>, <#const size_t *b4#>, <#const size_t *b5#>, <#const float *b6#>);
}
