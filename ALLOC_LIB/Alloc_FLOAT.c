#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include "Common_IMMA.h"
#include "Alloc.h"

/* Function to free a 2D array
   params: **data, 2D array (of type float)
   	   [] d1, an integer denoting the width of the array, the first dimension
   returns: nothing
   example: if your array is data[200][500] then d1 is 200
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	freeFLOAT2D(float **data, int d1){
	int	i;
	for(i=0;i<d1;i++) free(data[i]);
	free(data);
}
/* Function to free a 3D array
   params: ***data, 3D array (of type float)
   	   [] d1, an integer denoting the first dimension of the array
   	   [] d2, an integer denoting the second dimension of the array
   returns: nothing
   example: if your array is data[200][500][100] then d1 is 200, d2 is 500
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	freeFLOAT3D(float ***data, int d1, int d2){
	int	i, j;
	for(i=0;i<d1;i++){
		for(j=0;j<d2;j++) free(data[i][j]);
		free(data[i]);
	}
	free(data);
}
/* Function to allocate memory for a 2D array
   params: d1, an integer denoting the first dimension of the array
   	   [] d2, an integer denoting the second dimension of the array
   returns: a pointer to the allocated 2D array of type float
   example: if your array is data[200][500] then d1 is 200 and d2 is 500
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
float 	**callocFLOAT2D(int d1, int d2){
	float	**ret;
	int		i;

	if( (ret=(float **)calloc(d1, sizeof(float *))) == NULL ) return NULL;
	for(i=0;i<d1;i++)
		if( (ret[i]=(float *)calloc(d2, sizeof(float))) == NULL ){
			freeFLOAT2D(ret, i);
			return NULL;
		}
	return ret;
}	
/* Function to allocate memory for a 3D array
   params: d1, an integer denoting the first dimension of the array
   	   [] d2, an integer denoting the second dimension of the array
   	   [] d3, an integer denoting the third dimension of the array
   returns: a pointer to the allocated 3D array of type float
   example: if your array is data[200][500][100] then d1 is 200, d2 is 500 and d3 is 100
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
float 	***callocFLOAT3D(int d1, int d2, int d3){
	float	***ret;
	int		i, j;

	if( (ret=(float ***)calloc(d1, sizeof(float **))) == NULL ) return NULL;
	for(i=0;i<d1;i++){
		if( (ret[i]=(float **)calloc(d2, sizeof(float *))) == NULL ){
			freeFLOAT3D(ret, i, d2);
			return NULL;
		}
		for(j=0;j<d2;j++)
			if( (ret[i][j]=(float *)calloc(d3, sizeof(float))) == NULL ){
				freeFLOAT3D(ret, i, d2);
				return NULL;
			}
	}
	return ret;
}	
