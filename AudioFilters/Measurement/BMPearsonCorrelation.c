//
//  BMPearsonCorrelation.c
//  OscilloScopeExtension
//
//  Created by hans anderson on 7/22/20.
//  We hereby release this file into the public domain with no restrictions
//

#include "BMPearsonCorrelation.h"
#include <Accelerate/Accelerate.h>




/*
 * The pearson correlation coefficient of X and Y is
 *    covariance(X,Y) / (stdDev(X) * stdDev(y))
 */
float BMPearsonCorrelation(const float *X, const float *Y, size_t length){
	// find sum(X[i]*Y[i]) for 0 <= i < length
	float sumXY;
	vDSP_conv(X, 1, Y, 1, &sumXY, 1, 1, length);
	float eXY = sumXY / (float)length;
	
	// find mean(X) and mean(y)
	float meanX, meanY;
	vDSP_meanv(X, 1, &meanX, length);
	vDSP_meanv(Y, 1, &meanY, length);
	
	// find the mean(X[i]^2) and mean(Y[i]^2)
	float meanSqX, meanSqY;
	vDSP_measqv(X, 1, &meanSqX, length);
	vDSP_measqv(Y, 1, &meanSqY, length);
	
	// find the standard deviation of X and Y
	float xStdDev = sqrtf(meanSqX - meanX*meanX);
	float yStdDev = sqrtf(meanSqY - meanY*meanY);
	
	// find the covariance of X and Y
	float covariance = eXY - meanX*meanY;
	
	// return the pearson correlation coefficient
	return covariance / (xStdDev * yStdDev);
}
