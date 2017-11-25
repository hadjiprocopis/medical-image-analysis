#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "filters.h"
#include "histogram.h"
#include "statistics.h"
#include "threshold.h"
#include "threshold_private.h"
#include "maps.h"


/* DATATYPE is defined in filters.h to be anythng you like (int or short int) */

/* pixel_thresholding is when you select pixels based on their pixel intensity and then
   replace their value with something else (normally uniform over the selection range).

   frequency_thresholding is when you select pixels based on the frequency of occurence
   (so we need a histogram first) of their values.
*/

/* Function to threshold (depending on pixel intensity)
   a 1D array of pixels, given spec for ranges and replacement colors
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
	   [] offset, where to start in the array
	   [] size, how many pixels to consider
	   [] *thresholdSpec, an array of threshold specifications (see threshold.h for its definition)
	      basically it holds 'low', 'high' and 'newValue'
	   [] numSpec, the number of elements in *thresholdSpec array
	   [] defaultColor, all other pixel intensities outside all ranges, will be replaced
	      by this color, unless it is set to THRESHOLD_LEAVE_UNCHANGED, then they will remain unchanged.
	   [] *changeMap, (optional set it to NULL if you do not want to use this feature), a pointer to 1D
	      array which will reflect the changes made to the data. For each pixel in data, this map
	      will have its respective element set to 'changed', 'unchanged', 'default'
	     (typedef changemap_entry - see threshold.h)
   returns: the number of pixels changed
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	pixel_threshold1D(DATATYPE *data, int offset, int size, spec *thresholdSpec, int numSpecs, DATATYPE defaultColor, char *changeMap){
	int		i, r, unchanged, count = 0;
	DATATYPE	*p = &(data[offset]);

	/* for efficiency */
	if( changeMap ){
		for(i=0;i<size;i++){
			unchanged = TRUE;
			for(r=0;r<numSpecs;r++){
				if( IS_WITHIN(p[i], thresholdSpec[r].low, thresholdSpec[r].high) ){
					unchanged = FALSE; changeMap[i] = unchanged;
					if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
						p[i] = thresholdSpec[r].newValue;
						changeMap[i] = changed;
						count++;
					}
					break; /* assume does not belong to any other range */
				}
			}
			if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
				p[i] = defaultColor;
				changeMap[i] = default_color;
				count++;
			}
		}
	} else {
		for(i=0;i<size;i++){
			unchanged = TRUE;
			for(r=0;r<numSpecs;r++){
				if( IS_WITHIN(p[i], thresholdSpec[r].low, thresholdSpec[r].high) ){
					unchanged = FALSE;
					if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
						p[i] = thresholdSpec[r].newValue;
						count++;
					}
					break; /* assume does not belong to any other range */
				}
			}
			if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
				p[i] = defaultColor;
				count++;
			}
		}
	}
	return count;
}

int	pixel_threshold2D(DATATYPE **data, int x, int y, int w, int h, spec *thresholdSpec, int numSpecs, DATATYPE defaultColor, char **changeMap){
	int		i, j, r, unchanged, count = 0;

	/* for efficiency */
	if( changeMap ){
		for(i=x;i<x+w;i++)
			for(j=y;j<y+h;j++){
				unchanged = TRUE;
				for(r=0;r<numSpecs;r++){
					if( IS_WITHIN(data[i][j], thresholdSpec[r].low, thresholdSpec[r].high) ){
						unchanged = FALSE; changeMap[i][j] = unchanged;
						if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
							data[i][j] = thresholdSpec[r].newValue;
							changeMap[i][j] = changed;
							count++;
						}
						break; /* assume does not belong to any other range */
					}
				}
				if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
					data[i][j] = defaultColor;
					changeMap[i][j] = default_color;
					count++;
				}
			}
	} else {
		for(i=x;i<x+w;i++)
			for(j=y;j<y+h;j++){
				unchanged = TRUE;
				for(r=0;r<numSpecs;r++){
					if( IS_WITHIN(data[i][j], thresholdSpec[r].low, thresholdSpec[r].high) ){
						unchanged = FALSE; /* so that defaultColor does not come here */
						if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
							data[i][j] = thresholdSpec[r].newValue;
							count++;
						}
						break; /* assume does not belong to any other range */
					}
				}
				if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
					data[i][j] = defaultColor;
					count++;
				}
			}
	}
	return count;
}

int	pixel_threshold3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, spec *thresholdSpec, int numSpecs, DATATYPE defaultColor, char ***changeMap){
	int		i, j, k, r, unchanged, count = 0;

	/* for efficiency */
	if( changeMap ){
		for(i=x;i<x+w;i++)
			for(j=y;j<y+h;j++)
				for(k=z;k<z+d;z++){
					unchanged = TRUE;
					for(r=0;r<numSpecs;r++){
						if( IS_WITHIN(data[k][i][j], thresholdSpec[r].low, thresholdSpec[r].high) ){
							unchanged = FALSE;
							if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
								data[k][i][j] = thresholdSpec[r].newValue;
								changeMap[k][i][j] = changed;
								count++;
							}
							break; /* assume does not belong to any other range */
						}
					}
					if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
						data[k][i][j] = defaultColor;
						changeMap[k][i][j] = default_color;
						count++;
					}
				}
	} else {
		for(i=x;i<x+w;i++)
			for(j=y;j<y+h;j++)
				for(k=z;k<z+d;z++){
					unchanged = TRUE;
					for(r=0;r<numSpecs;r++){
						if( IS_WITHIN(data[k][i][j], thresholdSpec[r].low, thresholdSpec[r].high) ){
							unchanged = FALSE;
							if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
								data[k][i][j] = thresholdSpec[r].newValue;
								count++;
							}
							break; /* assume does not belong to any other range */
						}
					}
					if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
						data[k][i][j] = defaultColor;
						count++;
					}
				}
	}
	return count;
}


/* frequency threshold */

/* Function to threshold (depending on pixel intensity frequencies)
   a 1D array of pixels, given spec for ranges and replacement colors
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
	   [] offset, where to start in the array
	   [] size, how many pixels to consider
	   [] numSpecs, the number of ranges you want to do thresholding
	   [] *rangeLow, *rangeHigh, arrays of frequencies of pixel intensities (must contain numSpecs items)
	      a range pair (low, high) is formed by the ith element of each array.
	      this will be our thresholding range what pixel intensity frequencies belong to this
	      level will either remain unchanged or change to the ith element of the newColor array
	   [] *newColor, an array holding numSpecs pixel intensities. The ith of those
	      will be used in replacing all pixels in the ith range (rangeLow[i], rangeHigh[i])
	      if newColor[i] == THRESHOLD_LEAVE_UNCHANGED then the all pixels in the range will remain unchanged
	   [] defaultColor, all other pixel intensities outside all ranges, will be replaced
	      by this color, unless it is set to THRESHOLD_LEAVE_UNCHANGED, then they will remain unchanged.
	   [] *changeMap, (optional set it to NULL if you do not want to use this feature), a pointer to 1D
	      array which will reflect the changes made to the data. For each pixel in data, this map
	      will have its respective element set to CHANGED, or UNCHANGED, or DEFAULT_COLOR
   returns: nothing

   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	frequency_threshold1D(DATATYPE *data, int offset, int size, spec *thresholdSpec, int numSpecs, DATATYPE defaultColor, histogram hist, char *changeMap){
	int		i, r, unchanged, count = 0;
	DATATYPE	*p = &(data[offset]);

	/* for efficiency */
	if( changeMap ){
		for(i=0;i<size;i++){
			unchanged = TRUE;
			for(r=0;r<numSpecs;r++){
				if( IS_WITHIN(hist.bins[(int )(p[i]-hist.minPixel)], thresholdSpec[r].low, thresholdSpec[r].high) ){
					unchanged = FALSE; changeMap[i] = unchanged;
					if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
						p[i] = thresholdSpec[r].newValue;
						changeMap[i] = changed;
						count++;
					}
					break; /* assume does not belong to any other range */
				}
			}
			if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
				p[i] = defaultColor;
				changeMap[i] = default_color;
				count++;
			}
		}
	} else {
		for(i=0;i<size;i++){
			unchanged = TRUE;
			for(r=0;r<numSpecs;r++){
				if( IS_WITHIN(hist.bins[(int )(p[i]-hist.minPixel)], thresholdSpec[r].low, thresholdSpec[r].high) ){
					unchanged = FALSE;
					if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
						p[i] = thresholdSpec[r].newValue;
						count++;
					}
					break; /* assume does not belong to any other range */
				}
			}
			if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
				p[i] = defaultColor;
				count++;
			}
		}
	}
	return count;
}

int	frequency_threshold2D(DATATYPE **data, int x, int y, int w, int h, spec *thresholdSpec, int numSpecs, DATATYPE defaultColor, histogram hist, char **changeMap){
	int		i, j, r, unchanged, count = 0;

	if( changeMap ){
		for(i=x;i<x+w;i++)
			for(j=y;j<y+h;j++){
				unchanged = TRUE;
				for(r=0;r<numSpecs;r++){
					if( IS_WITHIN(hist.bins[(int )(data[i][j]-hist.minPixel)], thresholdSpec[r].low, thresholdSpec[r].high) ){
						unchanged = FALSE; changeMap[i][j] = unchanged;
						if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
							data[i][j] = thresholdSpec[r].newValue;
							changeMap[i][j] = changed;
							count++;
						}
						break; /* assume does not belong to any other range */
					}
				}
				if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
					data[i][j] = defaultColor;
					changeMap[i][j] = default_color;
					count++;
				}
			}
	} else {
		for(i=x;i<x+w;i++)
			for(j=y;j<y+h;j++){
				unchanged = TRUE;
				for(r=0;r<numSpecs;r++){
					if( IS_WITHIN(hist.bins[(int )(data[i][j]-hist.minPixel)], thresholdSpec[r].low, thresholdSpec[r].high) ){
						unchanged = FALSE; /* so that defaultColor does not come here */
						if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
							data[i][j] = thresholdSpec[r].newValue;
							count++;
						}
						break; /* assume does not belong to any other range */
					}
				}
				if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
					data[i][j] = defaultColor;
					count++;
				}
			}
	}
	return count;
}

int	frequency_threshold3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, spec *thresholdSpec, int numSpecs, DATATYPE defaultColor, histogram hist, char ***changeMap){
	int		i, j, k, r, unchanged, count = 0;

	if( changeMap ){
		for(i=x;i<x+w;i++)
			for(j=y;j<y+h;j++)
				for(k=z;k<z+d;z++){
					unchanged = TRUE;
					for(r=0;r<numSpecs;r++){
						if( IS_WITHIN(hist.bins[(int )(data[k][i][j]-hist.minPixel)], thresholdSpec[r].low, thresholdSpec[r].high) ){
							unchanged = FALSE;  changeMap[k][i][j] = unchanged;
							if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
								data[k][i][j] = thresholdSpec[r].newValue;
								changeMap[k][i][j] = changed;
								count++;
							}
							break; /* assume does not belong to any other range */
						}
					}
					if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
						data[k][i][j] = defaultColor;
						changeMap[k][i][j] = default_color;
						count++;
					}
				}
	} else {
		for(i=x;i<x+w;i++)
			for(j=y;j<y+h;j++)
				for(k=z;k<z+d;z++){
					unchanged = TRUE;
					for(r=0;r<numSpecs;r++){
						if( IS_WITHIN(hist.bins[(int )(data[k][i][j]-hist.minPixel)], thresholdSpec[r].low, thresholdSpec[r].high) ){
							unchanged = FALSE;
							if( thresholdSpec[r].newValue != THRESHOLD_LEAVE_UNCHANGED ){
								data[k][i][j] = thresholdSpec[r].newValue;
								count++;
							}
							break; /* assume does not belong to any other range */
						}
					}
					if( unchanged && (defaultColor!=THRESHOLD_LEAVE_UNCHANGED) ){
						data[k][i][j] = defaultColor;
						count++;
					}
				}
	}
	return count;
}

