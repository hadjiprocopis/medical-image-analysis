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

#include "clustering.h"

/* private routines for the qsort comparator, for sorting clusters in pixel order */
/* the user should call
	'clengine_sort_clusters_wrt_pixelvalue_ascending' and
	'clengine_sort_clusters_wrt_pixelvalue_descending'
or (for normalised pixel values)
	'clengine_sort_clusters_wrt_normalisedpixelvalue_ascending' and
	'clengine_sort_clusters_wrt_normalisedpixelvalue_descending'
*/
static	int	_cluster_comparator_pixelvalue_ascending(const void *, const void *);
static	int	_cluster_comparator_pixelvalue_descending(const void *, const void *);
//static	int	_cluster_comparator_normalisedpixelvalue_ascending(const void *, const void *);
//static	int	_cluster_comparator_normalisedpixelvalue_descending(const void *, const void *);

clengine	*new_clengine(int id, int numFeatures, int numPoints, int numClusters){
	int		i;
	clengine	*c;

	if( (c=(clengine *)malloc(sizeof(clengine))) == NULL ){
		fprintf(stderr, "new_clengine : could not allocate %zd bytes for 'clengine'.\n", sizeof(clengine));
		return NULL;
	}
	c->id = id;
	c->nC = numClusters; c->nP = numPoints; c->nF = numFeatures;

	if( (c->c=(cluster **)malloc(numClusters * sizeof(cluster *))) == NULL ){
		fprintf(stderr, "new_clengine : could not allocate %zd bytes for 'c->c'.\n", numClusters * sizeof(cluster *));
		free(c);
		return NULL;
	}
	if( (c->p=(point **)malloc(numPoints * sizeof(point *))) == NULL ){
		fprintf(stderr, "new_clengine : could not allocate %zd bytes for 'c->p'.\n", numPoints * sizeof(point *));
		free(c->c); free(c);
		return NULL;
	}
	for(i=0;i<numClusters;i++){
		if( (c->c[i]=new_cluster(i, numFeatures, numPoints, numClusters)) == NULL ){
			fprintf(stderr, "new_clengine : call to new_cluster has failed for cluster %d.\n", i);
			destroy_clusters(c->c, i); free(c->p); free(c->c); free(c);
			return NULL;
		}
		c->c[i]->cl = c; /* the clengine of the cluster is us */
		c->c[i]->all_points = c->p;
	}
	for(i=0;i<numPoints;i++)
		if( (c->p[i]=new_point(i, numFeatures, numClusters)) == NULL ){
			fprintf(stderr, "new_clengine : call to new_point has failed for point %d.\n", i);
			destroy_points(c->p, i); destroy_clusters(c->c, numClusters); free(c->p); free(c->c); free(c);
			return NULL;
		}
	if( (c->stats=new_clestats(numFeatures)) == NULL ){
		fprintf(stderr, "new_clengine : call to new_clestats has failed for clengine %d.\n", c->id);
		destroy_points(c->p, numPoints); destroy_clusters(c->c, numClusters); free(c->p); free(c->c); free(c);		
		return NULL;
	}

	/* needs to be set any time an image is read */
	if( (c->im=(clunc *)malloc(sizeof(clunc))) == NULL ){
		fprintf(stderr, "new_clengine : could not allocate %zd bytes for c->im.\n", sizeof(clunc));
		destroy_clestats(c->stats); destroy_points(c->p, numPoints); destroy_clusters(c->c, numClusters); free(c->p); free(c->c); free(c);		
		return NULL;
	}
	c->im->w = c->im->h = c->im->w = c->im->h = c->im->W = c->im->H = -1;

	c->interruptFlag = FALSE;

	printf("new_clengine : clengine created with %d feature(s), %d points and %d clusters.\n", c->nF, c->nP, c->nC);

	return c;
}
void	clengine_interrupt(clengine *cl, int flag){ cl->interruptFlag = flag; }

void	destroy_clengine(clengine *c){
	destroy_clusters(c->c, c->nC);
	destroy_points(c->p, c->nP);
	destroy_clestats(c->stats);
	free(c->im);
	free(c);
}
char	*toString_clengine(void *p){
	clengine	*a_clengine = (clengine *)p;
	char	*s, *ret,
		*sS = toString_clestats(a_clengine->stats);

	if( (s=(char *)malloc(strlen(sS)+250)) == NULL ){
		fprintf(stderr, "toString_clengine : could not allocate %zd bytes.\n", strlen(sS)+250);
		return "<empty clengine>";
	}
	sprintf(s, "clengine %d with %d clusters and %d points, stats:\n%s",
			a_clengine->id,
			a_clengine->nC, a_clengine->nP,
			sS);
	free(sS);
	ret = strdup(s);
	free(s);
	return ret;
}
void	print_clengine(FILE *handle, clengine *a_clengine){
	char	*s = toString_clengine((void *)a_clengine);

	fprintf(handle, "%s\n", s);
	free(s);
}
clengine	*duplicate_clengine(clengine *a_clengine, int new_id){
	clengine	*dup;

	if( (dup=new_clengine(a_clengine->id, a_clengine->nF, a_clengine->nP, a_clengine->nC)) == NULL ){
		fprintf(stderr, "duplicate_clengine : call to new_clengine has failed for %d features, %d points, %d clusters, id is %d\n", a_clengine->nF, a_clengine->nP, a_clengine->nC, a_clengine->id);
		return NULL;
	}
	copy_clengine(a_clengine, dup);
	dup->id = new_id;
	return dup;
}

/* copy c1 onto c2 */
void		copy_clengine(clengine *cl1, clengine *cl2){
	int	i;
	register point	**p1, **p2;
	register cluster **c1, **c2;

	for(i=0,p1=&(cl1->p[i]),p2=&(cl2->p[i]);i<cl1->nP;i++,p1++,p2++) copy_point(*p1, *p2);
	for(i=0,c1=&(cl1->c[i]),c2=&(cl2->c[i]);i<cl1->nC;i++,c1++,c2++){
		copy_cluster(*c1, *c2);
		(*c2)->cl = cl2;
	}
	copy_clestats(cl1->stats, cl2->stats);
	cl1->interruptFlag = cl2->interruptFlag;
}
void	clengine_save_state(clengine *cle, clengine *saved){
	if( saved == NULL )
		if( (saved=new_clengine(cle->id, cle->nF, cle->nP, cle->nC)) == NULL ){
			fprintf(stderr, "clengine_save_state : call to new_clengine has for %d features, %d points, %d clusters, id is %d\n", cle->nF, cle->nP, cle->nC, cle->id);
			return;
		}
	copy_clengine(cle, saved);
}
void	clengine_load_state(clengine *cle, clengine *saved){
	copy_clengine(saved, cle);
}
void	print_clengine_brief(FILE *handle, clengine *a_clengine){
}
void	print_clengine_for_plotting(FILE *handle, clengine *a_clengine){
}
void	clengine_print_clusters(FILE *handle, clengine *cl){
	int			i;

	for(i=0;i<cl->nC;i++) print_cluster(handle, cl->c[i]);
	fflush(handle);
}
void	clengine_print_points(FILE *handle, clengine *cl){
	int			i;

	for(i=0;i<cl->nP;i++) print_point(handle, cl->p[i]);
	fflush(handle);
}
void	clengine_unnormalise_clusters(clengine *cl){
	int	i, f;

	for(i=0;i<cl->nC;i++) for(f=0;f<cl->nF;f++)
		cl->c[i]->centroid->v->c[f] =
	(cl->c[i]->centroid->f->c[f] - cl->stats->min_point_values_normalised->c[f]) /
		cl->stats->tantheta->c[f] + cl->stats->min_point_values->c[f];
}
int	clengine_normalise_points(clengine *cl, float rangeMin, float rangeMax, char overAllFeaturesFlag){
	/* normalise either over all feature space -- e.g. find pixel range by searching over all
	   features, or normalise over each feature, min and max will be only for the pixels
	   of that feature */
	register int	i, f, j;
	float	minP, maxP;
	float	*scrap;

/*
	// if you want 3 clear-cut clusters for testing:
	for(f=0;f<cl->nF;f++) for(i=0;i<cl->nP;i++){
		if( i < (cl->nP/3) ) cl->p[i]->f->c[f] = 0.2*drand48();
		else if( i < (2*cl->nP/3) ) cl->p[i]->f->c[f] = 0.3 + 0.2*drand48();
		else if( i < cl->nP ) cl->p[i]->f->c[f] = 0.6 + 0.2*drand48();
	}
	return TRUE;
*/
	if( overAllFeaturesFlag ){
		/* normalisation will be over all the features of all the points --
		   e.g. the min and max values will be over all feature space */
		if( (scrap=(float *)malloc(cl->nP * cl->nF * sizeof(float))) == NULL ){
			fprintf(stderr, "clengine_normalise_points : could not allocate %zd bytes for 'scrap'.\n", cl->nP * cl->nF * sizeof(float));
			return FALSE;
		}
		j = 0;
		for(f=0;f<cl->nF;f++) for(i=0;i<cl->nP;i++) scrap[j++] = cl->p[i]->v->c[f];
		min_maxPixel1D_float(scrap, 0, cl->nP * cl->nF, &minP, &maxP);
		for(f=0;f<cl->nF;f++){
			i = 0;
			cl->p[i]->f->c[f] = SCALE_OUTPUT(cl->p[i]->v->c[f], rangeMin, rangeMax, minP, maxP);
			cl->stats->min_point_values_normalised->c[f] = cl->p[i]->f->c[f];
			cl->stats->max_point_values_normalised->c[f] = cl->p[i]->f->c[f];
			for(i=1;i<cl->nP;i++){
				cl->p[i]->f->c[f] = SCALE_OUTPUT(cl->p[i]->v->c[f], rangeMin, rangeMax, minP, maxP);
				if( cl->p[i]->f->c[f] < cl->stats->min_point_values_normalised->c[f] ) cl->stats->min_point_values_normalised->c[f] = cl->p[i]->f->c[f];
				if( cl->p[i]->f->c[f] > cl->stats->max_point_values_normalised->c[f] ) cl->stats->max_point_values_normalised->c[f] = cl->p[i]->f->c[f];
			}
		}
	} else {
		if( (scrap=(float *)malloc(cl->nP * sizeof(float))) == NULL ){
			fprintf(stderr, "clengine_normalise_points : could not allocated %zd bytes for 'scrap'.\n", cl->nP * sizeof(float));
			return FALSE;
		}
		for(f=0;f<cl->nF;f++){
			for(i=0;i<cl->nP;i++) scrap[i] = cl->p[i]->v->c[f];
			min_maxPixel1D_float(scrap, 0, cl->nP, &minP, &maxP);
			for(i=0;i<cl->nP;i++) cl->p[i]->f->c[f] = SCALE_OUTPUT(cl->p[i]->v->c[f], rangeMin, rangeMax, minP, maxP);
			cl->stats->min_point_values_normalised->c[f] = rangeMin;
			cl->stats->max_point_values_normalised->c[f] = rangeMax;
		}
	}

	for(f=0;f<cl->nF;f++)
		cl->stats->tantheta->c[f] =
	(cl->stats->max_point_values_normalised->c[f] - cl->stats->min_point_values_normalised->c[f])
	 /
	(cl->stats->max_point_values->c[f] - cl->stats->min_point_values->c[f]);

	free(scrap);
	return TRUE;
}		
void	calculate_clengine(clengine *cl){
	register int	i, j;
	register point	 **p;
	register float   *v, *vv;
	register cluster **c;

	clengine_sort_clusters_wrt_pixelvalue_ascending(cl);

	/* find distances, sum of distances, transformed distances, probs etc. */
	cl->stats->total_distance = 0.0;	
	for(i=0,p=&(cl->p[i]);i<cl->nP;i++,p++){
		/* point to cluster distances */
		(*p)->sD = 0.0;
		for(j=0,v=&((*p)->d->c[j]),c=&(cl->c[j]);j<cl->nC;j++,v++,c++){
			*v = euclidean_distance_between_two_vectors((*p)->f, (*c)->centroid->f);
			(*p)->sD += *v;
		}
		cl->stats->total_distance += (*p)->sD;
		/* transformed distances */
		(*p)->sTD = 0.0;
		for(j=0,v=&((*p)->td->c[j]),vv=&((*p)->d->c[j]),c=&(cl->c[j]);j<cl->nC;j++,v++,vv++,c++){
/* beta is the locality term of the transformed distance metric. There are 2 betas.
   one is in cluster - which means that there is one beta for each cluster and all the points
   use that beta when they make their distance calculations.
   The other beta is in each point which means that each point has a different beta from other points
   but it uses the same beta for calculating distances from different clusters.
   By combining the two betas, we could have a different beta for each point/cluster combination. */
			*v = exp( - (*c)->beta * (*vv) * (*p)->spm->c[j] / ((*p)->sD) );
			(*p)->sTD += *v;
		}
		/* probabilities */
		for(j=0,v=&((*p)->p->c[j]),vv=&((*p)->td->c[j]);j<cl->nC;j++,v++,vv++)
			*v = (*vv) / (*p)->sTD;
		/* entropy */
		(*p)->entropy = 0.0;
		for(j=0,v=&((*p)->p->c[j]);j<cl->nC;j++,v++)
			(*p)->entropy -= (*v) == 0.0 ? 0.0 : (*v) * log( (*v) );
	}
}
int	clengine_assign_points_to_clusters_without_relocations(clengine *cl){
	register int	i, j, nearestCluster;
	register float	minDistance;

	register	float	*v;
	register	point	**p;
	register	cluster *c;

	for(j=0;j<cl->nC;j++) cl->c[j]->n = 0;
	for(i=0,p=&(cl->p[i]);i<cl->nP;i++,p++){
		minDistance = (*p)->d->c[0];
		nearestCluster = 0; c = cl->c[nearestCluster];
		for(j=1,v=&(cl->p[i]->d->c[j]);j<cl->nC;j++,v++)
			if( *v < minDistance ){ minDistance = *v; nearestCluster = j; c = cl->c[nearestCluster]; }
		c->points[c->n++] = i;
		(*p)->c = c;
	}
	return TRUE;
}

int	clengine_assign_points_to_clusters(clengine *cl){
	register int	i, j, nearestCluster, numRelocations = 0;
	register float	minDistance, check;
	float		*checksums;
	char		keepGoingFlag;

	register	float	*cP, *v;
	register	point	**p;
	register	cluster *c;

	if( (checksums=(float *)malloc(cl->nC * sizeof(float))) == NULL ){
		fprintf(stderr, "clengine_assign_points_to_clusters : could not allocate %zd bytes for checksums.\n", cl->nC * sizeof(float));
		return FALSE;
	}
	cP = &(checksums[0]);

	for(j=0,cP=&(checksums[j]);j<cl->nC;j++,cP++) *cP = -1.0;
	do {
		/* distribute points to nearest clusters and repeat this process
		   until there are no point relocations (do-while) */
		for(j=0;j<cl->nC;j++) cl->c[j]->n = 0;
		for(i=0,p=&(cl->p[i]);i<cl->nP;i++,p++){
			minDistance = (*p)->d->c[0];
			nearestCluster = 0; c = cl->c[nearestCluster];
			for(j=1,v=&(cl->p[i]->d->c[j]);j<cl->nC;j++,v++)
				if( *v < minDistance ){ minDistance = *v; nearestCluster = j; c = cl->c[nearestCluster]; }
			c->points[c->n++] = i;
			(*p)->c = c;
		}

		/* calculate centroid of clusters */
		for(j=0;j<cl->nC;j++) calculate_cluster(cl->c[j]);
		/* calculate new distances/probs for points */
		calculate_clengine(cl);

		/* this is a way to find out if there were any relocations */
		keepGoingFlag = FALSE;
		for(j=0,cP=&(checksums[j]);j<cl->nC;j++,cP++){
			for(i=0,check=0.0;i<cl->c[j]->n;i++)
				check += ((i+1.0) * cl->c[j]->points[i]) / 10000000.0;
			if( check != (*cP) ){ *cP = check; keepGoingFlag = TRUE; }
		}
		if( (numRelocations++ > 500) || (cl->interruptFlag==TRUE) ) break;
	} while( keepGoingFlag );

	free(checksums);

	calculate_clengine(cl);
	calculate_clengine_stats(cl);
	for(j=0;j<cl->nC;j++) calculate_cluster(cl->c[j]);
#ifdef	CLENGINE_VERBOSE
	for(j=0;j<cl->nC;j++) print_cluster_brief(stderr, cl->c[j]);
	printf("\nFinal stats:\ttE=%f, tD=%f,\n\t\tmH=%f, mC=%f, stdH=%f, stdC=%f, relocs=%d\n\n",
		cl->stats->total_entropy, cl->stats->total_distance, cl->stats->mean_homogeneity,
		cl->stats->mean_compactness,
		cl->stats->stdev_homogeneity, cl->stats->stdev_compactness, numRelocations);
#endif

	return TRUE;
}
void	clengine_shake_clusters(clengine *cl, float p){
	/* p is a percentage (0-1) of noise to be added around the centroid of each cluster */
	/* noise = rand() scaled around +/- p * centroid */
	/* NOTE: the random number obtained by rand() will be different for each centroid dimension and for each cluster */

	int	i;
	for(i=0;i<cl->nC;i++)
		shake_cluster(cl->c[i], p);
}

void	calculate_clengine_stats(clengine *cl){
	float	nearestCD, furthestCD, x;
	int	nearestC, furthestC;
	int	j, f, *n;
	float	nC = cl->nC;

	if( (n=(int *)malloc(cl->nF*sizeof(int))) == NULL ){
		fprintf(stderr, "calculate_clengine_stats : could not allocate %zd bytes.\n", cl->nF*sizeof(int));
		return;
	}

	/* do the clusters first */
	for(j=0;j<cl->nC;j++) calculate_cluster_stats(cl->c[j]);
		
	cl->stats->mean_homogeneity = cl->stats->mean_compactness =
	cl->stats->stdev_homogeneity= cl->stats->stdev_compactness= 0.0;
	for(f=0;f<cl->nF;f++) cl->stats->mean_fitness->c[f] = cl->stats->stdev_fitness->c[f] = 0.0;

	for(j=0,n[0]=0;j<cl->nC;j++)
		if( cl->c[j]->stats->homogeneity > 0.0 ){ cl->stats->mean_homogeneity += cl->c[j]->stats->homogeneity; n[0]++; }
	if( n[0] > 0 ) cl->stats->mean_homogeneity /= n[0];
	for(j=0,n[0]=0;j<cl->nC;j++)
		if( cl->c[j]->stats->compactness > 0.0 ){ cl->stats->mean_compactness += cl->c[j]->stats->compactness; n[0]++; }
	if( n[0] > 0 ) cl->stats->mean_compactness /= n[0];

	for(f=0;f<cl->nF;f++) n[f] = 0;
	for(j=0;j<cl->nC;j++){
		for(f=0;f<cl->nF;f++)
			if( cl->c[j]->stats->fitness->c[f] > 0.0 ){ cl->stats->mean_fitness->c[f] += cl->c[j]->stats->fitness->c[f]; n[f]++; }
	}
	for(f=0;f<cl->nF;f++){
		if( n[f] > 0 ) cl->stats->mean_fitness->c[f] /= n[f];
		cl->stats->stdev_fitness->c[f] = 0.0;
	}
	for(j=0;j<cl->nC;j++){
		cl->stats->stdev_homogeneity += SQR(cl->stats->mean_homogeneity - cl->c[j]->stats->homogeneity);
		cl->stats->stdev_compactness += SQR(cl->stats->mean_compactness - cl->c[j]->stats->compactness);
		for(f=0;f<cl->nF;f++)
			cl->stats->stdev_fitness->c[f] += SQR(cl->stats->mean_fitness->c[f] - cl->c[j]->stats->fitness->c[f]);
	}
	cl->stats->stdev_homogeneity = sqrt(cl->stats->stdev_homogeneity / nC);
	cl->stats->stdev_compactness = sqrt(cl->stats->stdev_compactness / nC);
	for(f=0;f<cl->nF;f++) cl->stats->stdev_fitness->c[f] = sqrt(cl->stats->stdev_fitness->c[f] / nC);
	cl->stats->total_entropy = 0.0;
	for(j=0;j<cl->nP;j++) cl->stats->total_entropy += cl->p[j]->entropy;

	/* calculate nearest and furthest cluster from each cluster */
	for(j=0;j<cl->nC;j++){
		nearestC = furthestC = j==0?1:0;
		nearestCD = furthestCD = euclidean_distance_between_two_vectors(cl->c[j]->centroid->f, cl->c[nearestC]->centroid->f);
		for(f=0;f<cl->nC;f++){
			if( f == j ) continue;
			x = euclidean_distance_between_two_vectors(cl->c[j]->centroid->f, cl->c[f]->centroid->f);
			if( x < nearestCD  ){ nearestCD  = x; nearestC  = f; }
			if( x > furthestCD ){ furthestCD = x; furthestC = f; }
		}
		cl->c[j]->stats->nC = cl->c[nearestC];
		cl->c[j]->stats->fC = cl->c[furthestC];
		cl->c[j]->stats->nCD = nearestCD;
		cl->c[j]->stats->fCD = furthestCD;
	}
	free(n); /* 2012 AHP */
}
void	reset_clengine(clengine *cl){
	int	i;

	register point	**p;
	register cluster **c;

	for(i=0,c=&(cl->c[i]);i<cl->nC;i++,c++) reset_cluster(*c);
	for(i=0,p=&(cl->p[i]);i<cl->nP;i++,p++) reset_point(*p);
	reset_clestats(cl->stats);
}
void	clengine_initialise_clusters(clengine *cl, vector **clusters){
	/* **clusters is the value to set the clusters, if null, then clusters get random values */
	/* they are NOT normalised. They are normal pixel intensity values, normalisation will be done here */

	int	j, f;

	if( clusters == NULL ){
		for(j=0;j<cl->nC;j++)
			for(f=0;f<cl->nF;f++)
				cl->c[j]->centroid->f->c[f] = SCALE_OUTPUT(drand48(), cl->stats->min_point_values_normalised->c[f], cl->stats->max_point_values_normalised->c[f], 0.0, 1.0);
	} else {
		for(j=0;j<cl->nC;j++)
			for(f=0;f<cl->nF;f++)
				cl->c[j]->centroid->f->c[f] = SCALE_OUTPUT(clusters[j]->c[f], cl->stats->min_point_values_normalised->c[f], cl->stats->max_point_values_normalised->c[f], cl->stats->min_point_values->c[f], cl->stats->max_point_values->c[f]);
	}
}
void	clengine_save_clusters(clengine *cl, vector **toSave, int **ids){
	int	i;
	for(i=0;i<cl->nC;i++){
		cluster_save_clusters(cl->c[cl->c[i]->id], toSave[i]);
		(*ids)[i] = cl->c[i]->id;
	}
}
void	clengine_load_clusters(clengine *cl, vector **toLoad, int *ids){
	int	i;
	for(i=0;i<cl->nC;i++)
		cluster_load_clusters(cl->c[ids[i]], toLoad[i]);
}
void	clengine_sort_clusters_wrt_pixelvalue_ascending(clengine *cl){
	int	i;
	qsort((void **)(cl->c), (cl->nC), sizeof(cluster *), _cluster_comparator_pixelvalue_ascending);
	for(i=0;i<cl->nC;i++) cl->c[i]->centroid->s = i;
}
void	clengine_sort_clusters_wrt_pixelvalue_descending(clengine *cl){
	int	i;
	qsort((void **)(cl->c), (cl->nC), sizeof(cluster *), _cluster_comparator_pixelvalue_descending);
	for(i=0;i<cl->nC;i++) cl->c[i]->centroid->s = i;
}
void	clengine_sort_clusters_wrt_normalisedpixelvalue_ascending(clengine *cl){
	int	i;
	qsort((void **)(cl->c), (cl->nC), sizeof(cluster *), _cluster_comparator_pixelvalue_ascending);
	for(i=0;i<cl->nC;i++) cl->c[i]->centroid->s = i;
}
void	clengine_sort_clusters_wrt_normaliseddixelvalue_descending(clengine *cl){
	int	i;
	qsort((void **)(cl->c), (cl->nC), sizeof(cluster *), _cluster_comparator_pixelvalue_descending);
	for(i=0;i<cl->nC;i++) cl->c[i]->centroid->s = i;
}
int	clengine_do_simulated_annealing_once(clengine *cle, float cluster_noise_range, float beta_noise_range){
	int	i, f;

	for(i=0;i<cle->nC;i++){
		for(f=0;f<cle->nF;f++){
			cle->c[i]->centroid->f->c[f] +=
//				SCALE_OUTPUT(drand48(), -cluster_noise_range, +cluster_noise_range, cle->stats->min_point_values_normalised->c[f], cle->stats->max_point_values_normalised->c[f]);
				random_number_gaussian(0.0, cluster_noise_range);
			cle->c[i]->centroid->f->c[f] =
				MIN(cle->stats->max_point_values_normalised->c[f],
					MAX(cle->stats->min_point_values_normalised->c[f],
						cle->c[i]->centroid->f->c[f]
					)
				);
		}
		/* change and the locality term */
		/* beta can not be negative */
		if( beta_noise_range > 0.0 ) cle->c[i]->beta = MAX(0.0, cle->c[i]->beta + random_number_gaussian(0.0, beta_noise_range));
	}

	/* clusters have changed, calculated distances */
	calculate_clengine(cle);
	calculate_clengine_stats(cle);
	clengine_assign_points_to_clusters_without_relocations(cle);

	/* and recalculate the clusters */
	for(i=0;i<cle->nC;i++)
		calculate_cluster(cle->c[i]);
	calculate_clengine_stats(cle);

	return TRUE;
}

int	clengine_do_markov_chain_monte_carlo_once(clengine *cle, int *pAllocation){
	int	cid, i, pid, count,
		numPtoChange = lrand48() % (cle->nP)/10;
	cluster	*a_cluster;
	
	for(i=0;i<cle->nP;i++) pAllocation[i] = -1;

	for(i=0;i<numPtoChange;i++){
		/* select a point which has not been selected yet */
		count = 0;
		do {
			pid = lrand48() % cle->nP;
			if( count++ > 10000000 ){
				fprintf(stderr, "clengine_do_markov_chain_monte_carlo_once : could not picked a point to change after 10000000 trials.\n");
				return FALSE;
			}
			if( cle->interruptFlag ){
				fprintf(stderr, "clengine_do_markov_chain_monte_carlo_once : interrupted, returning (1) ...\n");
				return FALSE;
			}
		} while( pAllocation[pid] >= 0 );
		/* send this point to another cluster, not the current one */
		if( cle->p[pid]->c != NULL ){
			cid = cle->p[pid]->c->id;
			while( (pAllocation[pid]=(lrand48() % cle->nC)) == cid );
		} else pAllocation[pid] = lrand48() % cle->nC;

		if( cle->interruptFlag ){
			fprintf(stderr, "clengine_do_markov_chain_monte_carlo_once : interrupted, returning (2) ...\n");
			return FALSE;
		}
	}

	for(i=0;i<cle->nP;i++)
		if( pAllocation[i] < 0 ){
			/* if this point was not chosen above, then it does not change */
			if( cle->p[i]->c != NULL )
				pAllocation[i] = cle->p[i]->c->id;
			else pAllocation[i] = lrand48() % cle->nC; 
		}

	if( cle->interruptFlag ){
		fprintf(stderr, "clengine_do_markov_chain_monte_carlo_once : interrupted, returning (2) ...\n");
		return FALSE;
	}

	/* now do the changes */
	for(i=0;i<cle->nC;i++) cle->c[i]->n = 0;
	for(i=0;i<cle->nP;i++){
		a_cluster = cle->p[i]->c = cle->c[pAllocation[i]];
		a_cluster->points[a_cluster->n++] = i;
	}

	/* and recalculate the clusters */
	for(i=0;i<cle->nC;i++) calculate_cluster(cle->c[i]);

	return TRUE;
}			

static	int	_cluster_comparator_pixelvalue_ascending(const void *a, const void *b){
	float	_a = (*((cluster **)a))->centroid->v->c[0],
		_b = (*((cluster **)b))->centroid->v->c[0];

	if( _a > _b ) return 1;
	if( _a < _b ) return -1;
	return 0;
}
static	int	_cluster_comparator_pixelvalue_descending(const void *a, const void *b){
	float	_a = (*((cluster **)a))->centroid->v->c[0],
		_b = (*((cluster **)b))->centroid->v->c[0];

	if( _a < _b ) return 1;
	if( _a > _b ) return -1;
	return 0;
}
/*
static	int	_cluster_comparator_normalisedpixelvalue_ascending(const void *a, const void *b){
	float	_a = (*((cluster **)a))->centroid->f->c[0],
		_b = (*((cluster **)b))->centroid->f->c[0];

	if( _a > _b ) return 1;
	if( _a < _b ) return -1;
	return 0;
}
static	int	_cluster_comparator_normalisedpixelvalue_descending(const void *a, const void *b){
	float	_a = (*((cluster **)a))->centroid->f->c[0],
		_b = (*((cluster **)b))->centroid->f->c[0];

	if( _a < _b ) return 1;
	if( _a > _b ) return -1;
	return 0;
}

*/
