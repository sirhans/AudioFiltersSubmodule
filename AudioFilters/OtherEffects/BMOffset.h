//
//  BMOffset.h
//  CPU-GPUSynchronization
//
//  Created by TienNM on 7/24/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#ifndef BMOffset_h
#define BMOffset_h

#include <stdio.h>

void BMOffset_process(const float* centre, float* left, float* right, float width, size_t length);
void BMOffset_processSeparated(const float* centreX, const float* centreY,
                               float* leftX, float* leftY,
                               float* rightX, float* rightY,
                               float width, size_t length);

#endif /* BMOffset_h */
