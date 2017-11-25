#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "IO_analyze.h"
#include "IO_analyzeP.h"
#include "../IO_constants.h"

#define	SWAP_TWO_BYTES(_b1,_b2,_tmp) ((_tmp)=(_b1);(_b1)=(_b2);(_b2)=(_tmp))

/* reverse the order of bytes (i.e. byte swapping but for n bytes rather than with two) */
/* this is private */
void	_byte_reverse(char */*b*/, size_t /*n*/);
void	_byte_reverse(char *b, size_t n){
	int	i;
	char	*f, *l, tmp;
	for(i=0,f=&(b[0]),l=&(b[n-1]);i<n/2;i++,f++,l--){
		tmp = *f;
		*f = *l;
		*l = tmp;
	}
}

/* function to allocate memory for a new analyze header with empty fields
   returns a new analyze header with empty fields on success
   or NULL on failure.

   author: Andreas Hadjiprocopis, NMR, ION, 2001, CING 2005
*/
analyze_header	*new_analyze_header(void){
	analyze_header	*ret;
	if( (ret=(analyze_header *)malloc(sizeof(analyze_header))) == NULL ){
		fprintf(stderr, "new_analyze_header : could not allocate %ld bytes for ret.\n", sizeof(analyze_header));
		return (analyze_header *)NULL;
	}
	return ret;
}	
/* function to copy an analyze header and return this copy
   returns a new analyze header with identical fields with input (clone) on success
   or NULL on failure.

   author: Andreas Hadjiprocopis, NMR, ION, 2001, CING 2005
*/
analyze_header	*clone_analyze_header(analyze_header *input){
	analyze_header	*ret;
	int	i, j;
	void	*p, *pr;

	if( (ret=new_analyze_header()) == NULL ){
		fprintf(stderr, "clone_analyze_header : call to new_analyze_header has failed.\n");
		return (analyze_header *)NULL;
	}

	/* we have to go through each entry of the header and call the byte_reverse
	   function with appropriate sizes */
	/* NOTE : if the order of the structures in struct dsr is ever changed, we
	   need to change this. If the data types or more data is added,
	   we need to correct IO_analyzeP.h */
	/* for the first sub-structure of the header (see analyze_db.h) called 'hk' */
	for(i=0,p=&(input->hk),pr=&(ret->hk);i<_analyze_header_entries_header_key_SIZE;i++){
		for(j=0;j<_analyze_header_entries_header_key[i].number_of_items;j++){
			memcpy(pr, p, _analyze_header_entries_header_key[i].size_of_each_item);
			p += _analyze_header_entries_header_key[i].size_of_each_item;
			pr += _analyze_header_entries_header_key[i].size_of_each_item;
		}
	}
	/* for the second sub-structure of the header (see analyze_db.h) called 'dime' */
	for(i=0,p=&(input->dime),pr=&(ret->dime);i<_analyze_header_entries_image_dimension_SIZE;i++){
		for(j=0;j<_analyze_header_entries_image_dimension[i].number_of_items;j++){
			memcpy(pr, p, _analyze_header_entries_image_dimension[i].size_of_each_item);
			p += _analyze_header_entries_image_dimension[i].size_of_each_item;
			pr += _analyze_header_entries_image_dimension[i].size_of_each_item;
		}
	}
	/* for the third sub-structure of the header (see analyze_db.h) called 'hist' */
	for(i=0,p=&(input->hist),pr=&(ret->hist);i<_analyze_header_entries_data_history_SIZE;i++){
		for(j=0;j<_analyze_header_entries_data_history[i].number_of_items;j++){
			memcpy(pr, p, _analyze_header_entries_data_history[i].size_of_each_item);
			p += _analyze_header_entries_data_history[i].size_of_each_item;
			pr += _analyze_header_entries_data_history[i].size_of_each_item;
		}
	}
	return ret;
}	

/* function which changes the endianess of an analyze header.

   returns a new analyze header with specified endianess
   or NULL on failure.

   NOTE : do not forget to free returned header when done 

   author: Andreas Hadjiprocopis, NMR, ION, 2001, CING 2005
*/
analyze_header	*set_endianess_of_analyze_header(analyze_header *header, int endian){
	unsigned char	*c;

	/* let's see what the endianess of the header is */
	c = (unsigned char *)(&(header->hk.sizeof_hdr));
	if( (header->hk.sizeof_hdr & 0x000000FF) == *c ){
		/* LSB is the first one, little endian */
		if( endian == IO_RAW_LITTLE_ENDIAN ){
			/* we need the output to be little endian, so no change just return input */
			return clone_analyze_header(header);
		}
	}
	/* we need to reverse bytes */
	return reverse_endianess_of_analyze_header(header);
}
/* function which swaps the bytes (actually reverses) of the data
   of an analyze header.
   Why do you want to use this function?
   You are reading an analyze file produced on a SUN machine - your machine is INTEL,
   The endianess of the machines differs, you will not read good data.
   So, you must call this function.
   The actual read/writeANALYZEHeader functions do this automatically for you
   so you do not need to call this function at all during read/writes unless
   you want to do something low-level.

   returns a new analyze header with bytes swapped (relative to the input header)
   or NULL on failure

   author: Andreas Hadjiprocopis, NMR, ION, 2001, CING 2005

*/
analyze_header	*reverse_endianess_of_analyze_header(analyze_header *header){
	analyze_header	*ret;
	int		i, j;
	char		*tmp;
	void		*p, *pr;

	if( (tmp=(char *)malloc(50*sizeof(char))) == NULL ){
		fprintf(stderr, "reverse_endianess_of_analyze_header : could not allocate %ld bytes for tmp.\n", 50*sizeof(char));
		return (analyze_header *)NULL;
	}

	if( (ret=(analyze_header *)malloc(sizeof(analyze_header))) == NULL ){
		fprintf(stderr, "reverse_endianess_of_analyze_header : could not allocate %ld bytes for ret.\n", sizeof(analyze_header));
		free(tmp);
		return (analyze_header *)NULL;
	}

	/* we have to go through each entry of the header and call the byte_reverse
	   function with appropriate sizes */
	/* NOTE : if the order of the structures in struct dsr is ever changed, we
	   need to change this. If the data types or more data is added,
	   we need to correct IO_analyzeP.h */
	/* for the first sub-structure of the header (see analyze_db.h) called 'hk' */
	for(i=0,p=&(header->hk),pr=&(ret->hk);i<_analyze_header_entries_header_key_SIZE;i++){
		if( !strcmp(_analyze_header_entries_header_key[i].type, "char") ) continue; /* no byte swapping for chars */
		for(j=0;j<_analyze_header_entries_header_key[i].number_of_items;j++){
			memcpy((void *)tmp, p, _analyze_header_entries_header_key[i].size_of_each_item);
			_byte_reverse(tmp, _analyze_header_entries_header_key[i].size_of_each_item);
			memcpy(pr, tmp, _analyze_header_entries_header_key[i].size_of_each_item);
			p += _analyze_header_entries_header_key[i].size_of_each_item;
			pr += _analyze_header_entries_header_key[i].size_of_each_item;
		}
	}
	/* for the second sub-structure of the header (see analyze_db.h) called 'dime' */
	for(i=0,p=&(header->dime),pr=&(ret->dime);i<_analyze_header_entries_image_dimension_SIZE;i++){
		if( !strcmp(_analyze_header_entries_image_dimension[i].type, "char") ) continue; /* no byte swapping for chars */
		for(j=0;j<_analyze_header_entries_image_dimension[i].number_of_items;j++){
			memcpy((void *)tmp, p, _analyze_header_entries_image_dimension[i].size_of_each_item);
			_byte_reverse(tmp, _analyze_header_entries_image_dimension[i].size_of_each_item);
			memcpy(pr, tmp, _analyze_header_entries_image_dimension[i].size_of_each_item);
			p += _analyze_header_entries_image_dimension[i].size_of_each_item;
			pr += _analyze_header_entries_image_dimension[i].size_of_each_item;
		}
	}
	/* for the third sub-structure of the header (see analyze_db.h) called 'hist' */
	for(i=0,p=&(header->hist),pr=&(ret->hist);i<_analyze_header_entries_data_history_SIZE;i++){
		if( !strcmp(_analyze_header_entries_data_history[i].type, "char") ) continue; /* no byte swapping for chars */
		for(j=0;j<_analyze_header_entries_data_history[i].number_of_items;j++){
			memcpy((void *)tmp, p, _analyze_header_entries_data_history[i].size_of_each_item);
			_byte_reverse(tmp, _analyze_header_entries_data_history[i].size_of_each_item);
			memcpy(pr, tmp, _analyze_header_entries_data_history[i].size_of_each_item);
			p += _analyze_header_entries_data_history[i].size_of_each_item;
			pr += _analyze_header_entries_data_history[i].size_of_each_item;
		}
	}
	return ret;
}

/* function to write analyze header
	fp is a file descriptor obtained by fopen("test.hdr", "wb"); wb=write/binary
	the flag do_byte_swapping if TRUE tells us to reverse the endianess of the
	header before we write it on file
	use check_system_endianess() (found in IO/MISC/IO_misc.c) to determine
	the host's system endianess (and therefore the endianess of the current
	header) and then depending on your target system's endianess decide
	to do byte swapping or not.

	'byte swapping' is not the exact term as it refers to the case of
	data types with only 2 bytes. the actual term should be perhaps
	'reverse the byte order'.

	author: Andreas Hadjiprocopis, NMR, ION, 2001, CING 2005

	returns TRUE(1) on success and FALSE(0) otherwise
*/
int		writeANALYZEHeader(FILE *fp, analyze_header *input, int do_byte_swapping){
	analyze_header	*tmp = NULL, *header = &(input[0]);
	int	i;
	void	*p;

	/* swap bytes if requested */
	if( do_byte_swapping ){
		if( (tmp=reverse_endianess_of_analyze_header(header)) ){
			fprintf(stderr, "writeANALYZEHeader : call to reverse_endianess_of_analyze_header has failed.\n");
			return FALSE;
		}
		header = &(tmp[0]);
	}
	/* we have to go through each entry of the header and save it */
	/* NOTE : if the order of the structures in struct dsr is ever changed, we
	   need to change this. If the data types or more data is added,
	   we need to correct IO_analyzeP.h */
	/* for the first sub-structure of the header (see analyze_db.h) called 'hk' */
	for(i=0,p=&(header->hk);i<_analyze_header_entries_header_key_SIZE;i++){
		if( fwrite(p, _analyze_header_entries_header_key[i].size_of_each_item, _analyze_header_entries_header_key[i].number_of_items, fp) != _analyze_header_entries_header_key[i].number_of_items ){
				fprintf(stderr, "writeANALYZEHeader : could not write %d items (%ld bytes each) for entry %d of the first structure.\n", _analyze_header_entries_header_key[i].number_of_items, _analyze_header_entries_header_key[i].size_of_each_item, i);
				return FALSE;
		}
		p += _analyze_header_entries_header_key[i].size_of_each_item * _analyze_header_entries_header_key[i].number_of_items;
	}
	/* for the second sub-structure of the header (see analyze_db.h) called 'dime' */
	for(i=0,p=&(header->dime);i<_analyze_header_entries_image_dimension_SIZE;i++){
		if( fwrite(p, _analyze_header_entries_image_dimension[i].size_of_each_item, _analyze_header_entries_image_dimension[i].number_of_items, fp) != _analyze_header_entries_image_dimension[i].number_of_items ){
				fprintf(stderr, "writeANALYZEHeader : could not write %d items (%ld bytes each) for entry %d of the first structure.\n", _analyze_header_entries_image_dimension[i].number_of_items, _analyze_header_entries_image_dimension[i].size_of_each_item, i);
				return FALSE;
		}
		p += _analyze_header_entries_image_dimension[i].size_of_each_item * _analyze_header_entries_image_dimension[i].number_of_items;
	}
	/* for the third sub-structure of the header (see analyze_db.h) called 'hist' */
	for(i=0,p=&(header->hist);i<_analyze_header_entries_data_history_SIZE;i++){
		if( fwrite(p, _analyze_header_entries_data_history[i].size_of_each_item, _analyze_header_entries_data_history[i].number_of_items, fp) != _analyze_header_entries_data_history[i].number_of_items ){
				fprintf(stderr, "writeANALYZEHeader : could not write %d items (%ld bytes each) for entry %d of the first structure.\n", _analyze_header_entries_data_history[i].number_of_items, _analyze_header_entries_data_history[i].size_of_each_item, i);
				return FALSE;
		}
		p += _analyze_header_entries_data_history[i].size_of_each_item * _analyze_header_entries_data_history[i].number_of_items;
	}
	if( do_byte_swapping ) free(tmp);
	return TRUE;
}

/* function to read analyze header
	fp is a file descriptor obtained by fopen("test.hdr", "rb"); rb=read binary
	swapped_bytes is a flag which works as follows,

	if set to DONTKNOW (see Common_IMMA.h) then it checks whether byte swapping
	is needed (see IO_analyze.h for the test) and it does it if necessary.
	If byte swap was done, then the swapped_bytes flag is set to TRUE
	so that you will know when you read the image data (would need swapping too probably!),
	if not, it is set to FALSE.

	if set to TRUE, it does byte swapping anyway even it is not needed (useful to export to other
	systems with different endianess with less intelligent analyze readers).

	if set to FALSE, it does not do byte swapping even if it is needed

	author: Andreas Hadjiprocopis, NMR, ION, 2001, CING 2005

	returns the header on success or NULL on failure
*/
analyze_header	*readANALYZEHeader(FILE *fp, int *swapped_bytes){
	size_t		size_analyze_header = sizeof(analyze_header);
	analyze_header	*header, *swapped;

	/* data to read the header in -- it is raw bytes because we might need to do byte swap */
	if( (header=(analyze_header *)malloc(size_analyze_header)) == NULL ){
		fprintf(stderr, "readANALYZEHeader : could not allocate %ld bytes for header.\n", size_analyze_header);
		return (analyze_header *)NULL;
	}

	/* read in the header bytes */		
	fread(header, size_analyze_header, 1, fp);
	if( ferror(fp) ){
		fprintf(stderr, "readANALYZEHeader : error reading header from file.\n");
		perror("fread failed");
		free(header);
		return (analyze_header *)NULL;
	}

	/* check to see if byte-swapping is needed */
	if( *swapped_bytes == DONTKNOW ){
		if( (*swapped_bytes=ANALYZE_HEADER_NEEDS_BYTE_SWAP(header)) == TRUE ){
			/* we need to swap: system's endianness does not match file's endianness */
			if( (swapped=reverse_endianess_of_analyze_header(header)) == NULL ){
				fprintf(stderr, "readANALYZEHeader : call to reverse_endianess_of_analyze_header has failed.\n");
				free(header);
				return (analyze_header *)NULL;
			}
			free(header); return swapped;
		} /* if byte swap is needed */
	} else if( *swapped_bytes == TRUE ){
		/* force byte swap */		
		if( (swapped=reverse_endianess_of_analyze_header(header)) == NULL ){
			fprintf(stderr, "readANALYZEHeader : call to reverse_endianess_of_analyze_header has failed (2).\n");
			free(header);
			return (analyze_header *)NULL;
		}
		free(header); return swapped;
	}
	return header;
}

/* Function to read a portion or all of a single slice in a file of type RAW
   (note, ANALYZE and UNC file formats store image pixels as RAW,
    this function can be used to read analyze image files (in conjunction with header readers above)

   params: filename, the name of the file that contains the data
	   [] src_width, src_height : dimensions of each slice of the data
	   [] dst_x, dst_y : the top-left corner of the data you want to read,
	   		     set them to zero if you want to read all
	   [] dst_width, dst_height : the slice dimensions of the data you want to read
	   [] slice, the slice number you want to read ** it starts from zero for first slice **
	   [] depth is the number of bytes per pixel (e.g. 2 for 16 bit, 3 for 24 etc)
	   [] mode it can be either IO_RAW_LITTLE_ENDIAN or IO_RAW_BIG_ENDIAN
	      and defines how bytes will be converted to integers, only in the case
	      where depth > 1
   returns: NULL on failure, a pointer to a 2D DATATYPE array on success
   NOTE : you must free the returned data when finished with it using:
	  indexed as data[column][row]
   	freeDATATYPE2D(data, dst_width);
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE		**getRAWSlice(
	char *filename,
				/* all offsets start from zero up to (bounds-1) */
	int src_width,		/* image dimensions */
	int src_height,
	int dst_x, int dst_y,	/* top-left corner of portion to read */
	int dst_width, int dst_height,	/* dimensions of portion to read */
	int slice 		/* the slice number to read */,
	int depth,		/* bytes per pixel, e.g. 1 for 8-bit(256 colors), 2 for 16-bit (65536 colors) etc. */
	int mode		/* IO_RAW_LITTLE_ENDIAN or IO_RAW_BIG_ENDIAN */
){
	long		length, offset;
	DATATYPE	**dataOut;
	register int	i, *c, j, k, *pC, maxRange2 = 128, maxRange = 256;
	register unsigned char	*p;
	unsigned char	*tmp;
	int		conv[10], fd, d;

/*if __BYTE_ORDER == __BIG_ENDIAN*/

	if( depth > 1 ){
		/* depth is >1 we have to cope with different endianess, construct an array
		   with multipliers for each byte, depending on its significance */
		switch(mode){
			case	IO_RAW_LITTLE_ENDIAN:
				for(i=1,conv[0]=1;i<depth;i++) conv[i] = conv[i-1] * 256;
				maxRange = conv[depth-1]; maxRange2 = maxRange / 2;
				break;
			case	IO_RAW_BIG_ENDIAN:
				for(i=1,conv[0]=(int )(pow(256,(depth-1)));i<depth;i++) conv[i] = conv[i-1] / 256;
				maxRange = conv[0]; maxRange2 = maxRange / 2;
				break;
			default:
				fprintf(stderr, "getRAWSlice : unknown mode '%d' (see IO.h for modes).\n", mode);
				return (DATATYPE **)NULL;
		}		
	} else {
		/* depth is 1, 1 byte data, don't care about endianess */			
		conv[0] = 1;
	}
	pC = &(conv[0]);

	if( (fd=open(filename, O_RDONLY)) == -1 ){
		fprintf(stderr, "getRAWSlice : could not open file '%s' for reading.\n", filename);
		perror("file open failed");
		return (DATATYPE **)NULL;
	}

	if( (offset=((slice * src_width * src_height + dst_y * src_width) * depth)) > 0 )
		if( lseek(fd, offset, SEEK_SET) != offset ){
			fprintf(stderr, "getRAWSlice : could not move to requested slice %d (to byte %ld) - out of bounds problem?\n", slice, offset);
			perror("lseek failed");
			close(fd);
			return (DATATYPE **)NULL;
		}
	/* read bytes */
	/* although we skip dst_y rows, we have to read the unwanted columns dst_x and then discard them */
	length = (src_width * dst_height) * depth;
	if( (tmp=(unsigned char *)malloc(length*sizeof(char))) == NULL ){
		fprintf(stderr, "IOLib, getANALYZESlices2D : failed to allocate %ld bytes for tmp.\n", length*sizeof(char));
		close(fd);
		return (DATATYPE **)NULL;
	}
	if( read(fd, tmp, length) != length ){
		fprintf(stderr, "getRAWSlice : could not read %ld bytes from file '%s'. I did an lseek before with offset = %ld.\n", length, filename, offset);
		perror("read failed");
		free(tmp); close(fd);
		return (DATATYPE **)NULL;
	}
	close(fd);
	if( (dataOut=callocDATATYPE2D(dst_width, dst_height)) == NULL ){
		fprintf(stderr, "IOLib, getANALYZESlices2D : failed to allocate %d x %d DATATYPES of size %ld bytes each (dataOut).\n", dst_width, dst_height, sizeof(DATATYPE));
		free(tmp);
		return (DATATYPE **)NULL;
	}

	for(j=0;j<dst_height;j++){
		/* find a line */
		p = &(tmp[(j*src_width + dst_x)*depth]);
		/* and read that line */
		for(i=0;i<dst_width;i++,d++){
			for(k=0,d=0,c=pC;k<depth;k++,c++,p++) d += (*c) * (*p);
			dataOut[i][j] = (d >= maxRange2) ? (d - maxRange) : (d);
		}
	}
	free(tmp);			
	return dataOut;
}	
/* Function to read a portion or all of multiple slices in a file of type RAW
   params: filename, the name of the file that contains the data
	   [] src_width, src_height : dimensions of each slice of the data
	   [] dst_x, dst_y : the top-left corner of the data you want to read,
	   		     set them to zero if you want to read all
	   [] dst_z : the slice you want to start reading data from,
	   	      this number will be ignored if 'slices' is NOT NULL.
	   [] dst_width, dst_height : the slice dimensions of the data you want to read
	   [] numSlices : how many slices you want to read
	   [] *slices : an array of integers denoting slice numbers you want to read
			for example, if you want to read 1st, 5th then second slice,
			set slices = {0, 4, 1} (because slice numbers start from zero, like
			all other coordinates).
			If you set this to NULL, then 'numSlices' will be read one
			after the other, starting from slice 'dst_z'
	   [] depth is the number of bytes per pixel (e.g. 2 for 16 bit, 3 for 24 etc)
	   [] mode it can be either IO_RAW_LITTLE_ENDIAN or IO_RAW_BIG_ENDIAN
	      and defines how bytes will be converted to integers, only in the case
	      where depth > 1
   returns: NULL on failure, a pointer to a 3D DATATYPE array on success
	    indexed as data[slice][column][row]
   returns: FALSE on failure, TRUE otherwise
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE		***getRAWSlices(
	char *filename,
				/* all offsets start from zero up to (bounds-1) */
	int src_width,		/* image dimensions */
	int src_height,
	int dst_x, int dst_y,	/* top-left corner of portion to read */
	int dst_z,		/* slice number to start from in the case when 'slices' is NULL,
				   otherwise it is ignored */
	int dst_width,
	int dst_height,		/* dimensions of portion to read */

	int numSlices,		/* how many slices do you want to read ? */

	int *slices,		/* if this is not NULL, it must be an array of length 'numSlices',
				   it contains a list of slice numbers to get, z_offset will be ignored,
				   if it is NULL, we will get all slices from z_offset to z_offset+numSlices */
	int depth,		/* bytes per pixel, e.g. 1 for 8-bit(256 colors), 2 for 16-bit (65536 colors) etc. */
	int mode		/* IO_RAW_LITTLE_ENDIAN or IO_RAW_BIG_ENDIAN */
){
	long		length, offset;
	DATATYPE	***dataOut;
	register int	i, *c, j, k, s, ss, X, *pC, maxRange2 = 128;
	register unsigned char	*p;
	unsigned char	*tmp;
	int		conv[10], fd, d, maxRange = 256;

/*if __BYTE_ORDER == __BIG_ENDIAN*/

	if( depth > 1 ){
		/* depth is >1 we have to cope with different endianess, construct an array
		   with multipliers for each byte, depending on its significance */
		switch(mode){
			case	IO_RAW_LITTLE_ENDIAN:
				for(i=1,conv[0]=1;i<depth;i++) conv[i] = conv[i-1] * 256;
				maxRange = conv[depth-1]; maxRange2 = maxRange / 2;
				break;
			case	IO_RAW_BIG_ENDIAN:
				for(i=1,conv[0]=(int )(pow(256,(depth-1)));i<depth;i++) conv[i] = conv[i-1] / 256;
				maxRange = conv[0]; maxRange2 = maxRange / 2;
				break;
			default:
				fprintf(stderr, "getRAWSlices : unknown mode '%d' (see IO.h for modes).\n", mode);
				return (DATATYPE ***)NULL;
		}		
	} else {
		/* depth is 1, 1 byte data, don't care about endianess */			
		conv[0] = 1;
	}
	pC = &(conv[0]);

	if( (fd=open(filename, O_RDONLY)) == -1 ){
		fprintf(stderr, "getRAWSlices : could not open file '%s' for reading.\n", filename);
		perror("file open failed");
		return (DATATYPE ***)NULL;
	}

	if( (dataOut=callocDATATYPE3D(numSlices, dst_width, dst_height)) == NULL ){
		fprintf(stderr, "IOLib, getANALYZESlices2D : failed to allocate %d x %d x %d DATATYPES of size %ld bytes each (dataOut).\n", numSlices, dst_width, dst_height, sizeof(DATATYPE));
		close(fd);
		return (DATATYPE ***)NULL;
	}

	if( slices == NULL ){
		/* this means we must read all slices from z_offset to z_offset + numSlices */
		/* so do an lseek now and then is just one single read */
		if( (offset=(dst_z * src_width * src_height * depth)) > 0 )
			if( lseek(fd, offset, SEEK_SET) != offset ){
				fprintf(stderr, "getRAWSlices : could not move to requested slice %d (to byte %ld) - out of bounds problem?\n", dst_z, offset);
				perror("lseek failed");
				freeDATATYPE3D(dataOut, numSlices, dst_width); close(fd);
				return (DATATYPE ***)NULL;
			}
		/* temporary array of bytes to fit all numSlices read from file */
		length = numSlices * src_width * src_height * depth;
		if( (tmp=(unsigned char *)malloc(length*sizeof(char))) == NULL ){
			fprintf(stderr, "IOLib, getANALYZESlices2D : failed to allocate %ld bytes for tmp.\n", length*sizeof(char));
			close(fd); freeDATATYPE3D(dataOut, numSlices, dst_width);
			return (DATATYPE ***)NULL;
		}
		/* this is the single read -- only in the case we read contiguous blocks of image */
		/* otherwise we will be doing smaller reads for each slice */
		if( read(fd, tmp, length) != length ){
			fprintf(stderr, "getRAWSlices : could not read %ld bytes from file '%s' (all in one go). Before I did an lseek with offset of %ld.\n", length, filename, offset);
			perror("read failed");
			freeDATATYPE3D(dataOut, numSlices, dst_width); free(tmp); close(fd);
			return (DATATYPE ***)NULL;
		}
	} else {
		/* temporary array of bytes to fit just one slice read from file -- this array will
		   be used once for each slice */
		length = (src_width * src_height) * depth;
		if( (tmp=(unsigned char *)malloc(length * depth * sizeof(char))) == NULL ){
			fprintf(stderr, "IOLib, getANALYZESlices2D : failed to allocate %ld bytes for tmp.\n", length * depth * sizeof(char));
			close(fd); freeDATATYPE3D(dataOut, numSlices, dst_width);
			return (DATATYPE ***)NULL;
		}
	}		

	for(ss=0;ss<numSlices;ss++){
		if( slices != NULL ){
			s = slices[ss];
			/* do an lseek and then a read for each slice specified in the array 'slices' */
			offset = s * src_width * src_height * depth;
			if( lseek(fd, offset, SEEK_SET) != offset ){
				fprintf(stderr, "getRAWSlices : could not move to requested slice %d (to byte %ld) while dealing with slice %d - out of bounds problem?\n", dst_z, offset, s);
				perror("lseek failed");
				freeDATATYPE3D(dataOut, numSlices, dst_width); free(tmp); close(fd);
				return (DATATYPE ***)NULL;
			}
			length = (src_width * src_height) * depth;
			if( read(fd, tmp, length) != length ){
				fprintf(stderr, "getRAWSlices : could not read %ld bytes from file '%s' while dealing with slice %d. Before I did an lseek with offset of %ld\n", length, filename, s, offset);
				perror("read failed");
				freeDATATYPE3D(dataOut, numSlices, dst_width); free(tmp); close(fd);
				return (DATATYPE ***)NULL;
			}
			s = 0;
		} else {
			s = ss;
		}

		X = s*src_width*src_height + dst_y*src_width + dst_x;
		for(j=0;j<dst_height;j++){
			p = &(tmp[(X + j*src_width)*depth]); /* find the beginning of a line */
			/* and now read a line */
			for(i=0;i<dst_width;i++,d++){
				for(k=0,d=0,c=pC;k<depth;k++,c++,p++) d += (*c) * (*p);
				dataOut[ss][i][j] = (d >= maxRange2) ? (d - maxRange) : (d);
			}
		}
	}
	close(fd);
	free(tmp);			
	return dataOut;
}	
/* Function to write a portion or all of a single slice in a file of type RAW
   (note, ANALYZE and UNC file formats store image pixels as RAW,
    this function can be used to write analyze image files (in conjunction with header writers)

   params: filename, the name of the file that contains the data
	   [] dataIn : the 2D array of DATATYPE containing the data you want to write to file
	   [] src_width, src_height : dimensions of each slice of the data in the dataIn array
	   [] dst_x, dst_y : the top-left corner of the data you want to write,
	   		     set them to zero if you want to write from the beginning
	   [] dst_width, dst_height : the slice dimensions of the data you want to write
	   [] depth is the number of bytes per pixel (e.g. 2 for 16 bit, 3 for 24 etc)
	   [] mode it can be either IO_RAW_LITTLE_ENDIAN or IO_RAW_BIG_ENDIAN
	      and defines how integers will be converted to bytes,
	      where depth > 1
   returns: FALSE on failure, TRUE otherwise
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	writeDATATYPE2DasRAW(
	char *filename,
	DATATYPE **dataIn,
				/* all offsets start from zero up to (bounds-1) */
	int src_width,		/* data array dimensions */
	int src_height,
	int dst_x, int dst_y,	/* top-left corner of portion to write */
	int dst_width, int dst_height,	/* dimensions of portion to write (e.g. the final image on file dimensions) */
	int depth,		/* specify bytes per pixel, e.g. 1 for 8-bit(256 colors), 2 for 16-bit (65536 colors) etc. */
	int mode		/* specify endianess : IO_RAW_LITTLE_ENDIAN or IO_RAW_BIG_ENDIAN */
){
	long		length;
	register int	i, *co, *sh, j, k, *pC, *pS, d, maxRange = 256;
	register unsigned char	*p;
	unsigned char	*tmp;
	int		conv[10], shift[10], fd;

/*if __BYTE_ORDER == __BIG_ENDIAN*/

	if( depth > 1 ){
		/* depth is >1 we have to cope with different endianess, construct an array
		   with multipliers for each byte, depending on its significance and an
		   array of shifts, if you want to write integer 12*256*256 + 19*256 + 15 = 791311
		   do:
		   	MSB = (791311 & 0xFF0000) >> 16
		   	next= (791311 & 0x00FF00) >> 8
		   	LSB = (791311 & 0x0000FF) >> 0

		   if the number is negative, we add 2^(8*depth) / 2 and convert that
	   	*/
		switch(mode){
			case	IO_RAW_LITTLE_ENDIAN:
				for(i=1,conv[0]=1,shift[0]=0;i<depth;i++){
					conv[i] = conv[i-1] * 256;
					shift[i] = shift[i-1] + 8;
				}
				maxRange = conv[depth-1];
				break;
			case	IO_RAW_BIG_ENDIAN:
				for(i=depth-2,conv[depth-1]=1,shift[0]=0;i>=0;i--){
					conv[i] = conv[i+1] * 256;
					shift[i] = shift[i+1] + 8;
				}
				maxRange = conv[0];
				break;
			default:
				fprintf(stderr, "writeDATATYPE2DasRAW : unknown mode '%d' (see IO.h for modes).\n", mode);
				return FALSE;
		}		
	} else {
		/* depth is 1, 1 byte data, don't care about endianess */			
		conv[0] = 1;
	}
	pC = &(conv[0]);
	pS = &(shift[0]);

	length = dst_width * dst_height * depth;
	if( (tmp=(unsigned char *)malloc(length*sizeof(char))) == NULL ){
		fprintf(stderr, "writeDATATYPE2DasRAW : could not allocate %ld bytes for tmp (image buffer).\n", length*sizeof(char));
		return FALSE;
	}
	if( (fd=open(filename, O_WRONLY|O_CREAT|O_TRUNC)) == -1 ){
		fprintf(stderr, "writeDATATYPE2DasRAW : could not open file '%s' for writing.\n", filename);
		perror("file open failed"); free(tmp);
		return FALSE;
	}
	for(i=dst_x,p=&(tmp[0]);i<(dst_x+dst_width);i++)
		for(j=dst_y;j<(dst_y+dst_height);j++){
			d = ((k=dataIn[i][j]) < 0) ? (k+maxRange) : k;
			for(k=0,co=pC,sh=pS;k<depth;k++,p++,co++,sh++)
				*p = (d & (*co)) >> (*sh);
		}
	if( write(fd, tmp, length * sizeof(unsigned char)) != length ){
		fprintf(stderr, "writeDATATYPE2DasRAW : could not write %ld bytes to file '%s'.\n", length, filename);
		perror("file write failed");
		free(tmp); close(fd);
		return FALSE;
	}		
	close(fd);
	free(tmp);
	return TRUE;
}

/* Function to write a portion or all of a single slice in a file of type RAW
   (note, ANALYZE and UNC file formats store image pixels as RAW,
    this function can be used to read analyze image files (in conjunction with header readers above)

   params: filename, the name of the file that contains the data
	   [] dataIn : the 3D array of DATATYPE containing the data you want to write to file
	   [] src_width, src_height : dimensions of each slice of the data in the dataIn array
	   [] dst_x, dst_y : the top-left corner of the data you want to write,
	   		     set them to zero if you want to write from the beginning
	   [] dst_width, dst_height : the slice dimensions of the data you want to write
	   [] depth is the number of bytes per pixel (e.g. 2 for 16 bit, 3 for 24 etc)
	   [] mode it can be either IO_RAW_LITTLE_ENDIAN or IO_RAW_BIG_ENDIAN
	      and defines how integers will be converted to bytes,
	      where depth > 1
   returns: NULL on failure, a pointer to a 2D DATATYPE array on success
   NOTE : you must free the returned data when finished with it using:
	  indexed as data[column][row]
   	freeDATATYPE2D(data, dst_width);
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	writeDATATYPE3DasRAW(
	char *filename,
	DATATYPE ***dataIn,
				/* all offsets start from zero up to (bounds-1) */
	int src_width,		/* input data array dimensions */
	int src_height,
	int dst_x, int dst_y,	/* top-left corner of portion to write */
	int dst_z,		/* slice number to start from in the case when 'slices' is NULL,
				   otherwise it is ignored */
	int dst_width,
	int dst_height,		/* dimensions of portion to write */

	int numSlices,		/* how many slices do you want to write ? */

	int *slices,		/* if this is not NULL, it must be an array of length 'numSlices',
				   it contains a list of slice numbers to write
				   and z_offset will be ignored.
				   If 'slices' is NULL,
				   we will write all slices from z_offset to z_offset+numSlices */

	int depth,		/* specify bytes per pixel, e.g. 1 for 8-bit(256 colors), 2 for 16-bit (65536 colors) etc. */
	int mode		/* specify endianess : IO_RAW_LITTLE_ENDIAN or IO_RAW_BIG_ENDIAN */
){
	long		length;
	register int	i, *co, *sh, j, k, *pC, *pS, d, maxRange = 256, s;
	register unsigned char	*p;
	unsigned char	*tmp;
	int		conv[10], shift[10], fd, ss;

/*if __BYTE_ORDER == __BIG_ENDIAN*/

	if( depth > 1 ){
		/* depth is >1 we have to cope with different endianess, construct an array
		   with multipliers for each byte, depending on its significance and an
		   array of shifts, if you want to write integer 12*256*256 + 19*256 + 15 = 791311
		   do:
		   	MSB = (791311 & 0xFF0000) >> 16
		   	next= (791311 & 0x00FF00) >> 8
		   	LSB = (791311 & 0x0000FF) >> 0

		   if the number is negative, we add 2^(8*depth) / 2 and convert that
	   	*/
		switch(mode){
			case	IO_RAW_LITTLE_ENDIAN:
				for(i=1,conv[0]=0xFF,shift[0]=0;i<depth;i++){
					conv[i] = conv[i-1] << 8;
					shift[i] = shift[i-1] + 8;
				}
				maxRange = conv[depth-1];
				break;
			case	IO_RAW_BIG_ENDIAN:
				for(i=depth-2,conv[depth-1]=0xFF,shift[0]=0;i>=0;i--){
					conv[i] = conv[i+1] << 8;
					shift[i] = shift[i+1] + 8;
				}
				maxRange = conv[0];
				break;
			default:
				fprintf(stderr, "writeDATATYPE3DasRAW : unknown mode '%d' (see IO.h for modes).\n", mode);
				return FALSE;
		}		
	} else {
		/* depth is 1, 1 byte data, don't care about endianess */			
		conv[0] = 1;
	}
	pC = &(conv[0]);
	pS = &(shift[0]);

	length = dst_width * dst_height * numSlices * depth;
	if( (tmp=(unsigned char *)malloc(length*sizeof(unsigned char))) == NULL ){
		fprintf(stderr, "writeDATATYPE3DasRAW : could not allocate %ld bytes for tmp (image buffer).\n", length*sizeof(unsigned char));
		return FALSE;
	}
	if( (fd=open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) == -1 ){
		fprintf(stderr, "writeDATATYPE3DasRAW : could not open file '%s' for writing.\n", filename);
		perror("file open failed"); free(tmp);
		return FALSE;
	}
	for(ss=0,p=&(tmp[0]);ss<numSlices;ss++){
		s = (slices == NULL ) ? ss : slices[ss];
		for(i=dst_x;i<(dst_x+dst_width);i++)
			for(j=dst_y;j<(dst_y+dst_height);j++){
				d = ((k=dataIn[s][i][j]) < 0) ? (k+maxRange) : k;
				for(k=0,co=pC,sh=pS;k<depth;k++,p++,co++,sh++)
					*p = (d & (*co)) >> (*sh);
			}
	}

	if( write(fd, tmp, length * sizeof(unsigned char)) != length ){
		fprintf(stderr, "writeDATATYPE3DasRAW : could not write %ld bytes to file '%s'.\n", length, filename);
		perror("file write failed");
		free(tmp); close(fd);
		return FALSE;
	}		
	close(fd);
	free(tmp);
	return TRUE;
}
