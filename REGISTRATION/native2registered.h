/* from AIR3.08/src/AIR.h */
typedef unsigned char my_pixels;
#define PIX_SIZE_ERR .0001      /* Voxel sizes that differ by less than this value are assumed identical */
#define	AIR_CONFIG_MAX_PATH_LENGTH	128
#define	AIR_CONFIG_MAX_COMMENT_LENGTH	128
#define	AIR_CONFIG_RESERVED_LENGTH	116

typedef	struct _NATIVE2REGISTERED_AIR_KEY_INFO {
	unsigned int	bits;
	unsigned int	x_dim;
	unsigned int	y_dim;
	unsigned int	z_dim;
	double		x_size;
	double		y_size;
	double		z_size;
} native2registered_AIR_Key_info;

typedef struct _NATIVE2REGISTERED_AIR_AIR16 {
	double					e[4][4];
	char			    		s_file[AIR_CONFIG_MAX_PATH_LENGTH];
	native2registered_AIR_Key_info		s;
	char					r_file[AIR_CONFIG_MAX_PATH_LENGTH];
	native2registered_AIR_Key_info		r;
	char					comment[AIR_CONFIG_MAX_COMMENT_LENGTH];
	unsigned long int			s_hash;
	unsigned long int			r_hash;
	unsigned short				s_volume; /* Not used in this version of AIR */
	unsigned short				r_volume; /* Not used in this version of AIR */
	char					reserved[AIR_CONFIG_RESERVED_LENGTH];
} native2registered_AIR_Air16;

int	native2registered(
		int **inputCoordinates,	/* input coordinates in native space - origin is top-left corner (e.g. dispunc) */
		int **outputCoordinates,/* output coordinates in registered space - origin the same as above (e.g. dispunc) */		
		int numCoordinates,	/* the number of coordinates to convert */
		native2registered_AIR_Air16 *air,
		char shouldUseAIR3_08Conventions, /* TRUE if should use version AIR 3.08, FALSE neans to use AIR 5.2.5 conventions */
		int x_dimout,
		int y_dimout,
		int z_dimout,
		float x_distance,
		float y_distance,
		float z_distance,
		float x_shift,
		float y_shift,
		float z_shift,
		int cubic);
int	native2registered_AIR_read_air16(const char *filename, native2registered_AIR_Air16 *air1);
void	native2registered_print_AIR_parameters(FILE *a_handle, native2registered_AIR_Air16 air1);
/*
void	analyseResliceMatrix(float **matrix, int dimX, int dimY, int dimZ, float pixel_size_x, float pixel_size_y, float pixel_size_z, float *theta, float *phi, float *psi, float *xshift, float *yshift, float *zshift, float *xscale, float *yscale, float *zscale, float *xscaleShift, float *yscaleShift, float *zscaleShift, float *xview, float *yview, float *zview, char verboseFlag);
void	native2registered(float **matrix, float **coordinates, int numCoordinates, float pixel_size_x, float pixel_size_y, float pixel_size_z, char verboseFlag, float **newCoordinates, float *T);
float	**native2registered_read_matrix_from_file(char *filename);
float	**native2registered_read_coordinates_from_file(char *filename, int *numCoordinates);
*/
