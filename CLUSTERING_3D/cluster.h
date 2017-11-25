#ifndef cluster_GUARDH
#define cluster_GUARDH
/* the number of random samples to be taken
   from the cluster's volume in order to estimate
   its density
*/
#define	DENSITY_CALCULATION_ITERATIONS		200

/* the cluster's density calculation involves a
   hypercube with side h[i] in the ith dimension
   and counting how many points (of the cluster)
   fall into that hcube. The fraction :
   h[i] / (max[i] - min[i]) is what we define here.
   min[i] and max[i] define a bounding rectangle
   around the cluster members (e.g. min[i] is the
   minimum of all point coordinates, in the ith dimension)
*/
#define	DENSITY_CALCULATION_CUBESIDE_FRACTION	0.2

struct	_CLUSTER {
	int		id;	/* optional cluster id */

	int		*points;/* an array of indices to points nearest to this cluster */
	int		n; /* number of points in this cluster */

	point		*centroid, /* its centroid */
			**all_points;	/* a pointer to the array of all points */
	cstats		*stats;	/* cluster statistics object */
	float		beta;	/* the transformed distance locality factor - not that point.h has one beta as
				   well, one or the other could be used */

	clengine	*cl;	/* our clengine (parent)*/
};

void	destroy_cluster(cluster */*a_cluster*/);
void	destroy_clusters(cluster **/*clusters*/, int /*numClusters*/);
cluster	*new_cluster(int /*id*/, int /*numFeatures*/, int /*totalNumPoints*/, int /*totalNumClusters*/);
char	*toString_cluster_brief(void */*a_cluster_but_void*/);
char	*toString_cluster(void */*a_cluster_but_void*/);
cluster	*duplicate_cluster(cluster */*a_cluster*/, int /*new_id*/);
void	copy_cluster(cluster */*c1*/, cluster */*c2*/);
void	print_cluster(FILE *, cluster *);
void	print_cluster_brief(FILE *, cluster *);
void	calculate_cluster(cluster *);
void	calculate_cluster_stats(cluster *);
void	shake_cluster(cluster */*c*/, float /*p*/); /* p: 0-1 */
void	reset_cluster(cluster */*a_cluster*/);
float	area_of_triangle_defined_by_centroids_of_three_clusters(vector */*c1*/, vector */*c2*/, vector */*c3*/);
void	cluster_save_clusters(cluster */*c*/, vector */*toSave*/);
void	cluster_load_clusters(cluster */*c*/, vector */*toLoad*/);
#endif
