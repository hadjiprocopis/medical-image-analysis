#ifndef	point_GUARDH
#define	point_GUARDH
struct	_POINT {
	int		id;	/* unique id */

	float		x, y, z;/* x, y and z coordinates in image space (z is slice number) */
	int		s;	/* the slice number this point came from */
	vector		*v,	/* value in image space (e.g. pixel value) */
			*f,	/* the features of this point -- e.g. location in feature space */
			*p,	/* the ith entry = probability that point belongs to ith cluster */
			*d,	/* distances from each cluster, the ith entry = distance from ith cluster */
			*td,	/* transformed distances from each cluster */
			*w,	/* weights weighting each feature */
			*spm,	/* SPM probability maps (if any) for each WM, GM and CSF */
			*beta;	/* the transformed distances locality factor */

	float		t,	/* threshold - associated with the weights above */
			sD,	/* sum of distances from each cluster and sum of transformed distances */
			sTD,
			entropy;/* sum of (p log p) over all clusters */

	point		*n[8];	/* neighbours of this point in image space, [0] is top-left, moving clockwise */

	void		*data;	/* data structure optionally associated with this point */

	cluster		*c;	/* nearest cluster -- probability to belong to this cluster is greater */
};		

point	*new_point(int /*id*/, int /*numFeatures*/, int /*numClusters*/);
void	destroy_point(point */*a_point*/);
void	destroy_points(point **/*p*/, int /*numPoints*/);
point	*duplicate_point(point *);
void	copy_point(point */*p1*/, point */*p2*/);
void	reset_point(point */*p*/);
char	*toString_point(void */*point_but_void*/);
char	*toString_point_brief(void */*point_but_void*/);
void	print_point(FILE *, point *);
void	print_point_brief(FILE *, point *);
float	euclidean_distance_between_two_points(point */*p1*/, point */*p2*/);
int	statistics_of_points(point **/*data*/, int /*numPoints*/, vector */*min*/, vector */*max*/, vector */*mean*/, vector */*stdev*/);
void	statistics_point(point **/*p*/, int /*numPoints*/, point */*minP*/, point */*maxP*/, point */*meanP*/, point */*stdevP*/);
void	statistics_point_from_indices(int */*indices*/, int /*numPoints*/, point **/*all_points*/, point */*minP*/, point */*maxP*/, point */*meanP*/, point */*stdevP*/);
#endif
