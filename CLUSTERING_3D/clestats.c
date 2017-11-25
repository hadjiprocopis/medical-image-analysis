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

clestats	*new_clestats(int numFeatures){
	clestats	*c;

	if( (c=(clestats *)malloc(sizeof(clestats))) == NULL ){
		fprintf(stderr, "new_clestats : could not allocate %zd bytes (1).\n", sizeof(clestats));
		return NULL;
	}
	if( (c->mean_fitness=new_vector(numFeatures)) == NULL ){
		fprintf(stderr, "new_clestats : call to new_vector has failed for 'mean_fitness', %d features.\n", numFeatures);
		free(c);
		return NULL;
	}
	if( (c->stdev_fitness=new_vector(numFeatures)) == NULL ){
		fprintf(stderr, "new_clestats : call to new_vector has failed for 'stdev_fitness', %d features.\n", numFeatures);
		destroy_vector(c->mean_fitness); free(c);
		return NULL;
	}
	if( (c->min_point_values=new_vector(numFeatures)) == NULL ){
		fprintf(stderr, "new_clestats : call to new_vector has failed for 'min_point_values', %d features.\n", numFeatures);
		destroy_vector(c->mean_fitness); destroy_vector(c->stdev_fitness); free(c);
		return NULL;
	}
	if( (c->max_point_values=new_vector(numFeatures)) == NULL ){
		fprintf(stderr, "new_clestats : call to new_vector has failed for 'max_point_values', %d features.\n", numFeatures);
		destroy_vector(c->min_point_values); destroy_vector(c->mean_fitness); destroy_vector(c->stdev_fitness); free(c);
		return NULL;
	}

	if( (c->min_point_values_normalised=new_vector(numFeatures)) == NULL ){
		fprintf(stderr, "new_clestats : call to new_vector has failed for 'min_point_values_normalised', %d features.\n", numFeatures);
		destroy_vector(c->max_point_values); destroy_vector(c->min_point_values); destroy_vector(c->mean_fitness); destroy_vector(c->stdev_fitness); free(c);
		return NULL;
	}
	if( (c->max_point_values_normalised=new_vector(numFeatures)) == NULL ){
		fprintf(stderr, "new_clestats : call to new_vector has failed for 'max_point_values_normalised', %d features.\n", numFeatures);
		destroy_vector(c->min_point_values_normalised); destroy_vector(c->max_point_values); destroy_vector(c->min_point_values); destroy_vector(c->mean_fitness); destroy_vector(c->stdev_fitness); free(c);
		return NULL;
	}
	if( (c->tantheta=new_vector(numFeatures)) == NULL ){
		fprintf(stderr, "new_clestats : call to new_vector has failed for 'tantheta', %d features.\n", numFeatures);
		destroy_vector(c->min_point_values_normalised); destroy_vector(c->max_point_values); destroy_vector(c->min_point_values); destroy_vector(c->mean_fitness); destroy_vector(c->stdev_fitness); free(c);
		return NULL;
	}

	reset_clestats(c);
	return c;
}
void	destroy_clestats(clestats *c){
	destroy_vector(c->mean_fitness);
	destroy_vector(c->stdev_fitness);
	destroy_vector(c->min_point_values);
	destroy_vector(c->max_point_values);
	destroy_vector(c->min_point_values_normalised);
	destroy_vector(c->max_point_values_normalised);
	destroy_vector(c->tantheta);
	free(c);
}
void	reset_clestats(clestats *c){
	reset_vector(c->mean_fitness);
	reset_vector(c->stdev_fitness);
	c->separation_area = c->separation_distance = c->penalty = c->total_distance =
	c->mean_homogeneity = c->mean_density = c->mean_compactness = -1.0;
	c->stdev_homogeneity = c->stdev_density = c->stdev_compactness = -1.0;
}
void	copy_clestats(clestats *c1, clestats *c2){
	c2->penalty = c1->penalty;
	c2->separation_area = c1->separation_area;
	c2->separation_distance = c1->separation_distance;

	c2->mean_homogeneity = c1->mean_homogeneity;
	c2->mean_density = c1->mean_density;
	c2->mean_compactness = c1->mean_compactness;

	c2->stdev_homogeneity = c1->stdev_homogeneity;
	c2->stdev_density = c1->stdev_density;
	c2->stdev_compactness = c1->stdev_compactness;

	c2->total_distance = c1->total_distance;

	copy_vector(c1->mean_fitness, c2->mean_fitness);
	copy_vector(c1->stdev_fitness, c2->stdev_fitness);
	copy_vector(c1->min_point_values, c2->min_point_values);
	copy_vector(c1->max_point_values, c2->max_point_values);
	copy_vector(c1->min_point_values_normalised, c2->min_point_values_normalised);
	copy_vector(c1->max_point_values_normalised, c2->max_point_values_normalised);
	copy_vector(c1->tantheta, c2->tantheta);
}	
clestats	*duplicate_clestats(clestats *a_clestats){
	clestats	*dup;

	if( (dup=new_clestats(a_clestats->mean_fitness->nC)) == NULL ){
		fprintf(stderr, "duplicate_clestats : call to new_clestats has failed.\n");
		return NULL;
	}
	copy_clestats(a_clestats, dup);
	return dup;
}
char	*toString_clestats(clestats *a_clestats){
	char	*s, *ret;

	if( (s=(char *)malloc(1000)) == NULL ){
		fprintf(stderr, "toString_clestats : could not allocate 1000 bytes.\n");
		return "<empty clestats>";
	}
	sprintf(s, "homogeneity=(%f,%f), penalty=(%f), separation_A=(%f), separation_D=(%f), compactness=(%f,%f), entropy=(%f), distance=(%f)",
		a_clestats->mean_homogeneity, a_clestats->stdev_homogeneity,
		a_clestats->penalty,
		a_clestats->separation_area, a_clestats->separation_distance,
		a_clestats->mean_compactness, a_clestats->stdev_compactness,
		a_clestats->total_entropy, a_clestats->total_distance);

	ret = strdup(s);
	free(s);
	return ret;
}
void	print_clestats(FILE *handle, clestats *a_clestats){
	char	*s = toString_clestats(a_clestats);

	fprintf(handle, "%s\n", s);
	free(s);
}
