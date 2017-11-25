#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <tiffio.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "IO_tiff.h"

int	_write_buffer_to_RGB_tiff_file(unsigned char **/*image_buffer*/, int /*width*/, int /*height*/, char */*filename*/);
int	_write_buffer_to_grayscale_tiff_file(unsigned char **/*image_buffer*/, int /*width*/, int /*height*/, char */*filename*/);

int	write_unc_slice_to_RGB_tiff_file(DATATYPE **data, int width, int height, char *filename){
	unsigned char	**image_buffer;
	int	i, j;

	if( (image_buffer=(unsigned char **)malloc(height*sizeof(unsigned char *))) == NULL ){
		fprintf(stderr, "write_tiff_file : could not allocate %zd bytes for image_buffer.\n", height*sizeof(unsigned char *));
		return FALSE;
	}
	for(i=0;i<height;i++)
		if( (image_buffer[i]=(unsigned char *)malloc(width*3*sizeof(unsigned char))) == NULL ){
			fprintf(stderr, "write_tiff_file : could not allocate %zd bytes for image_buffer[%d].\n", 3*sizeof(unsigned char *), i);
			return FALSE;
		}
	for(i=0;i<width;i++) for(j=0;j<height;j++)
		image_buffer[j][i*3+0] =
		image_buffer[j][i*3+1] =
		image_buffer[j][i*3+2] = data[i][j];

	_write_buffer_to_RGB_tiff_file(image_buffer, width, height, filename);

	for(i=0;i<height;i++) free(image_buffer[i]);
	free(image_buffer);

	return TRUE;
}

int	write_RGB_to_RGB_tiff_file(DATATYPE **R, DATATYPE **G, DATATYPE **B, int width, int height, char *filename){
	unsigned char	**image_buffer;
	int	i, j;

	if( (image_buffer=(unsigned char **)malloc(height*sizeof(unsigned char *))) == NULL ){
		fprintf(stderr, "write_tiff_file : could not allocate %zd bytes for image_buffer.\n", height*sizeof(unsigned char *));
		return FALSE;
	}
	for(i=0;i<height;i++)
		if( (image_buffer[i]=(unsigned char *)malloc(width*3*sizeof(unsigned char))) == NULL ){
			fprintf(stderr, "write_tiff_file : could not allocate %zd bytes for image_buffer[%d].\n", 3*sizeof(unsigned char *), i);
			return FALSE;
		}
	for(i=0;i<width;i++) for(j=0;j<height;j++){
		image_buffer[j][i*3+0] = R[i][j];
		image_buffer[j][i*3+1] = G[i][j];
		image_buffer[j][i*3+2] = B[i][j];
	}
	_write_buffer_to_RGB_tiff_file(image_buffer, width, height, filename);

	for(i=0;i<height;i++) free(image_buffer[i]);
	free(image_buffer);

	return TRUE;
}

int	write_unc_slice_to_grayscale_tiff_file(DATATYPE **data, int width, int height, char *filename){
	unsigned char	**image_buffer;
	int	i, j;

	if( (image_buffer=(unsigned char **)malloc(height*sizeof(unsigned char *))) == NULL ){
		fprintf(stderr, "write_tiff_file : could not allocate %zd bytes for image_buffer.\n", height*sizeof(unsigned char *));
		return FALSE;
	}
	for(i=0;i<height;i++)
		if( (image_buffer[i]=(unsigned char *)malloc(width*sizeof(unsigned char))) == NULL ){
			fprintf(stderr, "write_tiff_file : could not allocate %zd bytes for image_buffer[%d].\n", 3*sizeof(unsigned char *), i);
			return FALSE;
		}
	for(i=0;i<width;i++) for(j=0;j<height;j++) image_buffer[j][i] = data[i][j];

	_write_buffer_to_grayscale_tiff_file(image_buffer, width, height, filename);

	for(i=0;i<height;i++) free(image_buffer[i]);
	free(image_buffer);

	return TRUE;
}

int	_write_buffer_to_RGB_tiff_file(unsigned char **image_buffer, int width, int height, char *filename){
	TIFF	*out;
	int	row, linebytes;

	if( (out=TIFFOpen(filename, "w")) == NULL ){
		fprintf(stderr, "_write_buffer_to_tiff_file : could not open file '%s' for writing.\n", filename);
		return FALSE;
	}
	TIFFSetField(out, TIFFTAG_IMAGEWIDTH,  width);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 3); /* RGB */
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
//	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);

	linebytes = 3 * width;
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, -1));
	for(row=0;row<height;row++){
		if( TIFFWriteScanline(out, image_buffer[row], row, 0) < 0 ){
			fprintf(stderr, "_write_buffer_to_RGB_tiff_file : call to TIFFWriteScanline has failed for row %d and output file %s.\n", row, filename);
			TIFFClose(out);
			return FALSE;
		}
	}
	TIFFClose(out);
	return TRUE;	
}

int	_write_buffer_to_grayscale_tiff_file(unsigned char **image_buffer, int width, int height, char *filename){
	TIFF	*out;
	int	row, linebytes;

	if( (out=TIFFOpen(filename, "w")) == NULL ){
		fprintf(stderr, "_write_buffer_to_tiff_file : could not open file '%s' for writing.\n", filename);
		return FALSE;
	}
	TIFFSetField(out, TIFFTAG_IMAGEWIDTH,  width);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1); /* grayscale */
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
//	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);

	linebytes = width;
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, -1));
	for(row=0;row<height;row++)
		if( TIFFWriteScanline(out, image_buffer[row], row, 0) < 0 ){
			fprintf(stderr, "_write_buffer_to_grayscale_tiff_file : call to TIFFWriteScanline has failed for row %d and output file %s.\n", row, filename);
			TIFFClose(out);
			return FALSE;
		}
	TIFFClose(out);
	return TRUE;	
}


