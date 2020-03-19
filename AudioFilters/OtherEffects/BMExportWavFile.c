//
//  BMExportWavFile.c
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 3/5/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMExportWavFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

typedef struct BMExportHeader
{
    char    ChunkID[4];     /*  4   */
    int32_t ChunkSize;      /*  4   */
    char    Format[4];      /*  4   */

    char    Subchunk1ID[4]; /*  4   */
    int32_t Subchunk1Size;  /*  4   */
    int16_t AudioFormat;    /*  2   */
    int16_t NumChannels;    /*  2   */
    int32_t SampleRate;     /*  4   */
    int32_t ByteRate;       /*  4   */
    int16_t BlockAlign;     /*  2   */
    int16_t BitsPerSample;  /*  2   */

    char    Subchunk2ID[4];
    int32_t Subchunk2Size;
} BMExportHeader;

/*Standard values for CD-quality audio*/
#define SUBCHUNK1SIZE   (16)
#define AUDIO_FORMAT    (1) /*For PCM*/
#define NUM_CHANNELS    (2)
#define SAMPLE_RATE     (48000)

#define BITS_PER_SAMPLE (16)

#define BYTE_RATE       (SAMPLE_RATE * NUM_CHANNELS * BITS_PER_SAMPLE/8)
#define BLOCK_ALIGN     (NUM_CHANNELS * BITS_PER_SAMPLE/8)

/*Return 0 on success and -1 on failure*/
void BMExportWavFile_init(BMExportWavFile* This,int32_t sr){
    This->sampleRate = sr;
}

int BMExportWavFile_writeHeader(BMExportWavFile* This,
                                int32_t SampleRate,
                                int32_t FrameCount)
{
    int ret;
    BMExportHeader wav_header;
    int32_t subchunk2_size;
    int32_t chunk_size;

    size_t write_count;

    subchunk2_size  = FrameCount * NUM_CHANNELS * BITS_PER_SAMPLE/8;
    chunk_size      = 4 + (8 + SUBCHUNK1SIZE) + (8 + subchunk2_size);

    wav_header.ChunkID[0] = 'R';
    wav_header.ChunkID[1] = 'I';
    wav_header.ChunkID[2] = 'F';
    wav_header.ChunkID[3] = 'F';

    wav_header.ChunkSize = chunk_size;

    wav_header.Format[0] = 'W';
    wav_header.Format[1] = 'A';
    wav_header.Format[2] = 'V';
    wav_header.Format[3] = 'E';

    wav_header.Subchunk1ID[0] = 'f';
    wav_header.Subchunk1ID[1] = 'm';
    wav_header.Subchunk1ID[2] = 't';
    wav_header.Subchunk1ID[3] = ' ';

    wav_header.Subchunk1Size = SUBCHUNK1SIZE;
    wav_header.AudioFormat = AUDIO_FORMAT;
    wav_header.NumChannels = NUM_CHANNELS;
    wav_header.SampleRate = SampleRate;
    wav_header.ByteRate = BYTE_RATE;
    wav_header.BlockAlign = BLOCK_ALIGN;
    wav_header.BitsPerSample = BITS_PER_SAMPLE;

    wav_header.Subchunk2ID[0] = 'd';
    wav_header.Subchunk2ID[1] = 'a';
    wav_header.Subchunk2ID[2] = 't';
    wav_header.Subchunk2ID[3] = 'a';
    wav_header.Subchunk2Size = subchunk2_size;

    write_count = fwrite(&wav_header,
                            sizeof(BMExportHeader), 1,
                            This->file_p);

    ret = (1 != write_count)? -1 : 0;

    return ret;
}

/*Data structure to hold a single frame with two channels*/
typedef struct PCM16_stereo_s
{
    int16_t left;
    int16_t right;
} PCM16_stereo_t;

PCM16_stereo_t *allocate_PCM16_stereo_buffer(   int32_t FrameCount)
{
    return (PCM16_stereo_t *)malloc(sizeof(PCM16_stereo_t) * FrameCount);
}


/*Return the number of audio frames sucessfully written*/
size_t  write_PCM16wav_data(FILE*           file_p,
                            int32_t         FrameCount,
                            PCM16_stereo_t  *buffer_p)
{
    size_t ret;

    ret = fwrite(   buffer_p,
                    sizeof(PCM16_stereo_t), FrameCount,
                    file_p);

    return ret;
}

/*Generate two saw-tooth signals at two frequencies and amplitudes*/
int generate_dual_sawtooth( double frequency1,
                            double amplitude1,
                            double frequency2,
                            double amplitude2,
                            int32_t SampleRate,
                            int32_t FrameCount,
                            PCM16_stereo_t  *buffer_p)
{
    int ret = 0;
    double SampleRate_d = (double)SampleRate;
    double SamplePeriod = 1.0/SampleRate_d;

    double Period1, Period2;
    double phase1, phase2;
    double Slope1, Slope2;

    int32_t k;

    /*Check for the violation of the Nyquist limit*/
    if( (frequency1*2 >= SampleRate_d) || (frequency2*2 >= SampleRate_d) )
    {
        ret = -1;
        goto error0;
    }

    /*Compute the period*/
    Period1 = 1.0/frequency1;
    Period2 = 1.0/frequency2;

    /*Compute the slope*/
    Slope1  = amplitude1/Period1;
    Slope2  = amplitude2/Period2;

    for(k = 0, phase1 = 0.0, phase2 = 0.0;
        k < FrameCount;
        k++)
    {
        phase1 += SamplePeriod;
        phase1 = (phase1 > Period1)? (phase1 - Period1) : phase1;

        phase2 += SamplePeriod;
        phase2 = (phase2 > Period2)? (phase2 - Period2) : phase2;

        buffer_p[k].left    = (int16_t)(phase1 * Slope1);
        buffer_p[k].right   = (int16_t)(phase2 * Slope2);
    }

error0:
    return ret;
}

int BMExportWavFile_exportAudioInt(BMExportWavFile* This,char* filePath,float* dataL,float* dataR,uint32_t length){
    int ret;

    PCM16_stereo_t  *buffer_p = NULL;

    size_t written;

    /*Open the wav file*/
    This->file_p = fopen(filePath, "w");
    if(NULL == This->file_p)
    {
        perror("fopen failed in main");
        ret = -1;
        goto error0;
    }

    /*Allocate the data buffer*/
    buffer_p = allocate_PCM16_stereo_buffer(length);
    if(NULL == buffer_p)
    {
        perror("fopen failed in main");
        ret = -1;
        goto error1;
    }

    double frequency1 = 493.9; /*B4*/
    double amplitude1 = 0.65 * (double)SHRT_MAX;
    double frequency2 = 392.0; /*G4*/
    double amplitude2 = 0.75 * (double)SHRT_MAX;

    /*Fill the buffer*/
    ret = generate_dual_sawtooth(   frequency1,
                                    amplitude1,
                                    frequency2,
                                    amplitude2,
                                    This->sampleRate,
                                    length,
                                    buffer_p);
    
    for(int i=0;i<length;i++){
//        printf("%d\n",buffer_p[i].left);
        buffer_p[i].left = dataL[i];
        buffer_p[i].right = dataR[i];
    }
    
//    if(ret < 0)
//    {
//        fprintf(stderr, "generate_dual_sawtooth failed in main\n");
//        ret = -1;
//        goto error2;
//    }

    /*Write the wav file header*/
    ret = BMExportWavFile_writeHeader(This, This->sampleRate, length);
    if(ret < 0)
    {
        perror("write_PCM16_stereo_header failed in main");
        ret = -1;
        goto error2;
    }

    /*Write the data out to file*/
    written = write_PCM16wav_data(  This->file_p,
                                    length,
                                    buffer_p);
    if(written < length)
    {
        perror("write_PCM16wav_data failed in main");
        ret = -1;
        goto error2;
    }

    /*Free and close everything*/
    error2:
        free(buffer_p);
    error1:
        fclose(This->file_p);
    error0:
        return ret;
}

int BMExportWavFile_exportAudioFloatToInt16(BMExportWavFile* This,char* filePath,float* dataL,float* dataR,uint32_t length){
    for(int i=0;i<length;i++){
        dataL[i] = dataL[i]*INT16_MAX;
        dataR[i] = dataR[i]*INT16_MAX;
    }
    return BMExportWavFile_exportAudioInt(This, filePath, dataL, dataR, length);
}



