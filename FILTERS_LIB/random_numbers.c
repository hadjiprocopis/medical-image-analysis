#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "Common_IMMA.h"

#include "filters.h"
#include "random_numbers.h"

/* produces gaussian distribution random numbers between 0 and 1 */
float   random_number_gaussian(float mean, float half_width){
	float x1, x2, w;

	do {
		x1 = 2.0 * drand48() - 1.0;
		x2 = 2.0 * drand48() - 1.0;
		w = SQR(x1) + SQR(x2);
	} while ( w >= 1.0 );

	return SCALE_OUTPUT(
		(x1 * sqrt( (-2.0 * log( w ) ) / w )),
		(mean-half_width), (mean+half_width), -5.0, 5.0
	       );
}
