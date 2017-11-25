#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include "Common_IMMA.h"
#include "Alloc.h"

/* Function to free a 2D array
   params: **data, 2D array (of type DATATYPE)
   	   [] d1, an integer denoting the width of the array, the first dimension
   returns: nothing
   example: if your array is data[200][500] then d1 is 200
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	freeDATATYPE2D(DATATYPE **data, int d1){
	int	i;
	for(i=0;i<d1;i++) free(data[i]);
	free(data);
}
/* Function to free a 3D array
   params: ***data, 3D array (of type DATATYPE)
   	   [] d1, an integer denoting the first dimension of the array
   	   [] d2, an integer denoting the second dimension of the array
   returns: nothing
   example: if your array is data[200][500][100] then d1 is 200, d2 is 500
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	freeDATATYPE3D(DATATYPE ***data, int d1, int d2){
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
   returns: a pointer to the allocated 2D array of type DATATYPE
   example: if your array is data[200][500] then d1 is 200 and d2 is 500
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE 	**callocDATATYPE2D(int d1, int d2){
	DATATYPE	**ret;
	int		i;

	if( (ret=(DATATYPE **)calloc(d1, sizeof(DATATYPE *))) == NULL ) return NULL;
	for(i=0;i<d1;i++)
		if( (ret[i]=(DATATYPE *)calloc(d2, sizeof(DATATYPE))) == NULL ){
			freeDATATYPE2D(ret, i);
			return NULL;
		}
	return ret;
}	
/* Function to allocate memory for a 3D array
   params: d1, an integer denoting the first dimension of the array
   	   [] d2, an integer denoting the second dimension of the array
   	   [] d3, an integer denoting the third dimension of the array
   returns: a pointer to the allocated 3D array of type DATATYPE
   example: if your array is data[200][500][100] then d1 is 200, d2 is 500 and d3 is 100
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE 	***callocDATATYPE3D(int d1, int d2, int d3){
	DATATYPE	***ret;
	int		i, j;

	if( (ret=(DATATYPE ***)calloc(d1, sizeof(DATATYPE **))) == NULL ) return NULL;
	for(i=0;i<d1;i++){
		if( (ret[i]=(DATATYPE **)calloc(d2, sizeof(DATATYPE *))) == NULL ){
			freeDATATYPE3D(ret, i, d2);
			return NULL;
		}
		for(j=0;j<d2;j++)
			if( (ret[i][j]=(DATATYPE *)calloc(d3, sizeof(DATATYPE))) == NULL ){
				freeDATATYPE3D(ret, i, d2);
				return NULL;
			}
	}
	return ret;
}	

/* Function to free a 2D array
   params: **data, 2D array (of type void)
   	   [] d1, an integer denoting the width of the array, the first dimension
   returns: nothing
   example: if your array is data[200][500] then d1 is 200
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	free2D(void **data, int d1){
	int	i;
	for(i=0;i<d1;i++) free(data[i]);
	free(data);
}
/* Function to free a 3D array
   params: ***data, 3D array (of type void)
   	   [] d1, an integer denoting the first dimension of the array
   	   [] d2, an integer denoting the second dimension of the array
   returns: nothing
   example: if your array is data[200][500][100] then d1 is 200, d2 is 500
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	free3D(void ***data, int d1, int d2){
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
   returns: a pointer to the allocated 2D array of type void
   example: if your array is data[200][500] then d1 is 200 and d2 is 500
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	**calloc2D(int d1, int d2, int size){
	void	**ret;
	int	i;

	if( (ret=(void **)calloc(d1, sizeof(int *))) == NULL ) return NULL;
	for(i=0;i<d1;i++)
		if( (ret[i]=(void *)calloc(d2, size)) == NULL ){
			free2D(ret, i);
			return NULL;
		}
	return ret;
}	
/* Function to allocate memory for a 3D array
   params: d1, an integer denoting the first dimension of the array
   	   [] d2, an integer denoting the second dimension of the array
   	   [] d3, an integer denoting the third dimension of the array
   returns: a pointer to the allocated 3D array of type void
   example: if your array is data[200][500][100] then d1 is 200, d2 is 500 and d3 is 100
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void 	***calloc3D(int d1, int d2, int d3, int size){
	void	***ret;
	int	i, j;

	if( (ret=(void ***)calloc(d1, sizeof(int **))) == NULL ) return NULL;
	for(i=0;i<d1;i++){
		if( (ret[i]=(void **)calloc(d2, sizeof(int *))) == NULL ){
			free3D(ret, i, d2);
			return NULL;
		}
		for(j=0;j<d2;j++)
			if( (ret[i][j]=(void *)calloc(d3, size)) == NULL ){
				free3D(ret, i, d2);
				return NULL;
			}
	}
	return ret;
}	

