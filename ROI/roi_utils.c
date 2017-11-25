#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "Common_IMMA.h"
#include "Alloc.h"
#include "IO.h"
#include "filters.h"
#include "spiral.h"

#include "roi_utils.h"

/* private (comparators for qsort) */
static	int	_roi_irregular_region_comparator_ascending(const void *, const void *);
//static	int	_roi_irregular_region_comparator_descending(const void *, const void *);
typedef struct  _ROI_IRREGULAR_REGION_SORT_DATA {
	float	value;
	int	index, x, y, z;
} _roi_irregular_region_sort_data;

/* private function wich checks if value is between rMin and rMax */
int	_check_filter_criterion(float /*value*/, float /*rMin*/, float /*rMax*/);
int	_check_filter_criterion_datatype(DATATYPE /*value*/, float /*rMin*/, float /*rMax*/);

extern	void	roiedge(int ix[], int iy[], int npt, int *uyp[], int np[], int ny, int fill_mode);


#define	ROI_SELECT_GROUPS_MAX_SEARCH	20000

/* it will select numGroups groups of points (each of numPointsPerGroup points)
   points may or may not be reused.
   if *numGroups contains 0, then all possible groups (until all points are exhausted)
   will be returned, if the shouldReusePoints flag is TRUE.

   returns the points as a *** pointer to int indexed as follows:
	[group][point in that group][X|Y|Z = 0|1|2]

   *numGroups will contain the final number of groups returned.

   ***uncData :	  3D (volume) UNC data - it is supplied so that only non-zero pixels are returned.
   		  DO NOT FORGET : uncData is indexed as [slice = z][x][y]

   x,y,z,W,H,D:   the bounding box where you want the points selected to be.

   you must free the returned array when finished with it. DO NOT free the contents of the array
   (e.g. the roiPoint * because those belong to each a_roi and will be fred when necessary by the system.
*/
/* notice that because pixel sizes vary, memory and image coordinates differ.
   Memory coordinates : the image data array read from a UNC file (xm, ym).
   Image coordinates  : the coordinates you get when using dispunc and click on image (xi, yi).
	xi = xm * pixel_size_x
	yi = ym * pixel_size_y
   roi coordinates are usually image coordinates (so they will have to be divided by pixel size
   in order to access the memory data array.

   for example if pixel_size_x is 0.1, then there is different pixel of x = 0.0, x = 0.1, x = 0.2 etc.
   but these pixels in memory are integers e.g. data[0], data[1] etc.
   e.g. 0.1 -> 1 e.g. 0.1 / pixel_size_x = 1
   e.g. memory = image / pixel_size;
*/
/* this routine will return points which are corrected for the pixel size, e.g. these points could
   be displayed but can not be used to access memory (in order to do that, you have to multiply by pixel_size */
int ***rois_select_groups_of_inside_points_negative(roi **a_rois, int numRois, char shouldReusePoints, int *numGroups, int numPointsPerGroup, DATATYPE ****uncData, int numUNCVolumes, int x, int y, int z, int W, int H, int D, int numSlicesBelow, int numSlicesAbove){
	char		***scratchPad;
	int		count;
	register int	count2 = 0,
			i, j, k, inp, numSelected = 0,
			groupIndex, xxx = 0, yyy = 0, zzz = 0;
	int		***returnedGroups, **selected;
	spiral		*mySpiral;

	if( (selected=(int **)malloc(numPointsPerGroup*sizeof(int *))) == NULL ){
		fprintf(stderr, "rois_select_groups_of_inside_points_negative : could not allocate %zd bytes for selected.\n", numPointsPerGroup*sizeof(int *));
		return NULL;
	}
	for(i=0;i<numPointsPerGroup;i++)
		if( (selected[i]=(int *)malloc(2*sizeof(int))) == NULL ){
			fprintf(stderr, "rois_select_groups_of_inside_points_negative : could not allocate %zd bytes for selected[%d].\n", 2*sizeof(int), i);
			return NULL;
		}
	if( (returnedGroups=(int ***)malloc(*numGroups*sizeof(int **))) == NULL ){
		fprintf(stderr, "rois_select_groups_of_inside_points_negative : could not allocate %zd bytes for returnedGroups.\n", *numGroups*sizeof(int **));
		for(i=0;i<numPointsPerGroup;i++) free(selected[i]); free(selected);
		return NULL;
	}
	for(i=0;i<*numGroups;i++){
		if( (returnedGroups[i]=(int **)malloc(numPointsPerGroup*sizeof(int *))) == NULL ){
			fprintf(stderr, "rois_select_groups_of_inside_points_negative : could not allocate %zd bytes for returnedGroups[%d].\n", numPointsPerGroup*sizeof(int *), i);
			for(i=0;i<numPointsPerGroup;i++) free(selected[i]); free(selected); free(returnedGroups);
			return NULL;
		}
		for(j=0;j<numPointsPerGroup;j++)
			if( (returnedGroups[i][j]=(int *)malloc(3*sizeof(int))) == NULL ){
				fprintf(stderr, "rois_select_groups_of_inside_points_negative : could not allocate %zd bytes for returnedGroups[%d][%d].\n", 3*sizeof(int), i, j);
				for(i=0;i<numPointsPerGroup;i++) free(selected[i]); free(selected); free(returnedGroups);
				return NULL;
			}
	}
	/* each roi, black pixels and pixels already selected will be marked on the scratched */
	if( (scratchPad=callocCHAR3D(D, W, H)) == NULL ){
		fprintf(stderr, "rois_select_groups_of_inside_points_negative : call to callocCHAR3D has failed for a scratch pad of %d x %d x %d.\n", W, H, D);
		for(i=0;i<*numGroups;i++) free(returnedGroups[i]); for(i=0;i<numPointsPerGroup;i++) free(selected[i]); free(selected); free(returnedGroups);
		return NULL;
	}
	/* scratch pad : 0 -> not used, 1 -> used, 2 -> used temporarily (e.g reset to unused if a new epicentre needs to be selected) */
	/* all black pixels and points inside rois should be marked as used (1) */
	if( (uncData==NULL) || (numUNCVolumes<=0) ) for(k=0;k<D;k++) for(i=0;i<W;i++) for(j=0;j<H;j++) scratchPad[k][i][j] = 0;
	else {
		/* can't use these first and last slices */
		for(k=0;k<numSlicesBelow;k++) for(i=0;i<W;i++) for(j=0;j<H;j++) scratchPad[k][i][j] = 1;
		for(k=D-numSlicesAbove;k<D;k++) for(i=0;i<W;i++) for(j=0;j<H;j++) scratchPad[k][i][j] = 1;
		
		/* now check if this point contains any backgr. points over all numslices above and below and current */
		for(k=numSlicesBelow;k<(D-numSlicesAbove);k++) for(i=0;i<W;i++) for(j=0;j<H;j++){
			/* yyy stands for the predicate `is not background' and can be true or false -
			   just to save on some registers */
			yyy = TRUE;
			for(inp=0;(inp<numUNCVolumes)&&yyy;inp++)
				for(xxx=(k-numSlicesBelow);(xxx<=(k+numSlicesAbove))&&yyy;xxx++)
					yyy = yyy && (uncData[inp][xxx][i][j]>0);
			/* if there is at least 1 background pixel in the pillar of pixels
			   from numSlicesBelow, current and numSlicesAbove, (in any of the input images/inp)
			   then mark scratchPad with 1, otherwise, if all pixels are non-background,
			   scratchPad is marked as 0 */
			scratchPad[k][i][j] = yyy ? 0 : 1;
			/*printf("checking %d,%d,%d = %d ", i,j,k, yyy);
			for(xxx=(k-numSlicesBelow);xxx<=(k+numSlicesAbove);xxx++) printf(" %d", uncData[0][xxx][i][j]);
			if( yyy == FALSE ) printf(" REJECTED");
			printf("\n");*/
		}
	}
	/* now roi points */	
	for(inp=0;inp<numRois;inp++) for(i=0;i<a_rois[inp]->num_points_inside;i++) scratchPad[a_rois[inp]->points_inside[i]->Z][a_rois[inp]->points_inside[i]->X][a_rois[inp]->points_inside[i]->Y] = 1;

	count = 0; groupIndex = 0;
	if( (mySpiral=spiral_new(
		0, 0, 0,	/* epicentre - will change as soon as we pick a point */
		x, y, z,	/* bottom left */
		x+W, y+H, z+D,	/* top right */
		1, 1, 1, 1	/* steps in left, right, up and down directions (see spiral.h) respectively */
	   )) == NULL ){
	   	fprintf(stderr, "rois_select_groups_of_inside_points_negative : call to spiral_new has failed.\n");
			for(i=0;i<*numGroups;i++) free(returnedGroups[i]); for(i=0;i<numPointsPerGroup;i++) free(selected[i]); free(selected); free(returnedGroups);
			freeCHAR3D(scratchPad, D, W);
			return NULL;
	}
	   	
	/* choose points outside the ROIS and also inside the boundaries (if any) given and non-zero (check uncData) */
	if( shouldReusePoints == FALSE ){
		count = 0;
		while( TRUE ){
			if( count++ > ROI_SELECT_GROUPS_MAX_SEARCH ){ count = -1; break; }

			count2 = 0;
			numSelected = 0; /* epicentre goes first in the list - use xxx, yyy, zzz for fast access */
			do {
				if( count2++ > ROI_SELECT_GROUPS_MAX_SEARCH ){ count2 = -1; break; }
				selected[numSelected][0] = xxx = x + lrand48() % W; /* pick a point within the memory bounds of the image */
				selected[numSelected][1] = yyy = y + lrand48() % H;
							   zzz = z + lrand48() % D;
			} while( scratchPad[zzz][xxx][yyy] != 0 ); /* get an epicentre which is not used */
			numSelected++;

			if( count2 == -1 ){
				fprintf(stderr, "rois_select_groups_of_inside_points_negative : could only manage to find %d points (of %d requested) for group %d (of %d requested) after %d iterations - can not continue.\n", numSelected, numPointsPerGroup, groupIndex, *numGroups, ROI_SELECT_GROUPS_MAX_SEARCH);
				for(i=0;i<numPointsPerGroup;i++) free(selected[i]); free(selected);
				for(i=0;i<*numGroups;i++){
					for(j=0;j<numPointsPerGroup;j++) free(returnedGroups[i][j]);
					free(returnedGroups[i]);
				}
				free(returnedGroups); freeCHAR3D(scratchPad, D, W); spiral_destroy(mySpiral);
				return NULL;
			}

			if( numPointsPerGroup > 1 ){
				/* if we need more points (sometimes we may need only 1 point, the epicentre */
				scratchPad[zzz][xxx][yyy] = 2; /* mark this point as temporary select */

				spiral_reset(mySpiral, xxx, yyy, zzz); /* tell spiral the new epicentre */

				while( spiral_next_point(mySpiral) ){
					if( !(mySpiral->hasMore) ) break;
					xxx=mySpiral->x; yyy=mySpiral->y;
					if( (xxx>=W) || (yyy>=H) || (zzz>=D) ) fprintf(stderr, "out of bounds %d,%d,%d (1)\n", xxx, yyy, zzz);
					if( scratchPad[zzz][xxx][yyy] == 0 ){
						selected[numSelected][0] = xxx;
						selected[numSelected][1] = yyy;
						/* z is the same for all the points */
						numSelected++;
					}
					if( numSelected == numPointsPerGroup ) break;
				}
			} /* if we need more points */
			
			/* got out of bounds without enough points ? , repeat process */
			if( numSelected < numPointsPerGroup ){
				/* reset the scratch first - we did not use these points after all */
				for(i=0;i<numSelected;i++) scratchPad[zzz][selected[i][0]][selected[i][1]] &= 1;
				continue;
			}

			/* everything is fine, finalise */
			for(i=0;i<numPointsPerGroup;i++){
				xxx = returnedGroups[groupIndex][i][0] = selected[i][0];
				yyy = returnedGroups[groupIndex][i][1] = selected[i][1];
				returnedGroups[groupIndex][i][2] = zzz;
				scratchPad[zzz][xxx][yyy] = 1; /* used that is */
			}

			/* go to the next group */
			if( ++groupIndex >= *numGroups ) break; /* done, got enough groups (lines of data that is) */

		} /* while (count2) */
		if( count2 < 0 ){
			fprintf(stderr, "rois_select_groups_of_inside_points_negative : could only manage to find %d points (of %d requested) for group %d (of %d requested) after %d iterations - can not continue.\n", numSelected, numPointsPerGroup, groupIndex, *numGroups, ROI_SELECT_GROUPS_MAX_SEARCH);
			for(i=0;i<numPointsPerGroup;i++) free(selected[i]); free(selected);
			for(i=0;i<*numGroups;i++){
				for(j=0;j<numPointsPerGroup;j++) free(returnedGroups[i][j]);
				free(returnedGroups[i]);
			}
			free(returnedGroups); freeCHAR3D(scratchPad, D, W); spiral_destroy(mySpiral);
			return NULL;
		}
	} else { /* if/else shouldReusePoints */
		/* here we can reuse points */
		count = 0;
		while( TRUE ){
			if( count++ > ROI_SELECT_GROUPS_MAX_SEARCH ){ count = -1; break; }

			count2 = 0;
			numSelected = 0; /* epicentre goes first in the list - use xxx, yyy, zzz for fast access */
			do {
				if( count2++ > ROI_SELECT_GROUPS_MAX_SEARCH ){ count2 = -1; break; }
				selected[numSelected][0] = xxx = x + lrand48() % W; /* pick a point within the memory bounds of the image */
				selected[numSelected][1] = yyy = y + lrand48() % H;
							   zzz = z + lrand48() % D;
//				if( (xxx>=W) || (yyy>=H) || (zzz>=D) ) fprintf(stderr, "out of bounds %d,%d,%d (1)\n", xxx, yyy, zzz);

			} while( scratchPad[zzz][xxx][yyy] != 0 ); /* get an epicentre which is not black pixel/within roi */
			numSelected++;

			if( count2 == -1 ){
				fprintf(stderr, "rois_select_groups_of_inside_points_negative : could only manage to find %d points (of %d requested) for group %d (of %d requested) after %d iterations - can not continue.\n", numSelected, numPointsPerGroup, groupIndex, *numGroups, ROI_SELECT_GROUPS_MAX_SEARCH);
				for(i=0;i<numPointsPerGroup;i++) free(selected[i]); free(selected);
				for(i=0;i<*numGroups;i++){
					for(j=0;j<numPointsPerGroup;j++) free(returnedGroups[i][j]);
					free(returnedGroups[i]);
				}
				free(returnedGroups); freeCHAR3D(scratchPad, D, W); spiral_destroy(mySpiral);
				return NULL;
			}

			if( numPointsPerGroup > 1 ){
				/* if we need more points (sometimes we may need only 1 point, the epicentre */
				spiral_reset(mySpiral, xxx, yyy, zzz); /* spiral with the new epicentre */

				while( spiral_next_point(mySpiral) ){
					if( !(mySpiral->hasMore) ) break;
					xxx=mySpiral->x; yyy=mySpiral->y;
//					if( (xxx>=W) || (yyy>=H) || (zzz>=D) ) fprintf(stderr, "out of bounds %d,%d,%d (1)\n", xxx, yyy, zzz);
					if( scratchPad[zzz][xxx][yyy] == 0 ){
						selected[numSelected][0] = xxx;
						selected[numSelected][1] = yyy;
						/* z is the same for all the points */
						numSelected++;
					}
					if( numSelected == numPointsPerGroup ) break;
				}
			} /* if we need more points */

			/* got out of bounds without enough points ? , repeat process */
			if( numSelected < numPointsPerGroup ){
				/* reset the scratch first - we did not use these points after all */
				for(i=0;i<numSelected;i++) scratchPad[zzz][selected[i][0]][selected[i][1]] &= 1;
				continue;
			}
				
			/* everything is fine, finalise */
			for(i=0;i<numPointsPerGroup;i++){
				returnedGroups[groupIndex][i][0] = selected[i][0];
				returnedGroups[groupIndex][i][1] = selected[i][1];
				returnedGroups[groupIndex][i][2] = zzz;
				/*printf("adding %d,%d,%d = %d,%d, s=%d\n", selected[i][0], selected[i][1], zzz, uncData[0][zzz][selected[i][0]][selected[i][1]], uncData[1][zzz][selected[i][0]][selected[i][1]], scratchPad[zzz][selected[i][0]][selected[i][1]]);*/
			}

			/* go to the next group */
			if( ++groupIndex >= *numGroups ) break; /* done, got enough groups (lines of data that is) */

		} /* while (count2) */
		if( count2 < 0 ){
			fprintf(stderr, "rois_select_groups_of_inside_points_negative : could only manage to find %d points (of %d requested) for group %d (of %d requested) after %d iterations - can not continue.\n", numSelected, numPointsPerGroup, groupIndex, *numGroups, ROI_SELECT_GROUPS_MAX_SEARCH);
			for(i=0;i<numPointsPerGroup;i++) free(selected[i]); free(selected);
			for(i=0;i<*numGroups;i++){
				for(j=0;j<numPointsPerGroup;j++) free(returnedGroups[i][j]);
				free(returnedGroups[i]);
			}
			free(returnedGroups); freeCHAR3D(scratchPad, D, W); spiral_destroy(mySpiral);
			return NULL;
		}
	} /* else shouldReusePoints */

	if( count == -1 ){
		fprintf(stderr, "rois_select_groups_of_inside_points_negative : warning, could only manage to find %d groups (of %d requested - %d points per group).\n", groupIndex, *numGroups, numPointsPerGroup);
		*numGroups = groupIndex;
	}

	freeCHAR3D(scratchPad, D, W); spiral_destroy(mySpiral);
	for(i=0;i<numPointsPerGroup;i++) free(selected[i]); free(selected);
	return returnedGroups;
}

/* it will select *numGroups groups of points (each of numPointsPerGroup points)
   from all the rois within the a_rois array from the parameters list.
   points may or may not be reused (use the shouldReusePoints flag).
   if *numGroups contains 0, then all possible groups (until all points are exhausted)
   will be returned, if the shouldReusePoints flag is TRUE.

   returns the points as a ** pointer to roiPoint indexed as follows:
	* [group][point in the group]

   *numGroups will contain the final number of groups returned.

   x,y,z,W,H,D:   the bounding box where you want the points selected to be.

   you must free the returned array when finished with it. DO NOT free the contents of the array
   (e.g. the roiPoint * because those belong to each a_roi and will be fred when necessary by the system.
*/
roiPoint ***rois_select_groups_of_inside_points(roi **a_rois, int numRois, char shouldReusePoints, int *numGroups, int numPointsPerGroup, int x, int y, int z, int W, int H, int D){
	int		*numGroupsPerCallReturned, numCalls, i, j, k, count, a_roi,
			totalNumberOfGroupsReturned, requestedGroupsPerCall;
	roiPoint	***roi_points, ***returnedGroups, ****tempStorage;

	if( (numGroupsPerCallReturned=(int *)malloc(10*(*numGroups)*sizeof(int))) == NULL ){
		fprintf(stderr, "rois_select_groups_of_inside_points : could not allocate %zd bytes for numGroupsPerCallReturned.\n", *numGroups*sizeof(int));
		return NULL;
	}
	if( (tempStorage=(roiPoint ****)malloc(10*(*numGroups)*sizeof(roiPoint ***))) == NULL ){
		fprintf(stderr, "rois_select_groups_of_inside_points : could not allocate %zd bytes for tempStorage.\n", *numGroups*sizeof(roiPoint ***));
		return NULL;
	}
	count = 0; numCalls = 0; totalNumberOfGroupsReturned = 0;
	while( totalNumberOfGroupsReturned < *numGroups ){
		if( count++ > ROI_SELECT_GROUPS_MAX_SEARCH ){ count = -1; break; }

		a_roi = lrand48() % numRois;
		/* if this roi does not contain enough points inside, then skip */
		if( a_rois[a_roi]->num_points_inside < numPointsPerGroup ) continue;

		requestedGroupsPerCall = 1; /* just ask for 1 group per roi - we can reuse rois */

		if( (tempStorage[numCalls]=roi_select_groups_of_inside_points(
				a_rois[a_roi],
				shouldReusePoints,
				&requestedGroupsPerCall,
				numPointsPerGroup,
				x, y, z, W, H, D)) == NULL ){
			fprintf(stderr, "rois_select_groups_of_inside_points : call to roi_select_groups_of_inside_points has failed for roi %d, number of groups per roi requested was %d, number of points per group was %d.\n", a_roi, requestedGroupsPerCall, numPointsPerGroup);
			for(i=0;i<numCalls;i++){
				roi_points = tempStorage[i];
				for(j=0;j<numGroupsPerCallReturned[i];j++) free(roi_points[j]);
				free(roi_points);
			}
			free(tempStorage);
			return NULL;
		}
		numGroupsPerCallReturned[numCalls] = requestedGroupsPerCall;
		totalNumberOfGroupsReturned += requestedGroupsPerCall;
		if( numCalls++ >= 10*(*numGroups) ){
			fprintf(stderr, "rois_select_groups_of_inside_points : number of calls was %d but only %d groups were yielded - will stop it.\n", 10*(*numGroups), totalNumberOfGroupsReturned);
			break;
		}
	}			

	if( count == -1 )
		fprintf(stderr, "rois_select_groups_of_inside_points : could not manage to find %d groups after %d iterations - num groups found was %d.\n", *numGroups, ROI_SELECT_GROUPS_MAX_SEARCH, totalNumberOfGroupsReturned);
	*numGroups = totalNumberOfGroupsReturned;

	/* now prepare an array of roiPoint *** to be returned - e.g. merge all groups from all rois into a common pool */
	if( (returnedGroups=(roiPoint ***)malloc(totalNumberOfGroupsReturned*sizeof(roiPoint **))) == NULL ){
		fprintf(stderr, "rois_select_groups_of_inside_points : could not allocate %zd bytes for returnedGroups.\n", totalNumberOfGroupsReturned*sizeof(roiPoint **));
		return NULL;
	}
	for(i=0,k=0;i<numCalls;i++){
		roi_points = (roiPoint ***)(tempStorage[i]);
		for(j=0;j<numGroupsPerCallReturned[i];j++)
			returnedGroups[k++] = roi_points[j];
	}
	free(tempStorage); free(numGroupsPerCallReturned);

	return returnedGroups;
}


/* it will select numGroups groups of points (each of numPointsPerGroup points)
   points may or may not be reused.
   if *numGroups contains 0, then all possible groups (until all points are exhausted)
   will be returned, if the shouldReusePoints flag is TRUE.

   returns the points as a ** pointer indexed as follows:
	* [group][point in that group]

   *numGroups will contain the final number of groups returned.

   x,y,z,W,H,D:   the bounding box where you want the points selected to be.

   you must free the returned array when finished with it. DO NOT free the contents of the array
   (e.g. the roiPoint * because those belong to each a_roi and will be fred when necessary by the system.
*/
roiPoint ***roi_select_groups_of_inside_points(roi *a_roi, char shouldReusePoints, int *numGroups, int numPointsPerGroup, int x, int y, int z, int W, int H, int D){
	int		*selected, count, numSelected, *points_selected = NULL,
			i, groupIndex, a_point, xW, xH, xD, num_points_selected;
	_roi_irregular_region_sort_data	*distances;
	roiPoint	***returnedGroups;
	char		shouldContinue, haveImageInfo = a_roi->minPixel>=0,
			boundariesExist = (x>=0) && (y>=0) && (z>=0) && (W>0) && (H>0) && (D>0);

	if( (selected=(int *)malloc(numPointsPerGroup*sizeof(int))) == NULL ){
		fprintf(stderr, "roi_select_groups_of_inside_points : could not allocate %zd bytes for selected.\n", numPointsPerGroup*sizeof(int));
		return NULL;
	}
	if( (returnedGroups=(roiPoint ***)malloc(*numGroups*sizeof(roiPoint **))) == NULL ){
		fprintf(stderr, "roi_select_groups_of_inside_points : could not allocate %zd bytes for returnedGroups.\n", *numGroups*sizeof(roiPoint *));
		free(selected);
		return NULL;
	}
	for(i=0;i<*numGroups;i++)
		if( (returnedGroups[i]=(roiPoint **)malloc(numPointsPerGroup*sizeof(roiPoint *))) == NULL ){
			fprintf(stderr, "roi_select_groups_of_inside_points : could not allocate %zd bytes for returnedGroups.\n", numPointsPerGroup*sizeof(roiPoint *));
			free(selected); free(returnedGroups);
			return NULL;
		}
	if( (distances=(_roi_irregular_region_sort_data *)malloc(a_roi->num_points_inside*sizeof(_roi_irregular_region_sort_data))) == NULL ){
		fprintf(stderr, "roi_select_groups_of_inside_points : could not allocate %zd bytes for selected.\n", a_roi->num_points_inside*sizeof(_roi_irregular_region_sort_data));
		free(selected); for(i=0;i<*numGroups;i++) free(returnedGroups[i]); free(returnedGroups);
		return NULL;
	}
	if( shouldReusePoints == FALSE )
		if( (points_selected=(int *)malloc(a_roi->num_points_inside*sizeof(int))) == NULL ){
			fprintf(stderr, "rois_select_groups_of_inside_points : could not allocate %zd bytes for points_selected.\n", a_roi->num_points_inside*sizeof(int));
			free(selected); for(i=0;i<*numGroups;i++) free(returnedGroups[i]); free(returnedGroups); free(distances);
			return NULL;
		}

	count = 0; groupIndex = 0; num_points_selected = 0;
	xW = x + W; xH = x + H; xD = z + D;
	/* choose points inside the ROIS and also inside the boundaries (if any) given */
	if( shouldReusePoints == FALSE ){
		while( TRUE ){
			if( count++ > ROI_SELECT_GROUPS_MAX_SEARCH ){ count = -1; break; }
			shouldContinue = TRUE;
			while( shouldContinue ){
				/* do not select for an epicentre a selected point */
				a_point = lrand48() % a_roi->num_points_inside;
				/* check if it is non-background */
				if( haveImageInfo ) shouldContinue = a_roi->points_inside[a_point]->v > 0;
				else shouldContinue = FALSE;
				for(i=0;i<num_points_selected;i++) if( a_point == points_selected[i] ){ shouldContinue = TRUE; break; }
			}

			for(i=0;i<a_roi->num_points_inside;i++){
				distances[i].index = i;
				if( boundariesExist &&
				    ((a_roi->points_inside[a_point]->x < x) || (a_roi->points_inside[a_point]->y < y) || (a_roi->points_inside[a_point]->z < z) ||
				     (a_roi->points_inside[a_point]->x >= xW) || (a_roi->points_inside[a_point]->y >= xH) || (a_roi->points_inside[a_point]->z >= xD)) )
				    	distances[i].value = -1.0; /* outside the boundaries */
				else distances[i].value = 0.0;
			}
			/* ignore epicentre and all selected points */
			distances[a_point].value = -1;
			for(i=0;i<num_points_selected;i++) distances[points_selected[i]].value = -1;
			
			/* count distances from this point to all other points in the roi and pick the nearest */
			for(i=0;i<a_roi->num_points_inside;i++){
				if( distances[i].value == -1 ) continue;
				distances[i].value =
					SQR(a_roi->points_inside[i]->x - a_roi->points_inside[a_point]->x) +
					SQR(a_roi->points_inside[i]->y - a_roi->points_inside[a_point]->y); /* no need for sqrt */
			}
			/* sort the distances in ascending order */
			qsort((void *)distances, a_roi->num_points_inside, sizeof(_roi_irregular_region_sort_data), _roi_irregular_region_comparator_ascending);

			/* pick up the nearest numPointsPerGroup points (if any) including the epicentre (a_point) */
			numSelected = 0; selected[numSelected++] = a_point; /* epicentre goes first in the list */
			for(i=0;i<a_roi->num_points_inside;i++){
				if( distances[i].value > 0 ) selected[numSelected++] = distances[i].index;
				if( numSelected >= numPointsPerGroup ) break;
			}
			if( numSelected < numPointsPerGroup ) continue; /* not enough points, choose another epicentre */

			/* otherwise, save the selected points into the returned array and also
			   tick them off as we are not allowed to reuse them */
			for(i=0;i<numSelected;i++)
				returnedGroups[groupIndex][i] = a_roi->points_inside[selected[i]];
			if( ++groupIndex >= *numGroups ) break;
			for(i=0;i<numSelected;i++) points_selected[i] = selected[i];
			num_points_selected += numSelected;
		} /* while */
	} else { /* if shouldReusePoints */
		/* here WE ARE ALLOWED to reuse points */
		while( TRUE ){
			if( count++ > ROI_SELECT_GROUPS_MAX_SEARCH ){ count = -1; break; }
			a_point = lrand48() % a_roi->num_points_inside;
			/* check if it is non-background */
			if( haveImageInfo ) if( a_roi->points_inside[a_point]->v <= 0 ) continue;
//			printf("%d,%.0f<%.0f point (%d,%d,%d)=%.0f success\n", haveImageInfo, a_roi->minPixel, a_roi->maxPixel, a_roi->points_inside[a_point]->X, a_roi->points_inside[a_point]->Y, a_roi->points_inside[a_point]->Z, a_roi->points_inside[a_point]->v);

			/* count distances from this point to all other points in the roi and pick the nearest */
			for(i=0;i<a_roi->num_points_inside;i++){
				distances[i].index = i;
				if( boundariesExist &&
				    ((a_roi->points_inside[a_point]->x < x) || (a_roi->points_inside[a_point]->y < y) || (a_roi->points_inside[a_point]->z < z) ||
				     (a_roi->points_inside[a_point]->x >= xW) || (a_roi->points_inside[a_point]->y >= xH) || (a_roi->points_inside[a_point]->z >= xD)) )
				    	distances[i].value = -1.0; /* outside the boundaries */
				else distances[i].value =
					SQR(a_roi->points_inside[i]->x - a_roi->points_inside[a_point]->x) +
					SQR(a_roi->points_inside[i]->y - a_roi->points_inside[a_point]->y) +
					/* the rand num (0 to 1) is added here in order to resolve the ambiguity
					   when 2 points have the same distance from the epicentre. In this case
					   solaris qsort and linux qsort give a different order to the two points
					   with the result that linux and solaris often select different points
					   (which of course have the same distance from the epicentre).
					   this is not very consistent so adding a small random pertrubation will
					   make sure that no two distances are the same. The fact that the pertrubation
					   is small means that no significant problems */
					drand48() / 10.0;
					/* no need for sqrt - it will add overhead but distance comparisons will be the same */
			}
			distances[a_point].value = -1; distances[a_point].index = i;

			/* sort the distances in ascending order */
			qsort((void *)distances, a_roi->num_points_inside, sizeof(_roi_irregular_region_sort_data), _roi_irregular_region_comparator_ascending);
			
			/* pick up the nearest numPointsPerGroup points (if any) including the epicentre (a_point) */
			numSelected = 0; selected[numSelected++] = a_point; /* epicentre goes first in the list */
			if( haveImageInfo ){
				/* here we will reject background pixels that happen to be inside the roi,
				   neighbouring our epicentre */
				for(i=0;i<a_roi->num_points_inside;i++){
					if( (distances[i].value > 0) &&
					    (a_roi->points_inside[distances[i].index]->v>0) )
							selected[numSelected++] = distances[i].index;
					if( numSelected >= numPointsPerGroup ) break;
				}
			} else {
				for(i=0;i<a_roi->num_points_inside;i++){
					if( distances[i].value > 0 ) selected[numSelected++] = distances[i].index;
					if( numSelected >= numPointsPerGroup ) break;
				}
			}
			if( numSelected < numPointsPerGroup ) continue; /* not enough points, choose another epicentre */

			for(i=0;i<numSelected;i++)
				returnedGroups[groupIndex][i] = a_roi->points_inside[selected[i]];

			if( ++groupIndex >= *numGroups ) break;
		} /* while */
	} /* else shouldReusePoints */

	if( count == -1 ){
//		fprintf(stderr, "roi_select_groups_of_inside_points : warning, could only manage to find %d groups (of %d requested - %d points per group).\n", groupIndex, *numGroups, numPointsPerGroup);
		*numGroups = groupIndex;
	}

	free(selected); free(distances);
	if( points_selected != NULL ) free(points_selected);
	return returnedGroups;
}

/* will give a list of all points inside a roi, the list roi->points_inside is filled */
int	rois_get_inside_points(roi **a_rois, int numRois){
	int	i;
	for(i=0;i<numRois;i++)
		if( roi_get_inside_points(a_rois[i]) == FALSE ){
			fprintf(stderr, "rois_get_inside_points : call to roi_get_inside_points has failed for the %d roi.\n", i);
			return FALSE;
		}
	return TRUE;
}		
int	roi_get_inside_points(roi *a_roi){
	int	i, n;
	float	*x, *y;

	switch( a_roi->type ){
		case RECTANGULAR_ROI_REGION:
			if( roi_region_rectangular_get_inside_points((roiRegionRectangular *)(a_roi->roi_region), &x, &y, &n) == FALSE ){
				fprintf(stderr, "roi_get_inside_points : call to roi_region_rectangular_get_inside_points has failed.\n");
				return FALSE;
			}
			break;
		case ELLIPTICAL_ROI_REGION:
			if( roi_region_elliptical_get_inside_points((roiRegionElliptical *)(a_roi->roi_region), (int )(a_roi->x0), (int )(a_roi->y0), (int )(a_roi->width), (int )(a_roi->height)+1, &x, &y, &n) == FALSE ){
				fprintf(stderr, "roi_get_inside_points : call to roi_region_elliptical_get_inside_points has failed.\n");
				return FALSE;
			}
			break;
		case IRREGULAR_ROI_REGION:
			if( roi_region_irregular_get_inside_points((roiRegionIrregular *)(a_roi->roi_region), (int )(a_roi->x0), (int )(a_roi->y0), (int )(a_roi->height)+2, &x, &y, &n) == FALSE ){
				fprintf(stderr, "roi_get_inside_points : call to roi_region_irregular_get_inside_points has failed.\n");
				return FALSE;
			}
			break;
		case UNSPECIFIED_ROI_REGION:
			fprintf(stderr, "roi_get_inside_points : region type 'UNSPECIFIED_ROI_REGION' is not implemented.\n");
			return FALSE;
		default:
			fprintf(stderr, "roi_get_inside_points : region type '%d' is not implemented.\n", a_roi->type);
			return FALSE;
	}

	if( n > 0 ){
		/* free the previous points - if any */
		if( a_roi->num_points_inside > 0 ) roi_points_destroy(a_roi->points_inside, a_roi->num_points_inside);
		a_roi->num_points_inside = n;
		/* and create the new ones */
		if( (a_roi->points_inside=roi_points_new(a_roi->num_points_inside, a_roi)) == NULL ){
			fprintf(stderr, "roi_get_inside_points : call to roi_points_new has failed for %d points.\n", a_roi->num_points_inside);
			free(x); free(y);
			return FALSE;
		}
		for(i=0;i<a_roi->num_points_inside;i++){
			a_roi->points_inside[i]->x = x[i];
			a_roi->points_inside[i]->y = y[i];
			a_roi->points_inside[i]->z = a_roi->slice;

			a_roi->points_inside[i]->X = (int )ROUND(x[i]);
			a_roi->points_inside[i]->Y = (int )ROUND(y[i]);
			a_roi->points_inside[i]->Z = a_roi->slice; /* this needs to be corrected for pixel_size when you do the changes */
		}
		free(x); free(y);
	}

	return TRUE;
}

int	roi_region_elliptical_get_inside_points(roiRegionElliptical *a_region, int x0, int y0, int w, int h, float **_x, float **_y, int *total_points){

	/* some day ... */
	*total_points = 0;
	return TRUE;

	/* the area of an allipsis is the following : */
	/* a_roi->num_points_inside = (int )(M_PI * elliReg->ea * elliReg->eb); */
}

int	roi_region_rectangular_get_inside_points(roiRegionRectangular *a_region, float **_x, float **_y, int *total_points){
	int	i, j, l;
	float	*x, *y;

	*total_points = a_region->width * a_region->height;

	if( (x=(float *)malloc((*total_points)*sizeof(float))) == NULL ){
		fprintf(stderr, "roi_region_rectangular_get_inside_points : could not allocate %zd bytes for x.\n", (*total_points)*sizeof(float));
		return FALSE;
	}
	if( (y=(float *)malloc((*total_points)*sizeof(float))) == NULL ){
		fprintf(stderr, "roi_region_rectangular_get_inside_points : could not allocate %zd bytes for y.\n", (*total_points)*sizeof(float));
		free(x);
		return FALSE;
	}

	for(i=0,l=0;i<a_region->width;i++)
		for(j=0;j<a_region->height;j++){
			x[l] = i + a_region->x0;
			y[l] = j + a_region->y0;
			l++;
		}

	*_x = x; *_y = y;

	return TRUE;
}
int	roi_region_irregular_get_inside_points(roiRegionIrregular *a_region, int x0, int y0, int h, float **_x, float **_y, int *total_points){
	/* (x0,y0) is the coordinates of the left-top corner of the rectangle enclosing the roi
	   h is the height of this rectangle (they are part of the roi struct)
	   */

	int	*X, *Y, i, j, x1, x2;
	int	**uyp, *np, mnp, *sp, iy;
	float	*x, *y;

	if( (X=(int *)malloc(a_region->num_points*sizeof(int))) == NULL ){
		fprintf(stderr, "roi_region_irregular_get_inside_points : could not allocate %zd bytes for X.\n", a_region->num_points*sizeof(int));
		return FALSE;
	}
	if( (Y=(int *)malloc(a_region->num_points*sizeof(int))) == NULL ){
		fprintf(stderr, "roi_region_irregular_get_inside_points : could not allocate %zd bytes for X.\n", a_region->num_points*sizeof(int));
		free(X);
		return FALSE;
	}
	if( (uyp=(int **)malloc(h*sizeof(int *))) == NULL ){
		fprintf(stderr, "roi_region_irregular_get_inside_points : could not allocate %zd bytes for uyp.\n", h*sizeof(int *));
		free(X); free(Y);
		return FALSE;
	}
	if( (np=(int *)malloc(h*sizeof(int))) == NULL ){
		fprintf(stderr, "roi_region_irregular_get_inside_points : could not allocate %zd bytes for np.\n", h*sizeof(int));
		free(X); free(Y); free(uyp);
		return FALSE;
	}

	for(i=0;i<a_region->num_points;i++){
		X[i] = a_region->points[i]->x - x0;
		Y[i] = a_region->points[i]->y - y0;
	}

	/* in file 'roiedge.c' there is a constant called roi_fill_mode which can be 0 or 1.
	   depending on this edge pixels will or will not be included as part of the roi.
	   Using '0' as the fill mode selects points more conservatively. */
	roiedge(X, Y, a_region->num_points, uyp, np, h, 0); /* ripped from disproi.c of dispunc code */
	free(X); free(Y);

	*total_points = 0; /* ripped from disproi.c of dispunc code */
	for(iy=0;iy<h;iy++)
		for(mnp=np[iy],sp=uyp[iy];mnp>0;mnp-=2){
			x1 = *sp++;
			x2 = *sp++;
			i = (x2-x1) + 1;
			while( i-- ) (*total_points)++;
		}

	if( (x=(float *)malloc((*total_points)*sizeof(float))) == NULL ){
		fprintf(stderr, "roi_region_irregular_get_inside_points : could not allocate %zd bytes for x.\n", (*total_points)*sizeof(float));
		free(uyp);
		return FALSE;
	}
	if( (y=(float *)malloc((*total_points)*sizeof(float))) == NULL ){
		fprintf(stderr, "roi_region_irregular_get_inside_points : could not allocate %zd bytes for y.\n", (*total_points)*sizeof(float));
		free(x); free(uyp);
		return FALSE;
	}

	for(iy=0,j=0;iy<h;iy++) /* ripped from disproi.c of dispunc code */
		for(mnp=np[iy],sp=uyp[iy];mnp>0;mnp-=2){
			x1 = *sp++;
			x2 = *sp++;
			i = (x2-x1) + 1;
			while( i-- ){ x[j] = x1 + x0 + i; y[j] = iy + y0; j++; }
		}

	for(iy=0;iy<h;iy++) free(uyp[iy]); free(uyp);
	free(np);

	*_x = x; *_y = y;

	return TRUE;
}

roi	**rois_filter(roi **a_r, int numRois, int *newNumRois, roiRegionType rType, int numCriteria, roiFilterType *fType, float *rMin, float *rMax, char reverse){
	int	i, ii;
	roi	**new_r = NULL;
	char	nR = !reverse;

	*newNumRois = 0;
	/* this time we count how many rois fit the criterion - those of not the same type fit the criterion by default */
	for(i=0;i<numRois;i++)
		(*newNumRois) += (nR==roi_filter(a_r[i], rType, numCriteria, fType, rMin, rMax));

	if( *newNumRois == 0 ) return NULL; /* no rois satisfy the criteria */
	
	if( (new_r=rois_new(*newNumRois)) == NULL ){
		fprintf(stderr, "rois_filter : call to rois_new has failed for %d rois.\n", *newNumRois);
		return NULL;
	}

	/* this time we repeat what we did above */
	for(i=0,ii=0;i<numRois;i++)
		if( nR == roi_filter(a_r[i], rType, numCriteria, fType, rMin, rMax) )
			if( (new_r[ii++]=roi_copy(a_r[i])) == NULL ){
				fprintf(stderr, "rois_filter : call to roi_copy has failed (3).\n");
				rois_destroy(new_r, ii-1);
				return NULL;
			}

	return new_r;
}

int	_check_filter_criterion(float value, float rMin, float rMax){
	if( (rMin==-1) && (rMax==-1) ){
		fprintf(stderr, "_check_filter_criterion : rMin and rMax can not both be DONT_CARE.\n");
		return FALSE;
	}
	else if( rMin == -1 ) return value < rMax;
	else if( rMax == -1 ) return value >= rMin;
	else if( rMin == rMax ) return value == rMin;
	return (value>=rMin) && (value<rMax);
}	
int	_check_filter_criterion_datatype(DATATYPE value, float rMin, float rMax){
	if( (rMin==-1) && (rMax==-1) ){
		fprintf(stderr, "_check_filter_criterion_datatype : rMin and rMax can not both be DONT_CARE.\n");
		return FALSE;
	}
	else if( rMin == -1 ) return value < rMax;
	else if( rMax == -1 ) return value >= rMin;
	else if( rMin == rMax ) return value == rMin;
	return (value>=rMin) && (value<rMax);
}	
int	roi_filter(roi *a_roi, roiRegionType rType, int numCriteria, roiFilterType *fType, float *rMin, float *rMax){
	if( rType == UNSPECIFIED_ROI_REGION )
		return roi_region_general_filter(a_roi, numCriteria, fType, rMin, rMax);
	else {
		if( rType != a_roi->type ) return FALSE;
		else {
			switch( rType ){
				case RECTANGULAR_ROI_REGION:
					return roi_region_rectangular_filter((roiRegionRectangular *)(a_roi->roi_region), numCriteria, fType, rMin, rMax);
				case ELLIPTICAL_ROI_REGION:
					return roi_region_elliptical_filter((roiRegionElliptical *)(a_roi->roi_region), numCriteria, fType, rMin, rMax);
				case IRREGULAR_ROI_REGION:
					return roi_region_irregular_filter((roiRegionIrregular *)(a_roi->roi_region), numCriteria, fType, rMin, rMax);
				case UNSPECIFIED_ROI_REGION:
					return roi_region_general_filter(a_roi, numCriteria, fType, rMin, rMax);
				default:
					fprintf(stderr, "roi_filter : region type '%d' is not implemented.\n", rType);
					return FALSE;
			}
		}
	} /* if UNSPECIFIED_ROI_REGION */

	/* return FALSE; */ /* does not come here */
}
int	roi_region_general_filter(roi *a_roi, int numCriteria, roiFilterType *fType, float *rMin, float *rMax){
	int	ret = TRUE, i;

	for(i=0;i<numCriteria;i++){
		switch( fType[i] ){
			case ROI_FILTER_TYPE_GENERAL_CENTROID_X:
				ret = ret && _check_filter_criterion(a_roi->centroid_x, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_GENERAL_CENTROID_Y:
				ret = ret && _check_filter_criterion(a_roi->centroid_y, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_GENERAL_SLICE_NUMBER:
				/* note that slices start from 0 but this criterion starts from 1 */
				ret = ret && _check_filter_criterion(a_roi->slice+1, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_GENERAL_NUM_POINTS_INSIDE:
				ret = ret && _check_filter_criterion(a_roi->num_points_inside, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_GENERAL_X0:
				ret = ret && _check_filter_criterion(a_roi->x0, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_GENERAL_Y0:
				ret = ret && _check_filter_criterion(a_roi->y0, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_GENERAL_WIDTH:
				ret = ret && _check_filter_criterion(a_roi->width, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_GENERAL_HEIGHT:
				ret = ret && _check_filter_criterion(a_roi->height, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_GENERAL_TYPE:
				ret = ret && (a_roi->type==(roiRegionType )(rMin[i])); /* hack or fuck */
				break;
			case ROI_FILTER_TYPE_GENERAL_MIN_PIXEL:
				ret = ret && _check_filter_criterion_datatype(a_roi->minPixel, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_GENERAL_MAX_PIXEL:
				ret = ret && _check_filter_criterion_datatype(a_roi->maxPixel, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_GENERAL_MEAN_PIXEL:
				ret = ret && _check_filter_criterion_datatype(a_roi->meanPixel, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_GENERAL_STDEV_PIXEL:
				ret = ret && _check_filter_criterion_datatype(a_roi->stdevPixel, rMin[i], rMax[i]);
				break;
			default:
				break;
		} /* switch */

		if( !ret ) return FALSE;
	} /* for loop */

	return ret;
}
int	roi_region_irregular_filter(roiRegionIrregular *a_region, int numCriteria, roiFilterType *fType, float *rMin, float *rMax){
	int	ret = TRUE, i;

	for(i=0;i<numCriteria;i++){
		/* all criteria are ANDed */
		switch( fType[i] ){
			case ROI_FILTER_TYPE_IRREGULAR_NUM_POINTS:
				ret = ret && _check_filter_criterion(a_region->num_points, rMin[i], rMax[i]);
				break;
			default:
				/* criteria for elliptical, irregular and rectangular, all come here */
				break;
		}
		if( !ret ) return FALSE;
	}

	return ret;
}
int	roi_region_rectangular_filter(roiRegionRectangular *a_region, int numCriteria, roiFilterType *fType, float *rMin, float *rMax){
	int	ret = TRUE, i;

	for(i=0;i<numCriteria;i++){
		/* all criteria are ANDed */
		switch( fType[i] ){
			case ROI_FILTER_TYPE_RECTANGULAR_X0:
				ret = ret && _check_filter_criterion(a_region->x0, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_RECTANGULAR_Y0:
				ret = ret && _check_filter_criterion(a_region->y0, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_RECTANGULAR_WIDTH:
				ret = ret && _check_filter_criterion(a_region->width, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_RECTANGULAR_HEIGHT:
				ret = ret && _check_filter_criterion(a_region->height, rMin[i], rMax[i]);
				break;
			default:
				/* criteria for elliptical, irregular and rectangular, all come here */
				break;
		}
		if( !ret ) return FALSE;
	}

	return ret;
}
int	roi_region_elliptical_filter(roiRegionElliptical *a_region, int numCriteria, roiFilterType *fType, float *rMin, float *rMax){
	int	ret = TRUE, i;

	for(i=0;i<numCriteria;i++){
		/* all criteria are ANDed */
		switch( fType[i] ){
			case ROI_FILTER_TYPE_ELLIPTICAL_EX0:
				ret = ret && _check_filter_criterion(a_region->ex0, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_ELLIPTICAL_EY0:
				ret = ret && _check_filter_criterion(a_region->ey0, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_ELLIPTICAL_EA:
				ret = ret && _check_filter_criterion(a_region->ea, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_ELLIPTICAL_EB:
				ret = ret && _check_filter_criterion(a_region->eb, rMin[i], rMax[i]);
				break;
			case ROI_FILTER_TYPE_ELLIPTICAL_ROT:
				ret = ret && _check_filter_criterion(a_region->rot, rMin[i], rMax[i]);
				break;
			default:
				/* criteria for elliptical, irregular and rectangular, all come here */
				break;
		}
		if( !ret ) return FALSE;
	}

	return ret;
}
	
int	rois_calculate(roi **a_r, int numRois){
	int	i;

	for(i=0;i<numRois;i++)
		if( !roi_calculate(a_r[i]) ){
			fprintf(stderr, "rois_calculate : call to roi_calculate has failed for %d roi.\n", i);
			return FALSE;
		}
	return TRUE;
}
int	roi_calculate(roi *a_r){

	switch( a_r->type ){
		case RECTANGULAR_ROI_REGION:
			roi_calculate_rectangular_region((roiRegionRectangular *)(a_r->roi_region), a_r);
 			break;
		case ELLIPTICAL_ROI_REGION:
			roi_calculate_elliptical_region((roiRegionElliptical *)(a_r->roi_region), a_r);
 			break;
		case IRREGULAR_ROI_REGION:
			roi_calculate_irregular_region((roiRegionIrregular *)(a_r->roi_region), a_r);
			break;
		default:
			fprintf(stderr, "roi_calculate : type %d not yet implemented.\n",  a_r->type);
			return FALSE;
	} /* end of switch */

	/* num of points inside the roi */
	if( roi_get_inside_points(a_r) == FALSE ){
		fprintf(stderr, "roi_calculate : call to roi_get_inside_points has failed.\n");
		return FALSE;
	}
	return TRUE;
}
void	roi_calculate_elliptical_region(roiRegionElliptical *elliReg, roi *a_roi){

	/* a_roi is optional, you can leave it null. if you supply it, then its centroid, area etc. fields will be filled here */
	if( a_roi != NULL ){
		a_roi->centroid_x = elliReg->ex0;
		a_roi->centroid_y = elliReg->ey0;
		/* bounding rectangle */
		a_roi->x0 = elliReg->ex0 - elliReg->ea;
		a_roi->y0 = elliReg->ey0 - elliReg->eb;
		a_roi->width = 2 * elliReg->ea;
		a_roi->height = 2 * elliReg->eb;
	}
}
void	roi_calculate_rectangular_region(roiRegionRectangular *rectReg, roi *a_roi){

	/* a_roi is optional, you can leave it null. if you supply it, then its centroid, area etc. fields will be filled here */
	if( a_roi != NULL ){
		a_roi->centroid_x = rectReg->x0 + rectReg->width / 2;
		a_roi->centroid_y = rectReg->y0 + rectReg->height/ 2;
		/* bounding rectangle */
		a_roi->x0 = rectReg->x0;
		a_roi->y0 = rectReg->y0;
		a_roi->width = rectReg->width;
		a_roi->height = rectReg->height;
	}
}
void	roi_calculate_irregular_region(roiRegionIrregular *irreReg, roi *a_roi){
	int	i;
	float	c_x, c_y;

	/* centroid : it has to be calculated but only if a_roi != NULL it will be set to the roi */
	for(i=0,c_x=c_y=0.0;i<irreReg->num_points;i++){
		c_x += irreReg->points[i]->x;
		c_y += irreReg->points[i]->y;
	}
	c_x /= irreReg->num_points;
	c_y /= irreReg->num_points;

	/* a_roi is optional, you can leave it null. if you supply it, then its centroid, area etc. fields will be filled here */
	if( a_roi != NULL ){
		/* centroid already calculated because is needed below for r and theta */
		a_roi->centroid_x = c_x;
		a_roi->centroid_y = c_y;

		/* bounding box */
		a_roi->x0 = irreReg->points[0]->x;
		a_roi->y0 = irreReg->points[0]->y;
		a_roi->width = a_roi->height = 0;
		for(i=0;i<irreReg->num_points;i++){
			/* bounding box top left corner */
			if( irreReg->points[i]->x < a_roi->x0 ) a_roi->x0 = irreReg->points[i]->x;
			if( irreReg->points[i]->y < a_roi->y0 ) a_roi->y0 = irreReg->points[i]->y;
		}

		for(i=0;i<irreReg->num_points;i++){
			/* bounding box dimensions */
			if( (irreReg->points[i]->x - a_roi->x0) > a_roi->width  ) a_roi->width = irreReg->points[i]->x - a_roi->x0;
			if( (irreReg->points[i]->y - a_roi->y0) > a_roi->height ) a_roi->height = irreReg->points[i]->y - a_roi->y0;
		}
	}
	
	/* calculate r and theta and deltas with previous point */
	for(i=0;i<irreReg->num_points;i++){
		irreReg->points[i]->r = sqrt( SQR(c_x - irreReg->points[i]->x) + SQR(c_y - irreReg->points[i]->y) );
		irreReg->points[i]->theta = atan2(c_y - irreReg->points[i]->y, c_x - irreReg->points[i]->x);
	}

	/* calculate deltas with previous point and slope */
	irreReg->points[0]->dr = irreReg->points[0]->dtheta = irreReg->points[0]->dx = irreReg->points[0]->dy = 0.0;
	for(i=1;i<irreReg->num_points;i++){
		irreReg->points[i]->dx = irreReg->points[i-1]->x - irreReg->points[i]->x;
		irreReg->points[i]->dy = irreReg->points[i-1]->y - irreReg->points[i]->y;
		irreReg->points[i]->dr = irreReg->points[i-1]->r - irreReg->points[i]->r;
		irreReg->points[i]->dtheta = irreReg->points[i-1]->theta - irreReg->points[i]->theta;

		irreReg->points[i]->slope = (irreReg->points[i]->dx==0) ? 10.0 : (irreReg->points[i]->dy / irreReg->points[i]->dx);
	}
}

/* This routine will write stats about a roi OR, if writeInfoForEachROIPeripheralPoint==TRUE,
   location information about each point in the periphery of the roi.
   The general stats about a roi are:
   	(centroid_x,centroid_y) (num_points_inside) bounding_box(x,y,w,h)
   if a unc file has been given, these fields will also be written:
   	(min,max,mean,stdev of pixels inside)
   The location info about a point of a roi in the periphery are:
   	(x, y) (r, theta) (dx, dy, dy/dx), (dr, dtheta)
*/
int	write_rois_stats_to_file(char *filename, roi **myRois, int numRois, char writeInfoForEachROIPeripheralPoint){
	FILE			*file;
	int			i, j;
	roiRegionIrregular	*irreReg;
//	roiRegionRectangular	*rectReg;
//	roiRegionElliptical	*elliReg;
	float			oldr, oldtheta;
	roi			*a_roi;

	if( (file=fopen(filename, "w")) == NULL ){
		fprintf(stderr, "write_rois_stats_to_file : could not open file '%s' for writing.\n", filename);
		return FALSE;
	}

	fprintf(file, "# name, image (from roi file)\n");
	fprintf(file, "# field descriptions for irregular rois:\n");
	if( writeInfoForEachROIPeripheralPoint == TRUE )
		fprintf(file, "# 1,2 : (x, y)\n# 3,4 : (r, theta)\n# 5,6,7 : (dx, dy, dy/dx)\n# 8,9 : (dr, dtheta)\n");
	else
		fprintf(file, "# 1,2 : (centroid_x,centroid_y)\n# 3 : (num_points_inside)\n# 4,5,6,7 : bounding_box(x,y,w,h)\n# 8,9,10,11 : (min,max,mean,stdev of pixels inside) ** in case a unc file is associated with this roi\n");

/* elliptical and rectangular are not going to be displayed because the different number of columns will confuse gnuplot */
/*	fprintf(file, "# field descriptions for elliptical rois:\n");
	fprintf(file, "# center(x,y) radii(horizontal,vertical) (rotation-in radians)\n");
	fprintf(file, "#    for long lines:\n");
	fprintf(file, "#    (num_points_inside,area) (min,max,mean,stdev)
	fprintf(file, "# field descriptions for rectangular rois:\n");
	fprintf(file, "# center(x,y) (width,height)\n");
	fprintf(file, "#    for long lines:\n");
	fprintf(file, "#    (num_points_inside,area) (min,max,mean,stdev)
*/

	for(i=0;i<numRois;i++){
		a_roi = myRois[i];
		fprintf(file, "# '%s' '%s %d'\n", a_roi->name, a_roi->image, a_roi->slice+1);

		switch( a_roi->type ){
			case RECTANGULAR_ROI_REGION:
			case ELLIPTICAL_ROI_REGION:
				break;
			case IRREGULAR_ROI_REGION:
				irreReg = (roiRegionIrregular *)(a_roi->roi_region);
				oldr = irreReg->points[0]->dr;
				oldtheta = irreReg->points[0]->dtheta;
				if( writeInfoForEachROIPeripheralPoint == TRUE ){
					for(j=0;j<irreReg->num_points;j++){
						/* because of pixel_sizes we have to recalculate r and theta - perhaps we should find a better way for this */
						fprintf(file, "%.2f %.2f %.2f %.2f", irreReg->points[j]->x, irreReg->points[j]->y, irreReg->points[j]->r, irreReg->points[j]->theta);
						fprintf(file," %.2f %.2f %.2f %.2f %.2f\n", irreReg->points[j]->dx, irreReg->points[j]->dy, irreReg->points[j]->slope, oldr-irreReg->points[j]->r, oldtheta-irreReg->points[j]->theta);
						oldr = irreReg->points[j]->dr;
						oldr = irreReg->points[j]->dtheta;
					}
				} else {
					fprintf(file, " %.2f %.2f %d %.2f %.2f %.2f %.2f", a_roi->centroid_x, a_roi->centroid_y, a_roi->num_points_inside, a_roi->x0, a_roi->y0, a_roi->width, a_roi->height);
					/* a_roi->maxPixel > 0.0 means an image is associated with this roi */
					if( a_roi->maxPixel > 0.0 ) fprintf(file, " %.2f %.2f %.2f %.2f", a_roi->minPixel, a_roi->maxPixel, a_roi->meanPixel, a_roi->stdevPixel);
					fprintf(file, "\n");						
				}
				break;
			default:
				fprintf(stderr, "write_rois_stats_to_file : warning, roi %d has type %d which is not yet implemented.\n", i, a_roi->type);
				break;
		}
	}
	fclose(file);
	return TRUE;
}

/* dispunc writes out rois in millimetres (what a bright idea...), in order to use
   coordinates to access an image's pixel values stored in memory (an array) we will have
   to convert those millimetres to integral pixel numbers by dividing by the pixel_size and
   then rounding up/down. */
char	rois_convert_millimetres_to_pixels(roi **a_rois, int numRois, float pixel_size_x, float pixel_size_y, float pixel_size_z){
	int	i;
	for(i=0;i<numRois;i++)
		if( roi_convert_millimetres_to_pixels(a_rois[i], pixel_size_x, pixel_size_y, pixel_size_z) == FALSE ){
			fprintf(stderr, "rois_convert_millimetres_to_pixels : call to roi_convert_millimetres_to_pixels has failed.\n");
			return FALSE;
		}
	return TRUE;
}
char	roi_convert_millimetres_to_pixels(roi *a_roi, float pixel_size_x, float pixel_size_y, float pixel_size_z){
	roiRegionRectangular	*a_rect_region;
	roiRegionIrregular	*a_irre_region;
	roiRegionElliptical	*a_elli_region;
	int			i;

	/* centroid */
	a_roi->centroid_x /= pixel_size_x;
	a_roi->centroid_y /= pixel_size_y;

	/* bounding box */
	a_roi->x0 /= pixel_size_x; a_roi->y0 /= pixel_size_y;
	a_roi->width /= pixel_size_x; a_roi->height /= pixel_size_y;

	/* points on the periphery */
	switch( a_roi->type ){
		case RECTANGULAR_ROI_REGION:
			a_rect_region = (roiRegionRectangular *)(a_roi->roi_region);
			a_rect_region->x0 /= pixel_size_x; a_rect_region->y0 /= pixel_size_y;
			a_rect_region->width /= pixel_size_x; a_rect_region->height /= pixel_size_y;
			break;
		case ELLIPTICAL_ROI_REGION:
			a_elli_region = (roiRegionElliptical *)(a_roi->roi_region);
			a_elli_region->ex0 /= pixel_size_x; a_elli_region->ey0 /= pixel_size_y;
			a_elli_region->ea /= pixel_size_x; a_elli_region->eb /= pixel_size_y;
			break;
		case IRREGULAR_ROI_REGION:
			a_irre_region = (roiRegionIrregular *)(a_roi->roi_region);
			for(i=0;i<a_irre_region->num_points;i++){
				a_irre_region->points[i]->x = a_irre_region->points[i]->x / pixel_size_x;
				a_irre_region->points[i]->y = a_irre_region->points[i]->y / pixel_size_y;
				a_irre_region->points[i]->z = a_irre_region->points[i]->z / pixel_size_z;
			}
			break;
		default:
			fprintf(stderr, "roi_convert_millimetres_to_pixels : unknown region type %d.\n", a_roi->type);
			return FALSE;
	}

	/* now recalculate the roi and that's it */
	if( roi_calculate(a_roi) == FALSE ){
		fprintf(stderr, "roi_convert_millimetres_to_pixels : call to roi_calculate has failed.\n");
		return FALSE;
	}

	/* correction, because z is read as the slice number - can you imagine? a roi which x and y are
	   in millimetres and z in pixels ? only cretins could have written that code ... fuck them */
	for(i=0;i<a_roi->num_points_inside;i++){
		a_roi->points_inside[i]->z = a_roi->points_inside[i]->z / pixel_size_z;
		a_roi->points_inside[i]->Z = (int )ROUND(a_roi->points_inside[i]->z);
	}

	return TRUE;
}
/* this does the opposite than above. */
char	rois_convert_pixels_to_millimetres(roi **a_rois, int numRois, float pixel_size_x, float pixel_size_y, float pixel_size_z){
	int	i;
	for(i=0;i<numRois;i++)
		if( roi_convert_pixels_to_millimetres(a_rois[i], pixel_size_x, pixel_size_y, pixel_size_z) == FALSE ){
			fprintf(stderr, "rois_convert_pixels_to_millimetres : call to roi_convert_pixels_to_millimetres has failed.\n");
			return FALSE;
		}
	return TRUE;
}
char	roi_convert_pixels_to_millimetres(roi *a_roi, float pixel_size_x, float pixel_size_y, float pixel_size_z){
	roiRegionRectangular	*a_rect_region;
	roiRegionIrregular	*a_irre_region;
	roiRegionElliptical	*a_elli_region;
	int			i;

	/* centroid */
	a_roi->centroid_x *= pixel_size_x;
	a_roi->centroid_y *= pixel_size_y;

	/* bounding box */
	a_roi->x0 *= pixel_size_x; a_roi->y0 *= pixel_size_y;
	a_roi->width *= pixel_size_x; a_roi->height *= pixel_size_y;

	/* points on the periphery */
	switch( a_roi->type ){
		case RECTANGULAR_ROI_REGION:
			a_rect_region = (roiRegionRectangular *)(a_roi->roi_region);
			a_rect_region->x0 *= pixel_size_x; a_rect_region->y0 *= pixel_size_y;
			a_rect_region->width *= pixel_size_x; a_rect_region->height *= pixel_size_y;
			break;
		case ELLIPTICAL_ROI_REGION:
			a_elli_region = (roiRegionElliptical *)(a_roi->roi_region);
			a_elli_region->ex0 *= pixel_size_x; a_elli_region->ey0 *= pixel_size_y;
			a_elli_region->ea *= pixel_size_x; a_elli_region->eb *= pixel_size_y;
			break;
		case IRREGULAR_ROI_REGION:
			a_irre_region = (roiRegionIrregular *)(a_roi->roi_region);
			for(i=0;i<a_irre_region->num_points;i++){
				a_irre_region->points[i]->x = a_irre_region->points[i]->x * pixel_size_x;
				a_irre_region->points[i]->y = a_irre_region->points[i]->y * pixel_size_y;
				a_irre_region->points[i]->z = a_irre_region->points[i]->z * pixel_size_z;
			}
			break;
		default:
			fprintf(stderr, "roi_convert_pixels_to_millimetres : unknown region type %d.\n", a_roi->type);
			return FALSE;
	}

	/* now recalculate the roi and that's it */
	if( roi_calculate(a_roi) == FALSE ){
		fprintf(stderr, "roi_convert_pixels_to_millimetres : call to roi_calculate has failed.\n");
		return FALSE;
	}

	/* correction, because z is read as the slice number - can you imagine? a roi which x and y are
	   in millimetres and z in pixels ? only cretins could have written that code ... fuck them */
	for(i=0;i<a_roi->num_points_inside;i++){
		a_roi->points_inside[i]->z = a_roi->points_inside[i]->z * pixel_size_z;
		a_roi->points_inside[i]->Z = (int )ROUND(a_roi->points_inside[i]->z);
	}

	return TRUE;
}

char	rois_convert_pixel_size(roi **a_rois, int numRois, float old_pixel_size_x, float old_pixel_size_y, float old_pixel_size_z, float new_pixel_size_x, float new_pixel_size_y, float new_pixel_size_z){
	int	i;

	for(i=0;i<numRois;i++)
		if( roi_convert_pixel_size(a_rois[i], old_pixel_size_x, old_pixel_size_y, old_pixel_size_z, new_pixel_size_x, new_pixel_size_y, new_pixel_size_z) == FALSE ){
			fprintf(stderr, "rois_convert_pixel_size : call to roi_convert_pixel_size has failed.\n");
			return FALSE;
		}
	return TRUE;
}
char	roi_convert_pixel_size(roi *a_roi, float old_pixel_size_x, float old_pixel_size_y, float old_pixel_size_z, float new_pixel_size_x, float new_pixel_size_y, float new_pixel_size_z){
	roiRegionRectangular	*a_rect_region;
	roiRegionIrregular	*a_irre_region;
	roiRegionElliptical	*a_elli_region;
	int			i;
	float			fx = old_pixel_size_x / new_pixel_size_x,
				fy = old_pixel_size_y / new_pixel_size_y,
				fz = old_pixel_size_z / new_pixel_size_z;
	/* centroid */
	a_roi->centroid_x *= fx;
	a_roi->centroid_y *= fy;

	/* bounding box */
	a_roi->x0 *= fx; a_roi->y0 *= fy;
	a_roi->width *= fx; a_roi->height *= fy;

	/* points on the periphery */
	switch( a_roi->type ){
		case RECTANGULAR_ROI_REGION:
			a_rect_region = (roiRegionRectangular *)(a_roi->roi_region);
			a_rect_region->x0 *= fx; a_rect_region->y0 *= fy;
			a_rect_region->width *= fx; a_rect_region->height *= fy;
			break;
		case ELLIPTICAL_ROI_REGION:
			a_elli_region = (roiRegionElliptical *)(a_roi->roi_region);
			a_elli_region->ex0 *= fx; a_elli_region->ey0 *= fy;
			a_elli_region->ea *= fx; a_elli_region->eb *= fy;
			break;
		case IRREGULAR_ROI_REGION:
			a_irre_region = (roiRegionIrregular *)(a_roi->roi_region);
			for(i=0;i<a_irre_region->num_points;i++){
				a_irre_region->points[i]->x = a_irre_region->points[i]->x * fx;
				a_irre_region->points[i]->y = a_irre_region->points[i]->y * fy;
				a_irre_region->points[i]->z = a_irre_region->points[i]->z * fz;
			}
			break;
		default:
			fprintf(stderr, "roi_convert_pixel_size : unknown region type %d.\n", a_roi->type);
			return FALSE;
	}

	/* now recalculate the roi and that's it */
	if( roi_calculate(a_roi) == FALSE ){
		fprintf(stderr, "roi_convert_pixel_size : call to roi_calculate has failed.\n");
		return FALSE;
	}

	/* correction, because z is read as the slice number - can you imagine? a roi which x and y are
	   in millimetres and z in pixels ? only cretins could have written that code ... fuck them */
	for(i=0;i<a_roi->num_points_inside;i++){
		a_roi->points_inside[i]->z = a_roi->points_inside[i]->z * fz;
		a_roi->points_inside[i]->Z = (int )ROUND(a_roi->points_inside[i]->z);
	}

	return TRUE;
}

/* it will read the pixel value onto each point inside each roi, given the UNC volume */	
/* scratch pad is a float array which we will need to do scratch ops. just make sure can
   take as many points as the number of points in each roi - for many rois use the max number
   of points. alternatively just allocate a UNCwidth x UNCheight float array and forget about it.
   Oh BTW: do not forget to free it when finished. */
void	rois_associate_with_unc_volume(roi **a_r, int numRois, DATATYPE ***volume_data, float *scratch_pad){
	int	i;
	for(i=0;i<numRois;i++)
		roi_associate_with_unc_volume(a_r[i], volume_data, scratch_pad);
}
void	roi_associate_with_unc_volume(roi *a_r, DATATYPE ***volume_data, float *scratch_pad){
	int	i, j;
	for(j=0,i=0;j<a_r->num_points_inside;j++){
		if( (a_r->points_inside[j]->v=volume_data[a_r->points_inside[j]->Z][a_r->points_inside[j]->X][a_r->points_inside[j]->Y]) > 0 ){
			/* only non-zero pixels participate in the stats */
			scratch_pad[i++] = a_r->points_inside[j]->v;
		}
	}
	statistics1D_float(scratch_pad, 0, i, &(a_r->minPixel), &(a_r->maxPixel), &(a_r->meanPixel), &(a_r->stdevPixel));
}


/* private : 2 routines for qsort to sort an array of integers in ascending and descending order */
/*static  int     _roi_irregular_region_comparator_descending(const void *a, const void *b){
	float   _a = (*((_roi_irregular_region_sort_data *)a)).value,
		_b = (*((_roi_irregular_region_sort_data *)b)).value;
	if( _a > _b ) return -1;
	if( _a < _b ) return 1;
	return 0;
}*/
static  int     _roi_irregular_region_comparator_ascending(const void *a, const void *b){
	float   _a = (*((_roi_irregular_region_sort_data *)a)).value,
		_b = (*((_roi_irregular_region_sort_data *)b)).value;
	if( _a > _b ) return 1;
	if( _a < _b ) return -1;
	return 0;
}

