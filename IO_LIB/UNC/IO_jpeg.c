#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <setjmp.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "IO_jpeg.h"

void	_write_buffer_to_RGB_jpeg_file(JSAMPLE */*image_buffer*/, int /*quality*/, int /*width*/, int /*height*/, FILE */*outfile*/);
void	_write_buffer_to_grayscale_jpeg_file(JSAMPLE */*image_buffer*/, int /*quality*/, int /*width*/, int /*height*/, FILE */*outfile*/);

int	write_unc_slice_to_RGB_jpeg_file(DATATYPE **data, int width, int height, int quality, char *filename){
	JSAMPLE	*image_buffer;
	FILE * outfile;
	int	i, j;

	if( (image_buffer=(JSAMPLE *)malloc(width*height*3*sizeof(JSAMPLE))) == NULL ){
		fprintf(stderr, "write_jpeg_file : could not allocate %zd bytes for image_buffer.\n", width*height*3*sizeof(JSAMPLE));
		return FALSE;
	}

	if( (outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "write_jpeg_file : could not open file '%s' for writing.\n", filename);
		free(image_buffer);
		return FALSE;
	}
	for(i=0;i<width;i++) for(j=0;j<height;j++)
		image_buffer[(i+j*width)*3 + 0] =
		image_buffer[(i+j*width)*3 + 1] =
		image_buffer[(i+j*width)*3 + 2] = data[i][j];

	_write_buffer_to_RGB_jpeg_file(image_buffer, quality, width, height, outfile);
	fclose(outfile);

	free(image_buffer);
	return TRUE;
}

int	write_unc_slice_to_grayscale_jpeg_file(DATATYPE **data, int width, int height, int quality, char *filename){
	JSAMPLE	*image_buffer;
	FILE * outfile;
	int	i, j;

	if( (image_buffer=(JSAMPLE *)malloc(width*height*sizeof(JSAMPLE))) == NULL ){
		fprintf(stderr, "write_jpeg_file : could not allocate %zd bytes for image_buffer.\n", width*height*3*sizeof(JSAMPLE));
		return FALSE;
	}

	if( (outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "write_jpeg_file : could not open file '%s' for writing.\n", filename);
		free(image_buffer);
		return FALSE;
	}
	for(i=0;i<width;i++) for(j=0;j<height;j++)
		image_buffer[i+j*width] = data[i][j];

	_write_buffer_to_grayscale_jpeg_file(image_buffer, quality, width, height, outfile);
	fclose(outfile);

	free(image_buffer);
	return TRUE;
}

int	write_RGB_to_RGB_jpeg_file(DATATYPE **R, DATATYPE **G, DATATYPE **B, int width, int height, int quality, char *filename){
	JSAMPLE	*image_buffer;
	FILE * outfile;
	int	i, j;

	if( (image_buffer=(JSAMPLE *)malloc(width*height*3*sizeof(JSAMPLE))) == NULL ){
		fprintf(stderr, "write_jpeg_file : could not allocate %zd bytes for image_buffer.\n", width*height*3*sizeof(JSAMPLE));
		return FALSE;
	}

	if( (outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "write_jpeg_file : could not open file '%s' for writing.\n", filename);
		free(image_buffer);
		return FALSE;
	}
	for(i=0;i<width;i++) for(j=0;j<height;j++){
		image_buffer[(i+j*width)*3 + 0] = R[i][j];
		image_buffer[(i+j*width)*3 + 1] = G[i][j];
		image_buffer[(i+j*width)*3 + 2] = B[i][j];
	}
	_write_buffer_to_RGB_jpeg_file(image_buffer, quality, width, height, outfile);
	fclose(outfile);
	free(image_buffer);
	return TRUE;
}

void	_write_buffer_to_RGB_jpeg_file(JSAMPLE *image_buffer, int quality, int width, int height, FILE *outfile){
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	int row_stride;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);
	row_stride = cinfo.image_width * cinfo.input_components;
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
}
void	_write_buffer_to_grayscale_jpeg_file(JSAMPLE *image_buffer, int quality, int width, int height, FILE *outfile){
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	int row_stride;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 1;
	cinfo.in_color_space = JCS_GRAYSCALE;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);
	row_stride = cinfo.image_width * cinfo.input_components;
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
}

