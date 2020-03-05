//
//  BMExportWavFile.h
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 3/5/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMExportWavFile_h
#define BMExportWavFile_h

#include <stdio.h>

typedef struct BMExportWavFile{
    FILE*   file_p;
    uint32_t sampleRate;
} BMExportWavFile;

void BMExportWavFile_init(BMExportWavFile* This,int32_t sr);
int BMExportWavFile_exportAudioInt(BMExportWavFile* This,char* filePath,float* dataL,float* dataR,uint32_t length);
int BMExportWavFile_exportAudioFloat(BMExportWavFile* This,char* filePath,float* dataL,float* dataR,uint32_t length);
#endif /* BMExportWavFile_h */
