#include <IO.h>

#ifndef	TRUE
#define	TRUE	1
#endif
#ifndef	FALSE
#define	FALSE	0
#endif
#ifndef SQR
#define SQR(a)	((a)*(a))
#endif

typedef	struct	_ENTROPY_CLUSTERING_DATA {
	FILE	*pointsPrintHandle, *pointsPlotHandle,
		*clustersPrintHandle, **clustersPlotHandles,
		*weightsPrintHandle, *weightsPlotHandle,
		*statsPrintHandle, *statsPlotHandle,
		*finalClustersPrintHandle, *finalClustersPlotHandle;

	DATATYPE	***data; /* image specific */
	int		W, H;

	int	numClusters,
		numPoints,
		numFeatures,
		numIterations,
		iter,
		printEvery,	/* every so many iterations to print status */
		plotEvery;	/* every so many iterations to plot clusters */

	char	haveWeights,
		canDoClusteringFlag;	/* if this is somehow set to true while on the clustering loop, the loop will stop, useful in conjuction with a signal handler */

	float	meanTimePerIteration;
	long	totalProcessingTime;

	int	*points_assignment,
		*points_per_cluster,
		*nearest_point_to_a_cluster;
	float	*points_assignment_probabilities;

	float	**clusters,
		**points,

		**weights,
		***weighting_function_results,
		***der_weighting_function_wrt_c,
		***der_weighting_function_wrt_w,

		**distance,
		*S,
		*SQR_S,
		***der_d_wrt_c,
		***der_d_wrt_w,
		**der_S_wrt_w,
		SS,
		**der_SS_wrt_c,
		**der_SS_wrt_w,
		
		**t_d,
		****der_t_d_wrt_c,
		***der_t_d_wrt_w,
		*t_S,
		***der_t_S_wrt_c,
		**der_t_S_wrt_w,
		*SQR_t_S,
		t_SS,
		**der_t_SS_wrt_c,
		**der_t_SS_wrt_w,
		SQR_t_SS,

		**probability,
		****der_P_wrt_c,
		***der_P_wrt_w,

		*entropy,
		***der_e_wrt_c,
		**der_e_wrt_w,
		total_entropy,
		**der_t_e_wrt_c,
		**der_t_e_wrt_w,

		factor1,
		**der_factor1_wrt_c,
		**der_factor1_wrt_w,
		factor2,
		**der_factor2_wrt_c,
		**der_factor2_wrt_w,
		factor3,
		**der_factor3_wrt_c,
		**der_factor3_wrt_w,

		fitness,
		**der_fitness_wrt_c,
		**der_fitness_wrt_w,
		old_fitness,
		roc_fitness, roc_roc_fitness,
		old_roc_fitness, old_roc_roc_fitness,
		mean_fitness, mean_roc_fitness, mean_roc_roc_fitness,

		**cluster_changes,
		**weight_changes,
		**cluster_centroids,
		**actual_point_values_cluster_centroids,
		*minActualPointValues, *maxActualPointValues, /* the actual data values range (e.g. pixel values) */
		minNormalisedPointValues, maxNormalisedPointValues, /* the normalised range set by the user */
		A;

	int	bytesAllocated;

	float	DELTA_CLUSTERS,
		DELTA_WEIGHTS;
} entropyClData;

entropyClData	*new_entropy_clustering(DATATYPE ***/*data*/, int /*W*/, int /*H*/, int /*numFeatures*/, int /*numPoints*/, int /*numClusters*/, char /*haveWeights*/, int /*numIterations*/, int /*printEvery*/, int /*plotEvery*/, char */*pointsPlotBasename*/, char */*clustersPlotBasename*/, char */*weightsPlotBasename*/, char */*statsPlotBasename*/, char */*finalClustersFilename*/, FILE */*pointsPrintHandle*/, FILE */*clustersPrintHandle*/, FILE */*weightsPrintHandle*/, FILE */*statsPrintHandle*/, FILE */*finalClustersPrintHandle*/);
void		do_entropy_clustering(entropyClData */*myData*/);
void		delete_entropy_clustering(entropyClData */*myData*/);

/* put this here because it is needed by users */
int		entropy_clustering_normalise_points(entropyClData */*myData*/, float /*range_min*/, float /*range_max*/);
