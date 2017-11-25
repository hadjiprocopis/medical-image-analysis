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
#include <clustering.h>

#include "cellular_automata.h"

cell	*ca_new_cell(int id, int num_features, int num_cells, int num_symbols){
	int	i;
	cell	*ret;

	if( (ret=(cell *)malloc(sizeof(cell))) == NULL ){
		fprintf(stderr, "ca_new_cell : could not allocate %zd bytes for ret.\n", sizeof(cell));
		return (cell *)NULL;
	}
	/* features */
	if( (ret->f=new_vector(num_features)) == NULL ){
		fprintf(stderr, "ca_new_cell : call to new_vector has failed for ret->f (%d features).\n", num_features);
		free(ret);
		return (cell *)NULL;
	}
	/* actual pixel values */
	if( (ret->v=new_vector(num_features)) == NULL ){
		fprintf(stderr, "ca_new_cell : call to new_vector has failed for ret->v (%d features).\n", num_features);
		destroy_vector(ret->f); free(ret);
		return (cell *)NULL;
	}
	/* weights */
	if( (ret->w=new_vector(num_features)) == NULL ){
		fprintf(stderr, "ca_new_cell : call to new_vector has failed for ret->w (%d features).\n", num_features);
		destroy_vector(ret->f); destroy_vector(ret->v); free(ret);
		return (cell *)NULL;
	}

	/* probabilities of symbol occurence in the nearest and furthest neighbours */
	if( (ret->pnn=new_vector(CA_MAX_NEIGHBOUR_CELLS)) == NULL ){
		fprintf(stderr, "ca_new_cell : call to new_vector has failed for ret->pnn (%d neighbours).\n", CA_MAX_NEIGHBOUR_CELLS);
		destroy_vector(ret->f); destroy_vector(ret->v); destroy_vector(ret->w); free(ret);
		return (cell *)NULL;
	}
	if( (ret->pfn=new_vector(CA_MAX_NEIGHBOUR_CELLS)) == NULL ){
		fprintf(stderr, "ca_new_cell : call to new_vector has failed for ret->pfn (%d neighbours).\n", CA_MAX_NEIGHBOUR_CELLS);
		destroy_vector(ret->pnn); destroy_vector(ret->f); destroy_vector(ret->v); destroy_vector(ret->w); free(ret);
		return (cell *)NULL;
	}
	/* distances of cell from neighbour cells */
	if( (ret->dnn=new_vector(CA_MAX_NEIGHBOUR_CELLS)) == NULL ){
		fprintf(stderr, "ca_new_cell : call to new_vector has failed for ret->dnn (%d neighbours).\n", CA_MAX_NEIGHBOUR_CELLS);
		destroy_vector(ret->pfn); destroy_vector(ret->pnn); destroy_vector(ret->f); destroy_vector(ret->v); destroy_vector(ret->w); free(ret);
		return (cell *)NULL;
	}
	if( (ret->dfn=new_vector(CA_MAX_NEIGHBOUR_CELLS)) == NULL ){
		fprintf(stderr, "ca_new_cell : call to new_vector has failed for ret->dfn (%d neighbours).\n", CA_MAX_NEIGHBOUR_CELLS);
		destroy_vector(ret->pfn); destroy_vector(ret->pnn); destroy_vector(ret->dnn); destroy_vector(ret->pnn); destroy_vector(ret->f); destroy_vector(ret->v); destroy_vector(ret->w); free(ret);
		return (cell *)NULL;
	}

	/* vector to hold the homogeneity for each symbol */
	if( (ret->hnn=new_vector(num_symbols)) == NULL ){
		fprintf(stderr, "ca_new_cell : call to new_vector has failed for ret->hnn (%d neighbours).\n", num_symbols);
		destroy_vector(ret->dfn); destroy_vector(ret->pfn); destroy_vector(ret->pnn); destroy_vector(ret->f); destroy_vector(ret->v); destroy_vector(ret->w); free(ret);
		return (cell *)NULL;
	}
	if( (ret->hfn=new_vector(num_symbols)) == NULL ){
		fprintf(stderr, "ca_new_cell : call to new_vector has failed for ret->pfn (%d neighbours).\n", num_symbols);
		destroy_vector(ret->hnn); destroy_vector(ret->dfn); destroy_vector(ret->pfn); destroy_vector(ret->pnn); destroy_vector(ret->dnn); destroy_vector(ret->pnn); destroy_vector(ret->f); destroy_vector(ret->v); destroy_vector(ret->w); free(ret);
		return (cell *)NULL;
	}

	if( (ret->state=new_vector(num_features)) == NULL ){
		fprintf(stderr, "ca_new_cell : call to new_vector has failed for ret->state (%d neighbours).\n", num_features);
		destroy_vector(ret->dfn); destroy_vector(ret->pfn); destroy_vector(ret->pnn); destroy_vector(ret->f); destroy_vector(ret->v); destroy_vector(ret->w); free(ret);
		return (cell *)NULL;
	}
	if( (ret->fitness=new_vector(num_features)) == NULL ){
		fprintf(stderr, "ca_new_cell : call to new_vector has failed for ret->fitness (%d neighbours).\n", num_features);
		destroy_vector(ret->hnn); destroy_vector(ret->dfn); destroy_vector(ret->pfn); destroy_vector(ret->pnn); destroy_vector(ret->dnn); destroy_vector(ret->pnn); destroy_vector(ret->f); destroy_vector(ret->v); destroy_vector(ret->w); free(ret);
		return (cell *)NULL;
	}

	/* initialise weights to neutral */
	for(i=0;i<num_features;i++) ret->w->c[i] = 1.0;
	ret->t = 0.0;

	ret->id = id;
	ret->x = ret->y = ret->z = 0.0;
	ret->X = ret->Y = ret->Z = ret->s = 0;

	/* no neighbours yet */
	ret->nNN = ret->nFN = 0;
	/* no symbols */
	ret->cs = ret->ps = (symbol *)NULL;

	ret->enn = ret->efn = -1.0;

	for(i=0;i<CA_MAX_NEIGHBOUR_CELLS;i++) ret->nn[i] = ret->fn[i] = NULL;

	for(i=0;i<num_symbols;i++) ret->hnn->c[i] = ret->hfn->c[i] = -1.0;

	return ret;
}
/* it will calculate a measure of how homogeneous this cell's neighbourhood is */
/* note that the cell's state is not considered */
/* homogeneity is different for each symbol - hence the array of 'result' */
/* the returned value (via parameter 'result') should be allocated by caller to as
   many floats as there are symbols */
void	ca_calculate_nn_homogeneity_cell(cell *a_cell, float *result){
	register int	n, s;
	float	*p, total = 0.0;

	/* homogeneity is defined as:
		for all the neighbouring cells with same symbol as current,
		sum up all the inverse distances (using sigmoid distance blah blah blah)
		finally convert homogeneities as percentage over the total sum
		of homogeneities for all symbols.
	*/

//	printf("%d = ", a_cell->id);
	for(s=0,p=&(result[s]);s<a_cell->ca->nS;s++,p++){ /* for each symbol */
		for(n=0,*p=0.0;n<a_cell->nNN;n++){ /* for each neighbouring cell */
			/* if the symbols are the same, count it */
			if( a_cell->nn[n]->cs->id == s ){ /* if they have same symbols */
				*p += 1.0 / (1.0 + exp(0.001*a_cell->dnn->c[n]));
//				printf("adding %f for distance %f\n", 1.0 / (1.0 + exp(a_cell->dnn->c[n])), a_cell->dnn->c[n]);
			}
		}
//		printf("%.2f ", *p);
		total += *p;
	}
	for(s=0;s<a_cell->ca->nS;s++) result[s] /= total; /* a percentage then */
//	printf("\n");
}

void	ca_calculate_state_cell(cell *a_cell){
	register int	i, f;
	float	total, dummy, *pS, *pF;

	for(f=0,pS=&(a_cell->state->c[f]);f<a_cell->ca->nF;f++,pS++){
		*pS = 0.0; total = 0.0;
		for(i=0;i<a_cell->nNN;i++){
			dummy = 1.0 / (1.0 + exp(0.001*a_cell->dnn->c[i]));
			*pS += dummy * a_cell->f->c[i];
			total += dummy;
		}
		*pS /= total;
	}

	/* now do fitness */
	for(f=0,pS=&(a_cell->state->c[f]),pF=&(a_cell->fitness->c[f]);f<a_cell->ca->nF;f++,pS++,pF++){
		*pF = 0.0; total = 0.0;
		for(i=0;i<a_cell->nNN;i++){
			dummy = 1.0 / (1.0 + exp(0.001*a_cell->dnn->c[i]));
			*pF += dummy * (SQR((*pS)-a_cell->nn[i]->state->c[f]));
			total += dummy;
		}
		*pF /= total;
	}
}	

void	ca_destroy_cell(cell *a_cell){
	destroy_vector(a_cell->f);
	destroy_vector(a_cell->v);
	destroy_vector(a_cell->w);
	destroy_vector(a_cell->pnn); destroy_vector(a_cell->pfn);
	destroy_vector(a_cell->dnn); destroy_vector(a_cell->dfn);
	destroy_vector(a_cell->hnn); destroy_vector(a_cell->hfn);
	destroy_vector(a_cell->state); destroy_vector(a_cell->fitness);
	free(a_cell);
}
char	*ca_toString_cell(cell *a_cell){
	char	*ret, *p0, *p1, *p2;

	if( (ret=(char *)malloc(2000)) == NULL ){
		fprintf(stderr, "ca_toString_cell : could not allocate %d bytes for ret.\n", 2000);
		return NULL;
	}

	p0 = toString_vector(a_cell->v);
	p1 = toString_vector(a_cell->f); p2 = toString_vector(a_cell->w);
	sprintf(ret, "[CELL(%d) pix=%s, fea=%s, wei=%s t=%f nNN=%d, nFN=%d]",
			a_cell->id, p0, p1, p2, a_cell->t, a_cell->nNN, a_cell->nFN);
	p0 = strdup(ret); free(ret); return strdup(p0);
}
void	ca_print_cell(FILE *stream, cell *a_cell){
	char	*p = ca_toString_cell(a_cell);
	fprintf(stream, "%s", p);
	free(p);
}
