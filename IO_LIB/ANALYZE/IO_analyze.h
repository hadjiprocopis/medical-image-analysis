#ifndef	_IO_ANALYZE_H
#define	_IO_ANALYZE_H

/* analyze_db.h is the original analyze file format header from mayo */
/* sometimes aka dbh.h is on the net */
#include <analyze_db.h>

typedef struct header_key       analyzeHK;
typedef struct image_dimension  analyzeID;
typedef struct data_history     analyzeDH;
typedef struct dsr              analyze_header;

/* this macro takes an analyze header and checks to see,
   through some doubtful heuristics, whether it needs to
   have its bytes swapped (reversed) because it came from
   a machine with different Endianess than the host */
#define	ANALYZE_HEADER_NEEDS_BYTE_SWAP(_header) (!(((_header)->hk.extents == 16384) || ((_header)->hk.sizeof_hdr == 348)))

/* prototypes */
analyze_header	*new_analyze_header(void);
analyze_header	*clone_analyze_header(analyze_header */*input*/);
analyze_header	*set_endianess_of_analyze_header(analyze_header */*header*/, int /*endian*/);
analyze_header	*reverse_endianess_of_analyze_header(analyze_header */*header*/);

analyze_header	*readANALYZEHeader(FILE */*fp*/, int */*swapped_bytes*/);
int		writeANALYZEHeader(FILE */*fp*/, analyze_header */*header*/, int /*do_byte_swapping*/);

DATATYPE		**getRAWSlice(
	char */*filename*/,
				/* all offsets start from zero up to (bounds-1) */
	int /*src_width*/,	/* image dimensions */
	int /*src_height*/,
				/* top-left corner of portion to read */
	int /*dst_x*/, int /*dst_y*/,
				/* dimensions of portion to read */
	int /*dst_width*/, int /*dst_height*/,
	int /*slice*/ 		/* the slice number to read */,
	int /*depth*/,		/* bytes per pixel, e.g. 1 for 8-bit(256 colors), 2 for 16-bit (65536 colors) etc. */
	int /*mode*/		/* BIG_ENDIAN OR LITTLE_ENDIAN */
);
DATATYPE		***getRAWSlices(
	char */*filename*/,
				/* all offsets start from zero up to (bounds-1) */
	int /*src_width*/,	/* image dimensions */
	int /*src_height*/,
				/* top-left corner of portion to read */
	int /*dst_x*/, int /*dst_y*/,	
	int /*dst_z*/,		/* slice number to start from in the case when 'slices' is NULL,
				   otherwise it is ignored */
	int /*dst_width*/,
	int /*dst_height*/,	/* dimensions of portion to read */

	int /*numSlices*/,	/* how many slices do you want to read ? */

	int */*slices*/,	/* if this is not NULL, it must be an array of length 'numSlices',
				   it contains a list of slice numbers to get, z_offset will be ignored,
				   if it is NULL, we will get all slices from z_offset to z_offset+numSlices */
	int /*depth*/,		/* bytes per pixel, e.g. 1 for 8-bit(256 colors), 2 for 16-bit (65536 colors) etc. */
	int /*mode*/		/* BIG_ENDIAN OR LITTLE_ENDIAN */
);
int	writeDATATYPE2DasRAW(
	char */*filename*/,	/* output file */
	DATATYPE **/*dataIn*/,	/* input data array 2D */
				/* all offsets start from zero up to (bounds-1) */
	int /*src_width*/,	/* data array dimensions */
	int /*src_height*/,
	int /*dst_x*/, int /*dst_y*/,	/* top-left corner of portion to write */
	int /*dst_width*/, int /*dst_height*/,	/* dimensions of portion to write (e.g. the final image on file dimensions) */
	int /*depth*/,		/* specify bytes per pixel, e.g. 1 for 8-bit(256 colors), 2 for 16-bit (65536 colors) etc. */
	int /*mode*/		/* specify endianess : IO_RAW_LITTLE_ENDIAN or IO_RAW_BIG_ENDIAN */
);
int	writeDATATYPE3DasRAW(
	char */*filename*/,	/* output file */
	DATATYPE ***/*dataIn*/,	/* input data array 3D */
				/* all offsets start from zero up to (bounds-1) */
	int /*src_width*/,	/* input data array dimensions */
	int /*src_height*/,
	int /*dst_x*/, int /*dst_y*/,	/* top-left corner of portion to write */
	int /*dst_z*/,		/* slice number to start from in the case when 'slices' is NULL,
				   otherwise it is ignored */
	int /*dst_width*/,
	int /*dst_height*/,	/* dimensions of portion to write */

	int /*numSlices*/,	/* how many slices do you want to write ? */

	int */*slices*/,	/* if this is not NULL, it must be an array of length 'numSlices',
				   it contains a list of slice numbers to write
				   and z_offset will be ignored.
				   If 'slices' is NULL,
				   we will write all slices from z_offset to z_offset+numSlices */

	int /*depth*/,		/* specify bytes per pixel, e.g. 1 for 8-bit(256 colors), 2 for 16-bit (65536 colors) etc. */
	int /*mode*/		/* specify endianess : IO_RAW_LITTLE_ENDIAN or IO_RAW_BIG_ENDIAN */
);
#endif
