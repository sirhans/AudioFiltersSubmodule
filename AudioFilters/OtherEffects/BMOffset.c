//
//  BMOffset.c
//  CPU-GPUSynchronization
//
//  Created by TienNM on 7/24/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#include "BMOffset.h"
#import "assert.h"
#import <Accelerate/Accelerate.h>


/*
 * generates two lists of points, in the arrays left and right, that are offset
 * on either side of the path given in centre
 *
 * @param centre interleaved 2d point data (x1,y1,x2,y2,...)
 * @param left   output array has same length and format at centre
 * @param right  same as left, but points on the other side
 * @param width  width of the line, spacing between the two offsets
 * @param length number of points in the input and output arrays
 */
void BMOffset_process(const float* centre, float* left, float* right, float width, size_t length){
    
    // find the difference between each pair or points
    // Buffer to left
    vDSP_vsub(centre, 1, centre+2, 1, left, 1, length-2);
    
    // copy the second from last result to get the last result
    left[length-2] = left[length-4];
    left[length-1] = left[length-3];
    
    // find the average between each adjacent pair of points
    // this smooths out discontinuities at sharp corners
    // buffer to right
    float one_half = 0.5;
    vDSP_vasm(left, 1, left+2, 1, &one_half, right+2, 1, length-2);
    
    // copy the first result to right
    right[0] = left[0];
    right[1] = left[1];
    
    // find the length of each difference vector
    vDSP_vdist(right, 2, right+1, 2, left, 1, length/2);
    
    // normalise each by dividing each coordinate by the length
    vDSP_vdiv(left, 1, right, 2, right, 2, length/2);
    vDSP_vdiv(left,1,right+1, 2, right+1, 2, length/2);
    
    // rotate the differences 90 degrees and multiply by width
    // buffer to right
    float negWidth = -1.0 * width / 2.0;
          width    =  width / 2.0;
    vDSP_vsmul(right, 2, &negWidth, left+1, 2, length/2);
    vDSP_vsmul(right+1, 2, &width, left, 2, length/2);
        
    // subtract from centre
    // write right
    vDSP_vsub(left, 1, centre, 1, right, 1, length);
    
    // add to centre
    // write left
    vDSP_vadd(left, 1, centre, 1, left, 1, length);
}


/*
 * generates two lists of points, in the arrays left and right, that are offset
 * on either side of the path given in centre
 *
 * @param centreX X coordinates of original line
 * @param centreY Y coordinates of original line
 * @param leftX   left X coordinates output
 * @param leftY   output array has same length and format at centre
 * @param rightX  right X coordinates output
 * @param rightY  same as left, but points on the other side
 * @param width   width of the line, spacing between the two offsets
 * @param length  number of points in the input and output arrays
 */
void BMOffset_processSeparated(const float* centreX, const float* centreY,
                               float* leftX, float* leftY,
                               float* rightX, float* rightY,
                               float width, size_t length){
    
    // find the difference between each pair or points
    // Buffer to left
    float* differencesX = leftX;
    float* differencesY = leftY;
    vDSP_vsub(centreX, 1, centreX+1, 1, differencesX, 1, length-1);
    vDSP_vsub(centreY, 1, centreY+1, 1, differencesY, 1, length-1);
    
    // copy the second from last result to get the last result
    differencesX[length-1] = differencesX[length-2];
    differencesY[length-1] = differencesY[length-2];
    
    // find the average between each adjacent pair of points
    // this smooths out discontinuities at sharp corners
    // buffer to right
    float* differencesSmoothedX = rightX;
    float* differencesSmoothedY = rightY;
    float one_half = 0.5;
    vDSP_vasm(differencesX, 1, differencesX+1, 1, &one_half, differencesSmoothedX+1, 1, length-1);
    vDSP_vasm(differencesY, 1, differencesY+1, 1, &one_half, differencesSmoothedY+1, 1, length-1);
    
    // copy the first result to right
    differencesSmoothedX[0] = differencesX[0];
    differencesSmoothedY[0] = differencesY[0];
    
    // find the length of each difference vector, buffer to leftX
    float* lengths = leftX;
    vDSP_vdist(differencesSmoothedX, 1, differencesSmoothedY, 1, lengths, 1, length);
    
    // normalise each by dividing each coordinate by the length
    vDSP_vdiv(lengths, 1, differencesSmoothedX, 1, differencesSmoothedX, 1, length);
    vDSP_vdiv(lengths, 1, differencesSmoothedY, 1, differencesSmoothedY, 1, length);
    
    // rotate the differences 90 degrees and multiply by width
    // buffer to left
    float* normalsX = leftX;
    float* normalsY = leftY;
    float negWidth = -1.0 * width / 2.0;
          width    =  width / 2.0;
    vDSP_vsmul(differencesSmoothedX, 1, &negWidth, normalsY, 1, length);
    vDSP_vsmul(differencesSmoothedY, 1, &width, normalsX, 1, length);
        
    // subtract from centre
    // write right
    vDSP_vsub(normalsX, 1, centreX, 1, rightX, 1, length);
    vDSP_vsub(normalsY, 1, centreY, 1, rightY, 1, length);
    
    // add to centre
    // write left
    vDSP_vadd(normalsX, 1, centreX, 1, leftX, 1, length);
    vDSP_vadd(normalsY, 1, centreY, 1, leftY, 1, length);
}
