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


static double BM_B(double alpha, double beta){
    return (tgamma(alpha)*tgamma(beta)) / tgamma(alpha + beta);
}

/*!
 *BM_betaPDF
 */
static double BM_betaPDF(double x, double alpha, double beta){
    double B = BM_B(alpha,beta);
    return pow(x,alpha - 1.0) * pow(1.0 - x, beta - 1.0) / B;
}




/*!
 *BM_betaCDF
 */
static double BM_betaCDF(double x, double alpha, double beta){
    // the beta CDF is given by the regularised incomplete beta function:
    return incbeta(alpha, beta, x);
}



// https://en.wikipedia.org/wiki/Beta_distribution#Mean_absolute_deviation_around_the_mean
/*!
 *BM_betaMeanAbsoluteDeviation
 */
static double BM_betaMeanAbsoluteDeviation(double alpha, double beta){
    double numerator = 2.0 * pow(alpha,alpha) * pow(beta,beta);
    double denominator = BM_B(alpha,beta) * pow(alpha + beta, alpha + beta + 1.0);
    return numerator / denominator;
}

/*!
 *BM_betaVariance
 */
static double BM_betaVariance(double alpha, double beta){
    return (alpha * beta) / ((alpha + beta) * (alpha + beta) * (alpha + beta + 1.0));
}

/*!
 *BM_betaStdDev
 */
static double BM_betaStdDev(double alpha, double beta){
    return sqrt(BM_betaVariance(alpha, beta));
}

/*!
 *BM_uniformMedianPDF
 *
 * @param x point at which we want to evalute the pdf
 * @param N size of sample
 *
 * @returns the probability density at x of the median of a random sample of size N taken from the uniform distribution on [0,1]
 */
static double BM_uniformMedianPDF(double x, size_t N){
    size_t k = ceil((float)N/2.0f);
    return BM_betaPDF(x, k, N+1-k);
}


#endif /* BMBetaPDF_h */
