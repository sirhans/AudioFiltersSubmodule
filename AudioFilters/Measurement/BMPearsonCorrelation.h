//
//  BMPearsonCorrelation.h
//  OscilloScopeExtension
//
//  Created by hans anderson on 7/22/20.
//  We hereby release this file into the public domain with no restrictions
//

#ifndef BMPearsonCorrelation_h
#define BMPearsonCorrelation_h

#include <stdio.h>

/*!
 *BMPearsonCorrelation
 *
 * returns the correlation between the arrays in left and right
 */
float BMPearsonCorrelation(float *left, float *right, size_t length);


#endif /* BMPearsonCorrelation_h */
