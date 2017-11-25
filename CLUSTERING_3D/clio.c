#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>
#include <LinkedList.h>
#include <LinkedListIterator.h>
#include <matrix.h>

#include "clustering.h"

int	read_data_from_ASCII_file(
	char		*filename,
	int		numClusters,
	int		*numFeatures,
	int		*numPoints,
	clengine	**cl)
{		
	FILE	*a_file;
	char	a_line[10000];
	float	dummy;
	int	i, l, f;

	if( (a_file=fopen(filename, "r")) == NULL ){
		fprintf(stderr, "read_data_from_ASCII_file : could not open file '%s' for reading.\n", filename);
		return FALSE;
	}

	if( *numPoints <= 0 ){ /* we do not know how many points (assume 1 line per point) */
		*numPoints = 0;
		while( fgets(a_line, 10000, a_file) && !feof(a_file) )
			for(i=0;i<strlen(a_line);i++) if( !((a_line[i]==' ')||(a_line[i]=='\t')||(a_line[i]=='\n')) ){ (*numPoints)++; break; }
		rewind(a_file);
	}

	if( *numFeatures <= 0 ){ /* we do not know how many features per point */
/*states:
   0: begin
   1: found number
   2: found space
   3: found newline
   */
		char	state = 0, prev_state = 0;

		*numFeatures = 0;
		while( !fgets(a_line, 10000, a_file) && !feof(a_file) );

		for(i=0;i<strlen(a_line);i++){
			switch( a_line[i] ){
				case '\n': prev_state = state; state = 3; break;
				case ' ' :
				case '\t': if( state != 2 ) prev_state = state;
					   state = 2; break;
				default:   prev_state = state; state = 1; break;
			}
			if( (prev_state == 0) && (state == 1) ) (*numFeatures)++;
			if( (prev_state == 2) && (state == 1) ) (*numFeatures)++;
		}
		rewind(a_file);
	}	

	/* create the clengine if not already created -- check alloc space if it is already created */
	if( *cl == NULL ){
		if( (*cl=new_clengine(0, *numFeatures, *numPoints, numClusters)) == NULL ){
			fprintf(stderr, "read_data_from_ASCII_file : call to new_clengine has failed while reading data from file '%s'.\n", filename);
			fclose(a_file);
			return FALSE;
		}
	} else {
		if( (*cl)->nF != *numFeatures ){
			fprintf(stderr, "read_data_from_ASCII_file : previously allocated clengine has capacity for %d features (data from file '%s' needs %d).\n", (*cl)->nF, filename, *numFeatures);
			if( (*cl)->nF < *numFeatures ){ /* can't continue -- not enough space allocated */
				fclose(a_file);
				return FALSE;
			}
		}			
		if( (*cl)->nP != *numPoints ){
			fprintf(stderr, "read_data_from_ASCII_file : previously allocated clengine has capacity for %d points (data from file '%s' needs %d).\n", (*cl)->nP, filename, *numPoints);
			if( (*cl)->nP < *numPoints ){ /* can't continue -- not enough space allocated */
				fclose(a_file);
				return FALSE;
			}
		}			
		if( (*cl)->nC != numClusters ){
			fprintf(stderr, "read_data_from_ASCII_file : previously allocated clengine has capacity for %d features (data from file '%s' needs %d).\n", (*cl)->nC, filename, numClusters);
			if( (*cl)->nC < numClusters ){ /* can't continue -- not enough space allocated */
				fclose(a_file);
				return FALSE;
			}
		}			
	}

	/* start filling the points */
	for(l=0;l<*numPoints;l++){
		for(f=0;f<*numFeatures;f++){
			if( feof(a_file) ){
				fprintf(stderr, "read_data_from_ASCII_file : EOF encountered while reading column %d, line %d of file '%s' - was expecting %d columns (features) and %d lines (points).\n", f, l, filename, *numFeatures, *numPoints);
				fclose(a_file);
				return FALSE;
			}
			fscanf(a_file, "%f", &dummy);
			if( l == 0 ){
				(*cl)->stats->min_point_values->c[f] =
				(*cl)->stats->max_point_values->c[f] = dummy;
			}
			(*cl)->p[l]->v->c[f] = dummy;
			(*cl)->p[l]->f->c[f] = dummy;

			/* find min and max pixel values */
			if( (*cl)->p[l]->v->c[f] < (*cl)->stats->min_point_values->c[f] ) (*cl)->stats->min_point_values->c[f] = (*cl)->p[l]->v->c[f];
			if( (*cl)->p[l]->v->c[f] > (*cl)->stats->max_point_values->c[f] ) (*cl)->stats->min_point_values->c[f] = (*cl)->p[l]->v->c[f];
		}
		(*cl)->p[l]->x = (*cl)->p[l]->v->c[0]; /* first feature is x */
		(*cl)->p[l]->y = (*cl)->p[l]->v->c[1]; /* second feature is y */
		(*cl)->p[l]->z = 0;
		(*cl)->p[l]->s = 0;
	}

	fclose(a_file);

	/* save some image data specs into the cl->im structure */
	/* roi */
	(*cl)->im->w = (*cl)->im->h =
	(*cl)->im->x = (*cl)->im->y = -1;
	/* actual image size */
	(*cl)->im->W = (*cl)->im->H = -1;
	/* actual number of slices */
	(*cl)->im->actualNumSlices = -1;
	/* number of slices considered */
	(*cl)->im->numSlices = -1;
	/* slice separation */
	(*cl)->im->sliceSeparation = -1;

	printf("read_data_from_ASCII_file : read %d points of %d features from file '%s'\n", *numPoints, *numFeatures, filename);

	return TRUE;

}	
/* read data from a unc file and produce an array of points. Each slice in the unc file represents
   a feature while the first slice also represents the pixel value entry in the point struct
   (so the first slice should be the actual image unmodified).
*/
int	read_data_from_UNC_file(
	char		**filenames,
	int		numFeatures,	/* e.g. number of filenames */
	int		x,		/* ROI */
	int		y,
	int		*w,
 	int		*h,
	int		numClusters,
	int		*slices,	/* which slices should participate in the clustering ? */
	int		*numSlices,	/* if zero or negative, we will read all slices and set it to the number of slices read, otherwise says how many elements in *slices */
	float		sliceSeparation,/* mm between 2 adjacent slices */
	DATATYPE	minThreshold,	/* ignore pixels falling outside this range */
	DATATYPE	maxThreshold,

			/* if the files have been segmented with spm, then this is the probability maps
			   for each of the WM,GM and CSF. If there are no SPM prob maps, set these to NULL */
	char		*spm_probability_mapsWM,
	char		*spm_probability_mapsGM,
	char		*spm_probability_mapsCSF,

	clengine	**cl ){

	register int	i, j, s, slice, id, allSlices = FALSE, ii, jj, f;
	register double	sum;
	DATATYPE	***data, ***firstFeatureData;
	point		***neighbours, *a_point;
	int		depth, format, actualNumSlices = 0, W = -1, H = -1,
			numPoints;
	char		*filename;

	if( cl == NULL ){
		fprintf(stderr, "read_data_from_UNC_file : cl parameter should point to a clengine, initialised or not.\n");
		return FALSE;
	}
	/* read the first feature and do size and slice checks */
	filename = filenames[0];
	if( (firstFeatureData=getUNCSlices3D(filename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
		return FALSE;
	}
	if( *w <= 0 ) *w = W;
	if( *h <= 0 ) *h = H;
	if( (x+*w) > W ){
		fprintf(stderr, "read_data_from_UNC_file : the actual image width (%d) of file '%s' is less than offset (%d) and width (%d) requested.\n", W, filename, x, *w);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	if( (y+*h) > H ){
		fprintf(stderr, "read_data_from_UNC_file : the actual image height (%d) of file '%s' is less than offset (%d) and height (%d) requested.\n", H, filename, y, *h);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	if( *numSlices <= 0 ){ *numSlices = actualNumSlices; allSlices = TRUE; }
	else {
		for(i=0;i<*numSlices;i++){
			if( slices[i] >= actualNumSlices ){
				fprintf(stderr, "read_data_from_UNC_file : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", actualNumSlices, filename);
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			} else if( slices[i] < 0 ){
				fprintf(stderr, "read_data_from_UNC_file : slice numbers must start from 1.\n");
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		}
	}
	/* count points */
	/* points will be included if the 1st feature falls within specified range - we do not care about the other features */
	numPoints = 0;
	for(s=0;s<*numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		for(i=x;i<x+*w;i++) for(j=y;j<y+*h;j++)
			if( IS_WITHIN(firstFeatureData[slice][i][j], minThreshold, maxThreshold) ) numPoints++;
	}

	/* now do the same checks for each feature */
	for(f=1;f<numFeatures;f++){
		filename = filenames[f];
		if( (data=getUNCSlices3D(filename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
			fprintf(stderr, "read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
			freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
			return FALSE;
		}
		/* check size */
		if( (*w != W) || (*h != H) ){
			fprintf(stderr, "read_data_from_UNC_file : size of images in file '%s' (%dx%d) differ from those of the first in collection (%dx%d).\n", filename, W, H, *w, *h);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
			return FALSE;
		}
		/* check slices */
		if( allSlices == TRUE ){
			if( *numSlices != actualNumSlices ){
				fprintf(stderr, "read_data_from_UNC_file : input volumes must have the same number of slices (first image has %d slices, image '%s' has %d slices).\n", actualNumSlices, filename, *numSlices);
				freeDATATYPE3D(data, actualNumSlices, W);
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		} else {
			for(i=0;i<*numSlices;i++){
				if( slices[i] >= actualNumSlices ){
					fprintf(stderr, "read_data_from_UNC_file : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", actualNumSlices, filename);
					freeDATATYPE3D(data, actualNumSlices, W);
					freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
					return FALSE;
				} else if( slices[i] < 0 ){
					fprintf(stderr, "read_data_from_UNC_file : slice numbers must start from 1.\n");
					freeDATATYPE3D(data, actualNumSlices, W);
					freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
					return FALSE;
				}
			}
		}
		freeDATATYPE3D(data, actualNumSlices, W);
	}
	if( numPoints == 0 ){
		fprintf(stderr, "read_data_from_UNC_file : could not find any points in the pixel interval %d to %d within the ROI starting at (%d,%d) with size (%d,%d).\n", minThreshold, maxThreshold, x, y, *w, *h);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}

	/* create the clengine if not already created -- check alloc space if it is already created */
	if( *cl == NULL ){
		if( (*cl=new_clengine(0, numFeatures, numPoints, numClusters)) == NULL ){
			fprintf(stderr, "read_data_from_UNC_file : call to new_clengine has failed while reading data from file '%s'.\n", filename);
			freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
			return FALSE;
		}
	} else {
		if( (*cl)->nF != numFeatures ){
			fprintf(stderr, "read_data_from_UNC_file : previously allocated clengine has capacity for %d features (data from file '%s' needs %d).\n", (*cl)->nF, filename, numFeatures);
			if( (*cl)->nF < numFeatures ){ /* can't continue -- not enough space allocated */
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		}			
		if( (*cl)->nP != numPoints ){
			fprintf(stderr, "read_data_from_UNC_file : previously allocated clengine has capacity for %d points (data from file '%s' needs %d).\n", (*cl)->nP, filename, numPoints);
			if( (*cl)->nP < numPoints ){ /* can't continue -- not enough space allocated */
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		}			
		if( (*cl)->nC != numClusters ){
			fprintf(stderr, "read_data_from_UNC_file : previously allocated clengine has capacity for %d features (data from file '%s' needs %d).\n", (*cl)->nC, filename, numClusters);
			if( (*cl)->nC < numClusters ){ /* can't continue -- not enough space allocated */
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		}			
	}

	/* allocate memory for neighbours array */
	if( (neighbours=(point ***)malloc(*w * sizeof(point **))) == NULL ){
		fprintf(stderr, "read_data_from_UNC_file : could not allocate %zd bytes for 'neighbours'.\n", *w * sizeof(point **));
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	for(i=0;i<*w;i++)
		if( (neighbours[i]=(point **)malloc(*h * sizeof(point *))) == NULL ){
			fprintf(stderr, "read_data_from_UNC_file : could not allocate %zd bytes for 'neighbours[%d]'.\n", *h * sizeof(point *), i);
			freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
			return FALSE;
		}
	for(i=0;i<*w;i++) for(j=0;j<*h;j++) neighbours[i][j] = (point *)NULL;

	/* start filling the points */
	for(f=0;f<numFeatures;f++){
		id = 0;
		filename = filenames[f];
		printf("doing file '%s'\n", filename);
		if( (data=getUNCSlices3D(filename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
			fprintf(stderr, "read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
			freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
			return FALSE;
		}
		(*cl)->stats->min_point_values->c[f] =
		(*cl)->stats->max_point_values->c[f] = (float )(data[(allSlices==FALSE) ? slices[0] : 0][x][y]);

		for(i=x,ii=0;i<x+*w;i++,ii++) for(j=y,jj=0;j<y+*h;j++,jj++){
			for(s=0;s<*numSlices;s++){
				slice = (allSlices==FALSE) ? slices[s] : s;

				/* the criterion for picking that point is that the 1st feature is within the specified range */
				if( !IS_WITHIN(firstFeatureData[slice][i][j], minThreshold, maxThreshold) ) continue;
				if( f == 0 ) neighbours[ii][jj] = (*cl)->p[id];

				(*cl)->p[id]->x = i; (*cl)->p[id]->y = j;
				(*cl)->p[id]->z = slice * sliceSeparation;
				(*cl)->p[id]->s = s;
				(*cl)->p[id]->v->c[f] = (float )(data[slice][i][j]);
				(*cl)->p[id]->f->c[f] = (float )(data[slice][i][j]);

				/* find min and max pixel values */
				if( (*cl)->p[id]->v->c[f] < (*cl)->stats->min_point_values->c[f] ) (*cl)->stats->min_point_values->c[f] = (*cl)->p[id]->v->c[f];
				if( (*cl)->p[id]->v->c[f] > (*cl)->stats->max_point_values->c[f] ) (*cl)->stats->max_point_values->c[f] = (*cl)->p[id]->v->c[f];

				id++;
			}
		}
		printf("id was %d num points %d\n", id, numPoints);
	}

	/* tell each point which is their neighbour (from a neighbourhood of 8) */
	for(id=0;id<numPoints;id++){
		i = (int )((*cl)->p[id]->x); j = (int )((*cl)->p[id]->y);
		if( (i>x)    && (j>y)    ) (*cl)->p[id]->n[0] = neighbours[i-1-x][j-1-y];
		if(             (j>y)    ) (*cl)->p[id]->n[1] = neighbours[i  -x][j-1-y];
		if( (i<*w-1) && (j>y)    ) (*cl)->p[id]->n[2] = neighbours[i+1-x][j-1-y];
		if( (i<*w-1)             ) (*cl)->p[id]->n[3] = neighbours[i+1-x][j  -y];
		if( (i<*w-1) && (j<*h-1) ) (*cl)->p[id]->n[4] = neighbours[i+1-x][j+1-y];
		if(             (j<*h-1) ) (*cl)->p[id]->n[5] = neighbours[i  -x][j+1-y];
		if( (i>x)    && (j<*h-1) ) (*cl)->p[id]->n[6] = neighbours[i-1-x][j+1-y];
		if( (i>x)                ) (*cl)->p[id]->n[7] = neighbours[i-1-x][j  -y];
	}
	for(i=0;i<*w;i++) free(neighbours[i]); free(neighbours);
	freeDATATYPE3D(firstFeatureData, actualNumSlices, W);

	/* save some image data specs into the cl->im structure */
	/* roi */
	(*cl)->im->w = *w; (*cl)->im->h = *h;
	(*cl)->im->x = x; (*cl)->im->y = y;
	/* actual image size */
	(*cl)->im->W = W; (*cl)->im->H = H;
	/* actual number of slices */
	(*cl)->im->actualNumSlices = actualNumSlices;
	/* number of slices considered */
	(*cl)->im->numSlices = *numSlices;
	/* slice separation */
	(*cl)->im->sliceSeparation = sliceSeparation;

	printf("read_data_from_UNC_file : read %d points of %d features from %d slices\n", numPoints, numFeatures, *numSlices);

	/* are there any SPM probability maps available? */
	if( (spm_probability_mapsWM==NULL) || (spm_probability_mapsGM==NULL) || (spm_probability_mapsCSF==NULL) ) return TRUE;
	if( (spm_probability_mapsWM[0]=='\0') || (spm_probability_mapsGM[0]=='\0') || (spm_probability_mapsCSF[0]=='\0') ) return TRUE;
	/* yes there are */
	/* do width/height/num slices checks for each probability map (must be the same as features) */
	filename = spm_probability_mapsWM;
	if( (data=getUNCSlices3D(filename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	/* check size */
	if( (*w != W) || (*h != H) ){
		fprintf(stderr, "read_data_from_UNC_file : size of images in file '%s' (%dx%d) differ from those of the first in collection (%dx%d).\n", filename, W, H, *w, *h);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	/* check slices */
	if( allSlices == TRUE ){
		if( *numSlices != actualNumSlices ){
			fprintf(stderr, "read_data_from_UNC_file : input volumes must have the same number of slices (first image has %d slices, image '%s' has %d slices).\n", actualNumSlices, filename, *numSlices);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
			return FALSE;
		}
	} else {
		for(i=0;i<*numSlices;i++){
			if( slices[i] >= actualNumSlices ){
				fprintf(stderr, "read_data_from_UNC_file : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", actualNumSlices, filename);
				freeDATATYPE3D(data, actualNumSlices, W);
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			} else if( slices[i] < 0 ){
				fprintf(stderr, "read_data_from_UNC_file : slice numbers must start from 1.\n");
				freeDATATYPE3D(data, actualNumSlices, W);
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		}
	}
	freeDATATYPE3D(data, actualNumSlices, W);

	filename = spm_probability_mapsGM;
	if( (data=getUNCSlices3D(filename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	/* check size */
	if( (*w != W) || (*h != H) ){
		fprintf(stderr, "read_data_from_UNC_file : size of images in file '%s' (%dx%d) differ from those of the first in collection (%dx%d).\n", filename, W, H, *w, *h);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	/* check slices */
	if( allSlices == TRUE ){
		if( *numSlices != actualNumSlices ){
			fprintf(stderr, "read_data_from_UNC_file : input volumes must have the same number of slices (first image has %d slices, image '%s' has %d slices).\n", actualNumSlices, filename, *numSlices);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
			return FALSE;
		}
	} else {
		for(i=0;i<*numSlices;i++){
			if( slices[i] >= actualNumSlices ){
				fprintf(stderr, "read_data_from_UNC_file : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", actualNumSlices, filename);
				freeDATATYPE3D(data, actualNumSlices, W);
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			} else if( slices[i] < 0 ){
				fprintf(stderr, "read_data_from_UNC_file : slice numbers must start from 1.\n");
				freeDATATYPE3D(data, actualNumSlices, W);
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		}
	}
	freeDATATYPE3D(data, actualNumSlices, W);

	filename = spm_probability_mapsCSF;
	if( (data=getUNCSlices3D(filename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	/* check size */
	if( (*w != W) || (*h != H) ){
		fprintf(stderr, "read_data_from_UNC_file : size of images in file '%s' (%dx%d) differ from those of the first in collection (%dx%d).\n", filename, W, H, *w, *h);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	/* check slices */
	if( allSlices == TRUE ){
		if( *numSlices != actualNumSlices ){
			fprintf(stderr, "read_data_from_UNC_file : input volumes must have the same number of slices (first image has %d slices, image '%s' has %d slices).\n", actualNumSlices, filename, *numSlices);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
			return FALSE;
		}
	} else {
		for(i=0;i<*numSlices;i++){
			if( slices[i] >= actualNumSlices ){
				fprintf(stderr, "read_data_from_UNC_file : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", actualNumSlices, filename);
				freeDATATYPE3D(data, actualNumSlices, W);
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			} else if( slices[i] < 0 ){
				fprintf(stderr, "read_data_from_UNC_file : slice numbers must start from 1.\n");
				freeDATATYPE3D(data, actualNumSlices, W);
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		}
	}
	freeDATATYPE3D(data, actualNumSlices, W);

	/* start filling the points with the SPM probability maps */

	printf("reading spm probability maps from files WM='%s', GM='%s', CSF='%s'\n", spm_probability_mapsWM, spm_probability_mapsGM, spm_probability_mapsCSF);

/* WM */
	filename = spm_probability_mapsWM;
	if( (data=getUNCSlices3D(filename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	for(id=0;id<numPoints;id++){
		a_point = (*cl)->p[id];
		s = a_point->s; slice = (allSlices==FALSE) ? slices[s] : s;
		i = a_point->x;
		j = a_point->y;
		a_point->spm->c[0] = data[slice][i][j];
	}
	freeDATATYPE3D(data, actualNumSlices, W);
/* GM */
	filename = spm_probability_mapsGM;
	if( (data=getUNCSlices3D(filename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	for(id=0;id<numPoints;id++){
		a_point = (*cl)->p[id];
		s = a_point->s; slice = (allSlices==FALSE) ? slices[s] : s;
		i = a_point->x;
		j = a_point->y;
		a_point->spm->c[1] = data[slice][i][j];
	}
	freeDATATYPE3D(data, actualNumSlices, W);
/* CSF */
	filename = spm_probability_mapsCSF;
	if( (data=getUNCSlices3D(filename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	for(id=0;id<numPoints;id++){
		a_point = (*cl)->p[id];
		s = a_point->s; slice = (allSlices==FALSE) ? slices[s] : s;
		i = a_point->x;
		j = a_point->y;
		a_point->spm->c[2] = data[slice][i][j];
	}
	freeDATATYPE3D(data, actualNumSlices, W);

	printf("read_data_from_UNC_file : read SPM probability maps OK\n");

	/* normalise probabilities please */
	for(id=0;id<numPoints;id++){
		a_point = (*cl)->p[id];
		for(i=0,sum=0;i<3;i++) sum += a_point->spm->c[i];
		for(i=0;i<3;i++) a_point->spm->c[i] /= sum;
	}

	return TRUE;
}
/* will write the probability of point-to-cluster assignment to numCluster
   slices by scaling them between min and max output pixel */
/* These transformed probabilities are defined as the transformed distance / sum of transformed distances
   for each point. */
int	write_transformed_probability_maps_to_UNC_file(
	char		*filename,
	clengine	*cl,
	DATATYPE	minOutputPixel,
	DATATYPE	maxOutputPixel ){

	DATATYPE	***data;
	register int	i, j, k;

	/* we will write as many slices as there are clusters */
	if( (data=callocDATATYPE3D(cl->nC * cl->im->numSlices, cl->im->W, cl->im->H)) == NULL ){
		fprintf(stderr, "write_transformed_probability_maps_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", cl->nC * cl->im->numSlices, cl->im->W, cl->im->H);
		return FALSE;
	}

	/* set all voxels to zero */
	for(k=0;k<cl->nC*cl->im->numSlices;k++) for(i=0;i<cl->im->W;i++) for(j=0;j<cl->im->H;j++) data[k][i][j] = 0;

	/* and now write the probabilities */		
	for(j=0;j<cl->nC;j++) for(i=0;i<cl->nP;i++){
		if( cl->p[i]->c == NULL ) continue;
		data[cl->p[i]->s*cl->nC+cl->c[j]->centroid->s][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] =
			SCALE_OUTPUT(cl->p[i]->p->c[cl->c[j]->id], (float )minOutputPixel, (float )maxOutputPixel, 0.0, 1.0);
	}
	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, cl->im->W, cl->im->H, 0, 0, cl->im->W, cl->im->H, NULL, cl->nC * cl->im->numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_transformed_probability_maps_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, cl->nC * cl->im->numSlices, cl->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, cl->nC * cl->im->numSlices, cl->im->W);
	return TRUE;
}
/* These are the real probabilities, e.g. the ratio of distance from cluster / sum of distances from all clusters
   for a given point. */
int	write_probability_maps_to_UNC_file(
	char		*filename,
	clengine	*cl,
	DATATYPE	minOutputPixel,
	DATATYPE	maxOutputPixel ){

	DATATYPE	***data;
	register int	i, j;

	/* we will write as many slices as there are clusters x numSlices */
	if( (data=callocDATATYPE3D(cl->nC * cl->im->numSlices, cl->im->W, cl->im->H)) == NULL ){
		fprintf(stderr, "write_probability_maps_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", cl->nC * cl->im->numSlices, cl->im->W, cl->im->H);
		return FALSE;
	}

	for(j=0;j<cl->nC*cl->im->numSlices;j++) for(i=0;i<cl->nP;i++) data[j][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] = 0;

	for(j=0;j<cl->nC;j++) for(i=0;i<cl->nP;i++){
		if( cl->p[i]->c == NULL ) continue;
		data[cl->p[i]->s*cl->nC+cl->c[j]->centroid->s][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] =
			(DATATYPE )(ROUND(SCALE_OUTPUT(((cl->p[i]->sD-cl->p[i]->d->c[cl->c[j]->id]) / (cl->p[i]->sD*(cl->nC-1))), (float )minOutputPixel, (float )maxOutputPixel, 0.0, 1.0)));
	}

	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, cl->im->W, cl->im->H, 0, 0, cl->im->W, cl->im->H, NULL, cl->nC * cl->im->numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_probability_maps_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, cl->nC * cl->im->numSlices, cl->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, cl->nC * cl->im->numSlices, cl->im->W);
	return TRUE;
}
int	write_masks_to_UNC_file(
	char		*filename,
	clengine	*cl,
	DATATYPE	minOutputPixel,
	DATATYPE	maxOutputPixel ){

	DATATYPE	***data;
	register int	i, j, k;

	/* we will write as many slices as there are clusters times num slices */
	if( (data=callocDATATYPE3D(cl->nC * cl->im->numSlices, cl->im->W, cl->im->H)) == NULL ){
		fprintf(stderr, "write_masks_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", cl->nC * cl->im->numSlices, cl->im->W, cl->im->H);
		return FALSE;
	}

	for(k=0;k<cl->nC*cl->im->numSlices;k++) for(i=0;i<cl->im->W;i++) for(j=0;j<cl->im->H;j++) data[k][i][j] = 0;

	for(i=0;i<cl->nP;i++){
		if( cl->p[i]->c == NULL ) continue;
		// cl->p[i]->c->centroid->s contains the id of the cluster c ! */
		data[cl->p[i]->s*cl->nC+cl->p[i]->c->centroid->s][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] = 100;
	}
	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, cl->im->W, cl->im->H, 0, 0, cl->im->W, cl->im->H, NULL, cl->nC * cl->im->numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_masks_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, cl->nC*cl->im->numSlices, cl->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, cl->nC * cl->im->numSlices, cl->im->W);
	return TRUE;
}

/* will write the confidence in a point's probability belonging to a certain cluster.
   confidence is defined over a 1pixel neighbourhood around each point (e.g. a 3x3
   grid of pixels) as the product of the pixel's prob times the sum of the probabilities
   of all other pixels in the grid. */
int	write_confidence_maps_to_UNC_file(
	char		*filename,
	clengine	*cl,
	DATATYPE	minOutputPixel,
	DATATYPE	maxOutputPixel ){

	DATATYPE	***data;
	register int	i, j, k;
	register float	sum, minP, maxP;

	/* we will write as many slices as there are clusters x numSlices */
	if( (data=callocDATATYPE3D(cl->nC * cl->im->numSlices, cl->im->W, cl->im->H)) == NULL ){
		fprintf(stderr, "write_confidence_maps_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", cl->nC * cl->im->numSlices, cl->im->W, cl->im->H);
		return FALSE;
	}

	for(j=0;j<cl->nC*cl->im->numSlices;j++) for(i=0;i<cl->nP;i++) data[j][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] = 0;

	minP = 100000000.0; maxP = -1.0;
	for(j=0;j<cl->nC;j++) for(i=0;i<cl->nP;i++){
		if( cl->p[i]->c == NULL ) continue;
		for(k=0,sum=0.0;k<8;k++){
			if( cl->p[i]->n[k] == NULL ) continue;
			sum += cl->p[i]->n[k]->p->c[j];
		}
		sum *= 3.0 * cl->p[i]->p->c[j] / ((float )k);
		if( sum < minP ) minP = sum;
		if( sum > maxP ) maxP = sum;
	}
	for(j=0;j<cl->nC;j++) for(i=0;i<cl->nP;i++){
		if( cl->p[i]->c == NULL ) continue;
		for(k=0,sum=0.0;k<8;k++){
			if( cl->p[i]->n[k] == NULL ) continue;
//			printf("%f ", cl->p[i]->n[k]->p->c[j]);
			sum += cl->p[i]->n[k]->p->c[j];
		}
		sum *= 3.0 * cl->p[i]->p->c[j] / ((float )k);
//		data[cl->p[i]->s*cl->nC+cl->p[i]->c->centroid->s][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] =
		data[cl->p[i]->s*cl->nC+cl->c[j]->centroid->s][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] =
			(DATATYPE )(ROUND(SCALE_OUTPUT(sum, (float )minOutputPixel, (float )maxOutputPixel, minP, maxP)));
//		printf(" (%d) %.0f %.0f = %f * %f  =  %d\n", cl->p[i]->c->id, cl->p[i]->x, cl->p[i]->y, sum, cl->p[i]->p->c[j], data[cl->p[i]->s*cl->nC+cl->p[i]->c->centroid->s][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)]);
	}
	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, cl->im->W, cl->im->H, 0, 0, cl->im->W, cl->im->H, NULL, cl->nC * cl->im->numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_confidence_maps_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, cl->nC * cl->im->numSlices, cl->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, cl->nC * cl->im->numSlices, cl->im->W);
	return TRUE;
}

/* this is useful when the points represent ascii data and not an image,
   then with this routine, each point (all its features) is dumped and also
   the cluster it belongs to, the distance from that cluster, the probability,
   the transformed probability and the confidence */
int	write_points_to_ASCII_file(
	char		*filename,
	clengine	*cl){

	FILE	*a_file;
	int	i, j, id;

	if( (a_file=fopen(filename, "w")) == NULL ){
		fprintf(stderr, "write_points_to_ASCII_file : could not open file '%s' for writing.\n", filename);
		return FALSE;
	}
	
fprintf(a_file, "# (id of nearest cluster) (total number of clusters) (total number of features) (distance from nearest cluster)\n");
fprintf(a_file, "# for each cluster (distance of point from that cluster, probability, transformed probability)\n");
fprintf(a_file, "# (all the features of the point)\n");

	for(i=0;i<cl->nP;i++){
		if( cl->p[i]->c == NULL ) continue;
		id = cl->p[i]->c->id; /* the id of the cluster this point belongs to */
		fprintf(a_file, "%d %d %d %f",
id, /* id of nearest cluster */
cl->nC, /* num of clusters */
cl->nF, /* num of features */
cl->p[i]->d->c[id]); /* distance of point from nearest (id) cluster  */

		for(j=0;j<cl->nC;j++)
			fprintf(a_file, " %f %f %f",
cl->p[i]->d->c[j], /* distance of point from the jth cluster */
cl->p[i]->p->c[j], /* probability that point belongs to the jth cluster */
cl->p[i]->td->c[j]); /* transformed probability that point belongs to the jth cluster */

		/* now dump all the features of the point */
		for(j=0;j<cl->nF;j++)
			fprintf(a_file, " %f", cl->p[i]->f->c[j]);

		fprintf(a_file, "\n");
	}

	return	TRUE;
}	

/* for each feature, replace each points pixel value by its nearest cluster centroid pixel */
/* leave minOutputPixel and maxOutputPixel -1 if you do not want any scaling */
int	write_clusters_to_UNC_file(
	char		*filename,
	clengine	*cl,
	DATATYPE	minOutputPixel,
	DATATYPE	maxOutputPixel ){

	DATATYPE	***data;
	register int	i, j, k;
	float		minP, maxP;

	/* for each slice of the input images and for each feature, 1 slice will be created displaying ALL clusters */
	/* e.g. numSlices * numFeatures */
	if( (data=callocDATATYPE3D(cl->nF * cl->im->numSlices , cl->im->W, cl->im->H)) == NULL ){
		fprintf(stderr, "write_clusters_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", cl->nF * cl->im->numSlices, cl->im->W, cl->im->H);
		return FALSE;
	}

	/* if you specified a ROI and do not want to include the original image when outside the ROI, change this below (set it to = 0; ) */
	for(k=0;k<cl->nF*cl->im->numSlices;k++) for(i=0;i<cl->im->W;i++) for(j=0;j<cl->im->H;j++) data[k][i][j] = 0;

	if( (minOutputPixel<0) || (maxOutputPixel<0) ){
		/* do not scale output */
		for(j=0;j<cl->nF;j++) for(i=0;i<cl->nP;i++)
			data[cl->p[i]->s*cl->nF+j][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] = cl->p[i]->c == NULL ? 0 : (DATATYPE )(ROUND(cl->p[i]->c->centroid->v->c[j]));
	} else {
		for(j=0;j<cl->nF;j++){
			minP = maxP = cl->p[0]->c->centroid->f->c[j];
			for(i=0;i<cl->nP;i++){
				if( cl->p[i]->c->centroid->f->c[j] < minP ) minP = cl->p[i]->c->centroid->f->c[j];
				if( cl->p[i]->c->centroid->f->c[j] > maxP ) maxP = cl->p[i]->c->centroid->f->c[j];
			}
			for(i=0;i<cl->nP;i++)
				data[cl->p[i]->s*cl->nF+j][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] = cl->p[i]->c == NULL ? 0 : (DATATYPE )(ROUND(SCALE_OUTPUT(cl->p[i]->c->centroid->f->c[j], (float )minOutputPixel, (float )maxOutputPixel, minP, maxP)));
		}
	}
	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, cl->im->W, cl->im->H, 0, 0, cl->im->W, cl->im->H, NULL, cl->nF*cl->im->numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_clusters_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, cl->nF*cl->im->numSlices, cl->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, cl->nF*cl->im->numSlices, cl->im->W);
	return TRUE;
}
void	write_clusters_to_ASCII_file(
	FILE		*handle,
	clengine	*cl){

	int	j, f;

	for(j=0;j<cl->nC;j++){
		//if( cl->c[j]->n <= 0 ) continue;

		/*
		printf("\n **** CLUSTER %d\n", cl->c[j]->id);
		for(f=0;f<cl->c[j]->n;f++){
			print_point_brief(stdout, cl->p[cl->c[j]->points[f]]);
		}
		*/

		f = 3;
		fprintf(handle, "# 1:numFeatures, 2:numPoints, 3-%d:pixel centroid, %d-%d:norm centroid, %d,%d:(x,y), %d:compactness, %d:homogeneity, %d:density, %d:penalty, %d-%d:[fitness for each feature], %d,%d:(distance from nearest and furthest point), %d,%d:(distance from nearest and furthest cluster), %d-%d:[min,max,mean,stdev pixels for each feature]\n",
			f+cl->nF-1, f+cl->nF, f+cl->nF*2-1,
			f+cl->nF*2, f+cl->nF*2+1,
			f+cl->nF*2+2, f+cl->nF*2+3, f+cl->nF*2+4, f+cl->nF*2+5,
			f+cl->nF*2+6, f+cl->nF*2+5+cl->nF,
			f+cl->nF*2+5+cl->nF+1,f+cl->nF*2+5+cl->nF+2,
			f+cl->nF*2+5+cl->nF+3,f+cl->nF*2+5+cl->nF+4,
			f+cl->nF*2+5+cl->nF+5, f+cl->nF*2+5+4*cl->nF+6);

		/* number of features & points it contains */
		fprintf(handle, "%d %d ", cl->nF, cl->c[j]->n);
		/* pixel centroid */
		for(f=0;f<cl->nF;f++)
			fprintf(handle, " %f", cl->c[j]->centroid->v->c[f]);
		/* normalised centroid */
		for(f=0;f<cl->nF;f++)
			fprintf(handle, " %f", cl->c[j]->centroid->f->c[f]);
		/* spatial centroid */
		fprintf(handle, " %f %f", cl->c[j]->centroid->x, cl->c[j]->centroid->y);
		/* stats */
		fprintf(handle, " %f %f %f %f",
			cl->c[j]->stats->compactness,
			cl->c[j]->stats->homogeneity,
			cl->c[j]->stats->density,
			cl->c[j]->stats->penalty);
		/* stats (cont), fitness */
		for(f=0;f<cl->nF;f++)
			fprintf(handle, " %f", cl->c[j]->stats->fitness->c[f]);
		/* stats (cont), distance from nearest and furthest point */
		fprintf(handle, " %f %f", cl->c[j]->stats->nPD, cl->c[j]->stats->fPD);
		/* stats (cont), distance from nearest and furthest cluster */
		fprintf(handle, " %f %f", cl->c[j]->stats->nCD, cl->c[j]->stats->fCD);

		/* stats (cont) min, max points (pixel values) */
		for(f=0;f<cl->nF;f++)
			fprintf(handle, " %f", cl->c[j]->stats->min_points->v->c[f]);
		for(f=0;f<cl->nF;f++)
			fprintf(handle, " %f", cl->c[j]->stats->max_points->v->c[f]);
		for(f=0;f<cl->nF;f++)
			fprintf(handle, " %f", cl->c[j]->stats->mean_points->v->c[f]);
		for(f=0;f<cl->nF;f++)
			fprintf(handle, " %f", cl->c[j]->stats->stdev_points->v->c[f]);

		fprintf(handle, "\n");
	}
	fflush(handle);
}

/* for each feature, replace each points pixel value by its nearest cluster centroid pixel */
/* leave minOutputPixel and maxOutputPixel = -1 if you do not want any scaling */
int	write_pixel_entropy_to_UNC_file(
	char		*filename,
	clengine	*cl,
	DATATYPE	minOutputPixel,
	DATATYPE	maxOutputPixel ){

	DATATYPE	***data;
	register int	i, j;
	float		sum, dummy;

	/* we will write as many slices as there are numSlices */
	if( (data=callocDATATYPE3D(cl->im->numSlices, cl->im->W, cl->im->H)) == NULL ){
		fprintf(stderr, "write_probability_maps_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", cl->im->numSlices, cl->im->W, cl->im->H);
		return FALSE;
	}

	for(j=0;j<cl->im->numSlices;j++) for(i=0;i<cl->nP;i++) data[j][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] = 0;

	for(i=0;i<cl->nP;i++){
		if( cl->p[i]->c == NULL ) continue;
		sum = 0.0;
		for(j=0;j<cl->nC;j++){
			dummy = (cl->p[i]->sD-cl->p[i]->d->c[cl->c[j]->id]) / (cl->p[i]->sD*(cl->nC-1));
			sum -= dummy * log(dummy);
		}
		/* the max of the sum is log(1/numClusters) */
		data[cl->p[i]->s][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] =
			(DATATYPE )(ROUND(SCALE_OUTPUT(sum, (float )minOutputPixel, (float )maxOutputPixel, 0.0, -log(1.0/cl->nC)+0.001)));
	}

	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, cl->im->W, cl->im->H, 0, 0, cl->im->W, cl->im->H, NULL, cl->im->numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_probability_maps_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, cl->nC * cl->im->numSlices, cl->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, cl->im->numSlices, cl->im->W);
	return TRUE;
}

int	write_partial_volume_pixels_to_UNC_file(
	char		*filename,
	clengine	*cl,
	DATATYPE	minOutputPixel,
	DATATYPE	maxOutputPixel ){

	DATATYPE	***data;
	float		***preData;
	register int	i, j, k;
	float		minP, maxP, d1, d2, d3, E1, E2,  E3, lD1, lD2, lD3, val;
	int		numCombinations = combinations(cl->nC, 2),
			numSlices = cl->im->numSlices * numCombinations, c1, c2, c3;

			/* the pv pairs(a,b) and the one remaining (c) */
	int		index[3][3] = { {0,1,2}, {0,2,1}, {1,2,0} }; 

	if( cl->nC != 3 ) {
		fprintf(stderr, "write_partial_volume_pixels_to_UNC_file : can only work with 3 clusters - skip.\n");
		return TRUE;
	}
	if( (minOutputPixel<0) || (maxOutputPixel<0) ){
		fprintf(stderr, "write_partial_volume_pixels_to_UNC_file : minOutputPixel and maxOutputPixel must be defined - put 0 and 10000\n");
		return FALSE;
	}
	/* for each slice of the input images and for each feature, 1 slice will be created displaying ALL clusters */
	/* e.g. numSlices * numFeatures */
	if( (data=callocDATATYPE3D(numSlices , cl->im->W, cl->im->H)) == NULL ){
		fprintf(stderr, "write_partial_volume_pixels_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", numSlices, cl->im->W, cl->im->H);
		return FALSE;
	}
	if( (preData=callocFLOAT3D(numSlices , cl->im->W, cl->im->H)) == NULL ){
		fprintf(stderr, "write_partial_volume_pixels_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d FLOATs.\n", numSlices, cl->im->W, cl->im->H);
		return FALSE;
	}
	/* if you specified a ROI and do not want to include the original image when outside the ROI, change this below (set it to = 0; ) */
	for(k=0;k<numSlices;k++) for(i=0;i<cl->im->W;i++) for(j=0;j<cl->im->H;j++) data[k][i][j] = minOutputPixel;

	minP = 100000000.0; maxP = -100000000.0;
	for(j=0;j<numCombinations;j++){
		c1 = cl->c[index[j][0]]->id;
		c2 = cl->c[index[j][1]]->id;
		c3 = cl->c[index[j][2]]->id;
		for(i=0;i<cl->nP;i++){
			d1 = cl->p[i]->d->c[c1];
			d2 = cl->p[i]->d->c[c2];
			d3 = cl->p[i]->d->c[c3];

//			if( (d1<d2) && (d1<d3) ){
				lD1 = -d1 * log(d1);
				lD2 = -d2 * log(d2);
				lD3 = -d3 * log(d3);
				E1 = lD1 + lD2; E2 = lD1 + lD3; E3 = lD2 + lD3;
				val = E1 / E2 + E1 / E3;
/*				H1 = H2 = -d1 * log(d1);
				H1 -= d2 * log(d2);
				H2 -= d3 * log(d3);
				val = 1.0/(H1 + H2);*/
				preData[cl->p[i]->s*numCombinations+j][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] = val;
				if( val < minP ) minP = val;
				if( val > maxP ) maxP = val;
//			} else preData[cl->p[i]->s*numCombinations+j][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] = -1.0;
		}
	}

	for(j=0;j<numCombinations;j++)
		for(i=0;i<cl->nP;i++){
			if( preData[cl->p[i]->s*numCombinations+j][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] < 0 ) data[cl->p[i]->s*numCombinations+j][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] = 0;
			else data[cl->p[i]->s*numCombinations+j][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)] =
				(DATATYPE )ROUND(SCALE_OUTPUT(preData[cl->p[i]->s*numCombinations+j][(int )(cl->p[i]->x)][(int )(cl->p[i]->y)], minOutputPixel, maxOutputPixel, minP, maxP));
		}
	
	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, cl->im->W, cl->im->H, 0, 0, cl->im->W, cl->im->H, NULL, numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_partial_volume_pixels_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, numSlices, cl->im->W);
		freeFLOAT3D(preData, numSlices, cl->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, numSlices, cl->im->W);
	freeFLOAT3D(preData, numSlices, cl->im->W);
	return TRUE;
}

int	create_clengine_from_matrix_object(matrix *m, int numClusters, clengine **cl)
{
	float	dummy;
	int	l, f;
	double	*pMDataColumn, **pMDataRow;

	/* create the clengine if not already created -- check alloc space if it is already created */
	if( *cl == NULL ){
		if( (*cl=new_clengine(0, m->nc, m->nr, numClusters)) == NULL ){
			fprintf(stderr, "read_data_from_ASCII_file : call to new_clengine has failed while reading matrix.\n");
			return FALSE;
		}
	} else {
		if( (*cl)->nF != m->nc ){
			fprintf(stderr, "read_data_from_ASCII_file : previously allocated clengine has capacity for %d features (data from matrix needs %d).\n", (*cl)->nF, m->nc);
			if( (*cl)->nF < m->nc ){ /* can't continue -- not enough space allocated */
				return FALSE;
			}
		}			
		if( (*cl)->nP != m->nr ){
			fprintf(stderr, "read_data_from_ASCII_file : previously allocated clengine has capacity for %d points (data from matrix needs %d).\n", (*cl)->nP, m->nr);
			if( (*cl)->nP < m->nr ){ /* can't continue -- not enough space allocated */
				return FALSE;
			}
		}			
		if( (*cl)->nC != numClusters ){
			fprintf(stderr, "read_data_from_ASCII_file : previously allocated clengine has capacity for %d features (data from matrix needs %d).\n", (*cl)->nC, numClusters);
			if( (*cl)->nC < numClusters ){ /* can't continue -- not enough space allocated */
				return FALSE;
			}
		}			
	}

	/* start filling the points */
	for(l=0,pMDataRow=&(m->d[0]);l<m->nr;l++,pMDataRow++){
		for(f=0,pMDataColumn=*pMDataRow;f<m->nc;f++,pMDataColumn++){
			dummy = *pMDataColumn;
			if( l == 0 ){
				(*cl)->stats->min_point_values->c[f] =
				(*cl)->stats->max_point_values->c[f] = dummy;
			}
			(*cl)->p[l]->v->c[f] = dummy;
			(*cl)->p[l]->f->c[f] = dummy;

			/* find min and max pixel values */
			if( (*cl)->p[l]->v->c[f] < (*cl)->stats->min_point_values->c[f] ) (*cl)->stats->min_point_values->c[f] = (*cl)->p[l]->v->c[f];
			if( (*cl)->p[l]->v->c[f] > (*cl)->stats->max_point_values->c[f] ) (*cl)->stats->min_point_values->c[f] = (*cl)->p[l]->v->c[f];
		}
		(*cl)->p[l]->x = (*cl)->p[l]->v->c[0]; /* first feature is x */
		(*cl)->p[l]->y = (*cl)->p[l]->v->c[1]; /* second feature is y */
		(*cl)->p[l]->z = 0;
		(*cl)->p[l]->s = 0;
	}

	/* save some image data specs into the cl->im structure */
	/* roi */
	(*cl)->im->w = (*cl)->im->h =
	(*cl)->im->x = (*cl)->im->y = -1;
	/* actual image size */
	(*cl)->im->W = (*cl)->im->H = -1;
	/* actual number of slices */
	(*cl)->im->actualNumSlices = -1;
	/* number of slices considered */
	(*cl)->im->numSlices = -1;
	/* slice separation */
	(*cl)->im->sliceSeparation = -1;

	printf("read_data_from_ASCII_file : created clengine from matrix with %d points, %d features and %d clusters.\n", m->nr, m->nc, numClusters);

	return TRUE;
}

