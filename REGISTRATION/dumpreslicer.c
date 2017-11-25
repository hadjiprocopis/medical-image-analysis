/* Copyright 1995 Roger P. Woods, M.D. */
/* Modified 12/12/95 */
/* Modified 23/08/2003 Andreas Hadjiprocopis, ION */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Common_IMMA.h>
#include <LinkedList.h>
#include <LinkedListIterator.h>
#include <registration.h>

/* private definitions for our convenience */
typedef	struct  _POINT {
	int	x, y, z;
} _point;
typedef struct _COORDS_DATA_STRUCT {
	_point	from, to;
	float	distance;
	_point	closest;
} _coords_data_struct;

#define __X     0
#define __Y     1
#define __Z     2

int	dumpNNreslicer_AIR308(
	int		nativeImageDimensions[],
	int		registeredImageDimensions[],
	double		**es,
	int 		**inputCoordinates,	/* input coordinates in native space - origin is top-left corner (e.g. dispunc) */
	int		**outputCoordinates,	/* output coordinates in registered space - origin the same as above (e.g. dispunc) */
	int		numCoordinates)		/* the number of coordinates to convert */
{
	register int 	i,j,k;
	int		x_max1,y_max1,z_max1;
	int		x_dim2,y_dim2,z_dim2;
	int		x_up;
	int		y_up;
	int		z_up;
	int		x_in;
	double		x_i,x_j,x_k,x_p;
	double		y_i,y_j,y_k,y_p;
	double		z_i,z_j,z_k,z_p;
	double		t_i,t_j,t_k;
	double		e00,e01,e02,e03,e10,e11,e12,e13,e20,e21,e22,e23,e30,e31,e32,e33;

	linkedlist_iterator		*ll_iterator;
	
	_coords_data_struct		**coords_data;
	linkedlist			***myLinkedLists, *a_LinkedList;
	register _coords_data_struct	**pS, *a_pS, **dataToFree;
	register float			a_distance;
	register int			foundSoFar;
	int				numUniqueCoordinates = 0, corrected,
					numLinkedLists;
	char				name[100];

	if( (myLinkedLists=(linkedlist ***)malloc(nativeImageDimensions[__Z]*sizeof(linkedlist **))) == NULL ){
		fprintf(stderr, "dumpNNreslicer_AIR308 : could not allocate %zd bytes for wrapper.\n", nativeImageDimensions[__Z]*sizeof(linkedlist **));
		return FALSE;
	}
	for(i=0;i<nativeImageDimensions[__Z];i++){
		if( (myLinkedLists[i]=(linkedlist **)malloc(nativeImageDimensions[__Y]*sizeof(linkedlist *))) == NULL ){
			fprintf(stderr, "dumpNNreslicer_AIR308 : could not allocate %zd bytes for wrapper.\n", nativeImageDimensions[__Y]*sizeof(linkedlist *));
			return FALSE;
		}
		for(j=0;j<nativeImageDimensions[__Y];j++) myLinkedLists[i][j] = NULL;
	}
	if( (coords_data=(_coords_data_struct **)malloc(numCoordinates*sizeof(_coords_data_struct *))) == NULL ){
		fprintf(stderr, "dumpNNreslicer_AIR308 : could not allocate %zd bytes for coords_data.\n", numCoordinates*sizeof(_coords_data_struct *));
		return FALSE;
	}
	if( (dataToFree=(_coords_data_struct **)malloc(numCoordinates*sizeof(_coords_data_struct *))) == NULL ){
		fprintf(stderr, "dumpNNreslicer_AIR308 : could not allocate %zd bytes for dataToFree.\n", numCoordinates*sizeof(_coords_data_struct *));
		return FALSE;
	}
	for(i=0,pS=&(coords_data[0]);i<numCoordinates;i++,pS++) *pS = NULL;
	for(i=0,numLinkedLists=0,pS=&(coords_data[0]);i<numCoordinates;i++,pS++){
		if( *pS == NULL ){
			/* allocate */
			if( (*pS=(_coords_data_struct *)malloc(sizeof(_coords_data_struct))) == NULL ){
				fprintf(stderr, "dumpNNreslicer_AIR308 : could not allocate %zd bytes for coords_data[%d].\n", sizeof(_coords_data_struct), i);
				return FALSE;
			}
			/* index of input/output coordinates [0|1|2][i]
			   0|1|2 for X, Y, Z (use __X etc)
			   and i is the line number */
			(*pS)->from.x = inputCoordinates[__X][i];
			(*pS)->from.y = inputCoordinates[__Y][i]; /* y-axis starts from bottom rather than top (dispunc) */
			corrected = nativeImageDimensions[__Y]-1-(*pS)->from.y; /* we will correct it later */
			(*pS)->from.z = inputCoordinates[__Z][i];
			(*pS)->distance = 1000000000.0; /* set to an astronomical distance */
			/* no registered-space point yet, nor closest point */
			(*pS)->to.x = (*pS)->to.y = (*pS)->to.z = -1;
			(*pS)->closest.x = (*pS)->closest.y = (*pS)->closest.z = 0;

			/* note it so that we free it later */
			dataToFree[numUniqueCoordinates++] = *pS;

			if( myLinkedLists[(*pS)->from.z][corrected] == NULL ){
				sprintf(name, "dumpNNreslicer_AIR308(*,%d,%d)", corrected, (*pS)->from.z);
				if( (myLinkedLists[(*pS)->from.z][corrected]=linkedlist_new(name)) == NULL ){
					fprintf(stderr, "dumpNNreslicer_AIR308 : call to linkedlist_new has failed for '%s'.\n", name);
					return FALSE;
				}
				numLinkedLists++;
			}
			if( linkedlist_add_item(myLinkedLists[(*pS)->from.z][corrected], (void *)(*pS), NULL, NULL) == FALSE ){
				fprintf(stderr, "dumpNNreslicer_AIR308 : call to linkedlist_add_item has failed for %d item (of %d).\n", i, numCoordinates);
				return FALSE;
			}
			/* now check in the input coordinates if any point is identical to this. if it is then just copy it there */
			for(j=i;j<numCoordinates;j++){
				if( ((*pS)->from.x==inputCoordinates[__X][j]) && ((*pS)->from.y==inputCoordinates[__Y][j]) && ((*pS)->from.z==inputCoordinates[__Z][j]) ){
					coords_data[j] = *pS;
					//printf("XX (%d,%d,%d)\n", coords_data[j]->from.x, coords_data[j]->from.y, coords_data[j]->from.z);
				}
			}
		}
	}
	//for(i=0;i<numCoordinates;i++) printf("YY (%d,%d,%d)\n", coords_data[i]->from.x, coords_data[i]->from.y, coords_data[i]->from.z);

	/* now correct the y dimension, AIR thinks it starts from bottom left but input coordinates have the y-axis starting from top-left ... */
	for(i=0;i<nativeImageDimensions[__Z];i++) for(j=0;j<nativeImageDimensions[__Y];j++){
		if( (a_LinkedList=myLinkedLists[i][j]) != NULL ){
			for(ll_iterator=linkedlist_iterator_init(a_LinkedList);linkedlist_iterator_has_more(ll_iterator);){
				a_pS = (_coords_data_struct *)linkedlist_iterator_next(ll_iterator);
				a_pS->from.y = nativeImageDimensions[__Y] - a_pS->from.y;
				//printf("LL (%d,%d,%d)\n", a_pS->from.x, a_pS->from.y, a_pS->from.z);
			}
		}
	}
	//for(i=0,pS=&(coords_data[0]);i<numCoordinates;i++,pS++) printf("CO (%d,%d,%d)\n", (*pS)->from.x, (*pS)->from.y, (*pS)->from.z);

	x_max1=nativeImageDimensions[__X] - 1;
	y_max1=nativeImageDimensions[__Y] - 1;
	z_max1=nativeImageDimensions[__Z] - 1;

	x_dim2=registeredImageDimensions[__X];
	y_dim2=registeredImageDimensions[__Y];
	z_dim2=registeredImageDimensions[__Z];

	e00=es[0][0];
	e01=es[0][1];
	e02=es[0][2];
	e03=es[0][3];
	e10=es[1][0];
	e11=es[1][1];
	e12=es[1][2];
	e13=es[1][3];
	e20=es[2][0];
	e21=es[2][1];
	e22=es[2][2];
	e23=es[2][3];
	e30=es[3][0];
	e31=es[3][1];
	e32=es[3][2];
	e33=es[3][3];

	fprintf(stderr, "(%d linkedlists)", numLinkedLists); fflush(stderr);
	foundSoFar = 0;
	/* i, j, k is the new volume coordinates (after reslice),
	   x_up, y_up, z_up are the original coordinates (before reslice) */
	for (k=0,x_k=e30,y_k=e31,z_k=e32,t_k=e33;k<z_dim2;k++,x_k+=e20,y_k+=e21,z_k+=e22,t_k+=e23){
		for (j=0,x_j=x_k,y_j=y_k,z_j=z_k,t_j=t_k;j<y_dim2;j++,x_j+=e10,y_j+=e11,z_j+=e12,t_j+=e13){
			x_in=0;
			for (i=0,x_i=x_j,y_i=y_j,z_i=z_j,t_i=t_j;i<x_dim2;i++,x_i+=e00,y_i+=e01,z_i+=e02,t_i+=e03){
				x_p=x_i/t_i;
				if(x_p>=0 && x_p<=x_max1){
					y_p=y_i/t_i;
					if(y_p>=0 && y_p<=y_max1){
						z_p=z_i/t_i;
						if(z_p>=0 && z_p<=z_max1){
							x_in=1;
							x_up=ROUND(x_p); /* originally was floor(x_p+.5) which is equivalent to ROUND (see Common_IMMA.h) which just rounds up or down 5.2 -> 5 and 5.6->6*/
							y_up=ROUND(y_p);
							z_up=ROUND(z_p);
							/* x_up, y_up, z_up are the original coordinates (before reslice) : go through
							   the list of input coordinates and see if any one is the `closest' or identical */
							if( foundSoFar >= numCoordinates ){
								/* found all the points, go home thank you very much */
								i = x_dim2 * 10; j = y_dim2 * 10; k = z_dim2 * 10;
								break;
							}
							if( (a_LinkedList=myLinkedLists[z_up][y_up]) == NULL ) continue;
							for(ll_iterator=linkedlist_iterator_init(a_LinkedList);linkedlist_iterator_has_more(ll_iterator);){
								a_pS = (_coords_data_struct *)linkedlist_iterator_next(ll_iterator);
								if( (x_up==a_pS->from.x) && (y_up==a_pS->from.y) && (z_up==a_pS->from.z) ){
									/* exact match */
									a_pS->distance = 0.0;
									a_pS->closest.x = x_up; a_pS->closest.y = y_up; a_pS->closest.z = z_up;
									a_pS->to.x = i; a_pS->to.y = j; a_pS->to.z = k;
									/* no need to go through the loop again */
									//printf("found exact match for (%d,%d,%d)=(%d,%d,%d)\n", a_pS->from.x, a_pS->from.y, a_pS->from.z, a_pS->to.x, a_pS->to.y, a_pS->to.z);
									linkedlist_remove_item(a_LinkedList, (void *)a_pS);
									foundSoFar++;
									break;
								}
								if( (a_distance=(SQR(x_up-a_pS->from.x)+SQR(y_up-a_pS->from.y)+SQR(z_up-a_pS->from.z))) < a_pS->distance ){
									/* no such point, find nearest */
									a_pS->distance = a_distance;
									a_pS->closest.x = x_up; a_pS->closest.y = y_up; a_pS->closest.z = z_up;
									a_pS->to.x = i; a_pS->to.y = j; a_pS->to.z = k;
								}
							}
							destroy_linkedlist_iterator(ll_iterator);
						} else{ if(x_in) break; }
					} else{ if(x_in) break; }
				} else{ if(x_in) break; }
			}
		}
	}

	for(i=0,pS=&(coords_data[0]);i<numCoordinates;i++,pS++){
		outputCoordinates[__X][i] = (*pS)->to.x;
		// y-axis has to be converted now to dispunc (e.g. zero on top left corner)
		outputCoordinates[__Y][i] = registeredImageDimensions[__Y] - (*pS)->to.y;
		outputCoordinates[__Z][i] = (*pS)->to.z;
		//printf("(%d,%d,%d) goes to (%d,%d,%d) d=%f\n", inputCoordinates[__X][i], inputCoordinates[__Y][i], inputCoordinates[__Z][i], outputCoordinates[__X][i], outputCoordinates[__Y][i], outputCoordinates[__Z][i], (*pS)->distance);
/*		if( (*pS)->distance > 0.0 )
			fprintf(stderr, "dumpNNreslicer_AIR308 : ** warning ** could not match input point (%d,%d,%d)cor=[%d,%d,%d] - closest was (%d,%d,%d) -> (%d,%d,%d).\n",
				inputCoordinates[__X][i], inputCoordinates[__Y][i], inputCoordinates[__Z][i],
		    		(*pS)->from.x, (*pS)->from.y, (*pS)->from.z,
		    		(*pS)->closest.x, (*pS)->closest.y, (*pS)->closest.z,
				outputCoordinates[__X][i], outputCoordinates[__Y][i], outputCoordinates[__Z][i]);*/
	}
	for(i=0;i<numUniqueCoordinates;i++) free(dataToFree[i]);
	free(dataToFree);
	free(coords_data);
	for(i=0;i<nativeImageDimensions[__Z];i++){
		for(j=0;j<nativeImageDimensions[__Y];j++)
			if( (a_LinkedList=myLinkedLists[i][j]) != NULL ) linkedlist_destroy(a_LinkedList, FALSE);
		free(myLinkedLists[i]);
	}
	free(myLinkedLists);

	return TRUE;
}

int	dumpNNreslicer_AIR525(
	int		nativeImageDimensions[],
	int		registeredImageDimensions[],
	double		**es,
	int 		**inputCoordinates,	/* input coordinates in native space - origin is top-left corner (e.g. dispunc) */
	int		**outputCoordinates,	/* output coordinates in registered space - origin the same as above (e.g. dispunc) */
	int		numCoordinates)		/* the number of coordinates to convert */
{
	double	x_max1, y_max1, z_max1;
	
	unsigned int	x_dim2, y_dim2, z_dim2;
	
	double	e00=es[0][0],
		e01=es[0][1],
		e02=es[0][2],
		e03=es[0][3],
		e10=es[1][0],
		e11=es[1][1],
		e12=es[1][2],
		e13=es[1][3],
		e20=es[2][0],
		e21=es[2][1],
		e22=es[2][2],
		e23=es[2][3],
		e30=es[3][0],
		e31=es[3][1],
		e32=es[3][2],
		e33=es[3][3];
	
	double	x_k, y_k, z_k, t_k,
		x_j, y_j, z_j, t_j,
		x_i, y_i, z_i, t_i;
	char	x_in;
	double	x_p, y_p, z_p;
	unsigned int	x_up, y_up, z_up;
	register int	i, j, k;

	linkedlist_iterator		*ll_iterator;
	
	_coords_data_struct		**coords_data;
	linkedlist			***myLinkedLists, *a_LinkedList;
	register _coords_data_struct	**pS, *a_pS, **dataToFree;
	register float			a_distance;
	register int			foundSoFar;
	int				numUniqueCoordinates = 0, corrected,
					numLinkedLists;
	char				name[100];

	if( (myLinkedLists=(linkedlist ***)malloc(nativeImageDimensions[__Z]*sizeof(linkedlist **))) == NULL ){
		fprintf(stderr, "dumpNNreslicer_AIR308 : could not allocate %zd bytes for wrapper.\n", nativeImageDimensions[__Z]*sizeof(linkedlist **));
		return FALSE;
	}
	for(i=0;i<nativeImageDimensions[__Z];i++){
		if( (myLinkedLists[i]=(linkedlist **)malloc(nativeImageDimensions[__Y]*sizeof(linkedlist *))) == NULL ){
			fprintf(stderr, "dumpNNreslicer_AIR308 : could not allocate %zd bytes for wrapper.\n", nativeImageDimensions[__Y]*sizeof(linkedlist *));
			return FALSE;
		}
		for(j=0;j<nativeImageDimensions[__Y];j++) myLinkedLists[i][j] = NULL;
	}
	if( (coords_data=(_coords_data_struct **)malloc(numCoordinates*sizeof(_coords_data_struct *))) == NULL ){
		fprintf(stderr, "dumpNNreslicer_AIR308 : could not allocate %zd bytes for coords_data.\n", numCoordinates*sizeof(_coords_data_struct *));
		return FALSE;
	}
	if( (dataToFree=(_coords_data_struct **)malloc(numCoordinates*sizeof(_coords_data_struct *))) == NULL ){
		fprintf(stderr, "dumpNNreslicer_AIR308 : could not allocate %zd bytes for dataToFree.\n", numCoordinates*sizeof(_coords_data_struct *));
		return FALSE;
	}
	for(i=0,pS=&(coords_data[0]);i<numCoordinates;i++,pS++) *pS = NULL;
	for(i=0,numLinkedLists=0,pS=&(coords_data[0]);i<numCoordinates;i++,pS++){
		if( *pS == NULL ){
			/* allocate */
			if( (*pS=(_coords_data_struct *)malloc(sizeof(_coords_data_struct))) == NULL ){
				fprintf(stderr, "dumpNNreslicer_AIR308 : could not allocate %zd bytes for coords_data[%d].\n", sizeof(_coords_data_struct), i);
				return FALSE;
			}
			/* index of input/output coordinates [0|1|2][i]
			   0|1|2 for X, Y, Z (use __X etc)
			   and i is the line number */
			(*pS)->from.x = inputCoordinates[__X][i];
			(*pS)->from.y = inputCoordinates[__Y][i]; /* y-axis starts from bottom rather than top (dispunc) */
			corrected = nativeImageDimensions[__Y]-(*pS)->from.y;
			(*pS)->from.z = inputCoordinates[__Z][i];
			(*pS)->distance = 1000000000.0; /* set to an astronomical distance */
			/* no registered-space point yet, nor closest point */
			(*pS)->to.x = (*pS)->to.y = (*pS)->to.z = -1;
			(*pS)->closest.x = (*pS)->closest.y = (*pS)->closest.z = 0;

			/* note it so that we free it later */
			dataToFree[numUniqueCoordinates++] = *pS;

			if( myLinkedLists[(*pS)->from.z][corrected] == NULL ){
				sprintf(name, "dumpNNreslicer_AIR308(*,%d,%d)", corrected, (*pS)->from.z);
				if( (myLinkedLists[(*pS)->from.z][corrected]=linkedlist_new(name)) == NULL ){
					fprintf(stderr, "dumpNNreslicer_AIR308 : call to linkedlist_new has failed for '%s'.\n", name);
					return FALSE;
				}
				numLinkedLists++;
			}
			if( linkedlist_add_item(myLinkedLists[(*pS)->from.z][corrected], (void *)(*pS), NULL, NULL) == FALSE ){
				fprintf(stderr, "dumpNNreslicer_AIR308 : call to linkedlist_add_item has failed for %d item (of %d).\n", i, numCoordinates);
				return FALSE;
			}
			/* now check in the input coordinates if any point is identical to this. if it is then just copy it there */
			for(j=i;j<numCoordinates;j++){
				if( ((*pS)->from.x==inputCoordinates[__X][j]) && ((*pS)->from.y==inputCoordinates[__Y][j]) && ((*pS)->from.z==inputCoordinates[__Z][j]) ){
					coords_data[j] = *pS;
					//printf("XX (%d,%d,%d)\n", coords_data[j]->from.x, coords_data[j]->from.y, coords_data[j]->from.z);
				}
			}
		}
	}
	fprintf(stderr, "(%d linkedlists)", numLinkedLists); fflush(stderr);
	//for(i=0;i<numCoordinates;i++) printf("YY (%d,%d,%d)\n", coords_data[i]->from.x, coords_data[i]->from.y, coords_data[i]->from.z);

	/* now correct the y dimension, AIR thinks it starts from bottom left but input coordinates have the y-axis starting from top-left ... */
	for(i=0;i<nativeImageDimensions[__Z];i++) for(j=0;j<nativeImageDimensions[__Y];j++){
		if( (a_LinkedList=myLinkedLists[i][j]) != NULL ){
			for(ll_iterator=linkedlist_iterator_init(a_LinkedList);linkedlist_iterator_has_more(ll_iterator);){
				a_pS = (_coords_data_struct *)linkedlist_iterator_next(ll_iterator);
				a_pS->from.y = nativeImageDimensions[__Y] - a_pS->from.y;
				//printf("LL (%d,%d,%d)\n", a_pS->from.x, a_pS->from.y, a_pS->from.z);
			}
		}
	}
	//for(i=0,pS=&(coords_data[0]);i<numCoordinates;i++,pS++) printf("CO (%d,%d,%d)\n", (*pS)->from.x, (*pS)->from.y, (*pS)->from.z);

	x_max1=nativeImageDimensions[__X] - 1;
	y_max1=nativeImageDimensions[__Y] - 1;
	z_max1=nativeImageDimensions[__Z] - 1;

	x_dim2=registeredImageDimensions[__X];
	y_dim2=registeredImageDimensions[__Y];
	z_dim2=registeredImageDimensions[__Z];

	foundSoFar = 0;
	for(k=0,x_k=e30,y_k=e31,z_k=e32,t_k=e33;k<z_dim2;x_k+=e20,y_k+=e21,z_k+=e22,t_k+=e23,k++){
		for(j=0,x_j=x_k,y_j=y_k,z_j=z_k,t_j=t_k;j<y_dim2;x_j+=e10,y_j+=e11,z_j+=e12,t_j+=e13,j++){
			x_in=FALSE;
			for(i=0,x_i=x_j,y_i=y_j,z_i=z_j,t_i=t_j;i<x_dim2;x_i+=e00,y_i+=e01,z_i+=e02,t_i+=e03,i++){
				x_p=x_i/t_i;
				if(x_p>=0.0 && x_p<=x_max1){
					y_p=y_i/t_i;
					if(y_p>=0.0 && y_p<=y_max1){
						z_p=z_i/t_i;
						if(z_p>=0.0 && z_p<=z_max1){
							x_up=(unsigned int)floor(x_p+.5);
							y_up=(unsigned int)floor(y_p+.5);
							z_up=(unsigned int)floor(z_p+.5);
							x_in=TRUE;
							/* x_up, y_up, z_up are the original coordinates (before reslice) : go through
							   the list of input coordinates and see if any one is the `closest' or identical */
							if( foundSoFar >= numCoordinates ){
								/* found all the points, go home thank you very much */
								i = x_dim2 * 10; j = y_dim2 * 10; k = z_dim2 * 10;
								break;
							}
							if( (a_LinkedList=myLinkedLists[z_up][y_up]) == NULL ) continue;
							for(ll_iterator=linkedlist_iterator_init(a_LinkedList);linkedlist_iterator_has_more(ll_iterator);){
								a_pS = (_coords_data_struct *)linkedlist_iterator_next(ll_iterator);
								if( (x_up==a_pS->from.x) && (y_up==a_pS->from.y) && (z_up==a_pS->from.z) ){
									/* exact match */
									a_pS->distance = 0.0;
									a_pS->closest.x = x_up; a_pS->closest.y = y_up; a_pS->closest.z = z_up;
									a_pS->to.x = i; a_pS->to.y = j; a_pS->to.z = k;
									/* no need to go through the loop again */
									//printf("found exact match for (%d,%d,%d)=(%d,%d,%d)\n", a_pS->from.x, a_pS->from.y, a_pS->from.z, a_pS->to.x, a_pS->to.y, a_pS->to.z);
									linkedlist_remove_item(a_LinkedList, (void *)a_pS);
									foundSoFar++;
									break;
								}
								if( (a_distance=(SQR(x_up-a_pS->from.x)+SQR(y_up-a_pS->from.y)+SQR(z_up-a_pS->from.z))) < a_pS->distance ){
									/* no such point, find nearest */
									a_pS->distance = a_distance;
									a_pS->closest.x = x_up; a_pS->closest.y = y_up; a_pS->closest.z = z_up;
									a_pS->to.x = i; a_pS->to.y = j; a_pS->to.z = k;
								}
							}
						} else { if(x_in) break; }
					} else { if(x_in) break; }
				} else { if(x_in) break; }
			}
		}
	}


	for(i=0,pS=&(coords_data[0]);i<numCoordinates;i++,pS++){
		outputCoordinates[__X][i] = (*pS)->to.x;
		outputCoordinates[__Y][i] = registeredImageDimensions[__Y] - (*pS)->to.y; /* y-axis has to be converted now to dispunc (e.g. zero on top left corner) */
		outputCoordinates[__Z][i] = (*pS)->to.z;
		//printf("(%d,%d,%d) goes to (%d,%d,%d) d=%f\n", inputCoordinates[__X][i], inputCoordinates[__Y][i], inputCoordinates[__Z][i], outputCoordinates[__X][i], outputCoordinates[__Y][i], outputCoordinates[__Z][i], (*pS)->distance);
		if( (*pS)->distance > 0.0 )
			fprintf(stderr, "dumpNNreslicer_AIR525 : ** warning ** could not match input point (%d,%d,%d)cor=[%d,%d,%d] - closest was (%d,%d,%d) -> (%d,%d,%d).\n",
				inputCoordinates[__X][i], inputCoordinates[__Y][i], inputCoordinates[__Z][i],
		    		(*pS)->from.x, (*pS)->from.y, (*pS)->from.z,
		    		(*pS)->closest.x, (*pS)->closest.y, (*pS)->closest.z,
				outputCoordinates[__X][i], outputCoordinates[__Y][i], outputCoordinates[__Z][i]);
	}
	for(i=0;i<numUniqueCoordinates;i++) free(dataToFree[i]);
	free(dataToFree);
	free(coords_data);
	for(i=0;i<nativeImageDimensions[__Z];i++){
		for(j=0;j<nativeImageDimensions[__Y];j++)
			if( (a_LinkedList=myLinkedLists[i][j]) != NULL ) linkedlist_destroy(a_LinkedList, FALSE);
		free(myLinkedLists[i]);
	}
	free(myLinkedLists);

	return TRUE;
}

