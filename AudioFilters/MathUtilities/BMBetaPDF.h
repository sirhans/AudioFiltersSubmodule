//
//  BMBetaPDF.h
//  AudioFiltersXcodeProject
//
//  A collection of functions related to the beta distribution
//  https://en.wikipedia.org/wiki/Beta_distribution
//
//  Created by hans anderson on 9/20/19.
//  Anyone may use this file without restrictions
//

#ifndef BMBetaPDF_h
#define BMBetaPDF_h

#include <stdio.h>
#include <math.h>
#include "incbeta.h" // incomplete beta function implementation Copyright (c) 2016, 2017 Lewis Van Winkle


double BM_B(double alpha, double beta);

/*!
 *BM_betaPDF
 */
double BM_betaPDF(double x, double alpha, double beta);




/*!
 *BM_betaCDF
 */
double BM_betaCDF(double x, double alpha, double beta);



// https://en.wikipedia.org/wiki/Beta_distribution#Mean_absolute_deviation_around_the_mean
/*!
 *BM_betaMeanAbsoluteDeviation
 */
double BM_betaMeanAbsoluteDeviation(double alpha, double beta);

/*!
 *BM_betaVariance
 */
double BM_betaVariance(double alpha, double beta);

/*!
 *BM_betaStdDev
 */
double BM_betaStdDev(double alpha, double beta);

/*!
 *BM_uniformMedianPDF
 *
 * @param x point at which we want to evalute the pdf
 * @param N size of sample
 *
 * @returns the probability density at x of the median of a random sample of size N taken from the uniform distribution on [0,1]
 */
double BM_uniformMedianPDF(double x, size_t N);


#endif /* BMBetaPDF_h */
