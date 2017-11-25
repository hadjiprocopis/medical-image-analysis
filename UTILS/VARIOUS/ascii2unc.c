#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "\
\
";

const	char	Usage[] = "options as follows:\n\
\t -i inputFilename\n\
	(ascii data file which contains entities consisting of:\n\
            3 coordinates (x,y,z) and a pixel value\n\
         all numbers are separated by white space. Newlines are\n\
         not necessary but do not do any harm.)]\n\
\n\
\t -o outputFilename\n\
	(Output filename of the UNC file to produce)\n\
\t -d w:h:s\n\
	(specify the output image dimensions\n\
		width 'w'\n\
		height 'h'\n\
		number of slices 's'.)]\n\
\n\
\t -c\n\
	flag to indicate that coordinates are not included in the input and instead,\n\
	the program should iterate between then min, max values of the input range,\n\
	step pixel_size in each direction - which you can specify below or use the\n\
	default values. The order of dimensions: slice, width, height, i.e.\n\
	the first slice first, and this contains the first row, then the second row (width) etc.\n\
	This way the program is compatible with UNCascii's output.\n\
\n\
\t[-X px\n\
	(pixel size in the x-dimension.)]\n\
\t[-Y py\n\
	(pixel size in the y-dimension.)]\n\
\t[-Z pz\n\
	(pixel size in the z-dimension.)]\n\
\n\
the following parameters are relevant only if you want to scale\n\
the data read - useful when the input filename has been normalised\n\
somehow and the coordinates as well as pixel values need to be mapped\n\
to their real values:\n\
\n\
\t[-x a:b:c:d\n\
	(specify that the x-coordinates in the input file vary\n\
	 from 'a' to 'b' and that they should be scaled to the\n\
	 output range 'c' to 'd'.\n\
	 For example if it was specified '-x 0:1:0:100'\n\
	 then an x-coordinate of '0.25' it will be scaled\n\
	 to '25'. Note that the value '100' should not exceed\n\
	 the width of the output image specified with the '-d'\n\
	 option.)]\n\
\t[-y a:b:c:d\n\
	(same as above but for the y-dimension - note that 'd' must\n\
	 not be greater than the height of the output image ('-d').)]\n\
\t[-z a:b:c:d\n\
	(same as above but for the slice number - note that 'd' must\n\
	not be greater than the number of slices specified with\n\
	the '-d' option).]\n\
\t[-v a:b:c:d\n\
	(same as above but for the pixel value - no restriction to the\n\
	 numbers here but make sure it falls within the range of\n\
	 short int 0:32767.)]\n\
\n\
This program will take an ascii file and create from it a UNC file.\n\
The input file must be of the form:\n\
X1 Y1 Z1 V1\n\
X2 Y2 Z2 V2\n\
...\n\
\n\
\n\
(new lines are not necessary, white space is all is needed)\n\
\n\
then the output image will have the pixel at position (X1,Y1,Z1)\n\
take the value V1.\n\
\n\
The output image will have the dimensions specified using the '-d'\n\
option.\n\
	 \n\
In case the input file contains normalised data, the options\n\
'-x', '-y', '-z' and '-v' can be used to rescale the contents\n\
back to their original values. Each of the above options needs\n\
the numbers: a:b:c:d\n\
it means that the values of either X, Y, Z or V were normalised\n\
between 'a' and 'b'. And their original range was from 'c' to 'd'.\n\
'a' and 'b' do not simply mean the min and max values in the input\n\
file.\n\
\n\
example:\n\
\n\
normalised file, original dimensions x->0:256, y->0:256, z->0:50, v->0:1000\n\
normalised to 0:1 for x, y and z and to 0:10 for the pixel value:\n\
\n\
-d 256:256:50\n\
-x 0:1:0:255       ***** <<< note the 255 (e.g. 256-1 instead of 256)\n\
-y 0:1:0:255       ***** <<< note the 255 (e.g. 256-1 instead of 256)\n\
-z 0:1:0:49        ***** <<< note the 49 (e.g. 50-1 instead of 50)\n\
-v 0:10:0:999	   ***** <<< note the 999 (e.g. 1000 - 1)\n\
\n\
**** All coordinates and slice numbers start from 0 *****\n";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

enum {X=0, Y=1, Z=2, V=3};
enum {Min=0, Max=1};

int	main(int argc, char **argv){
	FILE		*oHandle;
	DATATYPE	***data;
	char		*inputFilename = NULL, *outputFilename = NULL,
			rangeXFound = FALSE, rangeYFound = FALSE, rangeZFound = FALSE, rangeVFound = FALSE,
			no_coordinates_present_flag = FALSE;
	int		optI, width = -1, height = -1, numSlices = -1;
	float		pixel_size_x = 1.0, pixel_size_y = 1.0, pixel_size_z = 1.0,
			x, y, z, v,
			inputRange[4][2], outputRange[4][2];
	register int	xx, yy, zz, vv, line, l, i, j;
	register float	fX, fY, fZ, fV;

	while( (optI=getopt(argc, argv, "i:o:ed:X:Y:Z:x:y:z:v:c")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 'd': sscanf(optarg, "%d:%d:%d", &width, &height, &numSlices); break;
			case 'c': no_coordinates_present_flag = TRUE; break;
			case 'X': pixel_size_x = atof(optarg); break;
			case 'Y': pixel_size_y = atof(optarg); break;
			case 'Z': pixel_size_z = atof(optarg); break;

			case 'x': sscanf(optarg, "%f:%f:%f:%f", &(inputRange[X][Min]), &(inputRange[X][Max]), &(outputRange[X][Min]), &(outputRange[X][Max])); rangeXFound = TRUE; break;
			case 'y': sscanf(optarg, "%f:%f:%f:%f", &(inputRange[Y][Min]), &(inputRange[Y][Max]), &(outputRange[Y][Min]), &(outputRange[Y][Max])); rangeYFound = TRUE; break;
			case 'z': sscanf(optarg, "%f:%f:%f:%f", &(inputRange[Z][Min]), &(inputRange[Z][Max]), &(outputRange[Z][Min]), &(outputRange[Z][Max])); rangeZFound = TRUE; break;
			case 'v': sscanf(optarg, "%f:%f:%f:%f", &(inputRange[V][Min]), &(inputRange[V][Max]), &(outputRange[V][Min]), &(outputRange[V][Max])); rangeVFound = TRUE; break;
			
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename (ascii) must be specified.\n");
		if( outputFilename != NULL ) free(outputFilename);
		exit(1);
	}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename);
		exit(1);
	}
	if( (width<=0) || (height<=0) || (numSlices<=0) ){
		fprintf(stderr, "%s : width, height and num slices must be positive integers (use the -d option).\n", argv[0]);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( rangeXFound && (width<outputRange[X][Max]) ){
		fprintf(stderr, "%s : width must not be less than the x-coordinate output range.\n", argv[0]);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( rangeYFound && (height<outputRange[Y][Max]) ){
		fprintf(stderr, "%s : width must not be less than the y-coordinate output range.\n", argv[0]);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( rangeZFound && (numSlices<outputRange[Z][Max]) ){
		fprintf(stderr, "%s : the number of slices must not be less than the slice number output range.\n", argv[0]);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	
	if( (data=callocDATATYPE3D(numSlices, width, height)) == NULL ){
		fprintf(stderr, "%s : call to mallocDATATYPE3D has failed for an image of dimensions %d x %d x %d.\n", argv[0], width, height, numSlices);
		free(inputFilename); free(outputFilename);
		exit(1);
	}

	if( (oHandle=fopen(inputFilename, "r")) == NULL ){
		fprintf(stderr, "%s : could not open file '%s' for reading.\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename); freeDATATYPE3D(data, numSlices, width);
		exit(1);
	}

	for(i=0;i<width;i++) for(j=0;j<height;j++) for(l=0;l<numSlices;l++) data[l][i][j] = 0;

	line = 0;

	if( rangeXFound ) fX = pixel_size_x * (outputRange[X][Max]-outputRange[X][Min]) / (inputRange[X][Max]-outputRange[X][Min]);
	else { fX = pixel_size_x; outputRange[X][Min] = inputRange[X][Min] = 0.0; }
	if( rangeYFound ) fY = pixel_size_y * (outputRange[Y][Max]-outputRange[Y][Min]) / (inputRange[Y][Max]-outputRange[Y][Min]);
	else { fY = pixel_size_y; outputRange[Y][Min] = inputRange[Y][Min] = 0.0; }
	if( rangeZFound ) fZ = pixel_size_z * (outputRange[Z][Max]-outputRange[Z][Min]) / (inputRange[Z][Max]-outputRange[Z][Min]);
	else { fZ = pixel_size_z; outputRange[Z][Min] = inputRange[Z][Min] = 0.0; }
	if( rangeVFound ) fV = (outputRange[V][Max]-outputRange[V][Min]) / (inputRange[V][Max]-outputRange[V][Min]);
	else { fV = 1.0; outputRange[V][Min] = inputRange[V][Min] = 0.0; }

	line = 1;
	if( no_coordinates_present_flag ){
		printf("%f-%f=%f\n", inputRange[X][Min], inputRange[X][Max], pixel_size_x);
		for(zz=0;zz<numSlices;zz++) for(xx=0;xx<width;xx++) for(yy=0;yy<height;yy++){
			vv = (int )ROUND(fV * (v - inputRange[V][Min]) + outputRange[V][Min]);

			fscanf(oHandle, "%f", &v);
			if( feof(oHandle) ) break;
			vv = (int )ROUND(fV * (v - inputRange[V][Min]) + outputRange[V][Min]);

			data[zz][xx][yy] = (DATATYPE )ROUND(vv);
			printf("(%d,%d,%d)=%d\n", xx,yy,zz,vv);
			line++;
		}
	} else {
		/* input specifies coordinates e.g. 4 5 6 7 (x=4,y=5,slice=6,pixelvalue=7) */
		while( TRUE ){
			fscanf(oHandle, "%f", &x);
			if( feof(oHandle) ) break;
			xx = (int )ROUND(fX * (x - inputRange[X][Min]) + outputRange[X][Min]);
			if( (xx<0) || (xx>=width) ){
				fprintf(stderr, "%s : line %d of file %s, x-coordinate (%f, %d) is out of range (0-%d).\n", argv[0], line, inputFilename, x, xx, width);
				break;
			}
			fscanf(oHandle, "%f", &y);
			if( feof(oHandle) ) break;
			yy = (int )ROUND(fY * (y - inputRange[Y][Min]) + outputRange[Y][Min]);
			if( (yy<0) || (yy>=width) ){
				fprintf(stderr, "%s : line %d of file %s, y-coordinate (%f, %d) is out of range (0-%d).\n", argv[0], line, inputFilename, y, yy, height);
				break;
			}
			fscanf(oHandle, "%f", &z);
			if( feof(oHandle) ) break;
			zz = (int )ROUND(fZ * (z - inputRange[Z][Min]) + outputRange[Z][Min]);
			if( (zz<0) || (zz>=numSlices) ){
				fprintf(stderr, "%s : line %d of file %s, slice number (%f, %d) is out of range (0-%d).\n", argv[0], line, inputFilename, z, zz, numSlices);
				break;
			}
			fscanf(oHandle, "%f", &v);
			if( feof(oHandle) ) break;
			vv = (int )ROUND(fV * (v - inputRange[V][Min]) + outputRange[V][Min]);

			data[zz][xx][yy] = (DATATYPE )ROUND(vv);
			line++;
		}
	}

	fclose(oHandle);

	/* format is 8 - god knows what this is, check /usr/image/include/Image.h */
	if( !writeUNCSlices3D(outputFilename, data, width, height, 0, 0, width, height, NULL, numSlices, 8, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
	}
	free(inputFilename); free(outputFilename);
	freeDATATYPE3D(data, numSlices, width);

	exit(0);
}




