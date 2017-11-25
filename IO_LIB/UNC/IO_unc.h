#ifndef _IO_UNC_HEADER
#define _IO_UNC_HEADER

/* image.h is part of libim (/local/image/include and libraries at /local/image/lib/) */
#include "image.h"

/* these protos should have been in iminfo.h but they are not ... */
//EXTERNAL_LINKAGE	int     imgetinfo();
//EXTERNAL_LINKAGE	int     put_info(); /* iam not using imputinfo because it does not return any value back ... strange */

/* constants */
#define	OVERWRITE	CREATE

/* various macros */
#ifndef ABS
#define	ABS(a)	((a) < 0 ? (-(a)):(a))
#endif

#ifndef DATATYPE
#define	DATATYPE	int
#endif

/* Operations during header copy of UNC files
   'header' refers to the output of the command
   header UNCfile
   wheareas 'info' refers to the output (much longer)
   of the command:
   header -i UNCfile
*/
#ifndef IO_TITLE_COPY
#define IO_TITLE_COPY           0x1
#define IO_INFO_COPY            0x2
#define IO_INFO_COPY_NOT        0x4
#endif /* IO_TITLE_COPY */

/* function prototypes */
EXTERNAL_LINKAGE	DATATYPE	*getUNCSlice(char *filename, int x_offset, int y_offset, int *width, int *height, int slice, int *depth, int *format);
EXTERNAL_LINKAGE	DATATYPE	*getUNCSlices1D(char *filename, int x_offset, int y_offset, int *width, int *height, int *slices, int *numSlices, int *depth, int *format);
EXTERNAL_LINKAGE	DATATYPE	**getUNCSlices2D(char *filename, int x_offset, int y_offset, int *width, int *height, int *slices, int *numSlices, int *depth, int *format);
EXTERNAL_LINKAGE	DATATYPE	***getUNCSlices3D(char *filename, int x_offset, int y_offset, int *width, int *height, int *slices, int *numSlices, int *depth, int *format);
EXTERNAL_LINKAGE	DATATYPE	***getUNCSlices3D_withImage(char *filename, int x_offset, int y_offset, int *width, int *height, int *slices, int *numSlices, int *depth, int *format, IMAGE **image);
EXTERNAL_LINKAGE	int		writeUNCSlice(char *filename, DATATYPE *data, int dataW, int dataH, int x_offset, int y_offset, int sliceW, int sliceH, int slice, int format, int mode);
EXTERNAL_LINKAGE	int		writeUNCSlices3D(char *filename, DATATYPE ***data, int dataW, int dataH, int x_offfset, int y_offset, int sliceW, int sliceH, int *slices, int numSlices, int format, int mode);
EXTERNAL_LINKAGE	int		writeUNCSlices2D(char *filename, DATATYPE **data, int dataW, int dataH, int x_offfset, int y_offset, int sliceW, int sliceH, int *slices, int numSlices, int format, int mode);
EXTERNAL_LINKAGE	int		writeUNCSlices1D(char *filename, DATATYPE *data, int dataW, int dataH, int x_offfset, int y_offset, int sliceW, int sliceH, int *slices, int numSlices, int format, int mode);
EXTERNAL_LINKAGE	IMAGE		*openImage(char *filename, int mode, int format, int numDimensions, int *dimensions);
EXTERNAL_LINKAGE	char		copyUNCInfo(char *filename_from, char *filename_to, int op, char **fields, int num_fields);
EXTERNAL_LINKAGE	void		imdestroy(IMAGE *image);
EXTERNAL_LINKAGE	void		infodestroy(Info *info);
EXTERNAL_LINKAGE	Info		*readUNCInfoFromTextFile(FILE */*fp*/, char ***/*names*/, char ***/*values*/, int */*numEntries*/);
EXTERNAL_LINKAGE	int		writeUNCInfoToTextFile(FILE */*fp*/, Info */*im_info_p*/);

#endif
