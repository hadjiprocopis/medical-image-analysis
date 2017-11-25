#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Common_IMMA.h"
#include "Alloc.h"
#include "IO.h"
#include "filters.h"
#include "LinkedList.h"

#include "clustering.h"

vector	*new_vector(int numElements){
	vector	*v;

	if( (v=(vector *)malloc(sizeof(vector))) == NULL ){
		fprintf(stderr, "new_vector : could not allocate %zd bytes (1).\n", sizeof(vector));
		return NULL;
	}
	if( (v->c=(float *)malloc(numElements * sizeof(float))) == NULL ){
		fprintf(stderr, "new_vector : could not allocate %zd bytes (2).\n", numElements * sizeof(float));
		free(v);
		return NULL;
	}
	v->nC = numElements;

	reset_vector(v);

	return v;
}
vector	**new_vectors(int numElements, int numVectors){
	int	i;
	vector	**v;

	if( (v=(vector **)malloc(numVectors * sizeof(vector *))) == NULL ){
		fprintf(stderr, "new_vectors : could not allocate %zd bytes (1).\n", sizeof(vector *) * numVectors);
		return NULL;
	}
	for(i=0;i<numVectors;i++){
		if( (v[i]=new_vector(numElements)) == NULL ){
			fprintf(stderr, "new_vectors : call to new_vector failed (%d).\n", i);
			destroy_vectors(v, i);
			return NULL;
		}
	}
	return v;
}
void	destroy_vector(vector *v){
	if( v->c != NULL ) free(v->c);
	free(v);
}
void	destroy_vectors(vector **v, int numVectors){
	int	i;
	for(i=0;i<numVectors;i++) destroy_vector(v[i]);
	free(v);
}
void	copy_vector(vector *v1, vector *v2){
	/* copies the contents of v1 onto v2 -- no allocation takes place here */
	int	i;
	for(i=0;i<v1->nC;i++) v2->c[i] = v1->c[i];
}
void	copy_vectors(vector **v1, vector **v2, int numVectors){
	/* copies the contents of v1 onto v2 -- no allocation takes place here */
	int	i, j;
	vector	**V1, **V2;

	for(j=0,V1=&(v1[0]),V2=&(v2[0]);j<numVectors;j++,V1++,V2++)
		for(i=0;i<(*V1)->nC;i++) (*V2)->c[i] = (*V1)->c[i];
}
vector	*duplicate_vector(vector *a_vector){
	vector	*dup;

	if( (dup=new_vector(a_vector->nC)) == NULL ){
		fprintf(stderr, "duplicate_vector : call to new_vector has failed.\n");
		return NULL;
	}
	copy_vector(a_vector, dup);
	return dup;
}

char	*toString_vector(vector *a_vector){
	if( a_vector->nC > 0 ){
		int	i;
		char	*s, *dummy, *ret;

		if( (s=(char *)malloc(a_vector->nC*30)) == NULL ){
			fprintf(stderr, "toString_vector : could not allocate %d bytes.\n", a_vector->nC*30);
			return strdup("<null vector>");
		}
		s[0] = '\0';
		for(i=0;i<a_vector->nC;i++){
			dummy = &(s[strlen(s)]);
			sprintf(dummy, "%f ", a_vector->c[i]);
		}
		ret = strdup(s);
		free(s);
		return ret;
	}
	return strdup("<empty>");
}

void	print_vector(FILE *handle, vector *a_vector){
	int	i;

	fprintf(handle, "(");
	for(i=0;i<a_vector->nC-1;i++)
		fprintf(handle, "%f, ", a_vector->c[i]);
	fprintf(handle, "%f)\n", a_vector->c[i]);
}

/* sets all the vector elements to zero */
void	reset_vector(vector *a_vector){
	int	i;
	for(i=0;i<a_vector->nC;i++) a_vector->c[i] = 0.0;
}

/* adds 2 vectors, result goes to first */
/* if the 2 vectors are not of the same dimensionality you get coredump */
void	add_vectors(vector *v1, vector *v2){
	int	i;
	for(i=0;i<v1->nC;i++) v1->c[i] += v2->c[i];
}
/* adds 1 + second*multiply , result goes to first */
/* if the 2 vectors are not of the same dimensionality you get coredump */
void	add_vectors_multiply(vector *v1, vector *v2, vector *mult){
	int	i;
	for(i=0;i<v1->nC;i++) v1->c[i] += v2->c[i] * mult->c[i];
}
/* adds 1 + second*constant , result goes to first */
/* if the 2 vectors are not of the same dimensionality you get coredump */
void	add_vectors_multiply_constant(vector *v1, vector *v2, float c){
	int	i;
	for(i=0;i<v1->nC;i++) v1->c[i] += v2->c[i] * c;
}
/* subtracts 2 vectors, result goes to first v1 = v1 - v2 */
/* if the 2 vectors are not of the same dimensionality you get coredump */
void	subtract_vectors(vector *v1, vector *v2){
	int	i;
	for(i=0;i<v1->nC;i++) v1->c[i] -= v2->c[i];
}
/* multiplies a vector by a constant */
void	multiply_vector_by_constant(vector *v1, float c){
	int	i;
	for(i=0;i<v1->nC;i++) v1->c[i] *= c;
}
/* returns TRUE if the two vectors are element-by-element equal
   or FALSE if not or if the number of their elements is not the same */
int	equal_vectors(vector *v1, vector *v2){
	int	i;
	if( v1->nC != v2->nC ) return FALSE;
	for(i=0;i<v1->nC;i++)
		if( v1->c[i] != v2->c[i] ) return FALSE;
	return TRUE;
}
	
/* find the euclidean distance between 2 vectors */
/* f the 2 vectors are not of the same dimensionality you get coredump */
float	euclidean_distance_between_two_vectors(vector *v1, vector *v2){
	int	i;
	float	sum = 0.0;
	for(i=0;i<v1->nC;i++) sum += SQR(v1->c[i] - v2->c[i]);
	return sqrt(sum);
}
/* variation with the above, but each vector has weights and thresholds */
float	euclidean_distance_between_two_vectors_with_weights(vector *v1, vector *w1, float t1, vector *v2, vector *w2, float t2){
	int	i;
	float	sum = 0.0;
	for(i=0;i<v1->nC;i++) sum += SQR(w1->c[i] * v1->c[i] + t1 - w2->c[i] * v2->c[i] - t2);
	return sqrt(sum);
}

/* Given two vectors a and b (e.g. points in Ndimensions) a line can be defined
   and the distance of another vector p from that line is returned */
float	euclidean_distance_of_vector_from_line(vector *p, vector *a, vector *b){
	/* parameteric equation of line joining a (a is a vector [a1,a2,...,aN]) and b :
		L = a + (b-a) * t
		Generic distance expression of p from any point of L is
			r^2 = (a1 + (b1-a1)*t - p1)^2 +
			     +(a2 + (b2-a2)*t - p2)^2 +
			     + ... +
			     +(aN + (bN-aN)*t - pN)^2

			    = Sum_i=1 to N [ (a_i + (b_i - a_i) * t - p_i)^2 ]

		The minimum distance is then given by
			dr^2/dt = 0
			e.g.
			dr^2/dt = Sum_i=1 to N [2 * (a_i + (b_i - a_i) * t - p_i) * (b_i - a_i) ]
				= 0 and solve for t

			=>    t = Sum_i [ (a_i - p_i) * (b_i - p_i) ] / Sum_i [ (b_i - a_i)^2 ]
			( we will call f the numerator and g the denominator)
			now substitute back to distance equation (r^2)
			distance = sqrt(r^2 when t is subst)
			...
			*/
	float	t, f, g, distance;
	int	i, N = p->nC;

	/* step 1: find t */
	for(i=0,f=0.0,g=0.0;i<N;i++){
		f += (a->c[i] - p->c[i]) * (b->c[i] - p->c[i]);
		g += SQR(b->c[i] - a->c[i]);
	}
	/* if points coincide, g will be zero -> better check this before hand */
	/* you can do this by calling equal_vectors(v1, v2) */
	t = f / g;

	/* step 2: substitute t into distance equation (r^2) */
	for(i=0,distance=0.0;i<N;i++)
		distance += SQR(a->c[i] + (b->c[i]-a->c[i]) * t - p->c[i]);

	return sqrt(distance);
}

/* it will calculate various statistics (min/max/mean/stdev) on a set of vectors,
   element-wise (e.g. for each dimension) and place them
   in the supplied vectors
*/
void	statistics_of_vectors(vector **v, int nV, vector *min, vector *max, vector *mean, vector *stdev){
	int	i, j;

	float	*vMin = &(min->c[0]),
		*vMax = &(max->c[0]),
		*vMean= &(mean->c[0]);

	for(i=0;i<v[0]->nC;i++,vMin++,vMax++,vMean++){
		*vMin = *vMax = v[0]->c[i]; *vMean = 0.0;
		for(j=0;j<nV;j++){
			*vMin = MIN(*vMin, v[j]->c[i]);
			*vMax = MAX(*vMax, v[j]->c[i]);
			*vMean += v[j]->c[i];
		}
		mean->c[i] /= (float )nV;
		stdev->c[i] = 0.0;
		for(j=0;j<nV;j++) stdev->c[i] += SQR(v[j]->c[i] - mean->c[i]);
		stdev->c[i] = sqrt(stdev->c[i] / ((float )nV));
	}
}
