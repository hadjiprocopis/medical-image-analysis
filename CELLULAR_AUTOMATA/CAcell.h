#ifndef	_CA_CELL_VISITED

#include <clustering.h>

/* the maximun number of neighbours a cell can have - use to allocate an array cell->c for fast access */
#define	CA_MAX_NEIGHBOUR_CELLS	500

struct	_CA_CELL {
	cellular_automaton	*ca;

	int	id;	/* the id of the cell */
	symbol	*cs,	/* current state  - these two are placeholders, symbols should be allocated separately */
		*ps,	/* previous state */
		*_ss;	/* suggested state - all suggested states will be adopted only when all decisions have been made */

	int	nNN,nFN;/* number of neighbouring cells to consider - may change on-the-fly */
	cell	*nn[CA_MAX_NEIGHBOUR_CELLS],	/* NEAREST neighbouring cells */
		*fn[CA_MAX_NEIGHBOUR_CELLS];	/* FURTHEST neighbouring cells */
	vector	*dnn, *dfn;	/* distances from each of those neighbours */
	vector	*pnn, *pfn;	/* probabilities of symbol occurence in the nearest and furthest neighbours */
	float	enn, efn;	/* the entropy of the nearest and furthest neighbours (measure of homogeneity) */
	vector	*hnn, *hfn;	/* the homogeneity (for each symbol) of the nearest and furthest neighbours */

	vector	*f;	/* the features of the cell - may be normalised */
	vector	*v;	/* pixel values - not normalised */

	vector	*w;	/* weights of this cell - same size as features */
	float	t;	/* the threshold */

	float	x,y,z;	/* spatial location of the cell */
	int	X,Y,Z,	/* spatial location but used to access arrays
			   - Z and z are NOT slice numbers - s is slice number */
		s;

	vector	*state, *fitness;
};

cell	*ca_new_cell(int /*id*/, int /*num_features*/, int /*num_cells*/, int /*num_symbols*/);
void	ca_destroy_cell(cell */*a_cell*/);
char	*ca_toString_cell(cell */*a_cell*/);
void	ca_print_cell(FILE */*stream*/, cell */*a_cell*/);
void	ca_calculate_nn_homogeneity_cell(cell */*a_cell*/, float */*result*/);
void	ca_calculate_state_cell(cell */*a_cell*/);
#define	_CA_CELL_VISITED
#endif
