#ifndef	_THRESHOLD_VISIT

/* this is a constant which instructs the threshold routines
   to leave the color unchanged within the specified range.
   it must be of type DATATYPE but given that we have negative pixel intensities,
   is it safe to have it -1?
   In case of funny behaviour with threshold, come here */
#define	THRESHOLD_LEAVE_UNCHANGED	-1

/* enum type for logical operations */
/* NOP means NoOperation, leaves values as are */
typedef enum {NOP=0, AND=1, OR=2, NOT=3, XOR=4} logicalOperation;
/* these constants here tell us what operation was done on a given pixel after threshold
   was it left unchanged, was it changed or was it given a default color.
   A 'change map' will be of the same size as the image and for each pixel
   in the image, it will hold one of these values below, so that we should know
   after threshold is done what was done - useful when we will want to AND/OR/NOT
   several threshold operations */
typedef	enum {changed=0,  unchanged=1, default_color=2} changemap_entry;
/* tables which tell us, given 2 changemap_entry values and a logical operation,
   what the result should be.
   [NOP/AND/OR/NOT/XOR][v1 value][v2 value] v1 and v2 can be unchanged or changed
   In the case of NOT, the result will be NOT(v1), v2 will be ignored.
   DO NOT CHANGE THE ORDER OF THE AND/OR/... they depend on the enum values of logicalOperation above
   DO NOT CHANGE THE ORDER OF THE changed/unchanged etc. they depend on the enum values of changemap_entry
*/
extern changemap_entry	thresholdTruthTable[5][2][2];

/* This structure and typedef are for 'spec'
   a spec in the Threshold functions tell us a range of values
   of some pixel property (e.g. intensity, or frequency of occurence)
   and a newValue pixel intensity. The threshold functions will
   replace the intensity of the pixels whose respective property falls within
   the specified range with the pixel intensity 'newValue'.
   Remember that thresholding can be on the basis of not only pixel intensity
   but also frequency, deviation from mean etc, so 'low' and 'high' could
   not have been DATATYPEs. I hope this does not cause any bug. Better typecast
   them to DATATYPE whenever used as pixel values. */
typedef struct {
	int		low,
			high;
	DATATYPE	newValue;
} spec;

/* Threshold function protos */
int	pixel_threshold1D(DATATYPE *data, int offset, int size, spec *rangeSpec, int numRangeSpec, DATATYPE defaultColor, char *changeMap);
int	pixel_threshold2D(DATATYPE **data, int x, int y, int w, int h, spec *rangeSpec, int numRangeSpec, DATATYPE defaultColor, char **changeMap);
int	pixel_threshold3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, spec *rangeSpec, int numRangeSpec, DATATYPE defaultColor, char ***changeMap);
int	frequency_threshold1D(DATATYPE *data, int offset, int size, spec *rangeSpec, int numRangeSpec, DATATYPE defaultColor, histogram hist, char *changeMap);
int	frequency_threshold2D(DATATYPE **data, int x, int y, int w, int h, spec *rangeSpec, int numRangeSpec, DATATYPE defaultColor, histogram hist, char **changeMap);
int	frequency_threshold3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, spec *rangeSpec, int numRangeSpec, DATATYPE defaultColor, histogram hist, char ***changeMap);

#define	_THRESHOLD_VISIT
#endif
