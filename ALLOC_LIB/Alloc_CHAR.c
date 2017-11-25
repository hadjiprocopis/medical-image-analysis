#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include "Common_IMMA.h"
#include "Alloc.h"

/* Function to free a 2D array
   params: **data, 2D array (of type CHAR)
   	   [] d1, an integer denoting the width of the array, the first dimension
   returns: nothing
   example: if your array is data[200][500] then d1 is 200
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	freeCHAR2D(char **data, int d1){
	int	i;
	for(i=0;i<d1;i++) free(data[i]);
	free(data);
}
/* Function to free a 3D array
   params: ***data, 3D array (of type CHAR)
   	   [] d1, an integer denoting the first dimension of the array
   	   [] d2, an integer denoting the second dimension of the array
   returns: nothing
   example: if your array is data[200][500][100] then d1 is 200, d2 is 500
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	freeCHAR3D(char ***data, int d1, int d2){
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
   returns: a pointer to the allocated 2D array of type CHAR
   example: if your array is data[200][500] then d1 is 200 and d2 is 500
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
char 	**callocCHAR2D(int d1, int d2){
	char	**ret;
	int	i;

	if( (ret=(char **)calloc(d1, sizeof(char *))) == NULL ) return NULL;
	for(i=0;i<d1;i++)
		if( (ret[i]=(char *)calloc(d2, sizeof(char))) == NULL ){
			freeCHAR2D(ret, i);
			return NULL;
		}
	return ret;
}	
/* Function to allocate memory for a 3D array
   params: d1, an integer denoting the first dimension of the array
   	   [] d2, an integer denoting the second dimension of the array
   	   [] d3, an integer denoting the third dimension of the array
   returns: a pointer to the allocated 3D array of type CHAR
   example: if your array is data[200][500][100] then d1 is 200, d2 is 500 and d3 is 100
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
char 	***callocCHAR3D(int d1, int d2, int d3){
	char	***ret;
	int	i, j;

	if( (ret=(char ***)calloc(d1, sizeof(char **))) == NULL ) return NULL;
	for(i=0;i<d1;i++){
		if( (ret[i]=(char **)calloc(d2, sizeof(char *))) == NULL ){
			freeCHAR3D(ret, i, d2);
			return NULL;
		}
		for(j=0;j<d2;j++)
			if( (ret[i][j]=(char *)calloc(d3, sizeof(char))) == NULL ){
				freeCHAR3D(ret, i, d2);
				return NULL;
			}
	}
	return ret;
}	
