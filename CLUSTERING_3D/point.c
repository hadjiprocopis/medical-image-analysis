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

/* leave parent null if this point does not belong to anybody */
point	*new_point(int id, int numFeatures, int numClusters){
	int	i;
	point	*p;

	if( (p=(point *)malloc(sizeof(point))) == NULL ){
		fprintf(stderr, "new_point : could not allocate %zd bytes.\n", sizeof(point));
		return NULL;
	}
	if( (p->f=new_vector(numFeatures)) == NULL ){
		fprintf(stderr, "new_point : call to new_vector has failed for %d features of 'f'.\n", numFeatures);
		free(p);
		return NULL;
	}
	if( (p->v=new_vector(numFeatures)) == NULL ){
		fprintf(stderr, "new_point : call to new_vector has failed for %d features of 'v'.\n", numFeatures);
		destroy_vector(p->f); free(p);
		return NULL;
	}
	if( (p->p=new_vector(numClusters)) == NULL ){
		fprintf(stderr, "new_point : call to new_vector has failed for %d clusters of 'p'.\n", numClusters);
		destroy_vector(p->v); destroy_vector(p->f); free(p);
		return NULL;
	}
	if( (p->d=new_vector(numClusters)) == NULL ){
		fprintf(stderr, "new_point : call to new_vector has failed for %d clusters of 'd'.\n", numClusters);
		destroy_vector(p->p); destroy_vector(p->v); destroy_vector(p->f); free(p);
		return NULL;
	}
	if( (p->td=new_vector(numClusters)) == NULL ){
		fprintf(stderr, "new_point : call to new_vector has failed for %d clusters of 'td'.\n", numClusters);
		destroy_vector(p->d); destroy_vector(p->p); destroy_vector(p->v); destroy_vector(p->f); free(p);
		return NULL;
	}
	if( (p->w=new_vector(numFeatures)) == NULL ){
		fprintf(stderr, "new_point : call to new_vector has failed for %d features of 'w'.\n", numFeatures);
		destroy_vector(p->td); destroy_vector(p->d); destroy_vector(p->p); destroy_vector(p->v); destroy_vector(p->f); free(p);
		return NULL;
	}
	if( (p->spm=new_vector(3)) == NULL ){
		fprintf(stderr, "new_point : call to new_vector has failed for 3 features of 'spm'.\n");
		destroy_vector(p->w); destroy_vector(p->td); destroy_vector(p->d); destroy_vector(p->p); destroy_vector(p->v); destroy_vector(p->f); free(p);
		return NULL;
	}
	if( (p->beta=new_vector(numClusters)) == NULL ){
		fprintf(stderr, "new_point : call to new_vector has failed for %d features of 'beta'.\n", numClusters);
		destroy_vector(p->spm); destroy_vector(p->w); destroy_vector(p->td); destroy_vector(p->d); destroy_vector(p->p); destroy_vector(p->v); destroy_vector(p->f); free(p);
		return NULL;
	}

	/* initialise weights and threshold to neutral */
	for(i=0;i<numFeatures;i++) p->w->c[i] = 1.0;
	p->t = 0.0;
	/* initialise spm prob maps (default is 1.0) */
	p->spm->c[0] = p->spm->c[1] = p->spm->c[2] = 1.0;	

	p->data = (void *)NULL;
	for(i=0;i<8;i++) p->n[i] = (point *)NULL;

	p->id = id;

	reset_point(p);

	return p;
}
void	destroy_point(point *p){
	destroy_vector(p->td);
	destroy_vector(p->d);
	destroy_vector(p->p);
	destroy_vector(p->v);
	destroy_vector(p->f);
	destroy_vector(p->w);
	destroy_vector(p->spm);
	destroy_vector(p->beta);
	free(p);
}
void	destroy_points(point **p, int numPoints){
	int	i;
	register point	**pp;
	for(i=0,pp=&(p[i]);i<numPoints;i++,pp++) destroy_point(*pp);
	free(p);
}
void	copy_point(point *p1, point *p2){
	/* copies contents of p1 onto p2 */
	int	i;

	copy_vector(p1->beta, p2->beta);
	copy_vector(p1->w, p2->w);
	copy_vector(p1->td, p2->td);
	copy_vector(p1->d, p2->d);
	copy_vector(p1->p, p2->p);
	copy_vector(p1->v, p2->v);
	copy_vector(p1->f, p2->f);
	copy_vector(p1->spm, p2->spm);

	p2->x = p1->x; p2->y = p1->y; p2->z = p1->z;
	p2->s = p1->s;
	p2->sD = p1->sD; p2->sTD = p1->sTD;

	for(i=0;i<8;i++) p2->n[i] = p1->n[i];

	/* might need to change these */
	p2->c = p1->c;
	p2->id = p1->id;
	p2->t = p1->t;
}
point	*duplicate_point(point *a_point){
	point	*dup;

			   /*numFeatures*/ /*numClusters*/
	if( (dup=new_point(a_point->id, a_point->f->nC, a_point->d->nC)) == NULL ){
		fprintf(stderr, "duplicate_point : call to new_point has failed.\n");
		return NULL;
	}
	copy_point(a_point, dup);
	return dup;
}

char	*toString_point(void *p){
	point	*a_point = (point *)p;
	int	l = 0, i = 0;
	char	*s, *ret,
		*sF[7];

	sF[i] = toString_vector(a_point->v); l += strlen(sF[i]); i++;
	sF[i] = toString_vector(a_point->f); l += strlen(sF[i]); i++;
	sF[i] = toString_vector(a_point->p); l += strlen(sF[i]); i++;
	sF[i] = toString_vector(a_point->d); l += strlen(sF[i]); i++;
	sF[i] = toString_vector(a_point->td); l += strlen(sF[i]); i++;
	sF[i] = toString_vector(a_point->w); l += strlen(sF[i]); i++;
	sF[i] = toString_vector(a_point->spm); l += strlen(sF[i]); i++;
	if( (s=(char *)malloc(l+1000)) == NULL ){
		fprintf(stderr, "toString_point : could not allocate %d bytes.\n", l+250);
		return "<empty point>";
	}
	sprintf(s, "point at (%.0f,%.0f,%.0f|%d) [%s]/[%s] nearest cluster %d:\nd  = [%s]\ntd = [%s]\np  = [%s]\nw  = [%s], t=%f\nSPM prob maps: %s\nsD = %f, sTD = %f\n",
		a_point->x, a_point->y, a_point->z, a_point->s,
		sF[0], sF[1],
		(a_point->c==NULL?-1:a_point->c->id),
		sF[3], sF[4], sF[2], sF[5], a_point->t,
		sF[6],
		a_point->sD, a_point->sTD );

	free(sF[0]); free(sF[1]); free(sF[2]); free(sF[3]); free(sF[4]); free(sF[5]); free(sF[6]);
	ret = strdup(s);
	free(s);
	return ret;
}

char	*toString_point_brief(void *p){
	point	*a_point = (point *)p;
	char	*s, *ret, *sF[5];
	int	l = 0;
	
	sF[0] = toString_vector(a_point->v); l += strlen(sF[0]);
	sF[1] = toString_vector(a_point->f); l += strlen(sF[1]);
	sF[2] = toString_vector(a_point->p); l += strlen(sF[2]);
	sF[3] = toString_vector(a_point->d); l += strlen(sF[3]);
	sF[4] = toString_vector(a_point->spm); l += strlen(sF[4]);

	if( (s=(char *)malloc(l+250)) == NULL ){
		fprintf(stderr, "toString_point : could not allocate %d bytes.\n", l+250);
		return "<empty point>";
	}
	sprintf(s, "point %d at (%.0f,%.0f,%.0f|%d) [%s]/[%s] d=[%s], p=[%s] -> %d, spm=%s",
		a_point->id,
		a_point->x, a_point->y, a_point->z, a_point->s,
		sF[0], sF[1],
		sF[3], sF[2],
		a_point->c==NULL ? -1 : a_point->c->id, sF[4]);
	free(sF[0]); free(sF[1]); free(sF[2]); free(sF[3]); free(sF[4]);
	ret = strdup(s);
	free(s);
	return ret;
}

void	print_point(FILE *handle, point *a_point){
	char	*s = toString_point((void *)a_point);
	fprintf(handle, "%s\n", s);
	free(s);
}
void	print_point_brief(FILE *handle, point *a_point){
	char	*s = toString_point_brief((void *)a_point);
	fprintf(handle, "%s\n", s);
	free(s);
}

float	euclidean_distance_between_two_points(point *p1, point *p2){
	return euclidean_distance_between_two_vectors(p1->f, p2->f);
}
int	statistics_of_points(point **data, int numPoints, vector *min, vector *max, vector *mean, vector *stdev){
	vector	**all_vectors_in_data;
	int	i;

	register point	**p;
	register vector	**v;

	if( (all_vectors_in_data=(vector **)malloc(numPoints * sizeof(vector *))) == NULL ){
		fprintf(stderr, "statistics_of_points : could not allocate %zd bytes.\n", numPoints * sizeof(vector *));
		return FALSE;
	}
	for(i=0,v=&(all_vectors_in_data[i]),p=&(data[i]);i<numPoints;i++,p++,v++) *v = (*p)->f;

	statistics_of_vectors(&(all_vectors_in_data[0]), numPoints, min, max, mean, stdev);
	free(all_vectors_in_data);
	return TRUE;
}
void	reset_point(point *p){
	reset_vector(p->w);
	reset_vector(p->td);
	reset_vector(p->d);
//	for(i=0;i<p->p->nC;i++) p->p->c[i] = 1.0 / p->p->nC; /* equal probability to each cluster */
	reset_vector(p->p);
//	reset_vector(p->v);
//	reset_vector(p->f);

	p->c = (cluster *)NULL;

//	p->x = p->y = p->z = 0.0; p->s = 0;
	p->sD = p->sTD = 0.0;
}
/* it will get min/max/mean/stdev from a list of numPoints points */
void	statistics_point(point **p, int numPoints, point *minP, point *maxP, point *meanP, point *stdevP){
	register int i, f, nF = p[0]->v->nC, nC = p[0]->d->nC;
	register point	**pP;

	/* be careful, w, v and f are of size nF, the rest (d, td etc) are of size nC */

	copy_point(p[0], minP);
	copy_point(p[0], maxP);
	copy_point(p[0], meanP);
	copy_point(p[0], stdevP);

	for(f=0;f<nF;f++)
		meanP->v->c[f]   =
		meanP->f->c[f]   =
		meanP->w->c[f]   =
		stdevP->v->c[f]  =
		stdevP->f->c[f]  =
		stdevP->w->c[f]  = 0.0;

	for(f=0;f<nC;f++)
		meanP->p->c[f]   =
		meanP->d->c[f]   =
		meanP->td->c[f]  =
		stdevP->p->c[f]  =
		stdevP->d->c[f]  =
		stdevP->td->c[f] = 0.0;

	for(i=0,pP=&(p[i]);i<numPoints;i++,pP++){
		for(f=0;f<nF;f++){
			if( (*pP)->v->c[f] < minP->v->c[f] ) minP->v->c[f] = (*pP)->v->c[f];
			if( (*pP)->v->c[f] > maxP->v->c[f] ) maxP->v->c[f] = (*pP)->v->c[f];
			if( (*pP)->f->c[f] < minP->f->c[f] ) minP->f->c[f] = (*pP)->f->c[f];
			if( (*pP)->f->c[f] > maxP->f->c[f] ) maxP->f->c[f] = (*pP)->f->c[f];
			if( (*pP)->w->c[f] < minP->w->c[f] ) minP->w->c[f] = (*pP)->w->c[f];
			if( (*pP)->w->c[f] > maxP->w->c[f] ) maxP->w->c[f] = (*pP)->w->c[f];

			meanP->v->c[f] += (*pP)->v->c[f];
			meanP->f->c[f] += (*pP)->f->c[f];
			meanP->w->c[f] += (*pP)->w->c[f];
		}
		for(f=0;f<nC;f++){
			if( (*pP)->p->c[f] < minP->p->c[f] ) minP->p->c[f] = (*pP)->p->c[f];
			if( (*pP)->p->c[f] > maxP->p->c[f] ) maxP->p->c[f] = (*pP)->p->c[f];
			if( (*pP)->d->c[f] < minP->d->c[f] ) minP->d->c[f] = (*pP)->d->c[f];
			if( (*pP)->d->c[f] > maxP->d->c[f] ) maxP->d->c[f] = (*pP)->d->c[f];
			if( (*pP)->td->c[f] < minP->td->c[f] ) minP->td->c[f] = (*pP)->td->c[f];
			if( (*pP)->td->c[f] > maxP->td->c[f] ) maxP->td->c[f] = (*pP)->td->c[f];

			meanP->td->c[f] += (*pP)->td->c[f];
			meanP->p->c[f] += (*pP)->p->c[f];
			meanP->d->c[f] += (*pP)->d->c[f];
		}
	}
	for(f=0;f<nF;f++){
		meanP->v->c[f]  /= numPoints;
		meanP->f->c[f]  /= numPoints;
		meanP->w->c[f]  /= numPoints;
	}
	for(f=0;f<nC;f++){
		meanP->d->c[f]  /= numPoints;
		meanP->td->c[f] /= numPoints;
		meanP->p->c[f]  /= numPoints;
	}
	for(i=0,pP=&(p[i]);i<numPoints;i++,pP++){
		for(f=0;f<nF;f++){
			stdevP->v->c[f] += SQR((*pP)->v->c[f] - meanP->v->c[f]);
			stdevP->f->c[f] += SQR((*pP)->f->c[f] - meanP->f->c[f]);
			stdevP->w->c[f] += SQR((*pP)->w->c[f] - meanP->w->c[f]);
		}
		for(f=0;f<nC;f++){
			stdevP->d->c[f] += SQR((*pP)->d->c[f] - meanP->d->c[f]);
			stdevP->td->c[f] += SQR((*pP)->td->c[f] - meanP->td->c[f]);
			stdevP->p->c[f] += SQR((*pP)->p->c[f] - meanP->p->c[f]);
		}
	}
	for(f=0;f<nF;f++){
		stdevP->v->c[f]  = sqrt(stdevP->v->c[f] / numPoints);
		stdevP->f->c[f]  = sqrt(stdevP->f->c[f] / numPoints);
		stdevP->w->c[f]  = sqrt(stdevP->w->c[f] / numPoints);
	}
	for(f=0;f<nC;f++){
		stdevP->d->c[f]  = sqrt(stdevP->d->c[f] / numPoints);
		stdevP->td->c[f] = sqrt(stdevP->td->c[f] / numPoints);
		stdevP->p->c[f]  = sqrt(stdevP->p->c[f] / numPoints);
	}
}
/* it will get min/max/mean/stdev from a list of numPoints points,
   this list is not given directly. an array of integer indices is given
   and the list of all points. */
void	statistics_point_from_indices(int *indices, int numPoints, point **all_points, point *minP, point *maxP, point *meanP, point *stdevP){
	register int i, f, nF = all_points[indices[0]]->v->nC,
			   nC = all_points[indices[0]]->d->nC;

	register int *I;
	register point **p = &(all_points[indices[0]]), *pp;

	/* be careful, w, v and f are of size nF, the rest (d, td etc) are of size nC */
	copy_point(*p, minP);
	copy_point(*p, maxP);
	copy_point(*p, meanP);
	copy_point(*p, stdevP);

	for(f=0;f<nF;f++)
		meanP->v->c[f]   =
		meanP->f->c[f]   =
		meanP->w->c[f]   =
		stdevP->v->c[f]  =
		stdevP->f->c[f]  =
		stdevP->w->c[f]  = 0.0;

	for(f=0;f<nC;f++)
		meanP->p->c[f]   =
		meanP->d->c[f]   =
		meanP->td->c[f]  =
		stdevP->p->c[f]  =
		stdevP->d->c[f]  =
		stdevP->td->c[f] = 0.0;

	for(i=0,I=&(indices[0]);i<numPoints;i++,I++){
		pp = all_points[*I];
		for(f=0;f<nF;f++){
			if( pp->v->c[f] < minP->v->c[f] ) minP->v->c[f] = pp->v->c[f];
			if( pp->f->c[f] < minP->f->c[f] ) minP->f->c[f] = pp->f->c[f];
			if( pp->w->c[f] < minP->w->c[f] ) minP->w->c[f] = pp->w->c[f];

			if( pp->v->c[f] > maxP->v->c[f] ) maxP->v->c[f] = pp->v->c[f];
			if( pp->f->c[f] > maxP->f->c[f] ) maxP->f->c[f] = pp->f->c[f];
			if( pp->w->c[f] > maxP->w->c[f] ) maxP->w->c[f] = pp->w->c[f];
			meanP->v->c[f] += pp->v->c[f];
			meanP->f->c[f] += pp->f->c[f];
			meanP->w->c[f] += pp->w->c[f];
		}
		for(f=0;f<nC;f++){
			if( pp->p->c[f] < minP->p->c[f] ) minP->p->c[f] = pp->p->c[f];
			if( pp->d->c[f] < minP->d->c[f] ) minP->d->c[f] = pp->d->c[f];
			if( pp->td->c[f] < minP->td->c[f] ) minP->td->c[f] = pp->td->c[f];

			if( pp->p->c[f] > maxP->p->c[f] ) maxP->p->c[f] = pp->p->c[f];
			if( pp->d->c[f] > maxP->d->c[f] ) maxP->d->c[f] = pp->d->c[f];
			if( pp->td->c[f] > maxP->td->c[f] ) maxP->td->c[f] = pp->td->c[f];
			meanP->p->c[f] += pp->p->c[f];
			meanP->d->c[f] += pp->d->c[f];
			meanP->td->c[f] += pp->td->c[f];
		}
	}
	for(f=0;f<nF;f++){
		meanP->v->c[f]  /= (float )numPoints;
		meanP->f->c[f]  /= (float )numPoints;
		meanP->w->c[f]  /= (float )numPoints;
	}
	for(f=0;f<nC;f++){		
		meanP->p->c[f]  /= (float )numPoints;
		meanP->d->c[f]  /= (float )numPoints;
		meanP->td->c[f] /= (float )numPoints;
	}
	for(i=0,I=&(indices[0]);i<numPoints;i++,I++){
		pp = all_points[*I];
		for(f=0;f<nF;f++){
			stdevP->v->c[f] += SQR(pp->v->c[f] - meanP->v->c[f]);
			stdevP->f->c[f] += SQR(pp->f->c[f] - meanP->f->c[f]);
			stdevP->w->c[f] += SQR(pp->w->c[f] - meanP->w->c[f]);
		}
		for(f=0;f<nC;f++){
			stdevP->p->c[f] += SQR(pp->p->c[f] - meanP->p->c[f]);
			stdevP->d->c[f] += SQR(pp->d->c[f] - meanP->d->c[f]);
			stdevP->td->c[f] += SQR(pp->td->c[f] - meanP->td->c[f]);
		}
	}
	for(f=0;f<nF;f++){
		stdevP->v->c[f]  = sqrt(stdevP->v->c[f] / ((float )numPoints));
		stdevP->f->c[f]  = sqrt(stdevP->f->c[f] / ((float )numPoints));
		stdevP->w->c[f]  = sqrt(stdevP->w->c[f] / ((float )numPoints));
	}
	for(f=0;f<nC;f++){
		stdevP->p->c[f]  = sqrt(stdevP->p->c[f] / ((float )numPoints));
		stdevP->d->c[f]  = sqrt(stdevP->d->c[f] / ((float )numPoints));
		stdevP->td->c[f] = sqrt(stdevP->td->c[f] / ((float )numPoints));
	}
}
