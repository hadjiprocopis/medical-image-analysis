#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>
#include <LinkedList.h>

#include "clustering.h"

cstats	*new_cstats(int numFeatures, int numPoints, int numClusters){
	cstats	*c;

	if( (c=(cstats *)malloc(sizeof(cstats))) == NULL ){
		fprintf(stderr, "new_cstats : could not allocate %zd bytes (1).\n", sizeof(cstats));
		return NULL;
	}
	if( (c->min_points=new_point(0, numFeatures, numClusters)) == NULL ){
		fprintf(stderr, "new_cstats : call to new_point has failed for c->min_points.\n");
		free(c);
		return NULL;
	}
	if( (c->max_points=new_point(0, numFeatures, numClusters)) == NULL ){
		fprintf(stderr, "new_cstats : call to new_point has failed for c->max_points.\n");
		destroy_point(c->min_points); free(c);
		return NULL;
	}
	if( (c->mean_points=new_point(0, numFeatures, numClusters)) == NULL ){
		fprintf(stderr, "new_cstats : call to new_point has failed for c->mean_points.\n");
		destroy_point(c->max_points); destroy_point(c->min_points); free(c);
		return NULL;
	}
	if( (c->stdev_points=new_point(0, numFeatures, numClusters)) == NULL ){
		fprintf(stderr, "new_cstats : call to new_point has failed for c->stdev_points.\n");
		destroy_point(c->mean_points); destroy_point(c->max_points); destroy_point(c->min_points); free(c);
		return NULL;
	}
	if( (c->fitness=new_vector(numFeatures)) == NULL ){
		fprintf(stderr, "new_cstats : call to new_vector has failed for c->fitness.\n");
		destroy_point(c->stdev_points); destroy_point(c->mean_points); destroy_point(c->max_points); destroy_point(c->min_points); free(c);
		return NULL;
	}

	c->numFeatures = numFeatures;
	c->numPoints   = numPoints;
	c->numClusters = numClusters;

	reset_cstats(c);

	return c;
}
void	destroy_cstats(cstats *c){
	destroy_point(c->stdev_points);
	destroy_point(c->mean_points);
	destroy_point(c->max_points);
	destroy_point(c->min_points);
	destroy_vector(c->fitness);
	free(c);
}
void	copy_cstats(cstats *c1, cstats *c2){
	copy_cstats(c1, c2);
	copy_point(c1->stdev_points, c2->stdev_points);
	copy_point(c1->mean_points, c2->mean_points);
	copy_point(c1->max_points, c2->max_points);
	copy_point(c1->min_points, c2->min_points);

	c2->homogeneity = c1->homogeneity;
	c2->density = c1->density;
	c2->penalty = c1->penalty;
	c2->compactness = c1->compactness;

	copy_vector(c1->fitness, c2->fitness);

	/* you have to do it manually later */
	c2->nP = c2->fP = NULL; c2->nC = c2->fC = NULL;
}	
cstats	*duplicate_cstats(cstats *a_cstats){
	cstats		*dup;

	if( (dup=new_cstats(a_cstats->numFeatures, a_cstats->numPoints, a_cstats->numClusters)) == NULL ){
		fprintf(stderr, "duplicate_cstats : call to new_cstats has failed.\n");
		return NULL;
	}
	return dup;
}
char	*toString_cstats(cstats *a_cstats){
	char	*s, *ret,
		*s1 = toString_point(a_cstats->min_points),
		*s2 = toString_point(a_cstats->max_points),
		*s3 = toString_point(a_cstats->mean_points),
		*s4 = toString_point(a_cstats->stdev_points),
		*s5 = a_cstats->nP == NULL ? strdup("<empty>") : toString_point_brief((void *)(a_cstats->nP)),
		*s6 = a_cstats->fP == NULL ? strdup("<empty>") : toString_point_brief((void *)(a_cstats->fP));

	if( (s=(char *)malloc(strlen(s1)+strlen(s2)+strlen(s3)+strlen(s4)+strlen(s5)+strlen(s6)+1000)) == NULL ){
		fprintf(stderr, "toString_cstats : could not allocate %zd bytes.\n", strlen(s1)+strlen(s2)+strlen(s3)+strlen(s4)+strlen(s5)+strlen(s6)+1000);
		return "<empty cstats>";
	}
	sprintf(s, "of %d features, fitness[0]=%f, homogeneity=%f, penalty=%f, compactness=%f, min_points:%s, max_points:%s, mean_points:%s, stdev_points:%s, nearest %s, farthest %s, nearest %d, farthest %d",
		a_cstats->numFeatures,
		a_cstats->fitness->c[0], a_cstats->homogeneity, a_cstats->penalty, a_cstats->compactness,
		s1, s2, s3, s4, s5, s6,
		a_cstats->nC==NULL?-1:a_cstats->nC->id,
		a_cstats->fC==NULL?-1:a_cstats->fC->id );

	free(s1); free(s2); free(s3); free(s4); free(s5); free(s6);
	ret = strdup(s);
	free(s);
	return ret;
}
char	*toString_cstats_brief(cstats *a_cstats){
	char	*s, *ret;

	if( (s=(char *)malloc(1000)) == NULL ){
		fprintf(stderr, "toString_cstats : could not allocate %d bytes.\n", 1000);
		return "<empty cstats>";
	}
	sprintf(s, "of %d features, fitness[0]=%f, homogeneity=%f, penalty=%f, compactness=%f",
		a_cstats->numFeatures,
		a_cstats->fitness->c[0], a_cstats->homogeneity, a_cstats->penalty, a_cstats->compactness);
	ret = strdup(s);
	free(s);
	return ret;
}
void	print_cstats(FILE *handle, cstats *a_cstats){
	char	*s = toString_cstats(a_cstats);

	fprintf(handle, "%s\n", s);
	free(s);
}
void	reset_cstats(cstats *a_cstats){
	a_cstats->nP = a_cstats->fP = NULL;
	a_cstats->nC = a_cstats->fC = NULL;

	a_cstats->homogeneity =
	a_cstats->density =
	a_cstats->penalty =
	a_cstats->compactness = -1.0;

	reset_point(a_cstats->min_points);
	reset_point(a_cstats->max_points);
	reset_point(a_cstats->mean_points);
	reset_point(a_cstats->stdev_points);

	reset_vector(a_cstats->fitness);
}
