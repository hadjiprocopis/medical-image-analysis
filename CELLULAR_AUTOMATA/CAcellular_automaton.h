#ifndef	_CA_CELLULAR_AUTOMATON_VISITED

/* private */
typedef	struct	_CA_SORT_DATA {
	float	value;
	int	id;
} _ca_sort_data;
/* end private */

struct	_CA_CELLULAR_AUTOMATON {
	int		id;	/* optional cellular_automaton id */
	cell		**c;	/* list of ALL cells we are dealing with */
	symbol		**s;	/* list of ALL symbols we are dealing with */
	int		nC,	/* number of cells */
			nS,	/* number of symbols */
			nF;	/* number of features */

	clestats	*stats;
	clunc		*im;

	char		interruptFlag,	/* set this to false when you want to interrupt the process of clustering (actually this will be done through methods provided - see below) */
			weightsChangeFlag;	/* if true, it means that the weights have changed and the cell-to-cell distances have to be re-calculated */
			
	/* private scratch pads */
	_ca_sort_data	*_sort_data_distances, *_sort_data_symbols_nearest, *_sort_data_symbols_furthest;
	float		**_homogeneities;

	/* various constants */
	float		kT;

	float		totalEntropy;
	cluster		**clusters; /* the same size as number of symbols */
};

cellular_automaton	*ca_new_cellular_automaton(int /*id*/, int /*numFeatures*/, int /*numCells*/, int /*numSymbols*/);
void			ca_destroy_cellular_automaton(cellular_automaton */*a_cellular_automaton*/);
int			ca_calculate_statistics(cellular_automaton */*aca*/);
char			*ca_toString_cellular_automaton(cellular_automaton *);
void			ca_print_cellular_automaton(FILE *, cellular_automaton *);
int			ca_interrupt_cellular_automaton(cellular_automaton *, int);
void			ca_initialise_to_random_state(cellular_automaton */*aca*/);
int			ca_calculate_next_state(cellular_automaton */*aca*/);
int			ca_find_neighbours(cellular_automaton */*aca*/, int /*numNearestNeighbours*/, int /*numFurthestNeighbours*/);
int			ca_calculate_cell_to_cell_distances(cellular_automaton */*aca*/, cell */*a_cell*/);
int			ca_calculate_cell_to_cell_distances_with_weights(cellular_automaton */*aca*/, cell */*a_cell*/);
int			ca_normalise_points(cellular_automaton */*aca*/, float /*rangeMin*/, float /*rangeMax*/, char /*overAllFeaturesFlag*/);
void			ca_finalise_next_state(cellular_automaton */*aca*/);
#define _CA_CELLULAR_AUTOMATON_VISITED
#endif
