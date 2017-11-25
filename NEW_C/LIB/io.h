#include <Alloc.h>
#include <IO.h>
#include <filters.h>

void    _entropy_clustering_print_point(entropyClData */*myData*/, int /*i*/);
void    _entropy_clustering_print_points(entropyClData */*myData*/);
void    _entropy_clustering_print_clusters(entropyClData */*myData*/);
void    _entropy_clustering_print_cluster(entropyClData */*myData*/, int /*i*/);
void    _entropy_clustering_print_final_clusters(entropyClData */*myData*/);
void    _entropy_clustering_plot_final_clusters(entropyClData */*myData*/);
void    _entropy_clustering_plot_points(entropyClData */*myData*/);
void    _entropy_clustering_plot_clusters(entropyClData */*myData*/);
void    _entropy_clustering_plot_weights(entropyClData */*myData*/);
void    _entropy_clustering_plot_stats(entropyClData */*myData*/);
void    _entropy_clustering_print_stats(entropyClData */*myData*/);
void    _entropy_clustering_print_weights(entropyClData */*myData*/);
int	entropy_clustering_write_clusters_to_UNC_file(char *filename, entropyClData *myData, int W, int H);
int	entropy_clustering_write_probability_maps_to_UNC_file(char *filename, entropyClData *myData, int W, int H, DATATYPE range);
int	entropy_clustering_read_points_from_UNC_file(char *filename, entropyClData *myData, int *W, int *H);
