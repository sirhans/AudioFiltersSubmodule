//
//  BMPolyphaseIIR2Designer.h
//  AudioFiltersXcodeProject
//
//  Ported from PolyphaseIIR2Designer.h from the HIIR library by
//  Laurend De Soras
//  http://ldesoras.free.fr/prod.html
//
//  Created by hans anderson on 7/12/19.
//  Anyone may use this file without restrictions of any kind.
//

#ifndef BMPolyphaseIIR2Designer_h
#define BMPolyphaseIIR2Designer_h

#include <stdio.h>
#include <stdbool.h>


/* public functions */
int     BMPolyphaseIIR2Designer_computeNbrCoefsFromProto (double attenuation, double transition);
double  BMPolyphaseIIR2Designer_computeAttenFromOrderTbw (int nbr_coefs, double transition);
int     BMPolyphaseIIR2Designer_computeCoefs (double coef_arr [], double attenuation, double transition);
void    BMPolyphaseIIR2Designer_computeCoefsSpecOrderTbw (double coef_arr [], int nbr_coefs, double transition);
double  BMPolyphaseIIR2Designer_computePhaseDelay (double a, double f_fs);
double  BMPolyphaseIIR2Designer_computeGroupDelay (double a, double f_fs, bool ph_flag);
double  BMPolyphaseIIR2Designer_computeGroupDelayCascade (const double coef_arr [], int nbr_coefs, double f_fs, bool ph_flag);



#endif /* BMPolyphaseIIR2Designer_h */
