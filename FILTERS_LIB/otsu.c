#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "filters.h"
#include "histogram.h"
#include "otsu.h"

/* this file was created by A.Hadjiprocopis based on 1 routine from :

 * ============================================================================
 *
 * ostu_thrd_val.c - Threshold value calculating module for eXKull routine
 *
 *      Created: 4 December 1995
 *
 * ----------------------------------------------------------------------------
 *
 *      eXKull: Image display and analysis package for EXtraction sKULL
 *
 *      by D-S YOO, 1994
 *
 *      This module is part of the eXKull suite of routines.
 *
 * ------------------------------ Modification --------------------------------
 *
 * 17.06.96 (ll)  Debugged for 16-bit data: negative values.
 * 24.01.97 (ll)  Mistake in final calculation for 8 bit data.
 * 12.02.97 (ll)  Created new 'otsu_thrd_val_image' and otsu_thrd_val_core.
 * 12.08.99 (ll)  Nothing.
 * ============================================================================

by permission of Louis Lemieux <l.lemieux@ion.ucl.ac.uk>

The method is based on:

Otsu N. A threshold selection method for grey-level histograms. IEEE Trans
Systems, Man and Cybernetics vol 9: 62-69.

*/

/* given an image (3D) will return the pixel value which is a threshold
   between background noise and brain image
   The 'LOHI' means that the cumulative histogram will be calculated
   normally, e.g. by starting to add from the lowest intensity to the
   highest.
*/
int	otsu_find_background_threshold3D_LOHI(
	DATATYPE 	***data,
	int		offset_x,
	int		offset_y,
	int		offset_z,
	int		size_x,
	int 		size_y,
	int		size_z,
	int		*threshold)
{
	histogram	*myHistogram;

	int		j, k;

	float		*f_ptr, *p_ptr, *pp_ptr,
			f_val_max, p_val, pp_sum, 
			pp_total, pp_val,  q_between,
			w0, w1, w_val;

	if( (myHistogram=histogram3D(data, offset_x, offset_y, offset_z, size_x, size_y, size_z, 1)) == NULL ){
		fprintf(stderr, "otsu_find_background_threshold3D : call to histogram3D has failed.\n");
		return FALSE;
	}

	if((f_ptr = (float *)malloc(myHistogram->numBins*sizeof(float))) == NULL) {
		fprintf(stderr, "otsu_find_background_threshold3D : could not allocate %zd bytes for f_ptr.\n", myHistogram->numBins*sizeof(float));
		destroy_histogram(myHistogram);
		return FALSE;
	}
	if((p_ptr = (float *)malloc(myHistogram->numBins*sizeof(float))) == NULL) {
		fprintf(stderr, "otsu_find_background_threshold3D : could not allocate %zd bytes for p_ptr.\n", myHistogram->numBins*sizeof(float));
		free(f_ptr);
		destroy_histogram(myHistogram);
		return FALSE;
	}
	if((pp_ptr = (float *)malloc(myHistogram->numBins*sizeof(float))) == NULL) {
		fprintf(stderr, "otsu_find_background_threshold3D : could not allocate %zd bytes for pp_ptr.\n", myHistogram->numBins*sizeof(float));
		free(f_ptr); free(p_ptr);
		destroy_histogram(myHistogram);
		return FALSE;
	}

	pp_total = 0.0;

	for(k=0;k<myHistogram->numBins;k++) {
		p_val = (float )(myHistogram->bins[k]) / (float )(myHistogram->cumulative_frequency[myHistogram->maxPixel-myHistogram->minPixel]);
		p_ptr[k] = p_val;
		pp_ptr[k] = (k+myHistogram->minPixel) * p_ptr[k];
		pp_total += pp_ptr[k];
	}

	for (k=0;k<myHistogram->numBins;k++) {
		w0 = pp_sum = 0.0;

		for (j=0; j<=k; j++) {
			w0 += p_ptr[j];
			pp_sum += pp_ptr[j];
		}

		w1 = 1.0 - w0;

		w_val = w0 * w1;

		if (w_val == 0.0) 
			w_val = 1.0;

		pp_val = pp_total*w0 - pp_sum;

		q_between = (pp_val * pp_val) / w_val;	

		f_ptr[k] = q_between;
	}
	f_val_max = f_ptr[0];
	for (k=0;k<myHistogram->numBins;k++) {
		if (f_ptr[k] > f_val_max) {
			f_val_max = f_ptr[k];
			*threshold = k;
		}
	}

	*threshold += myHistogram->minPixel;

	free (f_ptr);
	free (p_ptr);
	free (pp_ptr);

	destroy_histogram(myHistogram);

	return TRUE;
}

/* Now this one, is the same as above but the cumulative
   histogram is calculated starting adding from the highest
   intensity to the lowest. */

int	otsu_find_background_threshold3D_HILO(
	DATATYPE 	***data,
	int		offset_x,
	int		offset_y,
	int		offset_z,
	int		size_x,
	int 		size_y,
	int		size_z,
	int		*threshold)
{
	histogram	*myHistogram;

	int		j, k, sum;

	float		*f_ptr, *p_ptr, *pp_ptr,
			f_val_max, p_val, pp_sum, 
			pp_total, pp_val,  q_between,
			w0, w1, w_val;

	if( (myHistogram=histogram3D(data, offset_x, offset_y, offset_z, size_x, size_y, size_z, 1)) == NULL ){
		fprintf(stderr, "otsu_find_background_threshold3D : call to histogram3D has failed.\n");
		return FALSE;
	}

	if((f_ptr = (float *)malloc(myHistogram->numBins*sizeof(float))) == NULL) {
		fprintf(stderr, "otsu_find_background_threshold3D : could not allocate %zd bytes for f_ptr.\n", myHistogram->numBins*sizeof(float));
		destroy_histogram(myHistogram);
		return FALSE;
	}
	if((p_ptr = (float *)malloc(myHistogram->numBins*sizeof(float))) == NULL) {
		fprintf(stderr, "otsu_find_background_threshold3D : could not allocate %zd bytes for p_ptr.\n", myHistogram->numBins*sizeof(float));
		free(f_ptr);
		destroy_histogram(myHistogram);
		return FALSE;
	}
	if((pp_ptr = (float *)malloc(myHistogram->numBins*sizeof(float))) == NULL) {
		fprintf(stderr, "otsu_find_background_threshold3D : could not allocate %zd bytes for pp_ptr.\n", myHistogram->numBins*sizeof(float));
		free(f_ptr); free(p_ptr);
		destroy_histogram(myHistogram);
		return FALSE;
	}

	pp_total = 0.0;

	for(sum=0,k=myHistogram->numBins-1;k>=0;k--){
		sum += myHistogram->bins[k];
		myHistogram->cumulative_frequency[k] = sum;
	}

	for(k=0;k<myHistogram->numBins;k++) {
		p_val = (float )(myHistogram->bins[k]) / (float )(myHistogram->cumulative_frequency[myHistogram->maxPixel-myHistogram->minPixel]);
		p_ptr[k] = p_val;
		pp_ptr[k] = (k+myHistogram->minPixel) * p_ptr[k];
		pp_total += pp_ptr[k];
	}

	for (k=0;k<myHistogram->numBins;k++) {
		w0 = pp_sum = 0.0;

		for (j=0; j<=k; j++) {
			w0 += p_ptr[j];
			pp_sum += pp_ptr[j];
		}

		w1 = 1.0 - w0;

		w_val = w0 * w1;

		if (w_val == 0.0) 
			w_val = 1.0;

		pp_val = pp_total*w0 - pp_sum;

		q_between = (pp_val * pp_val) / w_val;	

		f_ptr[k] = q_between;
	}
	f_val_max = f_ptr[0];
	for (k=0;k<myHistogram->numBins;k++) {
		if (f_ptr[k] > f_val_max) {
			f_val_max = f_ptr[k];
			*threshold = k;
		}
	}

	*threshold += myHistogram->minPixel;

	free (f_ptr);
	free (p_ptr);
	free (pp_ptr);

	destroy_histogram(myHistogram);

	return TRUE;
}

