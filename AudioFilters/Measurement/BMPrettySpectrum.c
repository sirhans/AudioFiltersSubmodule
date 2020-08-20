//
//  BMPrettySpectrum.c
//  OscilloScope
//
//  Created by hans anderson on 8/19/20.
//  This file is public domain with no restrictions
//

#include "BMPrettySpectrum.h"

#define BMPS_FFT_INPUT_LENGTH 4096
#define BMPS_BUFFER_LENGTH BMPS_FFT_LENGTH * 2
#define BMPS_FFT_OUTPUT_LENGTH 1 + BMPS_FFT_INPUT_LENGTH/2


//typedef struct BMPrettySpectrum {
//	BMSpectrum spectrum;
//	TPCircularBuffer buffer;
//	float sampleRate, decayRate, minFreq, maxFreq;
//	size_t outputLength;
//	enum BMPSScale scale;
//} BMPrettySpectrum;



/*!
 *BMPrettySpectrum_init
 */
void BMPrettySpectrum_init(BMPrettySpectrum *This, size_t maxOutputLength, float sampleRate){
	This->sampleRate = sampleRate;
	
	// init the circular buffer
	TPCircularBufferInit(&This->buffer, sizeof(float)*BMPS_BUFFER_LENGTH);
	
	// init the spectrum
	BMSpectrum_initWithLength(&This->spectrum, BMPS_FFT_INPUT_LENGTH);
	
	// allocate space for the output buffers
	This->b1 = malloc(sizeof(float)*BMPS_FFT_OUTPUT_LENGTH);
	This->b2 = malloc(sizeof(float)*BMPS_FFT_OUTPUT_LENGTH);
}




/*!
 *BMPrettySpectrum_free
 */
void BMPrettySpectrum_free(BMPrettySpectrum *This){
	TPCircularBufferCleanup(&This->buffer);
	BMSpectrum_free(&This->spectrum);
	
	free(This->b1);
	This->b1 = NULL;
	free(This->b2);
	This->b2 = NULL;
}




/*!
 *BMPrettySpectrum_inputBuffer
 *
 * This inserts samples into the buffer. It does not trigger any processing at
 * all. All processing is done from the graphics thread through
 */
void BMPrettySpectrum_inputBuffer(BMPrettySpectrum *This, const float *input, size_t length){
	uint32_t bytesInserted = (uint32_t)(sizeof(float)*length);
	
	// insert new data to the circular buffer
	TPCircularBufferProduceBytes(&This->buffer, (void*)input, bytesInserted);
	
	// consume old data to allow it to be overwritten
	TPCircularBufferConsume(&This->buffer, bytesInserted);
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
	uint32_t bytesRequiredForSpectrum = BMPS_FFT_INPUT_LENGTH*sizeof(float);
	
	// check how many bytes are available for reading in the buffer
	uint32_t bytesAvailableForReading;
	float *readPointer = TPCircularBufferTail(&This->buffer, &bytesAvailableForReading);
	
	// if there are not enough bytes available to generate the output we will
	// insert zeros into the buffer
	if(bytesAvailableForReading < bytesRequiredForSpectrum){
		// find out how many bytes we have to write
		uint32_t bytesWriting = bytesRequiredForSpectrum - bytesAvailableForReading;
		
		// confirm that there is sufficient space to write that number of bytes
		uint32_t bytesAvailableForWriting;
		float *writePointer = TPCircularBufferHead(&This->buffer, &bytesAvailableForWriting);
		assert(bytesAvailableForWriting >= bytesWriting);
		
		// write zeros to the buffer
		memset(writePointer, 0, bytesWriting);
		
		// mark the bytes written
		TPCircularBufferProduce(&This->buffer, bytesWriting);
	}
	
	// confirm that there are now sufficient bytes available for reading and
	// update the read pointer
	readPointer = TPCircularBufferTail(&This->buffer, &bytesAvailableForReading);
	assert(bytesAvailableForReading >= bytesRequiredForSpectrum);
	
	// compute the spectrum. Buffer into b1
	float nyquist;
	BMSpectrum_processData(&This->spectrum, readPointer, This->b1, BMPS_FFT_INPUT_LENGTH, &outputLength, &nyquist);
	This->b1[BMPS_FFT_INPUT_LENGTH/2] = nyquist;
	
	// allow the old buffered data to decay in volume
	vDSP_vsmul(This->b2, 1, &This->decayRate, This->b2, 1, BMPS_FFT_OUTPUT_LENGTH);
	
	// if the new data is greater than the old data, replace the old with the new
	vDSP_vmax(This->b2, 1, This->b1, 1, This->b2, 1, BMPS_FFT_OUTPUT_LENGTH);
	
	// convert to decibels
	vDSP_vdbcon(<#const float * _Nonnull __A#>, <#vDSP_Stride __IA#>, <#const float * _Nonnull __B#>, <#float * _Nonnull __C#>, <#vDSP_Stride __IC#>, <#vDSP_Length __N#>, <#unsigned int __F#>);
}
