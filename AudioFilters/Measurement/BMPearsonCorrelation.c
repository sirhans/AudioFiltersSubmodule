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
float BMPearsonCorrelation(float *X, float *Y, size_t length){
	// find sum(X[i]*Y[i]) for 0 <= i < length
	float sumXY;
	vDSP_conv(X, 1, Y, 1, &sumXY, 1, 1, length);
	
	// find mean(X) and mean(y)
	float meanX, meanY;
	vDSP_meanv(X, 1, &meanX, length);
	vDSP_meanv(Y, 1, &meanY, length);
	
	// find sum(X[i]^2) for 0 <= i < length
	float sumSquaresX, sumSquaresY;
	vDSP_svesq(X, 1, &sumSquaresX, length);
	vDSP_svesq(Y, 1, &sumSquaresY, length);
	
	// find the standard deviation of X and Y
	float xSqEv = sumSquaresX/(float)length;
	float ySqEv = sumSquaresY/(float)length;
	float xStdDev = sqrtf(xSqEv - meanX*meanX);
	float yStdDev = sqrtf(ySqEv - meanY*meanY);
	
	// find the covariance of X and Y
	float covariance = sumXY - meanX*meanY;
	
	// return the pearson correlation coefficient
	return covariance / (xStdDev * yStdDev);
}
