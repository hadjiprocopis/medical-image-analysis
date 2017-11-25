#ifndef	clengine_GUARDH
#define	clengine_GUARDH

#undef	CLENGINE_VERBOSE /* no extra output ... */

struct	_CLENGINE {
	int		id;	/* optional clengine id */
	point		**p;	/* list of points we are dealing with */
	cluster		**c;	/* list of clusters we are dealing with */
	int		nP,	/* number of points */
			nC,	/* number of clusters */
			nF;	/* number of features */

	clestats	*stats;	/* clengine statistics object */

	clunc		*im;	/* details about the UNC image we are clustering */

	char		interruptFlag;	/* set this to false when you want to interrupt the process of clustering (actually this will be done through methods provided - see below) */
};

void		destroy_clengine(clengine */*a_clengine*/);
clengine	*new_clengine(int /*id*/, int /*numFeatures*/, int /*numPoints*/, int /*numClusters*/);
clengine	*duplicate_clengine(clengine *, int /*new_id*/);
void		copy_clengine(clengine */*c1*/, clengine */*c2*/);
char		*toString_clengine(void *);
void		print_clengine(FILE *, clengine *);
void		print_clengine_brief(FILE *, clengine *);
void		print_clengine_for_plotting(FILE *, clengine *);
void		calculate_clengine(clengine *);
void		calculate_clengine_stats(clengine *);
int		clengine_assign_points_to_clusters(clengine */*c*/);
int		clengine_assign_points_to_clusters_without_relocations(clengine */*c*/);
void		clengine_shake_clusters(clengine */*cle*/, float /*p*/); /* p: 0-1 */
void		clengine_print_clusters(FILE */*handle*/, clengine */*a_clengine*/);
void		clengine_print_points(FILE */*handle*/, clengine */*a_clengine*/);
void		clengine_unnormalise_clusters(clengine */*cl*/);
int		clengine_normalise_points(clengine */*cl*/, float /*rangeMin*/, float /*rangeMax*/, char /*overAllFeaturesFlag*/);
void		clengine_initialise_clusters(clengine */*cl*/, vector **/*clusters*/);
void		reset_clengine(clengine */*cl*/);
void		clengine_sort_clusters_wrt_pixelvalue_ascending(clengine */*cl*/);
void		clengine_sort_clusters_wrt_pixelvalue_descending(clengine */*cl*/);
void		clengine_sort_clusters_wrt_normalisedpixelvalue_ascending(clengine */*cl*/);
void		clengine_sort_clusters_wrt_normalisedpixelvalue_descending(clengine */*cl*/);
void		clengine_shake_clusters(clengine */*cl*/, float/*percentage_noise*/);
void		clengine_save_clusters(clengine */*cl*/, vector **/*toSave*/, int **/*ids*/);
void		clengine_load_clusters(clengine */*cl*/, vector **/*toLoad*/, int */*ids*/);

void		clengine_interrupt(clengine */*cl*/, int /*flag*/);

int		clengine_do_markov_chain_monte_carlo_once(clengine */*cle*/, int */*pAllocation*/);
int		clengine_do_simulated_annealing_once(clengine */*cle*/, float /*cluster_noise_range*/, float /*beta_noise_range*/);
#endif
