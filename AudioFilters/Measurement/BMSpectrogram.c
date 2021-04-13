//
//  BMSpectrogram.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/15/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMSpectrogram.h"
#include "BMIntegerMath.h"
#include "Constants.h"
#include "BMUnitConversion.h"
#include <sys/qos.h>

#define BMSG_BYTES_PER_PIXEL 4
#define BMSG_FLOATS_PER_COLOUR 3
#define SG_MIN(a,b) (((a)<(b))?(a):(b))
#define SG_MAX(a,b) (((a)>(b))?(a):(b))

void BMSpectrogram_init(BMSpectrogram *This,
						size_t maxFFTSize,
						size_t maxImageHeight,
						float sampleRate){
	
	This->sampleRate = sampleRate;
	This->prevMinF = This->prevMaxF = 0.0f;
	This->prevImageHeight = This->prevFFTSize = 0;
	
	// init one spectrum object per thread
	for(size_t i=0; i<BMSG_NUM_THREADS; i++)
		BMSpectrum_initWithLength(&This->spectrum[i], maxFFTSize);
	
	This->maxFFTSize = maxFFTSize;
	This->maxImageHeight = maxImageHeight;
	
	// we need some extra samples at the end of the fftBin array because the
	// interpolating function that converts from linear scale to bark scale
	// reads beyond the interpolation index
	This->fftBinInterpolationPadding = 3;
	
	size_t maxFFTOutput = 1 + maxFFTSize/2;
	for(size_t i=0; i<BMSG_NUM_THREADS; i++){
		This->b1[i] = malloc(sizeof(float)*(maxFFTOutput+This->fftBinInterpolationPadding));
		This->b2[i] = malloc(sizeof(float)*maxImageHeight);
		This->t1[i] = malloc(sizeof(float)*maxImageHeight*BMSG_FLOATS_PER_COLOUR);
		This->t2[i] = malloc(sizeof(float)*maxImageHeight);
	}
	This->b3 = malloc(sizeof(float)*maxImageHeight);
	This->b4 = malloc(sizeof(size_t)*maxImageHeight);
	This->b5 = malloc(sizeof(size_t)*maxImageHeight);
	This->colours = malloc(sizeof(simd_float3)*maxImageHeight);
	
	if(BMSG_NUM_THREADS > 1){
		// get the global concurrent dispatch queue with highest priority
		This->globalQueue = dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE,0);
		
		// create a new dispatch group
		This->dispatchGroup = dispatch_group_create();
	}
}

void BMSpectrogram_free(BMSpectrogram *This){
	// free per-thread resources
	for(size_t i=0; i<BMSG_NUM_THREADS; i++){
		free(This->b1[i]);
		free(This->b2[i]);
		free(This->t1[i]);
		free(This->t2[i]);
		This->b1[i] = NULL;
		This->b2[i] = NULL;
		This->t1[i] = NULL;
		This->t2[i] = NULL;
		BMSpectrum_free(&This->spectrum[i]);
	}
	
	if(BMSG_NUM_THREADS > 1){
		dispatch_release(This->dispatchGroup);
	}
	
	free(This->b3);
	free(This->b4);
	free(This->b5);
	free(This->colours);
	This->b3 = NULL;
	This->b4 = NULL;
	This->b5 = NULL;
	This->colours = NULL;
}




void BMSpectrogram_updateImageHeight(BMSpectrogram *This,
									 size_t fftSize,
									 size_t imageHeight,
									 float minF,
									 float maxF){
	// rename buffer 3 and 4 for readablility
	float* interpolatedIndices = This->b3;
	size_t* startIndices = This->b4;
	size_t* binIntervalLengths = This->b5;
	
	// if the settings have changed since last time we did this, recompute
	// the floating point array indices for interpolated reading of bark scale
	// frequency spectrum
	if(minF != This->prevMinF ||
	   maxF != This->prevMaxF ||
	   imageHeight != This->prevImageHeight ||
	   fftSize != This->prevFFTSize){
		This->prevMinF = minF;
		This->prevMaxF = maxF;
		This->prevImageHeight = imageHeight;
		This->prevFFTSize = fftSize;
		
		// find the floating point fft bin indices to be used for interpolated
		// copy of the fft output to bark scale frequency spectrogram output
		BMSpectrum_barkScaleFFTBinIndices(interpolatedIndices, fftSize, minF,maxF, This->sampleRate, imageHeight);
		
		// find the start and end indices for the region of the output in which
		// we are downsampling from the fft bins to the output image
		BMSpectrum_fftDownsamplingIndices(startIndices, binIntervalLengths, interpolatedIndices, minF, maxF, This->sampleRate, fftSize, imageHeight);
		
		// find the number of output pixels that are upsampled. The remaining
		// pixels will be downsampled.
		This->upsampledPixels = BMSpectrum_numUpsampledPixels(fftSize, minF, maxF, This->sampleRate, imageHeight);
	}
}




void BMSpectrogram_fftBinsToBarkScale(const float* fftBins,
									  float *output,
									  size_t fftSize,
									  size_t outputLength,
									  float minFrequency,
									  float maxFrequency,
									  size_t fftBinInterpolationPadding,
									  size_t upsampledPixels,
									  const float *interpolatedIndices,
									  const size_t *startIndices,
									  const size_t *binIntervalLengths){
	
	/*************************************
	 * now that we have the fft bin indices we can interpolate and get the results
	 *************************************/
	size_t numFFTBins = 1 + fftSize/2;
	assert(numFFTBins + fftBinInterpolationPadding - 2 >= floor(interpolatedIndices[outputLength-1])); // requirement of vDSP_vqint. can be ignored if there is extra space at the end of the fftBins array and zeros are written there.
	
	// upsample the lower part of the image
	vDSP_vqint(fftBins, interpolatedIndices, 1, output, 1, upsampledPixels, numFFTBins);
	
	// if we didn't get it all, downsample the upper half
	if (upsampledPixels < outputLength){
		// get the sum of squares of all the bins represented by the ith pixel
		for(size_t i=upsampledPixels; i<outputLength; i++){
//			vDSP_svesq(fftBins + startIndices[i], 1, output + i, binIntervalLengths[i]);
//            vDSP_sve(fftBins + startIndices[i], 1, output + i, binIntervalLengths[i]);
            vDSP_maxv(fftBins + startIndices[i], 1, output + i, binIntervalLengths[i]);
		}
//
//		// scale and square root to get a scaled norm of downsampled pixels
//		float *downsampledOutput = output + upsampledPixels;
//		int numDownsampledPixels_i = (int)(outputLength - upsampledPixels);
//		size_t numDownsampledPixels_ui = numDownsampledPixels_i;
//		vDSP_vmul(downsampledOutput, 1,
//				  downsamplingScales + upsampledPixels, 1,
//				  downsampledOutput, 1, numDownsampledPixels_ui);
//		vvsqrtf(downsampledOutput,
//				downsampledOutput,
//                &numDownsampledPixels_i);
	}
}





void BMSpectrogram_toRGBAColour(float* input, float *temp1, float *temp2, uint8_t *output, size_t pixelWidth, size_t pixelHeight){
	
	// fill the temp buffer with the rgb colour of the 100% saturated pixels
	float r = 0.0f;
	float g = 0.433f;
	float b = 1.0f;
	vDSP_vfill(&r, temp1, BMSG_FLOATS_PER_COLOUR, pixelHeight);
	vDSP_vfill(&g, temp1+1, BMSG_FLOATS_PER_COLOUR, pixelHeight);
	vDSP_vfill(&b, temp1+2, BMSG_FLOATS_PER_COLOUR, pixelHeight);
	
	// find out how much we need to scale down the saturated pixel to apply the
	// saturation and make headroom for the lightness
	//    float s = 0.5f;
	//    float c = (fabsf(2.0f * v - 1.0f) - 1.0f) * (-s);
	float s = 0.5f;
	float negS = -0.5f;
	float negOne = -1.0f;
	float two = 2.0f;
	// 2.0f * v - 1.0f
	vDSP_vsmsa(input, 1, &two, &negOne, temp2, 1, pixelHeight);
	// fabsf
	vDSP_vabs(temp2, 1, temp2, 1, pixelHeight);
	// (result - 1.0f) * -s
	// => result * -s + s
	vDSP_vsmsa(temp2, 1, &negS, &s, temp2, 1, pixelHeight);
	
	// scale the rgb pixel and mix with the lightness
	//    rgb = (rgb - 0.5f) * c + v;
	//
	// rgb - 0.5f
	float negHalf = -0.5f;
	vDSP_vsadd(temp1, 1, &negHalf, temp1, 1, pixelHeight*BMSG_FLOATS_PER_COLOUR);
	// result * c + v
	vDSP_vma(temp1, BMSG_FLOATS_PER_COLOUR, temp2, 1, input, 1, temp1, BMSG_FLOATS_PER_COLOUR, pixelHeight);
	vDSP_vma(temp1+1, BMSG_FLOATS_PER_COLOUR, temp2, 1, input, 1, temp1+1, BMSG_FLOATS_PER_COLOUR, pixelHeight);
	vDSP_vma(temp1+2, BMSG_FLOATS_PER_COLOUR, temp2, 1, input, 1, temp1+2, BMSG_FLOATS_PER_COLOUR, pixelHeight);
	
	// convert to 8 bit RGBA colour
	float two55 = 255.0;
	vDSP_vsmul(temp1, 1, &two55, temp1, 1, BMSG_FLOATS_PER_COLOUR*pixelHeight);
	vDSP_vfixru8(temp1, BMSG_FLOATS_PER_COLOUR, output, BMSG_BYTES_PER_PIXEL, pixelHeight);
	vDSP_vfixru8(temp1+1, BMSG_FLOATS_PER_COLOUR, output+1, BMSG_BYTES_PER_PIXEL, pixelHeight);
	vDSP_vfixru8(temp1+2, BMSG_FLOATS_PER_COLOUR, output+2, BMSG_BYTES_PER_PIXEL, pixelHeight);
	
	// write the alpha values
	size_t outputSizeInBytes = BMSG_BYTES_PER_PIXEL * pixelHeight;
	for(size_t i=3; i<outputSizeInBytes; i+= BMSG_BYTES_PER_PIXEL)
		output[i] = 255;
}





void BMSpectrogram_toDbScaleAndClip(const float* input, float* output, size_t fftSize, size_t length){
	// compensate for the scaling of the vDSP FFT
	float scale = 32.0f / (float)fftSize;
	vDSP_vsmul(input, 1, &scale, output, 1, length);
	
	// eliminate zeros and negative values
	float threshold = BM_DB_TO_GAIN(-110);
	vDSP_vthr(output, 1, &threshold, output, 1, length);
	
	// convert to db
	float zeroDb = sqrt((float)fftSize)/2.0f;
	uint32_t use20not10 = 1;
	vDSP_vdbcon(output, 1, &zeroDb, output, 1, length, use20not10);
	
	// clip to [-105,100]
	float min = -105.0f;
	float max = 100.0f;
	vDSP_vclip(output, 1, &min, &max, output, 1, length);
	
	// shift and scale [-105,0] to [0,1]
	scale = 1.0f/fabsf(min);
	float shift = 1.0f;
	vDSP_vsmsa(output, 1, &scale, &shift, output, 1, length);
}




float BMSpectrogram_getPaddingLeft(size_t fftSize){
	if(4 > fftSize)
		printf("[BMSpectrogram] WARNING: fftSize must be >=4\n");
	if(!isPowerOfTwo((size_t)fftSize))
		printf("[BMSpectrogram] WARNING: fftSize must be a power of two\n");
	return (fftSize/2) - 1;
}


float BMSpectrogram_getPaddingRight(size_t fftSize){
	if(4 > fftSize)
		printf("[BMSpectrogram] WARNING: fftSize must be >=4\n");
	if(!isPowerOfTwo((size_t)fftSize))
		printf("[BMSpectrogram] WARNING: fftSize must be a power of two\n");
	return fftSize/2;
}




void BMSpectrogram_genColumn(SInt32 i,
							 uint8_t *imageOutput,
							 float fftStride,
							 SInt32 fftStartIndex,
							 SInt32 pixelWidth,
							 SInt32 pixelHeight,
							 SInt32 fftEndIndex,
							 SInt32 fftSize,
							 SInt32 fftOutputSize,
							 size_t fftBinInterpolationPadding,
							 float minFrequency,
							 float maxFrequency,
							 size_t upsampledPixels,
							 BMSpectrum *spectrum,
							 const float *inputAudio,
							 float *b1,
							 float *b2,
							 float *t1,
							 float *t2,
							 const float *b3,
							 const size_t *b4,
							 const size_t *b5){
	// find the index where the current fft window starts
	SInt32 fftCurrentWindowStart = (SInt32)roundf((float)i*fftStride + FLT_EPSILON) + fftStartIndex;
	
	// set the last window to start in the correct place in case the multiplication above isn't accurate
	if(i==pixelWidth-1)
		fftCurrentWindowStart = fftEndIndex - fftSize + 1;
	
	// take abs(fft(windowFunctin*windowSamples))
	b1[fftOutputSize-1] = BMSpectrum_processDataBasic(spectrum,
													  inputAudio + fftCurrentWindowStart,
													  b1,
													  true,
													  fftSize);
	
	// write some zeros after the end as padding for the interpolation function
	memset(b1 + fftOutputSize, 0, sizeof(float)*fftBinInterpolationPadding);
	
	// convert to dB, scale to [0,1] and clip values outside that range
	BMSpectrogram_toDbScaleAndClip(b1, b1, fftSize, fftOutputSize);
	
	// interpolate the output from linear scale frequency to Bark Scale
	BMSpectrogram_fftBinsToBarkScale(b1,
									 b2,
									 fftSize,
									 pixelHeight,
									 minFrequency,
									 maxFrequency,
									 fftBinInterpolationPadding,
									 upsampledPixels,
									 b3,
									 b4,
									 b5);
	
	// clamp the outputs to [0,1]
	float lowerLimit = 0;
	float upperLimit = 1;
	vDSP_vclip(b2, 1, &lowerLimit, &upperLimit, b2, 1, pixelHeight);
	
	// reverse
	vDSP_vrvrs(b2, 1, pixelHeight);
	
	// convert to RGBA colours and write to output
	BMSpectrogram_toRGBAColour(b2, t1, t2, &imageOutput[i*BMSG_BYTES_PER_PIXEL*pixelHeight],pixelWidth, pixelHeight);
}




void BMSpectrogram_shiftColumns(BMSpectrogramCache *cache, int width, int height, int shift){
	if(abs(shift) < width && shift != 0) {
		int32_t bytesOffset = shift * height * BMSG_BYTES_PER_PIXEL;
		TPCircularBufferShift(&cache->cBuffer, bytesOffset);
		
//		size_t columnsToMove = width - abs(shift);
//		size_t bytesToMove = columnsToMove * height * BMSG_BYTES_PER_PIXEL;
//		const uint8_t *src = NULL;
//		uint8_t *dest = NULL;
//
//		uint32_t bytes;
//		uint8_t *imageCache = TPCircularBufferTail(&cache->cBuffer, &bytes);
		
		// left shift
//		if (shift > 0){
//			TPCircularBufferConsume(&cache->cBuffer, (uint32_t)bytesOffset);
//			TPCircularBufferProduce(&cache->cBuffer, (uint32_t)bytesOffset);
//		}

		// right shift
//		else if (shift < 0) {
			//			// copy from beginning of input
//			src = imageCache;
//			// write to the output with offset
//			dest = imageCache + bytesOffset;
//
//			// do the shift
//			memmove(dest, src, bytesToMove);
//		}
	}
}





void BMSpectrogram_transposeImage(const uint8_t *imageInput, uint8_t *imageOutput, size_t inputWidth, size_t inputHeight){
	// confirm that float32 has the same number of bytes we use for a single pixel
	// so that we can operate on each pixel as a single int to simplify the operation
	assert(sizeof(Float32) == BMSG_BYTES_PER_PIXEL);
	
	// use floating point matrix transposition
	vDSP_mtrans((Float32*)imageInput, 1, (Float32*)imageOutput, 1, inputHeight, inputWidth);
}




void BMSpectrogram_process(BMSpectrogram *This,
						   const float* inputAudio,
						   SInt32 inputLength,
						   SInt32 startSampleIndex,
						   SInt32 endSampleIndex,
						   SInt32 fftSize,
						   uint8_t *imageOutput,
						   SInt32 pixelWidth,
						   SInt32 pixelHeight,
						   float minFrequency,
						   float maxFrequency){
	assert(4 <= fftSize && fftSize <= This->maxFFTSize);
	assert(isPowerOfTwo((size_t)fftSize));
	assert(2 <= pixelHeight && pixelHeight < This->maxImageHeight);
	
	// we will need this later
	SInt32 fftOutputSize = 1 + fftSize / 2;
	
	// check that the fft start and end indices are within the bounds of the input array
	// note that we define the fft centred at sample n to mean that the fft is
	// calculated on samples in the interval [n-(fftSize/2)+1, n+(fftSize/2)]
	SInt32 fftStartIndex = startSampleIndex - (fftSize/2) + 1;
	SInt32 fftEndIndex = endSampleIndex + (fftSize/2);
	assert(fftStartIndex >= 0 && (fftEndIndex-fftStartIndex+1) <= inputLength);
	
	// calculate the FFT stride to get the pixelWidth. this is the number of
	// samples we shift the fft window each time we compute it. It may take a
	// non-integer value. If stride is a non-integer then the actual stride used
	// will still be an integer but it will vary between two consecutive values.
	float sampleWidth = endSampleIndex - startSampleIndex + 1;
	float fftStride = sampleWidth / (float)(pixelWidth-1);
	if(pixelWidth == 1)
		fftStride = 0;
	
	// if the configuration has changed, update some stuff
	BMSpectrogram_updateImageHeight(This, fftSize, pixelHeight, minFrequency, maxFrequency);
	
	if(BMSG_NUM_THREADS > 1){
		// create threads for parallel processing of spectrogram
		SInt32 blockStartIndex = 0;
		for(size_t j=0; j<BMSG_NUM_THREADS; j++){
			// divide into BMSG_NUM_THREADS blocks with roughly equal size.
			SInt32 nextBlockStartIndex = (pixelWidth * (SInt32)(j + 1)) / BMSG_NUM_THREADS;
			
			// ensure that the last block doesn't exceed the width of the image
			if(nextBlockStartIndex > pixelWidth)
				nextBlockStartIndex = pixelWidth;
			
			// add a block to the group and start processing
			dispatch_group_async(This->dispatchGroup, This->globalQueue, ^{
				for(SInt32 i=blockStartIndex; i<nextBlockStartIndex; i++){
					BMSpectrogram_genColumn(i,
											imageOutput,
											fftStride,
											fftStartIndex,
											pixelWidth,
											pixelHeight,
											fftEndIndex,
											fftSize,
											fftOutputSize,
											This->fftBinInterpolationPadding,
											minFrequency,
											maxFrequency,
											This->upsampledPixels,
											&This->spectrum[j],
											inputAudio,
											This->b1[j],
											This->b2[j],
											This->t1[j],
											This->t2[j],
											This->b3,
											This->b4,
											This->b5);
				}
			});
			
			// advance to the next block
			blockStartIndex = nextBlockStartIndex;
		}
		// if there is only one thread then don't use the dispatch queue
	} else {
		for(SInt32 i=0; i<pixelWidth; i++){
			BMSpectrogram_genColumn(i,
									imageOutput,
									fftStride,
									fftStartIndex,
									pixelWidth,
									pixelHeight,
									fftEndIndex,
									fftSize,
									fftOutputSize,
									This->fftBinInterpolationPadding,
									minFrequency,
									maxFrequency,
									This->upsampledPixels,
									&This->spectrum[0],
									inputAudio,
									This->b1[0],
									This->b2[0],
									This->t1[0],
									This->t2[0],
									This->b3,
									This->b4,
									This->b5);
		}
	}
	
	if(BMSG_NUM_THREADS > 1){
		// Don't continue execution of this thread until all blocks have executed.
		dispatch_group_wait(This->dispatchGroup, DISPATCH_TIME_FOREVER);
	}
}

size_t nearestPowerOfTwo(float x){
	float lower = pow(2.0f,floor(log2(x)));
	float upper = pow(2.0f,ceil(log2(x)));
	return x-lower<upper-x? lower:upper;
}

size_t nearestUpperPowerOfTwo(float x){
    float lower = pow(2.0f,floor(log2(x)));
    float upper = pow(2.0f,ceil(log2(x)));
    return x-lower<upper-x? lower:upper;
}

size_t BMSpectrogram_GetFFTSizeFor(BMSpectrogram *This, size_t pixelWidth, size_t sampleWidth){
	// set this to the fft size we want
	// when pixelWidth = sampleWidth
	float k = 128.0;
	
	// estimate the ideal fft size
	float fftSizeFloat = k * (float)sampleWidth / (float)pixelWidth;
	
	// round to the nearest size we can use (power of two)
	size_t fftSize = nearestPowerOfTwo(fftSizeFloat);
	
	// don't allow it to be longer than the max or shorter
	// than the min
	fftSize = SG_MIN(fftSize, This->maxFFTSize);
	fftSize = SG_MAX(fftSize, 512);
	return fftSize;
}




double BMSpectrogram_getColumnWidthSamples(size_t pixelWidth, size_t sampleWidth){
	return (double)sampleWidth / (double)pixelWidth;
}





int32_t BMSpectrogram_inputTime(float * const inputPointer, TPCircularBuffer *audioBuffer, int32_t audioBufferEndTime){
	uint32_t bytesAvailable;
	float *readPointer = TPCircularBufferTail(audioBuffer, &bytesAvailable);
	return audioBufferEndTime + (int)(inputPointer - readPointer);
}




// returns z, the largest integer multiple of x such that z <= y
double BMSpectrogram_lastMultipleBelow(double x, double y){
	return x * floor(y/x);
}




// returns z, the smallest integer multiple of x such that z >= y
double BMSpectrogram_firstMultipleAbove(double x, double y){
	return x * ceil(y/x);
}




int32_t BMSpectrogram_inputFirstColumnTime(int32_t inputStartTime, double samplesPerColumn, int32_t cacheFirstColumnTime){
	int32_t inputStartOffset = inputStartTime - cacheFirstColumnTime;
	int32_t inputFirstColumnOffset = (int32_t)BMSpectrogram_firstMultipleAbove(samplesPerColumn,inputStartOffset);
	return cacheFirstColumnTime + inputFirstColumnOffset;
}




int32_t BMSpectrogram_inputLastColumnTime(int32_t inputEndTime, double samplesPerColumn, int32_t cacheFirstColumnTime){
	int32_t inputEndOffset = inputEndTime - cacheFirstColumnTime;
	int32_t inputLastColumnOffset = (int32_t)BMSpectrogram_lastMultipleBelow(samplesPerColumn, inputEndOffset);
	return cacheFirstColumnTime + inputLastColumnOffset;
}




/*!
 * columnsInInterval
 *
 * @param firstColumnTime the exact time index of the first column
 * @param lastColumnTime the exact time index of the last column
 * @param samplesPerColumn non-integer width of each column in samples
 */
int32_t BMSpectrogram_columnsInInterval(int32_t firstColumnTime, int32_t lastColumnTime, double samplesPerColumn){
	int32_t length = lastColumnTime - firstColumnTime + 1;
	return 1 + (int32_t)round(length / samplesPerColumn);
}



int32_t stochasticRound(double x){
	// generate a random double in [0,1]
	double random01 = (double)arc4random() / (double)INT32_MAX;
	
	// get the fractional part of x
	double intPart, fracPart;
	fracPart = modf(x, &intPart);

	// the larger the fractional part, the more likley that we round up
	if(fracPart > random01) return ceil(x);
	
	// otherwise we round down
	return floor(x);
}



int32_t BMSpectrogram_firstNewColumnTime(int32_t cacheFirstColumnTime,
										 int32_t cacheLastColumnTime,
										 int32_t inputFirstColumnTime,
										 int32_t inputLastColumnTime,
										 double samplesPerColumn){
	// if the new columns are on the left of the cache then the first
	// input column is the first new column
	if(inputFirstColumnTime < cacheFirstColumnTime)
		return inputFirstColumnTime;
	
	// if the first input column comes after the last cache column then
	// it is the first new column
	if (inputFirstColumnTime > cacheLastColumnTime)
		return inputFirstColumnTime;
	
	// if the first input column is within the cache and the last input column is
	// right of the cache then we only need to draw spectrogram columns for part
	// of the input beginning one column after the last column in the cache
	if (cacheFirstColumnTime <= inputFirstColumnTime &&
		inputFirstColumnTime <= cacheLastColumnTime &&
		inputLastColumnTime > cacheLastColumnTime)
		return cacheLastColumnTime + (int32_t)stochasticRound(samplesPerColumn);
	
	// if none of the three cases above were satisfied, return an error
	return -1;
}




int32_t BMSpectrogram_lastNewColumnTime(int32_t cacheFirstColumnTime,
										int32_t cacheLastColumnTime,
										int32_t inputFirstColumnTime,
										int32_t inputLastColumnTime,
										double samplesPerColumn){
	// if the new columns are on the right of the cache then the last
	// input column is the last new column
	if(inputLastColumnTime > cacheLastColumnTime)
		return inputLastColumnTime;
	
	// if the last input column is before the cache starts then the
	// last input column is the last new column
	if (inputLastColumnTime < cacheFirstColumnTime)
		return inputLastColumnTime;
	
	// if the last input column is within the cache and the first input column
	// is left of the cache then we only need to draw the spectrogram for
	// columns beginning one position to the left of the cache start column
	if (cacheFirstColumnTime <= inputLastColumnTime &&
		inputLastColumnTime  <= cacheLastColumnTime &&
		inputFirstColumnTime < cacheFirstColumnTime)
		return cacheFirstColumnTime - (int32_t)stochasticRound(samplesPerColumn);
	
	// if we get here, return an error
	return -1;
}




void BMSpectrogram_prepareAlignment(int32_t widthPixels,
									int32_t heightPixels,
									int32_t widthSamples,
									float * const startPointer,
									size_t slotIndex,
									int32_t fftSize,
									TPCircularBuffer *audioBuffer,
									BMSpectrogramCache *cache,
									int32_t *audioBufferTimeInSamples,
									float **sgInputPtr,
									uint8_t **imageCachePtr,
									int32_t *sgInputLengthWithPadding,
									int32_t *sgFirstColumnIndexInSamples,
									int32_t *sgLastColumnIndexInSamples,
									int32_t *newColumns){
	
	// find out how much padding we need
	SInt32 paddingLeft = BMSpectrogram_getPaddingLeft(fftSize);
	SInt32 paddingRight = BMSpectrogram_getPaddingRight(fftSize);
	
	
	// if settings have not changed, don't redraw the entire image
	bool drawEntireImage = true;
	if(cache->prevFFTSize == fftSize &&
	   cache->prevHeightPixels == heightPixels &&
	   cache->prevWidthPixels == widthPixels &&
	   cache->prevWidthSamples == widthSamples){
		drawEntireImage = false;
	}
	
	// Calculate samplesPerColumn
	double samplesPerColumn = BMSpectrogram_getColumnWidthSamples(widthPixels, widthSamples);
	
	// Calculate the time of the first sample in the input
	int32_t inputStartTime = BMSpectrogram_inputTime(startPointer, audioBuffer, *audioBufferTimeInSamples);
	/* Careful! Is audioBufferEndTime correct? */
	
	// Calculate the time of the last sample in the input
	int32_t inputEndTime = BMSpectrogram_inputTime(startPointer+widthSamples-1, audioBuffer, *audioBufferTimeInSamples);
	
	int32_t inputFirstColumnTime;
	int32_t inputLastColumnTime;
	int32_t firstNewColumnTime;
	int32_t lastNewColumnTime;
	int32_t oldColumns;
	int32_t newSamples;
	
	// calculate settings for drawing the whole image
	if(drawEntireImage){
		inputFirstColumnTime = inputStartTime;
		inputLastColumnTime = inputEndTime;
		firstNewColumnTime = inputFirstColumnTime;
		lastNewColumnTime = inputLastColumnTime;
		*newColumns = widthPixels;
		oldColumns = 0;
		newSamples = widthSamples;
	}
	// if we aren't drawing the entire image then we draw a subset of the
	// columns in the spectrogram
	else {
		// Calculate the time of the first column we can get from the input
		inputFirstColumnTime = BMSpectrogram_inputFirstColumnTime(inputStartTime, samplesPerColumn, cache->firstColumnTime);
		
		// Calculate the time of the last column we can get from the input
		inputLastColumnTime = BMSpectrogram_inputLastColumnTime(inputEndTime, samplesPerColumn, cache->firstColumnTime);
		
		// Calculate the time of the first new column
		firstNewColumnTime = BMSpectrogram_firstNewColumnTime(cache->firstColumnTime,
															  cache->lastColumnTime,
															  inputFirstColumnTime,
															  inputLastColumnTime,
															  samplesPerColumn);
		
		// Calculate the time of the last new column
		lastNewColumnTime = BMSpectrogram_lastNewColumnTime(cache->firstColumnTime,
															cache->lastColumnTime,
															inputFirstColumnTime,
															inputLastColumnTime,
															samplesPerColumn);
		
		// if these numbers are negative then we don't have new columns
		bool hasNewColumns = (firstNewColumnTime >= 0 && lastNewColumnTime >= 0);
		
		if(hasNewColumns){
		// calculate the number of new columns
		*newColumns = BMSpectrogram_columnsInInterval(firstNewColumnTime, lastNewColumnTime, samplesPerColumn);
		
		// calculate the number of old columns
		oldColumns = widthPixels - *newColumns;
		
		// calculate the number of audio samples to generate the new columns
		newSamples = lastNewColumnTime - firstNewColumnTime + 1;
		} else {
			*newColumns = 0;
			oldColumns = widthPixels;
			newSamples = 0;
		}
	}
	
	// calculate the offset in samples from the start of the input buffer to the first column
	int32_t firstNewColumnOffset = firstNewColumnTime - inputStartTime;
	
	// calculate the offset in samples from the start of the input buffer to the last column
	int32_t lastNewColumnOffset = lastNewColumnTime - inputStartTime;
	
	// are we adding new columns to the left or right of the old columns?
	bool newOnRight = firstNewColumnTime > cache->firstColumnTime;
	
	if (!drawEntireImage){
		// shift the image in the cache to make room for the new data
		int shift = newOnRight ? *newColumns : -*newColumns;
		BMSpectrogram_shiftColumns(cache, widthPixels, heightPixels, shift);
	}
	
	// calculate the address in the image cache where we start writing the new columns
	uint32_t bytes;
	*imageCachePtr = TPCircularBufferTail(&cache->cBuffer, &bytes);
	if(newOnRight) *imageCachePtr += oldColumns * heightPixels * BMSG_BYTES_PER_PIXEL;
	
	// output the result
	*sgInputPtr = startPointer - paddingLeft;
	*sgInputLengthWithPadding = newSamples + paddingLeft + paddingRight;
	*sgFirstColumnIndexInSamples = paddingLeft + firstNewColumnOffset;
	*sgLastColumnIndexInSamples = paddingLeft + lastNewColumnOffset;
	
	// get the cache ready for the next time
	cache->firstColumnTime = inputFirstColumnTime;
	cache->lastColumnTime = inputLastColumnTime;
	cache->prevWidthPixels = widthPixels;
	cache->prevHeightPixels = heightPixels;
	cache->prevFFTSize = fftSize;
	cache->prevWidthSamples = widthSamples;
	
	// NOTE: some of the operations above must be done atomically because if the
	// audio thread is writing while we operate then the audioBufferEndTime and
	// the audioBufferTail will change
}



