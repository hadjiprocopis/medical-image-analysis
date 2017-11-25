#ifndef vector_GUARDH
#define	vector_GUARDH

struct	_VECTOR {
	float	*c;	/* the contents of this vector */
	int	nC;	/* the number of elements in the vector */
};

void	destroy_vector(vector */*a_vector*/);
void	destroy_vectors(vector **/*v*/, int /*numVectors*/);
vector	*new_vector(int /*numElements*/);
vector	**new_vectors(int /*numElements*/, int /*numVectors*/);
vector	*duplicate_vector(vector *);
void	copy_vector(vector */*v1*/, vector */*v2*/);
void	copy_vectors(vector **/*v1*/, vector **/*v2*/, int /*numVectors*/);
char	*toString_vector(vector *);
int	equal_vectors(vector */*v1*/, vector */*v2*/);
void	print_vector(FILE *, vector *);
void	reset_vector(vector *);
void	add_vectors(vector *, vector *);
void	add_vectors_multiply(vector *, vector *, vector *);
void	add_vectors_multiply_constant(vector *, vector *, float);
void	subtract_vectors(vector *, vector *);
void	multiply_vector_by_constant(vector *, float);
float	euclidean_distance_between_two_vectors(vector *, vector *);
float	euclidean_distance_between_two_vectors_with_weights(vector *, vector *, float, vector *, vector *, float);
float	euclidean_distance_of_vector_from_line(vector */*p*/, vector */*a*/, vector */*b*/);
float	distance_metric_vectors(vector */*v*/, vector */*c*/, vector */*w*/, vector */*t*/);
void	statistics_of_vectors(vector **/*vectors*/, int /*numVectors*/, vector */*min*/, vector */*max*/, vector */*mean*/, vector */*stdev*/);
#endif
