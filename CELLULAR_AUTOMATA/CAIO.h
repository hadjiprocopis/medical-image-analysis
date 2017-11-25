#ifndef	_CA_IO_VISITED
int	ca_read_data_from_UNC_file(
	char	**/*filenames*/,
	int	/*numFeatures*/,
	int	/*x*/,
	int	/*y*/,
	int	*/*w*/,
	int	*/*h*/,
	int	/*numClusters*/,
	int	*/*slices*/,
	int	*/*numSlices*/, /* to be returned back */
	float	/*sliceSeparation*/,
	DATATYPE	/*minThreshold*/,
	DATATYPE	/*maxThreshold*/,
	cellular_automaton **/*cl*/ );
int     ca_write_masks_to_UNC_file(
	char	    */*filename*/,
	cellular_automaton      */*aca*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
int     ca_write_probs_to_UNC_file(
	char	    */*filename*/,
	cellular_automaton      */*aca*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
int     ca_write_counts_to_UNC_file(
	char	    */*filename*/,
	cellular_automaton      */*aca*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
int     ca_write_states_to_UNC_file(
	char	    */*filename*/,
	cellular_automaton      */*aca*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
int     ca_write_fitness_to_UNC_file(
	char	    */*filename*/,
	cellular_automaton      */*aca*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
void	ca_print_cells_and_neighbours(FILE */*stream*/, cellular_automaton */*aca*/);

#define	_CA_IO_VISITED
#endif
