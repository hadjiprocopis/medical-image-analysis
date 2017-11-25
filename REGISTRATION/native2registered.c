#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#include <Common_IMMA.h>

#include "native2registered.h"
#include "dumpreslicer.h"

/* Allows AIR 2.0 and later to read AIR 1.0 .air files*/
typedef	struct _NATIVE2REGISTERED_AIR_OLDAIR {
	double					e[4][3];
	char					s_file[128];
	native2registered_AIR_Key_info	s;
	char					r_file[128];
	native2registered_AIR_Key_info	r;
	char					comment[128];
	unsigned long int			s_hash;
	unsigned long int			r_hash;
	unsigned short				s_volume; /* Not used in this version of AIR */
	unsigned short				r_volume; /* Not used in this version of AIR */
	char					reserved[116];
} _native2registered_AIR_Oldair;

void _AIR_swapbytes(void *array, const size_t byte_pairs, const size_t elements);

#define	__X	0
#define	__Y	1
#define	__Z	2

/* Copyright 1995-97 Roger P. Woods, M.D. */
/* Modified: 10/25/97 */

/* This program will reslice a volume using the information
 *  contained in the specified .air file.
 *
 * Trilinear interpolation is default, but nearest neighbor or sinc
 *  can be explicitly requested.
 *
 * The default is to interpolate the output to cubic voxels,
 *  and not to overwrite existing files.
 *
 * Defaults can be overridden and other options utilized by
 *  various flags.
 *
 * Flags can be displayed by typing the name of the program
 *  without additional arguments.
 */

int	native2registered(
		int **inputCoordinates,	/* input coordinates in native space - origin is top-left corner (e.g. dispunc) */
		int **outputCoordinates,/* output coordinates in registered space - origin the same as above (e.g. dispunc) */		
		int numCoordinates,	/* the number of coordinates to convert */
		native2registered_AIR_Air16 *air1,
		char	shouldUseAIR3_08Conventions,
		int x_dimout,
		int y_dimout,
		int z_dimout,
		float x_distance,
		float y_distance,
		float z_distance,
		float x_shift,
		float y_shift,
		float z_shift,
		int cubic)
{

	float			xoom1;
	float			yoom1;
	float			zoom1;

	double			pixel_size_s;

	double			*e[4];

	int			nativeImageDimensions[3],
				registeredImageDimensions[3];

	e[0]=air1->e[0];
	e[1]=air1->e[1];
	e[2]=air1->e[2];
	e[3]=air1->e[3];

	pixel_size_s=air1->s.x_size;
	if(air1->s.y_size<pixel_size_s) pixel_size_s=air1->s.y_size;
	if(air1->s.z_size<pixel_size_s) pixel_size_s=air1->s.z_size;

	/*Sort out how to interpolate*/
	if (!cubic){
		/* If z_dimout==0, keep the same z_dim as the standard file*/
		if(x_dimout!=0){
			air1->s.x_dim=x_dimout;
			air1->s.x_size=x_distance;
		}
		if(y_dimout!=0){
			air1->s.y_dim=y_dimout;
			air1->s.y_size=y_distance;
		}
		if(z_dimout!=0){
			air1->s.z_dim=z_dimout;
			air1->s.z_size=z_distance;
		}
	}
	else{
		if(fabs(air1->s.x_size-pixel_size_s)>PIX_SIZE_ERR){
			xoom1=air1->s.x_size/pixel_size_s;
			air1->s.x_dim=(air1->s.x_dim-1)*xoom1+1;
			air1->s.x_size=pixel_size_s;
		}
		if(fabs(air1->s.y_size-pixel_size_s)>PIX_SIZE_ERR){
			yoom1=air1->s.y_size/pixel_size_s;
			air1->s.y_dim=(air1->s.y_dim-1)*yoom1+1;
			air1->s.y_size=pixel_size_s;
		}
		if(fabs(air1->s.z_size-pixel_size_s)>PIX_SIZE_ERR){
			zoom1=air1->s.z_size/pixel_size_s;
			air1->s.z_dim=(air1->s.z_dim-1)*zoom1+1;
			air1->s.z_size=pixel_size_s;
		}
	}


	/*Adjust for modified voxel sizes*/
	if(fabs(air1->s.x_size-pixel_size_s)>PIX_SIZE_ERR){
		e[0][0]*=(air1->s.x_size/pixel_size_s);
		e[0][1]*=(air1->s.x_size/pixel_size_s);
		e[0][2]*=(air1->s.x_size/pixel_size_s);
		e[0][3]*=(air1->s.x_size/pixel_size_s);
	}

	if(fabs(air1->s.y_size-pixel_size_s)>PIX_SIZE_ERR){
		e[1][0]*=(air1->s.y_size/pixel_size_s);
		e[1][1]*=(air1->s.y_size/pixel_size_s);
		e[1][2]*=(air1->s.y_size/pixel_size_s);
		e[1][3]*=(air1->s.y_size/pixel_size_s);
	}


	if(fabs(air1->s.z_size-pixel_size_s)>PIX_SIZE_ERR){
		e[2][0]*=(air1->s.z_size/pixel_size_s);
		e[2][1]*=(air1->s.z_size/pixel_size_s);
		e[2][2]*=(air1->s.z_size/pixel_size_s);
		e[2][3]*=(air1->s.z_size/pixel_size_s);
	}

	/*Adjust for shifts*/
	e[3][0]+=e[0][0]*x_shift+e[1][0]*y_shift+e[2][0]*z_shift;
	e[3][1]+=e[0][1]*x_shift+e[1][1]*y_shift+e[2][1]*z_shift;
	e[3][2]+=e[0][2]*x_shift+e[1][2]*y_shift+e[2][2]*z_shift;
	e[3][3]+=e[0][3]*x_shift+e[1][3]*y_shift+e[2][3]*z_shift;

	/* read the image dimensions from the air struct */
	nativeImageDimensions[__X] = air1->r.x_dim;
	nativeImageDimensions[__Y] = air1->r.y_dim;
	nativeImageDimensions[__Z] = air1->r.z_dim;
	registeredImageDimensions[__X] = air1->s.x_dim;
	registeredImageDimensions[__Y] = air1->s.y_dim;
	registeredImageDimensions[__Z] = air1->s.z_dim;

	/* Reslice the data : remember we can only do nearest neighbours or linear interpolation */
	if( shouldUseAIR3_08Conventions ){
		if( dumpNNreslicer_AIR308(
			nativeImageDimensions,
			registeredImageDimensions,
			e, /* this is the parameters matrix i guess */
			inputCoordinates,
			outputCoordinates,
			numCoordinates) == FALSE ){
				fprintf(stderr, "native2registered : call to dumpNNreslicer_AIR308 has failed.\n");
				return FALSE;
			}
	} else {
		if( dumpNNreslicer_AIR525(
			nativeImageDimensions,
			registeredImageDimensions,
			e, /* this is the parameters matrix i guess */
			inputCoordinates,
			outputCoordinates,
			numCoordinates) == FALSE ){
				fprintf(stderr, "native2registered : call to dumpNNreslicer_AIR308 has failed.\n");
				return FALSE;
			}
	}

	return TRUE;
}
int native2registered_AIR_read_air16(const char *filename, native2registered_AIR_Air16 *air1)
{
	FILE *fp;
	if( (fp=fopen(filename,"rb")) == NULL ){
		fprintf(stderr, "AIR_read_air16 : could not open file '%s' for reading.\n", filename);
		return FALSE;
	}

	/*Read in AIR file*/

	if(fread(air1,1,sizeof(native2registered_AIR_Air16),fp)!=sizeof(native2registered_AIR_Air16)){

		_native2registered_AIR_Oldair	air2;

		/*Try reading it as an old 12 parameter air file*/
		rewind(fp);
		if(fread(&air2,1,sizeof(_native2registered_AIR_Oldair),fp)!=sizeof(_native2registered_AIR_Oldair)){
			printf("%s: %d: ",__FILE__,__LINE__);
			printf("file read error for file %s\n",filename);
			(void)fclose(fp);
			return 0;
		}
		strncpy(air1->s_file,air2.s_file,127);
		strncpy(air1->r_file,air2.r_file,127);
		strncpy(air1->comment,air2.comment,127);
		strncpy(air1->reserved,air2.reserved,115);
		air1->s.bits=air2.s.bits;
		air1->s.x_dim=air2.s.x_dim;
		air1->s.y_dim=air2.s.y_dim;
		air1->s.z_dim=air2.s.z_dim;
		air1->s.x_size=air2.s.x_size;
		air1->s.y_size=air2.s.y_size;
		air1->s.z_size=air2.s.z_size;
		air1->r.bits=air2.r.bits;
		air1->r.x_dim=air2.r.x_dim;
		air1->r.y_dim=air2.r.y_dim;
		air1->r.z_dim=air2.r.z_dim;
		air1->r.x_size=air2.r.x_size;
		air1->r.y_size=air2.r.y_size;
		air1->r.z_size=air2.r.z_size;
		air1->s_hash=air2.s_hash;
		air1->r_hash=air2.r_hash;
		air1->s_volume=air2.s_volume;
		air1->r_volume=air2.r_volume;
		air1->e[0][0]=air2.e[1][0];
		air1->e[0][1]=air2.e[1][1];
		air1->e[0][2]=air2.e[1][2];
		air1->e[0][3]=0.0;
		air1->e[1][0]=air2.e[2][0];
		air1->e[1][1]=air2.e[2][1];
		air1->e[1][2]=air2.e[2][2];
		air1->e[1][3]=0.0;
		air1->e[2][0]=air2.e[3][0];
		air1->e[2][1]=air2.e[3][1];
		air1->e[2][2]=air2.e[3][2];
		air1->e[2][3]=0.0;
		air1->e[3][0]=air2.e[0][0];
		air1->e[3][1]=air2.e[0][1];
		air1->e[3][2]=air2.e[0][2];
		air1->e[3][3]=1.0;
		
		/* byteswap */
		if(air1->s.bits>sqrt(UINT_MAX)){
			/* Pre-swap these values so that they'll be swapped back to their starting values */
			_AIR_swapbytes(&(air1->e[0][3]),sizeof(air1->e[0][3])/2,1);
			_AIR_swapbytes(&(air1->e[1][3]),sizeof(air1->e[1][3])/2,1);
			_AIR_swapbytes(&(air1->e[2][3]),sizeof(air1->e[2][3])/2,1);
			_AIR_swapbytes(&(air1->e[3][3]),sizeof(air1->e[3][3])/2,1);
		}
		/* end byteswap */
	}
	
	/* byteswap */
	if(air1->s.bits>sqrt(UINT_MAX)){
	
		/* Byte swap */
		
		_AIR_swapbytes(&(air1->s.bits),sizeof(air1->s.bits)/2,1);
		_AIR_swapbytes(&(air1->s.x_dim),sizeof(air1->s.x_dim)/2,1);
		_AIR_swapbytes(&(air1->s.y_dim),sizeof(air1->s.y_dim)/2,1);
		_AIR_swapbytes(&(air1->s.z_dim),sizeof(air1->s.z_dim)/2,1);
		_AIR_swapbytes(&(air1->s.x_size),sizeof(air1->s.x_size)/2,1);
		_AIR_swapbytes(&(air1->s.y_size),sizeof(air1->s.y_size)/2,1);
		_AIR_swapbytes(&(air1->s.z_size),sizeof(air1->s.z_size)/2,1);
		
		_AIR_swapbytes(&(air1->r.bits),sizeof(air1->r.bits)/2,1);
		_AIR_swapbytes(&(air1->r.x_dim),sizeof(air1->r.x_dim)/2,1);
		_AIR_swapbytes(&(air1->r.y_dim),sizeof(air1->r.y_dim)/2,1);
		_AIR_swapbytes(&(air1->r.z_dim),sizeof(air1->r.z_dim)/2,1);
		_AIR_swapbytes(&(air1->r.x_size),sizeof(air1->r.x_size)/2,1);
		_AIR_swapbytes(&(air1->r.y_size),sizeof(air1->r.y_size)/2,1);
		_AIR_swapbytes(&(air1->r.z_size),sizeof(air1->r.z_size)/2,1);
		
		_AIR_swapbytes(&(air1->s_hash),sizeof(air1->s_hash)/2,1);
		_AIR_swapbytes(&(air1->r_hash),sizeof(air1->r_hash)/2,1);
		
		_AIR_swapbytes(&(air1->s_volume),sizeof(air1->s_volume)/2,1);
		_AIR_swapbytes(&(air1->r_volume),sizeof(air1->r_volume)/2,1);
				
		_AIR_swapbytes(&(air1->e[0][0]),sizeof(air1->e[0][0])/2,1);
		_AIR_swapbytes(&(air1->e[0][1]),sizeof(air1->e[0][1])/2,1);
		_AIR_swapbytes(&(air1->e[0][2]),sizeof(air1->e[0][2])/2,1);
		_AIR_swapbytes(&(air1->e[0][3]),sizeof(air1->e[0][3])/2,1);
		_AIR_swapbytes(&(air1->e[1][0]),sizeof(air1->e[1][0])/2,1);
		_AIR_swapbytes(&(air1->e[1][1]),sizeof(air1->e[1][1])/2,1);
		_AIR_swapbytes(&(air1->e[1][2]),sizeof(air1->e[1][2])/2,1);
		_AIR_swapbytes(&(air1->e[1][3]),sizeof(air1->e[1][3])/2,1);
		_AIR_swapbytes(&(air1->e[2][0]),sizeof(air1->e[2][0])/2,1);
		_AIR_swapbytes(&(air1->e[2][1]),sizeof(air1->e[2][1])/2,1);
		_AIR_swapbytes(&(air1->e[2][2]),sizeof(air1->e[2][2])/2,1);
		_AIR_swapbytes(&(air1->e[2][3]),sizeof(air1->e[2][3])/2,1);
		_AIR_swapbytes(&(air1->e[3][0]),sizeof(air1->e[3][0])/2,1);
		_AIR_swapbytes(&(air1->e[3][1]),sizeof(air1->e[3][1])/2,1);
		_AIR_swapbytes(&(air1->e[3][2]),sizeof(air1->e[3][2])/2,1);
		_AIR_swapbytes(&(air1->e[3][3]),sizeof(air1->e[3][3])/2,1);
	}
	/* end byteswap */

	fclose(fp);

	return 1;
}

void _AIR_swapbytes(void *array, const size_t byte_pairs, const size_t elements)
{
	unsigned char *p=(unsigned char *)array;
	size_t i;

	for(i=0;i<elements;i++){

		unsigned char *lo=p;
		p+=byte_pairs*2;
		{
			unsigned char *hi=p-1;
			size_t j;

			for(j=0;j<byte_pairs;j++,lo++,hi--){

				unsigned char c=*lo;
				*lo=*hi;
				*hi=c;
			}
		}
	}
}

/*
AIR's reslicer usage
void	usage(char *appName){
	printf("Note: program interpolates output to create cubic voxels unless overridden by -k, -x, -y or -z options.\n");
	printf("Usage: %s reslice_parameter_file -i inputCoordinatesFile -o outputCoordinatesFile [options]\n\n", appName);
	printf("\toptions:\n");
	printf("\t-i input coordinates file\n");
	printf("\t-o output coordinates (resliced) file\n");
	printf("\t[-I] (should the input coordinates included in the output file?)\n");
	printf("\t[-a] alternate_reslice_file]\n");
	printf("\t[-k] (keeps voxel dimensions same as standard file's)\n");
	printf("\t[-s intensity_scale_factor]\n");
	printf("\t[-x x_dim x_size [x_shift]]\n");
	printf("\t[-y y_dim y_size [y_shift]]\n");
	printf("\t[-z z_dim z_size [z_shift]]\n");
	printf("\t[-n model {x_half-window_width y_half-window_width z_half-window_width}]\n");
	printf("\n\tInterpolation models (trilinear is default interpolation model)\n");
	printf("\t\t0. nearest neighbor\n");
	printf("\t\t1. trilinear\n");
	printf("\n\t\t2. windowed sinc in original xy plane, linear along z\n");
	printf("\t\t3. windowed sinc in original xz plane. linear along y\n");
	printf("\t\t4. windowed sinc in original yz plane, linear along x\n");
	printf("\t\t5. 3D windowed sinc\n");
	printf("\n\t\t6. 3D windowed scanline sinc\n");
	printf("\t\t7. 3D unwindowed scanline sinc\n");
	printf("\n\t\t10. 3D scanline chirp-z\n");
	printf("\t\t11. scanline chirp-z in original xy plane, linear along z\n");
	
	printf("\n\tScanline models are only valid for moderate angles and don't allow perspective distortions\n");
	printf("\n\tWindowed models require appropriate half-window widths\n");
}	
*/

void	native2registered_print_AIR_parameters(FILE *a_handle, native2registered_AIR_Air16 air1)
{
	double			*e[4];

	int			i,j;
	double			pixel_size_s;

	e[0]=air1.e[0];
	e[1]=air1.e[1];
	e[2]=air1.e[2];
	e[3]=air1.e[3];

	pixel_size_s=air1.s.x_size;
	if(air1.s.y_size<pixel_size_s) pixel_size_s=air1.s.y_size;
	if(air1.s.z_size<pixel_size_s) pixel_size_s=air1.s.z_size;

	fprintf(a_handle, "\n");
	fprintf(a_handle, "standard file: %s\n",air1.s_file);
	fprintf(a_handle, "identifier: %010lu\n",air1.s_hash);
	fprintf(a_handle, "file dimensions: %i by %i by %i pixels (x,y,z)\n",air1.s.x_dim,air1.s.y_dim,air1.s.z_dim);
	fprintf(a_handle, "voxel dimensions: %e by %e by %e (x,y,z)\n",air1.s.x_size,air1.s.y_size,air1.s.z_size);
	fprintf(a_handle, "\n");
	fprintf(a_handle, "reslice file: %s\n",air1.r_file);
	fprintf(a_handle, "identifier: %010lu\n",air1.r_hash);
	fprintf(a_handle, "file dimensions: %i by %i by %i pixels (x,y,z)\n",air1.r.x_dim,air1.r.y_dim,air1.r.z_dim);
	fprintf(a_handle, "voxel dimensions: %e by %e by %e (x,y,z)\n",air1.r.x_size,air1.r.y_size,air1.r.z_size);
	fprintf(a_handle, "\n");

	/* the '-r' option : Real world transformation matrix in millimeters */
	for(j=0;j<3;j++){	/*Note: j<3 instead of j<4 is not an error*/
		for(i=0;i<4;i++){
			e[j][i]/=pixel_size_s;
		}
	}
	for(j=0;j<4;j++){
		e[j][0]*=air1.r.x_size;
		e[j][1]*=air1.r.y_size;
		e[j][2]*=air1.r.z_size;
	}
	fprintf(a_handle, "Real world transformation matrix in millimeters:\n");
	fprintf(a_handle, "R*standard spatial location=reslice spatial location\n");
	fprintf(a_handle, "R=\n");
	fprintf(a_handle, "[%e %e %e %e\n",e[0][0],e[1][0],e[2][0],e[3][0]);
	fprintf(a_handle, "%e %e %e %e\n",e[0][1],e[1][1],e[2][1],e[3][1]);
	fprintf(a_handle, "%e %e %e %e\n",e[0][2],e[1][2],e[2][2],e[3][2]);
	fprintf(a_handle, "%e %e %e %e]\n",e[0][3],e[1][3],e[2][3],e[3][3]);

	/* the '-v' option : Voxel based transformation matrix */
	e[0]=air1.e[0];
	e[1]=air1.e[1];
	e[2]=air1.e[2];
	e[3]=air1.e[3];
	for(i=0;i<4;i++){
		e[0][i]*=(air1.s.x_size/pixel_size_s);
		e[1][i]*=(air1.s.y_size/pixel_size_s);
		e[2][i]*=(air1.s.z_size/pixel_size_s);
	}
	fprintf(a_handle, "Voxel based transformation matrix:\n");
	fprintf(a_handle, "V*standard coordinates=reslice coordinates\n");
	fprintf(a_handle, "V=\n");
	fprintf(a_handle, "[%e %e %e %e\n",e[0][0],e[1][0],e[2][0],e[3][0]);
	fprintf(a_handle, "%e %e %e %e\n",e[0][1],e[1][1],e[2][1],e[3][1]);
	fprintf(a_handle, "%e %e %e %e\n",e[0][2],e[1][2],e[2][2],e[3][2]);
	fprintf(a_handle, "%e %e %e %e]\n",e[0][3],e[1][3],e[2][3],e[3][3]);

	/* Default */
	e[0]=air1.e[0];
	e[1]=air1.e[1];
	e[2]=air1.e[2];
	e[3]=air1.e[3];
	fprintf(a_handle, "Default :\n");
	fprintf(a_handle, "E*cubic standard coordinates=reslice coordinates\n");
	fprintf(a_handle, "E=\n");
	fprintf(a_handle, "[%e %e %e %e\n",e[0][0],e[1][0],e[2][0],e[3][0]);
	fprintf(a_handle, "%e %e %e %e\n",e[0][1],e[1][1],e[2][1],e[3][1]);
	fprintf(a_handle, "%e %e %e %e\n",e[0][2],e[1][2],e[2][2],e[3][2]);
	fprintf(a_handle, "%e %e %e %e]\n",e[0][3],e[1][3],e[2][3],e[3][3]);

	fprintf(a_handle, "\nComment: %s\n",air1.comment);
}
