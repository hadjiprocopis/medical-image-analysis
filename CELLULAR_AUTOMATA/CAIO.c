#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "Common_IMMA.h"
#include "Alloc.h"
#include "IO.h"
#include "filters.h"
#include "LinkedList.h"
#include "LinkedListIterator.h"

#include "cellular_automata.h"

/* read data from a unc file and produce an array of points. Each slice in the unc file represents
   a feature while the first slice also represents the pixel value entry in the point struct
   (so the first slice should be the actual image unmodified).
*/
int	ca_read_data_from_UNC_file(
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
	cellular_automaton	**cl ){

	register int	i, j, s, slice, id, allSlices = FALSE, ii, jj, f;
	DATATYPE	***data, ***firstFeatureData;
	int		depth, format, actualNumSlices = 0, W = -1, H = -1,
			numPoints;
	char		*filename;

	if( cl == NULL ){
		fprintf(stderr, "ca_read_data_from_UNC_file : cl parameter should point to a clengine, initialised or not.\n");
		return FALSE;
	}
	/* read the first feature and do size and slice checks */
	filename = filenames[0];
	if( (firstFeatureData=getUNCSlices3D(filename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "ca_read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
		return FALSE;
	}
	if( *w <= 0 ) *w = W;
	if( *h <= 0 ) *h = H;
	if( (x+*w) > W ){
		fprintf(stderr, "ca_read_data_from_UNC_file : the actual image width (%d) of file '%s' is less than offset (%d) and width (%d) requested.\n", W, filename, x, *w);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	if( (y+*h) > H ){
		fprintf(stderr, "ca_read_data_from_UNC_file : the actual image height (%d) of file '%s' is less than offset (%d) and height (%d) requested.\n", H, filename, y, *h);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}
	if( *numSlices <= 0 ){ *numSlices = actualNumSlices; allSlices = TRUE; }
	else {
		for(i=0;i<*numSlices;i++){
			if( slices[i] >= actualNumSlices ){
				fprintf(stderr, "ca_read_data_from_UNC_file : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", actualNumSlices, filename);
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			} else if( slices[i] < 0 ){
				fprintf(stderr, "ca_read_data_from_UNC_file : slice numbers must start from 1.\n");
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
			fprintf(stderr, "ca_read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
			freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
			return FALSE;
		}
		/* check size */
		if( (*w != W) || (*h != H) ){
			fprintf(stderr, "ca_read_data_from_UNC_file : size of images in file '%s' (%dx%d) differ from those of the first in collection (%dx%d).\n", filename, W, H, *w, *h);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
			return FALSE;
		}
		/* check slices */
		if( allSlices == TRUE ){
			if( *numSlices != actualNumSlices ){
				fprintf(stderr, "ca_read_data_from_UNC_file : input volumes must have the same number of slices (first image has %d slices, image '%s' has %d slices).\n", actualNumSlices, filename, *numSlices);
				freeDATATYPE3D(data, actualNumSlices, W);
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		} else {
			for(i=0;i<*numSlices;i++){
				if( slices[i] >= actualNumSlices ){
					fprintf(stderr, "ca_read_data_from_UNC_file : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", actualNumSlices, filename);
					freeDATATYPE3D(data, actualNumSlices, W);
					freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
					return FALSE;
				} else if( slices[i] < 0 ){
					fprintf(stderr, "ca_read_data_from_UNC_file : slice numbers must start from 1.\n");
					freeDATATYPE3D(data, actualNumSlices, W);
					freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
					return FALSE;
				}
			}
		}
		freeDATATYPE3D(data, actualNumSlices, W);
	}
	if( numPoints == 0 ){
		fprintf(stderr, "ca_read_data_from_UNC_file : could not find any points in the pixel interval %d to %d within the ROI starting at (%d,%d) with size (%d,%d).\n", minThreshold, maxThreshold, x, y, *w, *h);
		freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
		return FALSE;
	}

	/* create the clengine if not already created -- check alloc space if it is already created */
	if( *cl == NULL ){
		if( (*cl=ca_new_cellular_automaton(0, numFeatures, numPoints, numClusters)) == NULL ){
			fprintf(stderr, "ca_read_data_from_UNC_file : call to new_clengine has failed while reading data from file '%s'.\n", filename);
			freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
			return FALSE;
		}
	} else {
		if( (*cl)->nF != numFeatures ){
			fprintf(stderr, "ca_read_data_from_UNC_file : previously allocated clengine has capacity for %d features (data from file '%s' needs %d).\n", (*cl)->nF, filename, numFeatures);
			if( (*cl)->nF < numFeatures ){ /* can't continue -- not enough space allocated */
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		}			
		if( (*cl)->nC != numPoints ){
			fprintf(stderr, "ca_read_data_from_UNC_file : previously allocated clengine has capacity for %d points (data from file '%s' needs %d).\n", (*cl)->nC, filename, numPoints);
			if( (*cl)->nC < numPoints ){ /* can't continue -- not enough space allocated */
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		}			
		if( (*cl)->nS != numClusters ){
			fprintf(stderr, "ca_read_data_from_UNC_file : previously allocated clengine has capacity for %d features (data from file '%s' needs %d).\n", (*cl)->nS, filename, numClusters);
			if( (*cl)->nC < numClusters ){ /* can't continue -- not enough space allocated */
				freeDATATYPE3D(firstFeatureData, actualNumSlices, W);
				return FALSE;
			}
		}			
	}

	/* start filling the points */
	for(f=0;f<numFeatures;f++){
		id = 0;
		filename = filenames[f];
		printf("doing file '%s'\n", filename);
		if( (data=getUNCSlices3D(filename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
			fprintf(stderr, "ca_read_data_from_UNC_file : call to getUNCSlices3D failed for file '%s'.\n", filename);
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
				(*cl)->c[id]->x = i; (*cl)->c[id]->y = j;
				(*cl)->c[id]->X = i; (*cl)->c[id]->Y = j;
				(*cl)->c[id]->z = slice * sliceSeparation;
				(*cl)->c[id]->Z = (int )(slice * sliceSeparation);
				(*cl)->c[id]->s = s;
				(*cl)->c[id]->v->c[f] = (float )(data[slice][i][j]);
				(*cl)->c[id]->f->c[f] = (float )(data[slice][i][j]);

				/* find min and max pixel values */
				if( (*cl)->c[id]->v->c[f] < (*cl)->stats->min_point_values->c[f] ) (*cl)->stats->min_point_values->c[f] = (*cl)->c[id]->v->c[f];
				if( (*cl)->c[id]->v->c[f] > (*cl)->stats->max_point_values->c[f] ) (*cl)->stats->max_point_values->c[f] = (*cl)->c[id]->v->c[f];

				id++;
			}
		}
		printf("id was %d num points %d\n", id, numPoints);
	}
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

	printf("ca_read_data_from_UNC_file : read %d points of %d features from %d slices\n", numPoints, numFeatures, *numSlices);

	return TRUE;
}

int	ca_write_masks_to_UNC_file(
	char			*filename,
	cellular_automaton	*aca,
	DATATYPE		minOutputPixel,
	DATATYPE		maxOutputPixel ){

	DATATYPE	***data;
	register int	i, j, k, numSlices = aca->im->numSlices * aca->nS;
	cell		*c;

	/* we will write as many slices as there are clusters times num slices */
	if( (data=callocDATATYPE3D(numSlices, aca->im->W, aca->im->H)) == NULL ){
		fprintf(stderr, "write_masks_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", numSlices, aca->im->W, aca->im->H);
		return FALSE;
	}

	/* zero */
	for(k=0;k<numSlices;k++) for(i=0;i<aca->im->W;i++) for(j=0;j<aca->im->H;j++) data[k][i][j] = 0;

	for(i=0;i<aca->nC;i++){
		c = aca->c[i];
		data[c->s*aca->nS+c->cs->id][c->X][c->Y] = maxOutputPixel;
	}

	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, aca->im->W, aca->im->H, 0, 0, aca->im->W, aca->im->H, NULL, numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_masks_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, numSlices, aca->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, numSlices, aca->im->W);
	return TRUE;
}
int	ca_write_probs_to_UNC_file(
	char			*filename,
	cellular_automaton	*aca,
	DATATYPE		minOutputPixel,
	DATATYPE		maxOutputPixel ){

	DATATYPE	***data;
	register int	i, j, k, numSlices = aca->im->numSlices * aca->nS;
	cell		*c;

	/* we will write as many slices as there are clusters times num slices */
	if( (data=callocDATATYPE3D(numSlices, aca->im->W, aca->im->H)) == NULL ){
		fprintf(stderr, "write_masks_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", numSlices, aca->im->W, aca->im->H);
		return FALSE;
	}

	/* zero */
	for(k=0;k<numSlices;k++) for(i=0;i<aca->im->W;i++) for(j=0;j<aca->im->H;j++) data[k][i][j] = 0;

	for(i=0;i<aca->nC;i++){
		c = aca->c[i];
		for(j=0;j<aca->nS;j++)
		data[c->s*aca->nS+j][c->X][c->Y] =
			(DATATYPE )(SCALE_OUTPUT(c->hnn->c[j], minOutputPixel, maxOutputPixel, 0.0, 1.0));
	}

	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, aca->im->W, aca->im->H, 0, 0, aca->im->W, aca->im->H, NULL, numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_masks_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, numSlices, aca->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, numSlices, aca->im->W);
	return TRUE;
}
int	ca_write_counts_to_UNC_file(
	char			*filename,
	cellular_automaton	*aca,
	DATATYPE		minOutputPixel,
	DATATYPE		maxOutputPixel ){

	DATATYPE	***data;
	register int	i, j, k;
	int		numSlices = aca->im->numSlices, agree, sid;
	cell		*c;

	/* we will write as many slices as there are clusters times num slices */
	if( (data=callocDATATYPE3D(numSlices, aca->im->W, aca->im->H)) == NULL ){
		fprintf(stderr, "write_counts_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", numSlices, aca->im->W, aca->im->H);
		return FALSE;
	}

	/* zero */
	for(k=0;k<numSlices;k++) for(i=0;i<aca->im->W;i++) for(j=0;j<aca->im->H;j++) data[k][i][j] = 0;

	for(i=0;i<aca->nC;i++){
		agree = 0; c = aca->c[i];
		sid = c->cs->id;
		for(j=0;j<c->nNN;j++) if( sid == c->nn[j]->cs->id ) agree++;
		data[c->s][c->X][c->Y] =
			(DATATYPE )(SCALE_OUTPUT(agree, minOutputPixel, maxOutputPixel, 0.0, c->nNN));
	}

	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, aca->im->W, aca->im->H, 0, 0, aca->im->W, aca->im->H, NULL, numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_counts_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, numSlices, aca->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, numSlices, aca->im->W);
	return TRUE;
}
int	ca_write_states_to_UNC_file(
	char			*filename,
	cellular_automaton	*aca,
	DATATYPE		minOutputPixel,
	DATATYPE		maxOutputPixel ){

	DATATYPE	***data;
	register int	i, j, k, f;
 	register float	minState, maxState;
	int		numSlices = aca->im->numSlices * aca->nF;
	cell		*c;

	/* we will write as many slices as there are clusters times num slices */
	if( (data=callocDATATYPE3D(numSlices, aca->im->W, aca->im->H)) == NULL ){
		fprintf(stderr, "write_states_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", numSlices, aca->im->W, aca->im->H);
		return FALSE;
	}

	/* zero */
	for(k=0;k<numSlices;k++) for(i=0;i<aca->im->W;i++) for(j=0;j<aca->im->H;j++) data[k][i][j] = 0;

	for(f=0;f<aca->nF;f++){
		maxState = minState = aca->c[0]->state->c[f];
		for(i=1;i<aca->nC;i++){
			c = aca->c[i];
			if( c->state->c[f] > maxState ) maxState = c->state->c[f];
			if( c->state->c[f] < minState ) minState = c->state->c[f];
		}
		for(i=0;i<aca->nC;i++){
			c = aca->c[i];
			data[c->s*aca->nF+f][c->X][c->Y] =
				(DATATYPE )(SCALE_OUTPUT(c->state->c[f], minOutputPixel, maxOutputPixel, minState, maxState));
		}
	}

	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, aca->im->W, aca->im->H, 0, 0, aca->im->W, aca->im->H, NULL, numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_states_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, numSlices, aca->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, numSlices, aca->im->W);
	return TRUE;
}
int	ca_write_fitness_to_UNC_file(
	char			*filename,
	cellular_automaton	*aca,
	DATATYPE		minOutputPixel,
	DATATYPE		maxOutputPixel ){

	DATATYPE	***data;
	register int	i, j, k, f;
	register float	minFitness, maxFitness;
	int		numSlices = aca->im->numSlices * aca->nF;
	cell		*c;

	/* we will write as many slices as there are clusters times num slices */
	if( (data=callocDATATYPE3D(numSlices, aca->im->W, aca->im->H)) == NULL ){
		fprintf(stderr, "write_fitness_to_UNC_file : call to callocDATATYPE3D has failed for %dx%dx%d DATATYPEs.\n", numSlices, aca->im->W, aca->im->H);
		return FALSE;
	}

	/* zero */
	for(k=0;k<numSlices;k++) for(i=0;i<aca->im->W;i++) for(j=0;j<aca->im->H;j++) data[k][i][j] = 0;

	for(f=0;f<aca->nF;f++){
		maxFitness = minFitness = aca->c[0]->fitness->c[f];
		for(i=1;i<aca->nC;i++){
			c = aca->c[i];
			if( c->fitness->c[f] > maxFitness ) maxFitness = c->fitness->c[f];
			if( c->fitness->c[f] < minFitness ) minFitness = c->fitness->c[f];
		}
		for(i=0;i<aca->nC;i++){
			c = aca->c[i];
			data[c->s*aca->nF+f][c->X][c->Y] =
				(DATATYPE )(SCALE_OUTPUT(c->fitness->c[f], minOutputPixel, maxOutputPixel, minFitness, maxFitness));
		}
	}

	/* depth=2, format=8, mode=OVERWRITE */
	if( writeUNCSlices3D(filename, data, aca->im->W, aca->im->H, 0, 0, aca->im->W, aca->im->H, NULL, numSlices, 8, OVERWRITE) == FALSE ){
		fprintf(stderr, "write_fitness_to_UNC_file : call to writeUNCSlices3D has failed for file '%s'.\n", filename);
		freeDATATYPE3D(data, numSlices, aca->im->W);
		return FALSE;
	}
	freeDATATYPE3D(data, numSlices, aca->im->W);
	return TRUE;
}
void	ca_print_cells_and_neighbours(FILE *stream, cellular_automaton *aca){
	int	i, j;
	cell	*c;
	char	*p;
	for(i=0;i<aca->nC;i++) fprintf(stream, "%d", aca->c[i]->cs->id);
	fprintf(stream, "\n");

	for(i=0;i<aca->nC;i++){
		c = aca->c[i];
		p = toString_vector(c->fitness);
		fprintf(stream, "%d, (%d,%d)=%.0f, state=%s, %d ->",
			c->id, c->X, c->Y, c->v->c[0],
			p,
			c->cs->id);
		free(p);
		for(j=0;j<c->nNN;j++){
			fprintf(stream, " %d(%d,%d)",
				c->nn[j]->cs->id,c->nn[j]->X, c->nn[j]->Y);
		}
		fprintf(stream, "\n");
	}
}
