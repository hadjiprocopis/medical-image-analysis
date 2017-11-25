#ifndef clio_GUARDH
#define	clio_GUARDH
#include <Common_IMMA.h>
#include <matrix.h>

int	read_data_from_ASCII_file(
	char	*/*filename*/,
	int	/*numClusters*/,
	int	*/*numFeatures*/,
	int	*/*numPoints*/,
	clengine	**/*cl*/);
int	create_clengine_from_matrix_object(
	matrix *m,
	int numClusters,
	clengine **cl
);
	
int	read_data_from_UNC_file(
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

		/* if the files have been segmented with spm, then this is the probability maps
		   there are 3 prob maps (WM,GM,CSF) for any number of features, when you
		   are going to produce them with spm, segment all features at the same time */
	char	*spm_probability_mapsWM,
	char	*spm_probability_mapsGM,
	char	*spm_probability_mapsCSF,

	clengine	**/*cl*/ );
int	write_transformed_probability_maps_to_UNC_file(
	char		*/*filename*/,
	clengine	*/*cl*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
int	write_probability_maps_to_UNC_file(
	char		*/*filename*/,
	clengine	*/*cl*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
int	write_confidence_maps_to_UNC_file(
	char		*/*filename*/,
	clengine	*/*cl*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
int	write_masks_to_UNC_file(
	char		*/*filename*/,
	clengine	*/*cl*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
int	write_clusters_to_UNC_file(
	char		*/*filename*/,
	clengine	*/*cl*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
int	write_partial_volume_pixels_to_UNC_file(
	char		*/*filename*/,
	clengine	*/*cl*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
int	write_pixel_entropy_to_UNC_file(
	char		*/*filename*/,
	clengine	*/*cl*/,
	DATATYPE	/*minOutputPixel*/,
	DATATYPE	/*maxOutputPixel*/ );
void	write_clusters_to_ASCII_file(
	FILE		*/*handle*/,
	clengine	*/*cl*/);
int	write_points_to_ASCII_file(
	char	*/*filename*/,
	clengine */*cl*/);
#endif
