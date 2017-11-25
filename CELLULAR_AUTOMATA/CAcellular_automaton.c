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

#include "cellular_automata.h"

/* private (comparators for qsort) */
static  int     _ca_comparator_ascending(const void *, const void *);
static  int     _ca_comparator_descending(const void *, const void *);

cellular_automaton	*ca_new_cellular_automaton(int id, int numFeatures, int numCells, int numSymbols){
	int			i;
	cellular_automaton	*ret;

	if( (ret=(cellular_automaton *)malloc(sizeof(cellular_automaton))) == NULL ){
		fprintf(stderr, "ca_new_cellular_automaton : could not allocate %zd bytes for cellular_automaton.\n", sizeof(cellular_automaton));
		return (cellular_automaton *)NULL;
	}
	if( (ret->c=(cell **)malloc(numCells * sizeof(cell *))) == NULL ){
		fprintf(stderr, "ca_new_cellular_automaton : could not allocate %zd bytes for ret->c.\n", numCells * sizeof(cell *));
		free(ret);
		return (cellular_automaton *)NULL;
	}	
	if( (ret->s=(symbol **)malloc(numSymbols * sizeof(cell *))) == NULL ){
		fprintf(stderr, "ca_new_cellular_automaton : could not allocate %zd bytes for ret->s.\n", numSymbols * sizeof(symbol *));
		free(ret->c); free(ret);
		return (cellular_automaton *)NULL;
	}	
	for(i=0;i<numCells;i++){
		if( (ret->c[i]=ca_new_cell(i, numFeatures, numCells, numSymbols)) == NULL ){
			fprintf(stderr, "ca_new_cellular_automaton : call to ca_new_cell has failed for cell %d (of %d) (feat=%d) (symb=%d).\n", i+1, numCells, numFeatures, numSymbols);
			free(ret->c); free(ret->s); free(ret);
			return (cellular_automaton *)NULL;
		}
		ret->c[i]->ca = ret; /* let the cell know about its parent ... */
	}
	for(i=0;i<numSymbols;i++)
		if( (ret->s[i]=ca_new_symbol(i, NULL)) == NULL ){
			fprintf(stderr, "ca_new_cellular_automaton : call to ca_new_symbol has failed for symbol %d (of %d).\n", i+1, numSymbols);
			free(ret->c); free(ret->s); free(ret);
			return (cellular_automaton *)NULL;
		}

	if( (ret->_sort_data_distances=(_ca_sort_data *)malloc(numCells*sizeof(_ca_sort_data))) == NULL ){
		fprintf(stderr, "ca_new_cellular_automaton : could not allocate %zd bytes for ret->_sort_data_distances (%d cells).\n", numCells*sizeof(_ca_sort_data), numCells);
		free(ret->c); free(ret->s); free(ret);
		return (cellular_automaton *)NULL;
	}
	if( (ret->_sort_data_symbols_nearest=(_ca_sort_data *)malloc(numSymbols*sizeof(_ca_sort_data))) == NULL ){
		fprintf(stderr, "ca_new_cellular_automaton : could not allocate %zd bytes for ret->_sort_data_symbols_nearest (%d symbols).\n", numSymbols*sizeof(_ca_sort_data), numSymbols);
		free(ret->_sort_data_distances); free(ret->c); free(ret->s); free(ret);
		return (cellular_automaton *)NULL;
	}
	if( (ret->_sort_data_symbols_furthest=(_ca_sort_data *)malloc(numSymbols*sizeof(_ca_sort_data))) == NULL ){
		fprintf(stderr, "ca_new_cellular_automaton : could not allocate %zd bytes for ret->_sort_data_symbols_furthest (%d symbols).\n", numSymbols*sizeof(_ca_sort_data), numSymbols);
		free(ret->_sort_data_symbols_nearest); free(ret->_sort_data_distances); free(ret->c); free(ret->s); free(ret);
		return (cellular_automaton *)NULL;
	}
	if( (ret->_homogeneities=(float **)malloc(numSymbols*sizeof(float *))) == NULL ){
		fprintf(stderr, "ca_new_cellular_automaton : could not allocate %zd bytes for ret->_homogeneities (%d symbols).\n", numSymbols*sizeof(float *), numSymbols);
		free(ret->_sort_data_symbols_furthest); free(ret->_sort_data_symbols_nearest); free(ret->_sort_data_distances); free(ret->c); free(ret->s); free(ret);
		return (cellular_automaton *)NULL;
	}
	for(i=0;i<numSymbols;i++)
		if( (ret->_homogeneities[i]=(float *)malloc(numSymbols*sizeof(float))) == NULL ){
			fprintf(stderr, "ca_new_cellular_automaton : could not allocate %zd bytes for ret->_homogeneities[%d] (%d symbols).\n", numSymbols*sizeof(float), i, numSymbols);
			free(ret->_sort_data_symbols_furthest); free(ret->_sort_data_symbols_nearest); free(ret->_sort_data_distances); free(ret->c); free(ret->s); free(ret);
			return (cellular_automaton *)NULL;
		}
	if( (ret->stats=new_clestats(numFeatures)) == NULL ){
		fprintf(stderr, "ca_new_cellular_automaton : call to new_clestats has failed for %d features.\n", numFeatures);
		free(ret->_sort_data_symbols_nearest); free(ret->_sort_data_symbols_furthest); free(ret->_sort_data_distances); free(ret->c); free(ret->s); free(ret);
		return (cellular_automaton *)NULL;
	}		
	if( (ret->im=(clunc *)malloc(sizeof(clunc))) == NULL ){
		fprintf(stderr, "ca_new_cellular_automaton : could not allocate %zd bytes for ret->im.\n", sizeof(clunc));
		destroy_clestats(ret->stats); free(ret->_sort_data_symbols_nearest); free(ret->_sort_data_symbols_furthest); free(ret->_sort_data_distances); free(ret->c); free(ret->s); free(ret);
		return (cellular_automaton *)NULL;
	}
	if( (ret->clusters=(cluster **)malloc(numSymbols*sizeof(cluster *))) == NULL ){
		fprintf(stderr, "ca_new_cellular_automaton : could not allocate %zd bytes for ret->clusters.\n", numSymbols*sizeof(cluster *));
		free(ret->im); destroy_clestats(ret->stats); free(ret->_sort_data_symbols_nearest); free(ret->_sort_data_symbols_furthest); free(ret->_sort_data_distances); free(ret->c); free(ret->s); free(ret);
		return (cellular_automaton *)NULL;
	}
	for(i=0;i<numSymbols;i++)
		if( (ret->clusters[i]=new_cluster(i, numFeatures, numCells, numSymbols)) == NULL ){
			fprintf(stderr, "ca_new_cellular_automaton : could not allocate %zd bytes for ret->clusters.\n", numSymbols*sizeof(cluster *));
			free(ret->im); destroy_clestats(ret->stats); free(ret->_sort_data_symbols_nearest); free(ret->_sort_data_symbols_furthest); free(ret->_sort_data_distances); free(ret->c); free(ret->s); free(ret);
			return (cellular_automaton *)NULL;
		}

	ret->id = id;
	ret->nF = numFeatures;
	ret->nC = numCells;
	ret->nS = numSymbols;
	ret->interruptFlag = FALSE;
	ret->kT = 1.0; /* the S.A. temperature constant, change it later */

	return ret;
}
void	ca_destroy_cellular_automaton(cellular_automaton *aca){
	int	i;
	for(i=0;i<aca->nC;i++) ca_destroy_cell(aca->c[i]); free(aca->c);
	for(i=0;i<aca->nS;i++) ca_destroy_symbol(aca->s[i]); free(aca->s);
	destroy_clestats(aca->stats);
	free(aca->im);
	free(aca->_sort_data_distances);
	free(aca->_sort_data_symbols_nearest);
	free(aca->_sort_data_symbols_furthest);
	for(i=0;i<aca->nS;i++) free(aca->_homogeneities[i]); free(aca->_homogeneities);
	destroy_clusters(aca->clusters, aca->nS);
	free(aca);
}
char	*ca_toString_cellular_automaton(cellular_automaton *aca){
	char	*ret, *p;
	if( (ret=(char *)malloc(200)) == NULL ){
		fprintf(stderr, "ca_toString_cellular_automaton : could not allocate %d bytes for ret.\n", 200);
		return NULL;
	}
	sprintf(ret, "CA(%d) cells=%d, features=%d, symbols=%d\n",
		aca->id, aca->nC, aca->nF, aca->nS);
	p = strdup(ret); free(ret);
	return p;
}
void	ca_print_cellular_automaton(FILE *stream, cellular_automaton *aca){
	char	*p = ca_toString_cellular_automaton(aca);
	fprintf(stream, "%s", p);
	free(p);
}
int	ca_interrupt_cellular_automaton(cellular_automaton *aca, int new_state){
	int	state = aca->interruptFlag;
	aca->interruptFlag = new_state;
	return state;
}
void	ca_initialise_to_random_state(cellular_automaton *aca){
	/* set all cell's current and previous state to the same symbol chosen randomly from the set of available symbols */
	int	i;
	for(i=0;i<aca->nC;i++) aca->c[i]->ps = aca->c[i]->cs = aca->c[i]->_ss = aca->s[lrand48() % aca->nS];
}

int	ca_calculate_cell_to_cell_distances_with_weights(cellular_automaton *aca, cell *a_cell){
	register int	i, cid1, cid2;
	cell		**c;

	/* calculate distances of a_cell from all other cells */
	cid1 = a_cell->id;
	for(i=0,c=&(aca->c[0]);(i<aca->nC)&&(aca->interruptFlag==FALSE);i++,c++){
		aca->_sort_data_distances[i].id = cid2 = (*c)->id;
		if( cid1 == cid2 ) aca->_sort_data_distances[i].value = -1.0;
		else aca->_sort_data_distances[i].value = euclidean_distance_between_two_vectors_with_weights(a_cell->f, a_cell->w, a_cell->t, (*c)->f, (*c)->w, (*c)->t);
	}
	return !(aca->interruptFlag); /* was it interrupted ? */
}
int	ca_calculate_cell_to_cell_distances(cellular_automaton *aca, cell *a_cell){
	register int	i, j, cid;
	cell		*c;

	/* calculate distances of a_cell from all other cells */
	cid = a_cell->id;
	for(i=0,j=0;(i<aca->nC)&&(aca->interruptFlag==FALSE);i++){
		c = aca->c[i];
		if( cid != c->id ){
			/* we do not count ourselves in the neighbours ... */
			aca->_sort_data_distances[j].id = c->id;
			/* distance metric */
//			aca->_sort_data_distances[j].value =
//				0.3 * sqrt(SQR(a_cell->x-c->x) + SQR(a_cell->y-c->y)) +
//				euclidean_distance_between_two_vectors(a_cell->f, c->f);
			aca->_sort_data_distances[j].value = euclidean_distance_between_two_vectors(a_cell->f, c->f);
			j++;
		}
	}
	if( aca->interruptFlag ) return FALSE; /* was it interrupted */
	return j; /* return the number of cells which can be our neighbours */
}
/* calculate the next state of the cellular automaton. in this process the number of
   nearest and furthest neighbours has to be supplied, or the max will be used (CA_MAX_NEIGHBOUR_CELLS) */
int	ca_find_neighbours(cellular_automaton *aca, int numNearestNeighbours, int numFurthestNeighbours){
	register int	i, j, k, l, nNN, nFN, dummy, numNeighbours;
	register cell	*c, *c1;
	char		canBeNeighbours;

	/* find the nearest and furthest neighbours of each cell */
	nNN = (numNearestNeighbours<=0) ? CA_MAX_NEIGHBOUR_CELLS : numNearestNeighbours;
	nFN = (numFurthestNeighbours<=0) ? CA_MAX_NEIGHBOUR_CELLS : numFurthestNeighbours;

	dummy = aca->nC - nFN;
	fprintf(stdout, "ca_find_neighbours :"); fflush(stdout);
	for(i=0;(i<aca->nC)&&(aca->interruptFlag==FALSE);i++){
		if( (i%100) == 0 ){ fprintf(stdout, "%d ", i); fflush(stdout); }
		c = aca->c[i];
		/* calculate distances from this cell to all other cells */
		numNeighbours = ca_calculate_cell_to_cell_distances(aca, c);

		/* the distances have gone into the _sort_data_distances array which we will sort in order to get nearest and furthest neighbours */
		/* note the number of these distances is stored in 'numNeighbours' returned from ca_calculate_cell_to_cell_distances */
		qsort((void *)(aca->_sort_data_distances), numNeighbours,
			sizeof(_ca_sort_data), _ca_comparator_ascending);

		/* nearest */
		for(j=0,k=0;(k<nNN)&&(j<numNeighbours);j++){
			canBeNeighbours = TRUE;
			c1 = aca->c[aca->_sort_data_distances[j].id];
			if( c1->nn[0] != NULL ){
				for(l=0;l<c1->nNN;l++) if( c1->nn[l]->id == c->id ){ canBeNeighbours = FALSE; break; }
			}
			if( canBeNeighbours == FALSE ) continue;
			/* include neighbours which are not already assigned to some other cell */
			c->nn[k] = aca->c[aca->_sort_data_distances[j].id]; /* cell */
			c->dnn->c[k] = aca->_sort_data_distances[j].value;  /* distance */
			k++;

		}
		c->nNN = k;

		/* furthest */
		/* we have to read the distances from the end - e.g. furthest */
		for(j=numNeighbours-1,k=0;(k<nFN)&&(j>=0);j--){
			canBeNeighbours = TRUE;
			c1 = aca->c[aca->_sort_data_distances[j].id];
			if( c1->fn[0] != NULL ){
				for(l=0;l<c1->nFN;l++) if( c1->fn[l]->id == c->id ){ canBeNeighbours = FALSE; break; }
			}
			if( canBeNeighbours == FALSE ) continue;
			/* include neighbours which are not already assigned to some other cell */
			c->fn[k] = aca->c[aca->_sort_data_distances[j].id]; /* cell */
			c->dnn->c[k] = aca->_sort_data_distances[j].value;  /* distance */
			k++;
		}
		c->nFN = k;
	}
	fprintf(stdout, "\n");

	return !(aca->interruptFlag); /* was it interrupted - the only reason to return FALSE */
}
int	ca_calculate_next_state(cellular_automaton *aca){
	register int	i, j, proposed_state, sid;
	cell		*c;
	int		ii;

	for(ii=0;(ii<aca->nC)&&(aca->interruptFlag==FALSE);ii++){
		i = ii;//lrand48() % aca->nC;
		c = aca->c[i];

		c->_ss = c->cs;	/* save current state */

		/* calculate the aca->_homogeneities for each symbol */
		/* in order to do that, the current cell's state iterates through
		   ALL the possible symbols and for each, the homogeneity is calculated. */
		for(j=0;j<aca->nS;j++){
			sid = aca->s[j]->id;
			ca_calculate_nn_homogeneity_cell(c, &(aca->_homogeneities[sid][0]));
			/* we are interested only on the homogeneity of the symbol = current cell's symbol */
			aca->_sort_data_symbols_nearest[j].id = sid;
			aca->_sort_data_symbols_nearest[j].value = aca->_homogeneities[sid][sid];
		}

		/* sort the aca->_homogeneities - descending, e.g. [0] is best homogeneity (e.g. highest) */
		qsort((void *)(aca->_sort_data_symbols_nearest), aca->nS, sizeof(_ca_sort_data),_ca_comparator_descending);
		
		/* ok, make a decision - new state should not be the same as the current one */
		//while( (proposed_state=lrand48() % aca->nS) == c->cs->id );

		proposed_state = aca->_sort_data_symbols_nearest[0].id;

		/* does this decision coincide with the best homogeneity scenario ? */
		if( proposed_state == aca->_sort_data_symbols_nearest[0].id ){
			/* yes - accept it */
			c->_ss = aca->s[proposed_state];
		} else {
			/* no - S.A. stuff */
			/* the difference between proposed and current homogeneities */
/*			dH = aca->_homogeneities[proposed_state][proposed_state] -
			     aca->_homogeneities[save_state->id][c->_ss->id];
			expo = exp( - dH / aca->kT );
			if( i == 30 ){
				printf("best %d, dH = %f, expo = %f, kT=%f, xx=%f\n", aca->_sort_data_symbols_nearest[0].id, dH, expo, aca->kT, - dH / aca->kT);
				printf("it was proposed %d and saved %d (%f = %f)\n", proposed_state, save_state->id, aca->_homogeneities[proposed_state][proposed_state], aca->_homogeneities[save_state->id][save_state->id]);
			}
			if( expo > drand48() ){
*/
			if( 0 ){
				/* accept it even if worse */
				c->_ss = aca->s[proposed_state];
			} else {
				/* reject proposed stated, revert back to old */
				c->_ss = c->cs;
 			}
		}
	}


	return !(aca->interruptFlag); /* was it interrupted - the only reason to return FALSE */
}
int	ca_normalise_points(cellular_automaton *aca, float rangeMin, float rangeMax, char overAllFeaturesFlag){
	/* normalise either over all feature space -- e.g. find pixel range by searching over all
	   features, or normalise over each feature, min and max will be only for the pixels
	   of that feature */
	register int	i, f, j;
	float	minP, maxP;
	float	*scrap;

/*
	// if you want 3 clear-cut clusters for testing:
	for(f=0;f<aca->nF;f++) for(i=0;i<aca->nP;i++){
		if( i < (aca->nP/3) ) aca->p[i]->f->c[f] = 0.2*drand48();
		else if( i < (2*aca->nP/3) ) aca->p[i]->f->c[f] = 0.3 + 0.2*drand48();
		else if( i < aca->nP ) aca->p[i]->f->c[f] = 0.6 + 0.2*drand48();
	}
	return TRUE;
*/

	if( overAllFeaturesFlag ){
		/* normalisation will be over all the features of all the points --
		   e.g. the min and max values will be over all feature space */
		if( (scrap=(float *)malloc(aca->nC * aca->nF * sizeof(float))) == NULL ){
			fprintf(stderr, "ca_normalise_points : could not allocate %zd bytes for 'scrap'.\n", aca->nC * aca->nF * sizeof(float));
			return FALSE;
		}
		j = 0;
		for(f=0;f<aca->nF;f++) for(i=0;i<aca->nC;i++) scrap[j++] = aca->c[i]->v->c[f];
		min_maxPixel1D_float(scrap, 0, aca->nC * aca->nF, &minP, &maxP);
		for(f=0;f<aca->nF;f++){
			aca->stats->min_point_values_normalised->c[f] = rangeMax; /* this is just to initialise -- do not worry */
			aca->stats->max_point_values_normalised->c[f] = rangeMin;
			for(i=0;i<aca->nC;i++){
				aca->c[i]->f->c[f] = SCALE_OUTPUT(aca->c[i]->v->c[f], rangeMin, rangeMax, minP, maxP);
				if( aca->c[i]->f->c[f] < aca->stats->min_point_values_normalised->c[f] ) aca->stats->min_point_values_normalised->c[f] = aca->c[i]->f->c[f];
				if( aca->c[i]->f->c[f] > aca->stats->max_point_values_normalised->c[f] ) aca->stats->max_point_values_normalised->c[f] = aca->c[i]->f->c[f];
			}
		}
	} else {
		if( (scrap=(float *)malloc(aca->nC * sizeof(float))) == NULL ){
			fprintf(stderr, "ca_normalise_points : could not allocated %zd bytes for 'scrap'.\n", aca->nC * sizeof(float));
			return FALSE;
		}
		for(f=0;f<aca->nF;f++){
			for(i=0;i<aca->nC;i++) scrap[i] = aca->c[i]->v->c[f];
			min_maxPixel1D_float(scrap, 0, aca->nC, &minP, &maxP);
			for(i=0;i<aca->nC;i++) aca->c[i]->f->c[f] = SCALE_OUTPUT(aca->c[i]->v->c[f], rangeMin, rangeMax, minP, maxP);
			aca->stats->min_point_values_normalised->c[f] = rangeMin;
			aca->stats->max_point_values_normalised->c[f] = rangeMax;
		}
	}

	free(scrap);
	return TRUE;
}		
void	ca_finalise_next_state(cellular_automaton *aca){
	/* all proposed states do not take effect until all decisions
	   are made. then this function is called to put _ss (suggested state)
	   to cs (current state) and save cs to ps (past state) */
	int	i;
	cell	*c;
	for(i=0;(i<aca->nC)&&(aca->interruptFlag==FALSE);i++){
		c = aca->c[i];
		c->ps = c->cs;
		c->cs = c->_ss;
	}
}
int	ca_calculate_statistics(cellular_automaton *aca){
	int	i, j, sid;
	float	expo;
	cluster	*cl;
	cell	*ce;

	aca->totalEntropy = 0.0;
	for(i=0;i<aca->nS;i++) aca->clusters[i]->n = 0;

	for(i=0;(i<aca->nC)&&(aca->interruptFlag==FALSE);i++){
		ce = aca->c[i]; sid = ce->cs->id;

		/* calculate the aca->_homogeneities for the current symbol */
		ca_calculate_nn_homogeneity_cell(ce, &(aca->_homogeneities[sid][0]));
		for(j=0,sid=ce->cs->id;j<aca->nS;j++){
			ce->hnn->c[j] = aca->_homogeneities[sid][j];
		}
		/* calculate entropy */
		for(j=0,ce->enn=0.0;j<aca->nS;j++){
			if( (expo=ce->hnn->c[j]) > 0.0 ) ce->enn -= expo * log(expo);
		}
		aca->totalEntropy += ce->enn;

		/* find which cells go to each cluster */
		cl = aca->clusters[aca->c[i]->cs->id];
		cl->points[cl->n] = aca->c[i]->id;
		cl->n++;

		ca_calculate_state_cell(ce);
	}

	for(i=0;i<aca->nS;i++){
		/* for each cluster, calculate its centroid by the number of cells it contains */
		cl = aca->clusters[i];
		reset_vector(cl->centroid->f);
		for(j=0;j<cl->n;j++)
			add_vectors(cl->centroid->f, aca->c[cl->points[j]]->f);
		if( cl->n > 0.0 ) multiply_vector_by_constant(cl->centroid->f, 1.0 / cl->n);
	}

	return !(aca->interruptFlag);
}

/* private : 2 routines for qsort to sort an array of integers in ascending and descending 
order */
static  int     _ca_comparator_descending(const void *a, const void *b){
	float   _a = ((_ca_sort_data *)a)->value,
		_b = ((_ca_sort_data *)b)->value;

	if( _a > _b ) return -1;
	if( _a < _b ) return 1;
	return 0;
}
static  int     _ca_comparator_ascending(const void *a, const void *b){
	float   _a = ((_ca_sort_data *)a)->value,
		_b = ((_ca_sort_data *)b)->value;

	if( _a > _b ) return 1;
	if( _a < _b ) return -1;
	return 0;
}

