#ifndef	clestats_GUARDH
#define	clestats_GUARDH

struct	_CLESTATS /*CLENGINE_STATISTICS*/ {
	vector	*mean_fitness,	/* one for each feature */
		*stdev_fitness;

	float	mean_density,
		mean_homogeneity,
		mean_compactness,

		stdev_density,
		stdev_homogeneity,
		stdev_compactness;

	float	penalty,
		separation_area,	/* take 3 clusters at a time, calculate the area of the triangle their 3 centroids form and then divide by their average compactness */
		separation_distance,	/* take 2 clusters at a time, calculate the distance between their centroids and then divide by their average compactness */
		total_entropy,
		total_distance;

	vector	*min_point_values,	/* the min and max pixel values for each feature */
		*max_point_values,	/* should be filled as soon as the data is read in */
		*min_point_values_normalised,	/* and their normalised equivalent */
		*max_point_values_normalised,
		*tantheta;		/* normalisation is the following formula:
						y = tantheta * (x-xmin) + ymin
						where y is the normalised value
						and x the un-normalised and
						x varies from xmin to xmax
						and y needs to be normalised
						within the range ymin to ymax
						tantheta = (ymax-ymin)/(xmax-xmin)
						tantheta is needed for normalisation
						and the opposite process of un-normalisation */
};

void		destroy_clestats(clestats *);
clestats	*new_clestats(int /*numFeatures*/);
clestats	*duplicate_clestats(clestats *);
void		copy_clestats(clestats */*c1*/, clestats */*c2*/);
char		*toString_clestats(clestats *);
void		print_clestats(FILE *, clestats *);
void		reset_clestats(clestats */*clit*/);
#endif
