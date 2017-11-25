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

cluster	*new_cluster(int id, int numFeatures, int totalNumPoints, int totalNumClusters){
	cluster	*c;

	if( (c=(cluster *)malloc(sizeof(cluster))) == NULL ){
		fprintf(stderr, "_new_cluster : could not allocate %zd bytes for 'c'.\n", sizeof(cluster));
		return NULL;
	}
	if( (c->points=(int *)malloc(totalNumPoints * sizeof(int))) == NULL ){
		fprintf(stderr, "_new_cluster : could not allocate %zd bytes for 'points'.\n", totalNumPoints * sizeof(int));
		free(c);
		return NULL;
	}
	if( (c->centroid=new_point(id, numFeatures, totalNumClusters)) == NULL ){
		fprintf(stderr, "_new_cluster : call to new_point has failed for %d features and %d clusters\n", numFeatures, totalNumClusters);
		free(c->points); free(c);
		return NULL;
	}
	if( (c->stats=new_cstats(numFeatures, totalNumPoints, totalNumClusters)) == NULL ){
		fprintf(stderr, "_new_cluster : call to new_cstats has failed for cluster id = %d.\n", c->id);
		destroy_point(c->centroid); free(c->points); free(c);
		return NULL;
	}
	c->centroid->s = c->id = id; /* c->centroid->s points to the slice number this cluster should be written to, useful when clusters are sorted */
	c->all_points = NULL; /* later */
	c->cl = (clengine *)NULL;
	c->beta = 3.0;
	reset_cluster(c);
	return c;
}	
cluster	*duplicate_cluster(cluster *a_cluster, int new_id){
	cluster	*dup;
				   /* numFeatures */           /* max num points*/
	if( (dup=new_cluster(new_id, a_cluster->stats->numFeatures, a_cluster->stats->numPoints, a_cluster->stats->numClusters)) == NULL ){
		fprintf(stderr, "duplicate_cluster : call to new_cluster (%d) has failed for %d features, %d total num points and %d clusters.\n", new_id, a_cluster->stats->numFeatures, a_cluster->stats->numPoints, a_cluster->stats->numClusters);
		return NULL;
	}
	copy_cluster(a_cluster, dup);
	return dup;
}
void	copy_cluster(cluster *c1, cluster *c2){
	/* copy contents of c1 onto c2 */
	int	i;
	register int	*p1, *p2;

	copy_point(c1->centroid, c2->centroid);
	copy_cstats(c1->stats, c2->stats);
	for(i=0,p1=&(c1->points[i]),p2=&(c2->points[i]);i<c1->n;i++,p1++,p2++) *p2 = *p1;
	c2->n = c1->n;
	c2->cl = c1->cl;
	c2->beta = c1->beta;
	c2->all_points = c1->all_points;
	c2->cl = c1->cl; /* the same clengine parent??*/
}	
void	destroy_cluster(cluster *c){
	destroy_point(c->centroid);
	destroy_cstats(c->stats);
	free(c->points);
	free(c);
}
void	reset_cluster(cluster *c){
	reset_point(c->centroid);
	reset_cstats(c->stats);
	c->n = 0;
	c->centroid->s = c->id;
}
void	destroy_clusters(cluster **c, int numClusters){
	int	i;
	for(i=0;i<numClusters;i++) destroy_cluster(c[i]);
	free(c);
}
char	*toString_cluster(void *p){
	cluster	*a_cluster = (cluster *)p;
	char	*s, *ret,
		*sC = toString_point_brief(a_cluster->centroid),
		*sS = toString_cstats(a_cluster->stats);
	
	if( (s=(char *)malloc(strlen(sC)+strlen(sS)+250)) == NULL ){
		fprintf(stderr, "toString_cluster : could not allocate %zd bytes.\n", strlen(sC)+strlen(sS)+250);
		return "<empty cluster>";
	}
	sprintf(s, "cluster <%d> with centroid at %s and stats %s, has %d points",
			a_cluster->id,
			sC, sS, a_cluster->n);

	free(sC); free(sS);
	ret = strdup(s);
	free(s);
	return ret;
}
char	*toString_cluster_brief(void *p){
	cluster	*a_cluster = (cluster *)p;
	char	*s, *ret,
		*sC = toString_point_brief(a_cluster->centroid),
		*sS = toString_cstats_brief(a_cluster->stats);
	
	if( (s=(char *)malloc(strlen(sC)+strlen(sS)+250)) == NULL ){
		fprintf(stderr, "toString_cluster : could not allocate %zd bytes.\n", strlen(sC)+strlen(sS)+250);
		return "<empty cluster>";
	}
	sprintf(s, "cluster %d with centroid at %s and stats %s, has %d points",
			a_cluster->id,
			sC, sS, a_cluster->n);

	free(sC); free(sS);
	ret = strdup(s);
	free(s);
	return ret;
}
void	print_cluster(FILE *handle, cluster *a_cluster){
	char	*s = toString_cluster((void *)a_cluster);
	fprintf(handle, "%s\n", s);
	free(s);
}
void	print_cluster_brief(FILE *handle, cluster *a_cluster){
	char	*s = toString_cluster_brief((void *)a_cluster);
	fprintf(handle, "%s\n", s);
	free(s);
}
/* assuming points have been allocated and the centroid of this cluster calculated, find mean, max etc. */
void	calculate_cluster_stats(cluster *c){
	register int	i, f, nF, nearestP, furthestP, id = c->id;
	register float	nearestPD, furthestPD;
	register int	*pI;

	reset_cstats(c->stats);
	if( c->n <= 0 ) return;

	statistics_point_from_indices(c->points, c->n, c->all_points, c->stats->min_points, c->stats->max_points, c->stats->mean_points, c->stats->stdev_points);

	nF = c->all_points[0]->v->nC;

	c->stats->compactness = c->stats->homogeneity = 0.0;
	for(i=0,pI=&(c->points[i]);i<c->n;i++,pI++)
		c->stats->compactness += c->all_points[*pI]->d->c[id];

	c->stats->compactness /= c->n;

	for(i=0,pI=&(c->points[i]);i<c->n;i++,pI++){
		c->stats->homogeneity += SQR(c->stats->compactness - c->all_points[*pI]->d->c[id]);
		for(f=0;f<nF;f++) c->stats->fitness->c[f] = SQR(c->stats->mean_points->v->c[f] - c->all_points[*pI]->v->c[f]);
	}
	for(f=0;f<nF;f++) c->stats->fitness->c[f] = sqrt(c->stats->fitness->c[f] / nF);

	/* calculate nearest and furthest points */
	nearestPD = furthestPD = c->all_points[c->points[0]]->d->c[id];
	nearestP = furthestP = 0;
	for(i=1,pI=&(c->points[i]);i<c->n;i++,pI++){
		if( c->all_points[*pI]->d->c[id] < nearestPD ){ nearestPD = c->all_points[*pI]->d->c[id]; nearestP = i; }
		if( c->all_points[*pI]->d->c[id] > furthestPD ){ furthestPD = c->all_points[*pI]->d->c[id]; furthestP = i; }
	}
	c->stats->nP = c->all_points[c->points[nearestP]];
	c->stats->fP = c->all_points[c->points[furthestP]];
	c->stats->nPD = nearestPD; c->stats->fPD = furthestPD;

	/* nearest and furthest clusters will be calculated from clengine's calculate function */
	c->stats->nC = c->stats->fC = (cluster *)NULL;
}

/* do all the calculations for a cluster, find its centroid etc. */
/* assuming that points have been allocated to this cluster before */
void	calculate_cluster(cluster *c){
	register int		i;
	register float		x;

	register int		*pI, id;
	register point		*p;

	id = c->id;

	/* centroid, in feature space, pixel value space and image space */
	reset_vector(c->centroid->v);
	reset_vector(c->centroid->f);
	reset_vector(c->centroid->p);
	reset_vector(c->centroid->d);
	reset_vector(c->centroid->td);
	c->centroid->x = c->centroid->y = c->centroid->z =
	c->centroid->sD = c->centroid->sTD = c->centroid->entropy = 0.0;

	if( c->n <= 0 ) return; /* no points in this cluster */

	for(i=0,pI=&(c->points[i]);i<c->n;i++,pI++){
		p = c->all_points[*pI];
/*		add_vectors_multiply_constant(c->centroid->v, p->v, p->p->c[id]);
		add_vectors_multiply_constant(c->centroid->f, p->f, p->p->c[id]);
		add_vectors_multiply_constant(c->centroid->p, p->p, p->p->c[id]);
		add_vectors_multiply_constant(c->centroid->d, p->d, p->p->c[id]);
		add_vectors_multiply_constant(c->centroid->td, p->td, p->p->c[id]);
*/
		add_vectors(c->centroid->v, p->v);
		add_vectors(c->centroid->f, p->f);
		add_vectors(c->centroid->p, p->p);
		add_vectors(c->centroid->d, p->d);
		add_vectors(c->centroid->td, p->td);
		c->centroid->x += p->x;
		c->centroid->y += p->y;
		c->centroid->z += p->z;
		c->centroid->entropy += p->entropy;
		c->centroid->sD += p->sD;
		c->centroid->sTD += p->sTD;
	}
	x = 1.0 / ((float )(c->n));
	multiply_vector_by_constant(c->centroid->v, x);
	multiply_vector_by_constant(c->centroid->f, x);
	multiply_vector_by_constant(c->centroid->p, x);
	multiply_vector_by_constant(c->centroid->d, x);
	multiply_vector_by_constant(c->centroid->td, x);
	c->centroid->x /= c->n; c->centroid->y /= c->n; c->centroid->z /= c->n;
	c->centroid->entropy /= c->n;
	c->centroid->sD /= c->n; c->centroid->sTD /= c->n;

	/* check for negative centroid? sometimes it makes sense */
/*
	for(i=0;i<c->centroid->f->nC;i++){
		if( c->centroid->f->c[i] < 0 ){
			printf("\n\n*****************\nWarning negative centroid\n*******\n");
			print_point(stdout, c->centroid);
			exit(1);
		}
	}
*/
}
/* add some noise (white noise with given percent of deviation (p) on each side ) to its centroid (p:0-1) */
void	shake_cluster(cluster *c, float p){
	int	i;

	register float	*v, dummy;
	for(i=0,v=&(c->centroid->f->c[i]);i<c->centroid->f->nC;i++,v++){
		dummy = p * (*v);
		*v += SCALE_OUTPUT(drand48(), -dummy, dummy, 0.0, 1.0);
	}
}
float	area_of_triangle_defined_by_centroids_of_three_clusters(vector *c1, vector *c2, vector *c3){
	float	d12, d3L12;

	/* first we will calculate the distance between c1 and c2 */
	d12 = euclidean_distance_between_two_vectors(c1, c2);

	/* then calculate the distance of c3 from the line joining c1 and c2 */
	d3L12 = euclidean_distance_of_vector_from_line(c3, c1, c2);

	/* the area of the triangle is then height * base / 2 */
	return d12 * d3L12 / 2.0;
}
void	cluster_save_clusters(cluster *c, vector *toSave){
	int	i;
	for(i=0;i<c->centroid->f->nC;i++)
		toSave->c[i] = c->centroid->f->c[i];
}
void	cluster_load_clusters(cluster *c, vector *toLoad){
	int		i;
	clestats	*st = c->cl->stats;
	for(i=0;i<c->centroid->f->nC;i++){
		c->centroid->f->c[i] = toLoad->c[i]; /* these are normalised */
		c->centroid->v->c[i] = SCALE_OUTPUT(
			c->centroid->f->c[i],
			st->min_point_values->c[i],
			st->max_point_values->c[i],
			st->min_point_values_normalised->c[i],
			st->max_point_values_normalised->c[i]);
	}
}

