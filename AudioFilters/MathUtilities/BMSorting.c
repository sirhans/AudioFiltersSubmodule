//
//  BMSorting.c
//  SaturatorAU
//
//  Created by hans anderson on 12/18/19.
//  Copyright © 2019 bluemangoo. All rights reserved.
//

#include "BMSorting.h"
#include <stdbool.h>


/*!
 *BMInsertionSort_size_t
 *
 * @abstract insertion sort on an array of size_t of length n
 */
void BMInsertionSort_size_t(size_t *a, size_t n) {
	for(size_t i = 1; i < n; ++i) {
		size_t tmp = a[i];
		size_t j = i;
		while(j > 0 && tmp < a[j - 1]) {
			a[j] = a[j - 1];
			--j;
		}
		a[j] = tmp;
	}
}
