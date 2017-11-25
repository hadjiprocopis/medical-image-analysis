#include <stdio.h>
#include <unistd.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "IO_unc.h"

/* copy the info/TITLE of the from filename to the to filename,
   'op' refers to which operation will be copied, e.g.
   IO_TITLE_COPY : copies the title (output of command 'header UNCfile')
   IO_INFO_COPY : copies the info (output of command 'header -i UNCfile')
   IO_INFO_COPY_NOT : is relevant only when **fields is not NULL,
	   and says that all fields NOT in the 'fields' array should be copied.
   ORing the first and ONE OF THE OTHER TWO will do both ops accordingly

   **fields is an array of strings that should be copied (or should not be, if op == IO_INFO_COPY_NOT)
   num_fields is the number of strings in the fields array.

   returns TRUE on success or FALSE on error

   the following program shows how to use this routine:
#include <stdio.h>
#include <stdlib.h>

#include <Common_IMMA.h>
#include <filters.h>
#include <Alloc.h>
#include <IO.h>

int     main(int argc, char **argv){
        int     i;
        char    **fields;

        fields = (char **)malloc(10*sizeof(char *));
        for(i=0;i<4;i++) fields[i] = (char *)malloc(1000*sizeof(char));

        strcpy(fields[0], "pulse_sequence_minor_version_number");
        strcpy(fields[1], "int_slop_5");
        strcpy(fields[2], "pixel_data_size-uncompressed");
        strcpy(fields[3], "image_allocation_key");
        copyUNCInfo("in.unc", "out.unc", IO_INFO_COPY|IO_TITLE_COPY, fields, 4);

        for(i=0;i<4;i++) free(fields[i]); free(fields);
        exit(0);
}
   
*/
char	copyUNCInfo(char *filename_from, char *filename_to, int op, char **fields, int num_fields){
	IMAGE	*image_from, *image_to;
	Info	*im_info_p;
	int	myop1, myop2, myop3, i;
	char	*a_field, *a_value, /* each info field is of the form 'a_field'='a_value' */
		foundFlag, title[10000]; 

	myop1 = op & IO_INFO_COPY;
	myop2 = op & IO_INFO_COPY_NOT;
	myop3 = op & IO_TITLE_COPY;

	if( (myop1==IO_INFO_COPY) && (myop2==IO_INFO_COPY_NOT) ){
		fprintf(stderr, "copyUNCInfo : op field contains mutually exclusive flags IO_INFO_COPY and IO_INFO_COPY_NOT - use only one of them.\n");
		return FALSE;
	}
	if( (myop1!=IO_INFO_COPY) && (myop2!=IO_INFO_COPY_NOT) && (myop3!=IO_TITLE_COPY) ){
		fprintf(stderr, "copyUNCInfo : op field contains no flags, please use at least one.\n");
		return FALSE;
	}

	if( (image_from=imopen(filename_from, READ)) == INVALID ){
		fprintf(stderr, "copyUNCInfo : call to imopen has failed for source file '%s'.\n", filename_from);
		return FALSE;
	}
	if( (image_to=imopen(filename_to, UPDATE)) == INVALID ){
		fprintf(stderr, "copyUNCInfo : call to imopen has failed for destination file '%s'.\n", filename_to);
		imclose(image_from);
		return FALSE;
	}
	/* get all the info fields */
	im_info_p = image_from->file_info;

	while( im_info_p != NULL ){
		a_field = strdup(im_info_p->nameval);
		if( strlen(a_field) == 0 ){
			/* empty */
			im_info_p = im_info_p->next;
			free(a_field);
			continue;
		}
			
		if( ((a_value=strchr(a_field, '=')) == NULL) || (a_field[0]=='\0') ) a_value = NULL;
		else a_field[strlen(a_field)-strlen(a_value++)] = '\0';

		if( myop1 == IO_INFO_COPY ){
			if( fields != NULL ){
				for(i=0;i<num_fields;i++)
					if( !strcmp(a_field, fields[i]) )
						if( put_info(&(image_to->file_info),  a_field, a_value) == 0 ){
							fprintf(stderr, "copyUNCInfo : call to put_info (%s=%s) has failed for source image '%s' and destination image '%s'.\n", a_field, a_value, filename_from, filename_to);
							imdestroy(image_from); imdestroy(image_to);
							free(a_field);
							return FALSE;
						}
			} else if( put_info(&(image_to->file_info),  a_field, a_value) == 0 ){
					fprintf(stderr, "copyUNCInfo : call to put_info (%s=%s) has failed for source image '%s' and destination image '%s'.\n", a_field, a_value, filename_from, filename_to);
					imdestroy(image_from); imdestroy(image_to);
					free(a_field);
					return FALSE;
				}
		} else if( myop2 == IO_INFO_COPY_NOT )
			if( fields != NULL ){
				for(i=0,foundFlag=FALSE;i<num_fields;i++)
					if( !strcmp(a_field, fields[i]) ){ foundFlag = TRUE; break; }
				if( !foundFlag )
					if( put_info(&(image_to->file_info),  a_field, a_value) == 0 ){
						fprintf(stderr, "copyUNCInfo : call to put_info (%s=%s) has failed for source image '%s' and destination image '%s'.\n", a_field, a_value, filename_from, filename_to);
						imdestroy(image_from); imdestroy(image_to);
						free(a_field);
						return FALSE;
					}
			}

		im_info_p = im_info_p->next;
		free(a_field);
	}
	if( (op&IO_TITLE_COPY) == IO_TITLE_COPY ){
		if( imgettitle(image_from, title)  == INVALID ){
			fprintf(stderr, "copyUNCInfo : call to imgettitle has failed for source image '%s'.\n", filename_from);
			imdestroy(image_from); imdestroy(image_to);
			return FALSE;
		}
		if( imputtitle(image_to, title) == INVALID ){
			fprintf(stderr, "copyUNCInfo : call to imputitle has failed for destination image '%s'.\n", filename_to);
			imdestroy(image_from); imdestroy(image_to);
			return FALSE;
		}
	}
			
	imclose(image_to); imdestroy(image_from);

	return TRUE;
}
/* function to read a text file with many lines, each line of which is of the form
	name=value
   and then create an Info structure and if 'name' is a valid UNC header name,
   then set its value to 'value'.

   it returns this Info structure or NULL on failure,
   The names and values are
   pointers to arrays to contain numEntries names and values respectively.
   If you don't want that, then just pass them as NULL

   use it as thus:
   char	**names, **values;
   int	numEntries;
   if( (info=readUNCInfoFromTextFile(fp, names, values, &numEntries)) == NULL ) exit(1);

   for(i=0;i<numEntries;i++){
	printf("%s = %s\n", names[i], values[i]);
	// and free
	free(names[i]), free(values[i]);
   }
   free(names); free(values);

   or

   if( (info=readUNCInfoFromTextFile(fp, NULL, NULL, NULL)) == NULL ) exit(1);

   Author : Andreas Hadjiprocopis, ION 2003, CING 2005
   */
Info	*readUNCInfoFromTextFile(FILE *fp, char ***names, char ***values, int *numEntries){
	IMAGE	*im;
	Info	*im_info_p;
	int	dimc = 3,
		dimv[] = {10/*slices*/, 256/*y*/, 256/*x*/},
		BUFFER_SIZE = 5000; /* max length of each line in the header file */
	char	*buffer, *name, *value, *dummy, *p;

	name = (char *)malloc(1000); value = (char *)malloc(1000); dummy = (char *)malloc(1000);
	buffer = (char *)malloc(BUFFER_SIZE*sizeof(char));

	if( (name==NULL) || (value==NULL) || (dummy==NULL) || (buffer==NULL) ){
		fprintf(stderr, "readUNCInfoFromTextFile : could not allocate 1000 bytes for one of name, value, dummy\n");
		return (Info *)NULL;
	}
	if( (im=imcreat_memory(GREY, dimc, dimv)) == INVALID ){
		fprintf(stderr, "readUNCInfoFromTextFile : call to imcreat_memory has failed .\n");
		free(name); free(value); free(dummy); free(buffer);
		return (Info *)NULL;
	}

	/* get all the info fields */
	im_info_p = im->file_info;
	*numEntries = 0;
	while( 1 ){
		fgets(buffer, BUFFER_SIZE-1, fp);
		if( feof(fp) ) break;
		if( (p=strrchr(buffer, '\n')) != NULL ) *p = '\0'; /* trim newline from end */
		read_name_value(buffer, &name, &value);
//		printf("read %s (%s)=(%s)\n", buffer, name, value);
		if( get_info(im_info_p, name, dummy) != 0 ){
			/* error */
			fprintf(stderr, "readUNCInfoFromTextFile : warning, name '%s' (with value '%s') is not a valid name for UNC header entry -- ignoring ...\n", name, value);
			continue;
		}
		if( put_info(&im_info_p, name, value) == 0 ){
			/* error */
			fprintf(stderr, "readUNCInfoFromTextFile : call to put_info has failed for pair '%s' = '%s'.\n", name, value);
			imdestroy(im); /* can handle memory image too */
			free(name); free(value); free(dummy); free(buffer);
			return (Info *)NULL;
		}
		(*numEntries)++;
	}
	free(im); /* do not call imdestroy because it will destroy the Info structure */

	if( (names != NULL) && (values != NULL) )
		if( (*numEntries=get_info_pairs_into_array(im_info_p, names, values)) < 0 ){
			fprintf(stderr, "readUNCInfoFromTextFile : call to get_info_pairs_into_array has failed.\n");
			free(name); free(value); free(dummy); free(buffer);
			return (Info *)NULL;
		}
	free(name); free(value); free(dummy); free(buffer);
	return im_info_p;
}
/* function to write a text file with many lines, each line of which is of the form
	name=value
   these names and values are from the UNC header specified

   it returns the number of entries written on success
   or -1 on failure

   Author : Andreas Hadjiprocopis, ION 2003, CING 2005
   */

int	writeUNCInfoToTextFile(FILE *fp, Info *im_info_p){
	char	**names = NULL, **values = NULL;
	int	numEntries, i;

	if( (numEntries=get_info_pairs_into_array(im_info_p, &names, &values)) < 0 ){
		fprintf(stderr, "writeUNCInfoFromTextFile : call to get_info_pairs_into_array has failed.\n");
		return -1;
	}
	if( numEntries == 0 ){
		fprintf(stderr, "writeUNCInfoFromTextFile : warning malformed UNC header -- wrote 0 entries.\n");
		return 0;
	}

	for(i=0;i<numEntries;i++){
		fprintf(fp, "%s=%s\n", names[i], values[i]);
		free(names[i]); free(values[i]);
	}
	free(names); free(values);
	return numEntries;
}


/* will close the file descriptor and destroy the image structure without committing any changes
   to the image (unlike 'imclose') */
void    imdestroy(IMAGE *image){
	if( image->Fd >= 0 ) close(image->Fd);
	infodestroy(image->file_info);
	free(image);
}
/* will free data allocated to Info */
void    infodestroy(Info *info){
	Info    *p = info, *p1;
	while(p!=NULL){
		p1 = p;
		p = p->next;
		free(p1);
	}
}
	
/* Function to read a portion or all of a single slice in a UNC file
   params: filename, the name of the file that contains the data
	   [] x_offset, y_offset integers specifying the upper left corner of the returned imaged
	   (set them to zero for getting the whole image)
	   [] *width, *height integers specifying the width and height of the returned image,
	   set them to zero or negative to acquire the whole image - in this case, when the function
	   returns, their contents will be equal to the returned image width and height minus the offsets specified 
	   [] slice, the slice number you are interested in
	   [] *depth and *format will be set to the PixelSize and PixelFormat fields of the IMAGE structure
   returns: NULL on failure, a pointer to a DATATYPE array on success
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE		*getUNCSlice(char *filename, int x_offset, int y_offset, int *width, int *height, int slice, int *depth, int *format){
	IMAGE		*image;
	int		sizeIn, sizeOut, wIn, wOut, hIn, hOut, totalSlices, offset;
	GREYTYPE	*dataIn;
	DATATYPE	*dataOut;
	register int	i, j, k;
	
	if( sizeof(GREYTYPE) > sizeof(DATATYPE) )
		fprintf(stderr, "IOLib, getUNCSlice : warning, size of image data (%zd bytes) is larger than that of data to be written (GREYTYPE, %zd bytes) - *possible* loss of precision in conversion.\n", sizeof(GREYTYPE), sizeof(DATATYPE));

	if( (image=imopen(filename, READ)) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlice : call to imopen failed for '%s'.\n", filename);
		return NULL;
	}
	/* width requested too big */
	wIn = image->Dimv[image->Dimc-1];
	if( *width <= 0 ) *width = image->Dimv[image->Dimc-1] - x_offset;
	else if( (x_offset+*width) > wIn ){
		fprintf(stderr, "IOLib, getUNCSlice : width+x_offset specified too large (%d+%d=%d > %d) for this image (%s).\n", *width, x_offset, *width+x_offset, wIn, filename);
		imclose(image);
		return NULL;
	}
	/* height requested too big */
	hIn = image->Dimv[image->Dimc-2];
	if( *height <= 0 ) *height = image->Dimv[image->Dimc-2] - y_offset;
	else if( (y_offset+*height) > hIn ){
		fprintf(stderr, "IOLib, getUNCSlice : height+y_offset specified too large (%d+%d=%d > %d) for this image (%s).\n", *height, y_offset, *height+y_offset, hIn, filename);
		imclose(image);
		return NULL;
	}
	/* only 1 slice in image and slice specified was not the first (0) */
	totalSlices = 1;
	if( image->Dimc < 3 ){
		if( slice != 0 ){
			fprintf(stderr, "IOLib, getUNCSlice : slice number specified (%d) too large - there is only 1 slice  in this image (%s).\n", slice, filename);
			imclose(image);
			return NULL;
		}
	} else {
		if( slice >= (totalSlices=image->Dimv[image->Dimc-3]) ){
			fprintf(stderr, "IOLib, getUNCSlice : slice number specified (%d) too large - there are only %d slices in this image (%s) (slice numbering starts from 0).\n", slice, totalSlices, filename);
			imclose(image);
			return NULL;
		}
	}
	/* the pixel format, look in /local/image/src/libim/image.h for the flags
	   to check if is GREY and BYTE do : *format & (GREY|BYTE) */
	*format = image->PixelFormat;
	*depth  = image->PixelSize;

	/* alloc for data in (all the image data) */
	sizeIn = wIn * hIn;
	if( (dataIn=(GREYTYPE *)calloc(sizeIn, sizeof(GREYTYPE))) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlice : failed to malloc %d GREYTYPEs (%zd bytes each).\n", sizeIn, sizeof(GREYTYPE));
		imclose(image);
		return NULL;
	}
	/* alloc for data out (the subimage we are interested in (x_offset, y_offset, *width, *height)) */
	wOut = *width - x_offset; hOut = *height - y_offset;
	sizeOut = wOut * hOut;
	if( (dataOut=(DATATYPE *)calloc(sizeOut, sizeof(DATATYPE))) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlice : failed to malloc %d DATATYPEs (%zd bytes each).\n", sizeOut, sizeof(DATATYPE));
		imclose(image);
		return NULL;
	}
	/* read whole image data but only for the slice we are interested in */
	offset = slice * sizeIn;
	if( imread(image, offset, offset+sizeIn-1, dataIn) == INVALID ){
		fprintf(stderr, "IOLib, getUNCSlice : call to imread failed (start=%d, end=%d pixels).\n", offset, offset+sizeIn);
		free(dataIn); free(dataOut); imclose(image);
		return NULL;
	}
	imclose(image);

	for(j=y_offset,k=0;j<*height;j++)
		for(i=x_offset;i<*width;i++,k++)
			dataOut[k] = (DATATYPE )dataIn[i + j * wIn];

	free(dataIn);
	return dataOut;
}	

/* Function to read a portion or all of a number of slices in a UNC file
   params: filename, the name of the file that contains the data
	   [] x_offset, y_offset integers specifying the upper left corner of the returned imaged
	   (set them to zero for getting the whole image)
	   [] *width, *height integers specifying the width and height of the returned image,
	   set them to zero or negative to acquire the whole image - in this case, when the function
	   returns, their contents will be equal to the returned image width and height minus the offsets specified 
	   [] *slices, an array of integers holding the slice numbers you want to obtain. If you leave this null,
	   it will get ALL the slices in the file
	   [] *numSlices, the size of the *slices array, e.g. how many slices in total do you need?
	   if the *slices array was null, then *numSlices will be equal to the number of slices returned
	   [] *depth and *format will be set to the PixelSize and PixelFormat fields of the IMAGE structure
   returns: NULL on failure, a pointer to a DATATYPE 1D array on success ([bytenumber])
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE	*getUNCSlices1D(char *filename, int x_offset, int y_offset, int *width, int *height, int *slices, int *numSlices, int *depth, int *format){
	IMAGE		*image;
	int		sizeIn, sizeOut, wIn, wOut, hIn, hOut, totalSlices, offset;
	GREYTYPE	*dataIn;
	DATATYPE	*dataOut;
	register int	i, j, k, s, slice;
	
	if( sizeof(GREYTYPE) > sizeof(DATATYPE) )
		fprintf(stderr, "IOLib, getUNCSlices1D : warning, size of image data (%zd bytes) is larger than that of data to be written (GREYTYPE, %zd bytes) - *possible* loss of precision in conversion.\n", sizeof(GREYTYPE), sizeof(DATATYPE));

	if( (image=imopen(filename, READ)) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlices1D : call to imopen failed for '%s'.\n", filename);
		return NULL;
	}
	wIn = image->Dimv[image->Dimc-1];
	if( *width <= 0 ) *width = image->Dimv[image->Dimc-1] - x_offset;
	else if( (*width+x_offset) > wIn ){ 	/* width requested too big */
		fprintf(stderr, "IOLib, getUNCSlices1D : width+x_offset specified too large (%d+%d=%d > %d) for this image (%s).\n", *width, x_offset, *width+x_offset, wIn, filename);
		imclose(image);
		return NULL;
	}
	hIn = image->Dimv[image->Dimc-2];
	if( *height <= 0 ) *height = image->Dimv[image->Dimc-2] - y_offset;
	else if( (*height+y_offset) > hIn ){ 	/* height requested too big */
		fprintf(stderr, "IOLib, getUNCSlices1D : height+y_offset specified too large (%d+%d=%d > %d) for this image (%s).\n", *height, y_offset, *height+y_offset, hIn, filename);
		imclose(image);
		return NULL;
	}
	if( image->Dimc < 3 ) totalSlices = 1;
	else totalSlices = image->Dimv[image->Dimc-3];
	if( slices == NULL ){ /* user did not specify which slices needs, assume all or at least *numSlices if > 0 */
		if( *numSlices <= 0 ) *numSlices = totalSlices;
		else if( *numSlices > totalSlices ){
			fprintf(stderr, "IOLib, getUNCSlices1D : the number of slices requested (%d) exceeds the total number of slices (%d) in the image (%s).\n", *numSlices, totalSlices, filename);
			imclose(image);
			return NULL;
		}
	} else {
		for(i=0;i<*numSlices;i++)
			if( slices[i] > totalSlices ){
				fprintf(stderr, "IOLib, getUNCSlices1D : there is no slice number %d in the image (%s) - it has only %d slices.\n", slices[i], filename, totalSlices);
				imclose(image);
				return NULL;
			}
	}
	/* the pixel format, look in /local/image/src/libim/image.h for the flags
	   to check if is GREY and BYTE do : *format & (GREY|BYTE) */
	*format = image->PixelFormat;
	*depth  = image->PixelSize;

	/* alloc for data in (all the image data) */
	sizeIn = wIn * hIn;
	if( (dataIn=(GREYTYPE *)calloc(sizeIn, sizeof(GREYTYPE))) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlices1D : failed to malloc %d GREYTYPEs (%zd bytes each).\n", sizeIn, sizeof(GREYTYPE));
		imclose(image);
		return NULL;
	}

	wOut = *width; hOut = *height;
	sizeOut = wOut * hOut;
	/* alloc for data out (the subimage we are interested in (x_offset, y_offset, *width, *height)) */
	if( (dataOut=(DATATYPE *)calloc(*numSlices*sizeOut, sizeof(DATATYPE))) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlices1D : failed to malloc %d DATATYPEs (%zd bytes each)\n", *numSlices*sizeOut, sizeof(DATATYPE));
		imclose(image);
		return NULL;
	}

	for(s=0,k=0;s<*numSlices;s++){
		if( slices == NULL ) slice = s; else slice = slices[s];

		/* read whole image data but only for the slice we are interested in */
		offset = slice * sizeIn;
		if( imread(image, offset, offset+sizeIn-1, &(dataIn[0])) == INVALID ){
			fprintf(stderr, "IOLib, getUNCSlices1D : call to imread failed (start=%d, end=%d pixels) for slice number %d.\n", offset, offset+sizeIn, slice);
			free(dataIn); free(dataOut); imclose(image);
			return NULL;
		}
		for(j=y_offset,k=0;j<*height+y_offset;j++)
			for(i=x_offset;i<*width+x_offset;i++,k++)
				dataOut[k] = (DATATYPE )dataIn[i + j * wIn];
	}
	imclose(image);

	free(dataIn);
	return dataOut;
}
/* Function to read a portion or all of a number of slices in a UNC file
   params: filename, the name of the file that contains the data
	   [] x_offset, y_offset integers specifying the upper left corner of the returned imaged
	   (set them to zero for getting the whole image)
	   [] *width, *height integers specifying the width and height of the returned image,
	   set them to zero or negative to acquire the whole image - in this case, when the function
	   returns, their contents will be equal to the returned image width and height minus the offsets specified 
	   [] *slices, an array of integers holding the slice numbers you want to obtain. If you leave this null,
	   it will get ALL the slices in the file
	   [] *numSlices, the size of the *slices array, e.g. how many slices in total do you need?
	   if the *slices array was null, then *numSlices will be equal to the number of slices returned
	   [] *depth and *format will be set to the PixelSize and PixelFormat fields of the IMAGE structure
   returns: NULL on failure, a pointer to a DATATYPE 2D array on success ([sliceNumber][bytenumber])
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE	**getUNCSlices2D(char *filename, int x_offset, int y_offset, int *width, int *height, int *slices, int *numSlices, int *depth, int *format){
	IMAGE		*image;
	int		sizeIn, sizeOut, wIn, wOut, hIn, hOut, totalSlices, offset;
	GREYTYPE	*dataIn;
	DATATYPE	**dataOut;
	register int	i, j, k, s, slice;
	
	if( sizeof(GREYTYPE) > sizeof(DATATYPE) )
		fprintf(stderr, "IOLib, getUNCSlices2D : warning, size of image data (%zd bytes) is larger than that of data to be written (GREYTYPE, %zd bytes) - *possible* loss of precision in conversion.\n", sizeof(GREYTYPE), sizeof(DATATYPE));

	if( (image=imopen(filename, READ)) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlices2D : call to imopen failed for '%s'.\n", filename);
		return NULL;
	}
	wIn = image->Dimv[image->Dimc-1];
	if( *width <= 0 ) *width = image->Dimv[image->Dimc-1] - x_offset;
	else if( (*width+x_offset) > wIn ){ 	/* width requested too big */
		fprintf(stderr, "IOLib, getUNCSlices2D : width+x_offset specified too large (%d+%d=%d > %d) for this image (%s).\n", *width, x_offset, *width+x_offset, wIn, filename);
		imclose(image);
		return NULL;
	}
	hIn = image->Dimv[image->Dimc-2];
	if( *height <= 0 ) *height = image->Dimv[image->Dimc-2] - y_offset;
	else if( (*height+y_offset) > hIn ){ 	/* height requested too big */
		fprintf(stderr, "IOLib, getUNCSlices2D : height+y_offset specified too large (%d+%d=%d > %d) for this image (%s).\n", *height, y_offset, *height+y_offset, hIn, filename);
		imclose(image);
		return NULL;
	}
	if( image->Dimc < 3 ) totalSlices = 1;
	else totalSlices = image->Dimv[image->Dimc-3];
	if( slices == NULL ){ /* user did not specify which slices needs, assume all or at least *numSlices if > 0 */
		if( *numSlices <= 0 ) *numSlices = totalSlices;
		else if( *numSlices > totalSlices ){
			fprintf(stderr, "IOLib, getUNCSlices2D : the number of slices requested (%d) exceeds the total number of slices (%d) in the image (%s).\n", *numSlices, totalSlices, filename);
			imclose(image);
			return NULL;
		}
	} else {
		for(i=0;i<*numSlices;i++)
			if( slices[i] > totalSlices ){
				fprintf(stderr, "IOLib, getUNCSlices2D : there is no slice number %d in the image (%s) - it has only %d slices.\n", slices[i], filename, totalSlices);
				imclose(image);
				return NULL;
			}
	}
	if(slices != NULL ) for(i=0;i<*numSlices;i++) printf("getting slice: %d\n", slices[i]+1);
	/* the pixel format, look in /local/image/src/libim/image.h for the flags
	   to check if is GREY and BYTE do : *format & (GREY|BYTE) */
	*format = image->PixelFormat;
	*depth  = image->PixelSize;

	/* alloc for data in (all the image data) */
	sizeIn = wIn * hIn;
	if( (dataIn=(GREYTYPE *)calloc(sizeIn, sizeof(GREYTYPE))) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlices2D : failed to malloc %d GREYTYPEs (%zd bytes each).\n", sizeIn, sizeof(GREYTYPE));
		imclose(image);
		return NULL;
	}
	/* alloc for data out (the subimage we are interested in (x_offset, y_offset, *width, *height)) */
	wOut = *width; hOut = *height;
	sizeOut = wOut * hOut;
	if( (dataOut=callocDATATYPE2D(*numSlices, sizeOut)) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlices2D : failed to allocate %d x %d DATATYPES of size %zd bytes each.\n", *numSlices, sizeOut, sizeof(DATATYPE));
		imclose(image);
		return NULL;
	}
	for(s=0,k=0;s<*numSlices;s++){
		if( slices == NULL ) slice = s; else slice = slices[s];

		/* read whole image data but only for the slice we are interested in */
		offset = slice * sizeIn;
		if( imread(image, offset, offset+sizeIn-1, &(dataIn[0])) == INVALID ){
			fprintf(stderr, "IOLib, getUNCSlices2D : call to imread failed (start=%d, end=%d pixels) for slice number %d.\n", offset, offset+sizeIn, slice);
			freeDATATYPE2D(dataOut, *numSlices);
			free(dataIn); imclose(image);
			return NULL;
		}
		for(j=y_offset,k=0;j<*height+y_offset;j++)
			for(i=x_offset;i<*width+x_offset;i++,k++)
				dataOut[s][k] = (DATATYPE )dataIn[i + j * wIn];
	}
	imclose(image);

	free(dataIn);
	return dataOut;
}
/* Function to read a portion or all of a number of slices in a UNC file
   params: filename, the name of the file that contains the data
	   [] x_offset, y_offset integers specifying the upper left corner of the returned imaged
	   (set them to zero for getting the whole image)
	   [] *width, *height integers specifying the width and height of the returned image,
	   set them to zero or negative to acquire the whole image - in this case, when the function
	   returns, their contents will be equal to the returned image width and height minus the offsets specified 
	   [] *slices, an array of integers holding the slice numbers you want to obtain. If you leave this null,
	   it will get ALL the slices in the file
	   [] *numSlices, the size of the *slices array, e.g. how many slices in total do you need?
	   if the *slices array was null, then *numSlices will be equal to the number of slices returned
	   [] *depth and *format will be set to the PixelSize and PixelFormat fields of the IMAGE structure
   returns: NULL on failure, a pointer to a DATATYPE 3D array on success ([sliceNumber][x][y])
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE	***getUNCSlices3D(char *filename, int x_offset, int y_offset, int *width, int *height, int *slices, int *numSlices, int *depth, int *format){
	IMAGE		*image;
	DATATYPE	***dataOut;
	if( (dataOut=getUNCSlices3D_withImage(filename, x_offset, y_offset, width, height, slices, numSlices, depth, format, &image)) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlices3D : call to getUNCSlices3D_withImage failed for '%s' (w=%d, h=%d, numSlices=%d, x_of=%d,y_of=%d,depth=%d,format=%d).\n", filename, *width, *height, *numSlices, x_offset, y_offset, *depth, *format);
		return NULL;
	}
	imclose(image);
	return dataOut;
}
/* Function to read a portion or all of a number of slices in a UNC file and also return the IMAGE data structure
   params: filename, the name of the file that contains the data
	   [] x_offset, y_offset integers specifying the upper left corner of the returned imaged
	   (set them to zero for getting the whole image)
	   [] *width, *height integers specifying the width and height of the returned image,
	   set them to zero or negative to acquire the whole image - in this case, when the function
	   returns, their contents will be equal to the returned image width and height minus the offsets specified 
	   [] *slices, an array of integers holding the slice numbers you want to obtain. If you leave this null,
	   it will get ALL the slices in the file
	   [] *numSlices, the size of the *slices array, e.g. how many slices in total do you need?
	   if the *slices array was null, then *numSlices will be equal to the number of slices returned
	   [] *depth and *format will be set to the PixelSize and PixelFormat fields of the IMAGE structure
	   [] **image : the returned IMAGE structure to read header info etc.
   returns: NULL on failure, a pointer to a DATATYPE 3D array on success ([sliceNumber][x][y])

   DO NOT FORGET TO free the image using imclose(image)

   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE	***getUNCSlices3D_withImage(char *filename, int x_offset, int y_offset, int *width, int *height, int *slices, int *numSlices, int *depth, int *format, IMAGE **image){
	int		sizeIn, wIn, wOut, hIn, hOut, totalSlices, offset;
	GREYTYPE	*dataIn;
	DATATYPE	***dataOut;
	register int	i, j, ii, jj, s, slice;

	if( sizeof(GREYTYPE) > sizeof(DATATYPE) )
		fprintf(stderr, "IOLib, getUNCSlices3D_withImage : warning, size of image data (%zd bytes) is larger than that of data to be written (GREYTYPE, %zd bytes) - *possible* loss of precision in conversion.\n", sizeof(GREYTYPE), sizeof(DATATYPE));

	if( (*image=imopen(filename, READ)) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlices3D_withImage : call to imopen failed for '%s'.\n", filename);
		return NULL;
	}
	wIn = (*image)->Dimv[(*image)->Dimc-1];
	if( *width <= 0 ) *width = (*image)->Dimv[(*image)->Dimc-1]-x_offset;
	else if( (*width+x_offset) > wIn ){ 	/* width requested too big */
		fprintf(stderr, "IOLib, getUNCSlices3D_withImage : width+x_offset specified too large (%d+%d=%d > %d) for this image (%s).\n", *width, x_offset, *width+x_offset, wIn, filename);
		imclose(*image);
		return NULL;
	}
	hIn = (*image)->Dimv[(*image)->Dimc-2];
	if( *height <= 0 ) *height = (*image)->Dimv[(*image)->Dimc-2]-y_offset;
	else if( (*height+y_offset) > hIn ){ 	/* height requested too big */
		fprintf(stderr, "IOLib, getUNCSlices3D_withImage : height+y_offset specified too large (%d+%d=%d > %d) for this image (%s).\n", *height, y_offset, *height+y_offset, hIn, filename);
		imclose(*image);
		return NULL;
	}
	if( (*image)->Dimc < 3 ) totalSlices = 1;
	else totalSlices = (*image)->Dimv[(*image)->Dimc-3];
	if( slices == NULL ){ /* user did not specify which slices needs, assume all or at least *numSlices if > 0 */
		if( *numSlices <= 0 ) *numSlices = totalSlices;
		else if( *numSlices > totalSlices ){
			fprintf(stderr, "IOLib, getUNCSlices3D_withImage : the number of slices requested (%d) exceeds the total number of slices (%d) in the image (%s).\n", *numSlices, totalSlices, filename);
			imclose(*image);
			return NULL;
		}
	} else {
		for(i=0;i<*numSlices;i++)
			if( slices[i] > totalSlices ){
				fprintf(stderr, "IOLib, getUNCSlices3D_withImage : there is no slice number %d in the image (%s) - it has only %d slices.\n", slices[i], filename, totalSlices);
				imclose(*image);
				return NULL;
			}
	}
	if(slices != NULL ) for(i=0;i<*numSlices;i++) printf("getting slice: %d\n", slices[i]+1);
	/* the pixel format, look in /local/image/src/libim/image.h for the flags
	   to check if is GREY and BYTE do : *format & (GREY|BYTE) */
	*format = (*image)->PixelFormat;
	*depth  = (*image)->PixelSize;

	/* alloc for data in (all the image data) */
	sizeIn = wIn * hIn;
	if( (dataIn=(GREYTYPE *)calloc(sizeIn, sizeof(GREYTYPE))) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlices3D_withImage : failed to malloc %d GREYTYPEs (%zd bytes each).\n", sizeIn, sizeof(GREYTYPE));
		imclose(*image);
		return NULL;
	}
	wOut = *width; hOut = *height;
	/* alloc for data out (the subimage we are interested in (x_offset, y_offset, *width, *height)) */
	if( (dataOut=callocDATATYPE3D(*numSlices, wOut, hOut)) == NULL ){
		fprintf(stderr, "IOLib, getUNCSlices3D_withImage : failed to allocate %d x %d x %d DATATYPES of size %zd bytes each.\n", *numSlices, wOut, hOut, sizeof(DATATYPE));
		imclose(*image);
		return NULL;
	}

	for(s=0;s<*numSlices;s++){
		if( slices == NULL ) slice = s; else slice = slices[s];

		/* read whole image data but only for the slice we are interested in */
		offset = slice * sizeIn;
		if( imread(*image, offset, offset+sizeIn-1, &(dataIn[0])) == INVALID ){
			fprintf(stderr, "IOLib, getUNCSlices3D_withImage : call to imread failed (start=%d, end=%d pixels) for slice number %d.\n", offset, offset+sizeIn, slice);
			freeDATATYPE3D(dataOut, *numSlices, wOut);
			free(dataIn); imclose(*image);
			return NULL;
		}
		for(jj=0,j=y_offset;j<*height+y_offset;j++,jj++)
			for(ii=0,i=x_offset;i<*width+x_offset;i++,ii++)
				dataOut[s][ii][jj] = (DATATYPE )dataIn[i + j * wIn];
	}
	free(dataIn);
	return dataOut;
}

/* Function to write a single slice of data in UNC format to file
   params: [] filename, the name of the file to write the data (it should not exist else it will be overwritten)
	   [] data, an array of pixels (DATATYPE format/type) to write to file, not necessarily
	      all of them will be written to file, that depends on width, height and slicenumber
	   [] dataW, dataH, the dimensions of data (e.g. the whole image)
	   [] sliceW, sliceH, the dimensions of the slice you want to write to file (must be smaller/equal than image)
	   [] x_offset, sliceYOffset the x and y offsets of the slice sub-image
	   [] format PixelFormat of the data (must be the same as that of image)
   returns: VALID on success, INVALID on failure
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	writeUNCSlice(char *filename, DATATYPE *data, int dataW, int dataH, int x_offset, int y_offset, int sliceW, int sliceH, int slice, int format, int mode){
	IMAGE	*image;
	int	dims[3], ww, hh;
	GREYTYPE *oData = NULL;
	register int	i, j, k;

/*	if( sizeof(GREYTYPE) < sizeof(DATATYPE) )
		fprintf(stderr, "IOLib, writeUNCSlice : warning, size of input data (%zd bytes) is larger than that of data to be written (GREYTYPE, %zd bytes) - *possible* loss of precision in conversion.\n", sizeof(DATATYPE), sizeof(GREYTYPE));
*/

	/* the dimensions of UNC (not the data because we might be writing a subimage) */
	dims[2] = sliceW; dims[1] = sliceH; dims[0] = 1;
	ww = sliceW+x_offset; hh = sliceH+y_offset;
	if( (oData=(GREYTYPE *)calloc(sliceW * sliceH, sizeof(GREYTYPE))) == NULL ){
		fprintf(stderr, "IOLib, writeUNCSlice : could not malloc %d GREYTYPES.\n", sliceW*sliceH);
		return INVALID;
	}
	for(j=y_offset,k=0;j<hh;j++)
		for(i=x_offset;i<ww;i++,k++)
			oData[k] = (GREYTYPE )data[i + j * dataW];
	if( (image=openImage(filename, mode, format, 3, dims)) == NULL ){
		fprintf(stderr, "IOLib, writeUNCSlice : call to openImage has failed.\n");
		free(oData);
		return INVALID;
	}
/* note that imwrite writes from A to B inclusive, hence A, B-1 below */
	if( imwrite(image, 0, sliceW * sliceH-1, oData) == INVALID ){
		fprintf(stderr, "IOLib, writeUNCSlice : call to imwrite has failed for file '%s' and %d pixels (slice was %d dims: %dx%d and data dims: %dx%d).\n", filename, sliceW * sliceH, slice, sliceW, sliceH, dataW, dataH); perror("");
		free(oData);
		imclose(image);
		return INVALID;
	}
	free(oData);
	imclose(image);
	return VALID;
}
/* Function to write many slices of data in UNC format to file, data is contained in 1D array (no split between different slices)
   params: [] filename, the name of the file to write the data (it should not exist else it will be overwritten)
	   [] data, a 1D array of pixels (DATATYPE format/type) to write to file, not necessarily
	      all of them will be written to file, that depends on width, height and slicenumber
	      Index starts from 0 for the top,left corner pixel of the 1st slice and increments
	      from left to right, top to bottom.
	   [] dataW, dataH, the dimensions of data (e.g. the whole image)
	   [] sliceW, sliceH, the dimensions of the slice you want to write to file (must be smaller/equal than image)
	   [] x_offset, y_offset the x and y offsets of the slice sub-image
	   [] format PixelFormat of the data (must be the same as that of image)
	   [] numSlices, the number of slices you want to write
	   [] *slices, an array of slice numbers you want to write, if this is null,
	      slices from 0 to numSlices will be written.
   returns: VALID on success, INVALID on failure
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	writeUNCSlices1D(char *filename, DATATYPE *data, int dataW, int dataH, int x_offset, int y_offset, int sliceW, int sliceH, int *slices, int numSlices, int format, int mode){
	IMAGE		*image;
	int		dims[3], s, ww, hh;
	GREYTYPE	*oData = NULL;
	DATATYPE	*p;
	register int	i, j, k, slice;

/*	if( sizeof(GREYTYPE) < sizeof(DATATYPE) )
		fprintf(stderr, "IOLib, writeUNCSlices1D : warning, size of input data (%zd bytes) is larger than that of data to be written (GREYTYPE, %zd bytes) - *possible* loss of precision in conversion.\n", sizeof(DATATYPE), sizeof(GREYTYPE));
*/

	/* the dimensions of UNC (not the data because we might be writing a subimage) */
	dims[2] = sliceW; dims[1] = sliceH; dims[0] = numSlices;
	ww = sliceW+x_offset; hh = sliceH+y_offset;
	if( (oData=(GREYTYPE *)calloc(sliceW * sliceH, sizeof(GREYTYPE))) == NULL ){
		fprintf(stderr, "IOLib, writeUNCSlices1D : could not malloc %d GREYTYPES.\n", sliceW*sliceH);
		return INVALID;
	}

	if( (image=openImage(filename, mode, format, 3, dims)) == NULL ){
		fprintf(stderr, "IOLib, writeUNCSlices1D : call to openImage has failed.\n");
		free(oData);
		return INVALID;
	}
	for(s=0;s<numSlices;s++){
		if( slices == NULL ) slice = s; else slice = slices[s];
		p = &(data[slice * dataW * dataH]);
		for(j=y_offset,k=0;j<hh;j++)
			for(i=x_offset;i<ww;i++,k++)
				oData[k] = (GREYTYPE )p[i + j * dataW];
/* note that imwrite writes from A to B inclusive, hence A, B-1 below */
		if( imwrite(image, s*sliceW*sliceH, s*sliceW*sliceH+sliceW*sliceH-1, oData) == INVALID ){
			fprintf(stderr, "IOLib, writeUNCSlices1D : call to imwrite has failed for file '%s' and %d pixels (slice was %d dims: %dx%d and data dims: %dx%d).\n", filename, sliceW * sliceH, slice, sliceW, sliceH, dataW, dataH);
			free(oData);
			imclose(image);
			return INVALID;
		}
	}
	free(oData);
	imclose(image);
	return VALID;
}
/* Function to write many slices of data in UNC format to file, data is contained in 2D array ([sliceNumber][pixelIndex])
   params: [] filename, the name of the file to write the data (it should not exist else it will be overwritten)
	   [] data, a 2D array of pixels (DATATYPE format/type) to write to file, not necessarily
	      all of them will be written to file, that depends on width, height and slicenumber,
	      the first element of data selects the slice number, the second the pixel's index.
	      Index starts from 0 in the top, left corner of the image and increments from left to right,
	      top to bottom
	   [] dataW, dataH, the dimensions of data (e.g. the whole image)
	   [] sliceW, sliceH, the dimensions of the slice you want to write to file (must be smaller/equal than image)
	   [] x_offset, y_offset the x and y offsets of the slice sub-image
	   [] format PixelFormat of the data (must be the same as that of image)
	   [] numSlices, the number of slices you want to write
	   [] *slices, an array of slice numbers you want to write, if this is null,
	      slices from 0 to numSlices will be written.
   returns: VALID on success, INVALID on failure
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	writeUNCSlices2D(char *filename, DATATYPE **data, int dataW, int dataH, int x_offset, int y_offset, int sliceW, int sliceH, int *slices, int numSlices, int format, int mode){
	IMAGE		*image;
	int		dims[3], s, ww, hh;
	GREYTYPE	*oData = NULL;
	register int	i, j, k, slice;

/*	if( sizeof(GREYTYPE) < sizeof(DATATYPE) )
		fprintf(stderr, "IOLib, writeUNCSlices2D : warning, size of input data (%zd bytes) is larger than that of data to be written (GREYTYPE, %zd bytes) - *possible* loss of precision in conversion.\n", sizeof(DATATYPE), sizeof(GREYTYPE));
*/

	/* the dimensions of UNC (not the data because we might be writing a subimage) */
	dims[2] = sliceW; dims[1] = sliceH; dims[0] = numSlices;
	ww = sliceW+x_offset; hh = sliceH+y_offset;
	if( (oData=(GREYTYPE *)calloc(sliceW * sliceH, sizeof(GREYTYPE))) == NULL ){
		fprintf(stderr, "IOLib, writeUNCSlice : could not malloc %d GREYTYPES.\n", sliceW*sliceH);
		return INVALID;
	}

	if( (image=openImage(filename, mode, format, 3, dims)) == NULL ){
		fprintf(stderr, "IOLib, writeUNCSlices2D : call to openImage has failed.\n");
		free(oData);
		return INVALID;
	}
	for(s=0;s<numSlices;s++){
		if( slices == NULL ) slice = s; else slice = slices[s];
		for(j=y_offset,k=0;j<hh;j++)
			for(i=x_offset;i<ww;i++,k++)
				oData[k] = (GREYTYPE )data[slice][i + j * dataW];
/* note that imwrite writes from A to B inclusive, hence A, B-1 below */
		if( imwrite(image, s*sliceW*sliceH, s*sliceW*sliceH+sliceW*sliceH-1, oData) == INVALID ){
			fprintf(stderr, "IOLib, writeUNCSlices2D : call to imwrite has failed for file '%s' and %d pixels (slice was %d dims: %dx%d and data dims: %dx%d).\n", filename, sliceW * sliceH, slice, sliceW, sliceH, dataW, dataH);
			free(oData);
			imclose(image);
			return INVALID;
		}
	}
	free(oData);
	imclose(image);
	return VALID;
}

/* Function to write many slices of data in UNC format to file, data is contained in 2D array ([sliceNumber][pixelIndex])
   params: [] filename, the name of the file to write the data (it should not exist else it will be overwritten)
	   [] data, a 3D array of pixels (DATATYPE format/type) to write to file, not necessarily
	      all of them will be written to file, that depends on width, height and slicenumber,
	      the first element of data selects the slice number, the second the pixel's X-direction
	      and the third, the pixel's Y-coordinate
	   [] dataW, dataH, the dimensions of data (e.g. the whole image)
	   [] sliceW, sliceH, the dimensions of the slice you want to write to file (must be smaller/equal than image)
	   [] x_offset, y_offset the x and y offsets of the slice sub-image
	   [] format PixelFormat of the data (must be the same as that of image)
	   [] numSlices, the number of slices you want to write
	   [] *slices, an array of slice numbers you want to write, if this is null,
	      slices from 0 to numSlices will be written.
   returns: VALID on success, INVALID on failure
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	writeUNCSlices3D(char *filename, DATATYPE ***data, int dataW, int dataH, int x_offset, int y_offset, int sliceW, int sliceH, int *slices, int numSlices, int format, int mode){
	IMAGE		*image;
	int		dims[3], s, ww, hh;
	GREYTYPE	*oData = NULL, *p;
	register int	i, j, slice;

/*	if( sizeof(GREYTYPE) < sizeof(DATATYPE) )
		fprintf(stderr, "IOLib, writeUNCSlice3D : warning, size of input data (%zd bytes) is larger than that of data to be written (GREYTYPE, %zd bytes) - *possible* loss of precision in conversion.\n", sizeof(DATATYPE), sizeof(GREYTYPE));
*/
	/* the dimensions of UNC (not the data because we might be writing a subimage) */
	dims[2] = sliceW; dims[1] = sliceH; dims[0] = numSlices;
	ww = sliceW+x_offset; hh = sliceH+y_offset;
	if( (oData=(GREYTYPE *)malloc(sliceW * sliceH * sizeof(GREYTYPE))) == NULL ){
		fprintf(stderr, "IOLib, writeUNCSlice : could not malloc %d GREYTYPES.\n", sliceW*sliceH);
		return INVALID;
	}

	if( (image=openImage(filename, mode, format, 3, dims)) == NULL ){
		fprintf(stderr, "IOLib, writeUNCSlices3D : call to openImage has failed for file '%s'.\n", filename);
		free(oData);
		return INVALID;
	}
	for(s=0;s<numSlices;s++){
		if( slices == NULL ) slice = s; else slice = slices[s];
		for(i=0,p=&(oData[0]);i<sliceW*sliceH;i++) *p = 0;
		for(j=y_offset,p=&(oData[0]);j<hh;j++)
			for(i=x_offset;i<ww;i++,p++)
				/* here you have danger to overflow since source is int and destination is short */
				/* when you see negative numbers in images (and not meant to be there),
				   here is most likely to be the problem */
				*p = (GREYTYPE )data[slice][i][j];
/* note that imwrite writes from A to B inclusive, hence A, B-1 below */
		if( imwrite(image, s*sliceW*sliceH, (s+1)*sliceW*sliceH-1, oData) == INVALID ){
			fprintf(stderr, "IOLib, writeUNCSlices3D : call to imwrite has failed for file '%s' and %d pixels (slice was %d dims: %dx%d and data dims: %dx%d).\n", filename, sliceW * sliceH, slice, sliceW, sliceH, dataW, dataH);
			free(oData);
			imclose(image);
			return INVALID;
		}
	}
	free(oData);
	imclose(image);
	return VALID;
}

/* Function to open/create UNC data file for reading/writing
   params: [] filename, the name of the file to write the data (it should not exist else it will be overwritten)
	   [] mode (see image.h also) can be UPDATE, CREATE, READ, OVERWRITE(which is the same as CREATE)
	the following 3 parameters are important when mode is CREATE.
	if mode is other, then some or all of these parameters can be set to <=0
	This function can do some checks in order to make sure that the data
	in the data file (if UPDATE/READ mode) is of the same size/format as the one we
	want to dump. In order to skip ALL the dimension checks, set numDimensions <= 0
	if you want to skip some dimension checks, set that dimension (e.g. dimensions[i]) <= 0
	   [] format, pixel format
	   [] numDimensions, the number of dimensions
	   [] *dimensions the dimensions array ([0]=slices, [1]=height, [2]=width
   returns: VALID on success, INVALID on failure
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
IMAGE	*openImage(char *filename, int mode, int format, int numDimensions, int *dimensions){
	IMAGE	*image;
	int	error, i;
	
	switch( mode ){
		case CREATE: /* also overwrite */
			unlink(filename);
			/* printf("file: %s\nformat: %d\n%d\n%d %d %d\n", filename, format, numDimensions, dimensions[0], dimensions[1], dimensions[2]);*/
			return imcreat(filename, 0644, format, numDimensions, dimensions);
		case UPDATE:
			if( (image=imopen(filename, mode)) == INVALID ){
				fprintf(stderr, "IOLib, openImage : call to imopen (UPDATE/READ) has failed for '%s'.\n", filename);
				return NULL;
			}
			/* do no checks if the parameters are 'don't care' (e.g. negative or zero) */
			if( numDimensions <= 0 ) return image;
			/* do checks */
			error  = (image->PixelFormat>0) && (image->PixelFormat!=format);
			error &= (image->Dimc>0) && (image->Dimc!=numDimensions);
			if( !error && (dimensions!=NULL) )
				for(i=0;(i<numDimensions)&&(!error);i++)
					error &= (image->Dimv[i]>0) && (image->Dimv[i]!=dimensions[i]);
			if( error ){
				fprintf(stderr, "IOLib, openImage(UPDATE) : parameter mismatch between file and parameters supplied (format=%d/%d, numDimensions=%d/%d it could also be that the image dimensions were smaller than the ones specified.\n", format, image->PixelFormat, numDimensions, image->Dimc);
				imclose(image);
				return NULL;
			}
			return image;
		case READ:
			if( (image=imopen(filename, mode)) == INVALID ){
				fprintf(stderr, "IOLib, openImage : call to imopen (READ) has failed for '%s'.\n", filename);
				return NULL;
			}
			/* do no checks if the parameters are 'don't care' (e.g. negative or zero) */
			if( numDimensions <= 0 ) return image;
			/* do checks */
			error  = (image->PixelFormat>0) && (image->PixelFormat!=format);
			error &= (image->Dimc>0) && (image->Dimc!=numDimensions);
			if( !error && (dimensions!=NULL) )
				for(i=0;(i<numDimensions)&&(!error);i++)
					error &= (image->Dimv[i]>0) && (image->Dimv[i]<dimensions[i]);
			if( error ){
				fprintf(stderr, "IOLib, openImage(READ) : parameter mismatch between file and parameters supplied (format=%d/%d, numDimensions=%d/%d it could also be that the image dimensions were not the same as the ones specified.\n", format, image->PixelFormat, numDimensions, image->Dimc);
				imclose(image);
				return NULL;
			}
			return image;
		default:
			fprintf(stderr, "IOLib, openImage : invalid mode - it can only be CREATE(=OVERWRITE), UPDATE, READ.\n");
			return NULL;
	}
}
