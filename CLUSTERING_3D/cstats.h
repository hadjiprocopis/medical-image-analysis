#ifndef	cstats_GUARDH
#define	cstats_GUARDH
struct	_CSTATS /*CLUSTER_STATISTICS*/ {
	int	numFeatures,	/* num of features */
		numClusters,	/* num of clusters */
		numPoints;	/* total number of points (not the same as the number of points belong to this cluster */

	point	*nP,	/* nearest point to this cluster's centroid from the set of this cluster's elements */
		*fP;	/* farthest point */
	cluster	*nC,	/* nearest cluster (centroid to centroid) from the set of all clusters in this state */
		*fC;	/* farthest cluster */
	float	nPD,	/* distances from this cluster to nearest and furthest POINTS (belonging to this cluster) */
		fPD,
		nCD,	/* distances from this cluster to nearest and furthest CLUSTERS */
		fCD;
	float	compactness,	/* the mean of the distances of all points of this cluster from its centroid - NOTE the smaller the better thus it should have been called 'spread' and not 'compactness' */
		homogeneity,	/* the stdev of the distances of all points of this cluster from its centroid - the smaller the better */
		density,	/* the density of the points around the cluster calculated dodgily using random sampling */
		penalty;
	vector	*fitness;	/* the standard deviation of pixel values (for each feature) of all the points in this cluster - the smaller the better *but not zero* */

	point	*min_points,	/* min, max, mean and stdev of point properties */
		*max_points,
		*mean_points,
		*stdev_points;
};

void	destroy_cstats(cstats *);
cstats	*new_cstats(int /*numFeatures*/, int /*numPoints*/, int /*numClusters*/);
cstats	*duplicate_cstats(cstats *);
void	copy_cstats(cstats */*c1*/, cstats */*c2*/);
char	*toString_cstats(cstats *);
char	*toString_cstats_brief(cstats *);
void	print_cstats(FILE *, cstats *);
void	reset_cstats(cstats *);
#endif
