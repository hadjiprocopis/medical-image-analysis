#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>

#include "Common_IMMA.h"
#include "registration.h"

const	char	Examples[] = "\
	-a files_VANDYKE_JEANdual.brain.odd.matched.air -i input.txt -o output.txt -I\
";

const	char	Usage[] = "options as follows:\
\t -i inputFilename\
	(ascii file containing triplets of input coordinates\
	 (in voxels, not millimetres) in native space\
	 to be converted to coordinates in the registered\
	 space. These numbers should be separated by\
	 white space.)\
\
\t -o outputFilename\
	(output filename which will contain the\
	 coordinates in registered space corresponding\
	 to each of the coordinates in the input\
	 file with the order they appear. This file\
	 is ascii.)\
\
\t -a airFilename\
	(AIR parameters filename - usually with the '.air' postfix.\
	 This file should have been produced after registering\
	 one image onto another and contains the registration\
	 parameter. Any reslice operation must use this file.)\
	\
\t[-I\
	(flag to denote that the output file should contain\
	 the input coordinates as well as their correspoding\
	 registered-space output coordinates)]\
\t[-5\
	(Use AIR 5.2.5 conventions. Simply, if you have done\
	 your registration with 5.2.5 then use the -5.\
	 Default is to use AIR 3.08 conventions)]\
\t[-v\
	(Be verbose. It will dump information to standard\
	 output about the air parameters file - similar to\
	 AIR's scanair.)]\
\
**\
These flags are similar to AIR's reslice parameters\
\t[-k\
	(flag which keeps voxel dimensions same as standard file)]\
\t[-x xDim:xSize:xShift\
	(xDim : image size in the X dimension\
	 xSize: pixel size in the X dimension\
	 xShift: shift in the X dimension\
\
	 alters output x-dimension, x-voxel size or causes shift of\
	 output along the x-axis.)]\
\t[-y yDim:ySize:yShift\
	(see above)]\
\t[-z zDim:zSize:zShift\
	(see above)]\
\
This program is a quick hack to the problem I encountered when I\
needed to know where, after registering an image onto another\
image, a voxel in native space goes to registered space.\
\
It seems to me that AIR does not provide any mechanism for\
explicitly stating that.\
\
So this program can be used to convert native space coordinates\
to registered space coordinates - or simply put, to tell you\
what is the location of a voxel in registered space, given\
its location in native space.\
\
By native space I mean the original image and by registered\
space I mean the image produced by the 'reslice' program\
part of the AIR (Roger P. Woods) package\
(see bishopw.loni.ucla.edu/AIR3/index.html)\
\
This program can cope only with linear or nearest neighbours\
interpolation. Adjusting it for other interpolation schemes\
is straight forward. Basically, for each interpolation\
scheme, the AIR package has a 'reslicer' function which\
is called. In the case of the nnreslicer (nearest neighbour)\
I have placed inside the 3 for-loops (for going through X, Y and Z\
dimensions) a statement to compare the input coordinates that\
need to be converted with the coordinates in the for-loops\
(x_up, y_up and z_up). If they match, then (i, j, k)\
represents the registered space coordinates.\
\
The program can cope with air files produced by\
either AIR 3.08 (the default) or AIR 5.2.5 (use\
the '-5' option).\
\
This program works also in Linux. It can cope with the\
problems arising from the two different standards\
(little and big endian) of binary files in Linux and Solaris.\
\
";


const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	FILE		*a_handle;
	char		*inputFilename = NULL, *outputFilename = NULL,
			*airFilename = NULL;
	int		optI;

	int		**inputCoordinates,
			**outputCoordinates,
			numCoordinates, dummy, i, j;
	char		shouldIncludeInput = FALSE;
	int		x_dimout = 0, y_dimout = 0, z_dimout = 0,
			cubic = 1, flag = 0, flagk = 0;
	float		x_shift = 0.0, y_shift = 0.0, z_shift = 0.0,
			x_distance = 0.0, y_distance = 0.0, z_distance = 0.0;
	native2registered_AIR_Air16	airParameters; /* struct cut&paste from AIR source code - it stores AIR reg. parameters as read from a .air file */
	char		shouldUseAIR3_08Conventions = TRUE,
			verboseFlag = FALSE;
	
	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:a:Ikx:y:z:n:5v")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 'a': airFilename = strdup(optarg); break;
			case 'I': shouldIncludeInput = TRUE; break;
			case '5': shouldUseAIR3_08Conventions = FALSE; break;
			case 'v': verboseFlag = TRUE; break;

			case 'k': flagk = 1; break;
			case 'x': sscanf(optarg, "%d:%f:%f", &x_dimout, &x_distance, &x_shift); break;
			case 'y': sscanf(optarg, "%d:%f:%f", &y_dimout, &y_distance, &y_shift); break;
			case 'z': sscanf(optarg, "%d:%f:%f", &z_dimout, &z_distance, &z_shift); break;
			
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		if( outputFilename != NULL ) free(outputFilename);
		exit(1);
	}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename);
		exit(1);
	}
	if( airFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "The AIR filename must be specified. This file contains the parameters of registration using AIR package. It usually has the postfix '.air'.\n");
		free(inputFilename);
		exit(1);
	}

	if( flagk ){
		/* -k */
		cubic = 0;
		if( flag ){
			fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
			fprintf(stderr, "Conflicting instructions provided, you can specify only one of -k or -p\n");
			exit(1);
		}
	}
	if( (x_dimout>0) || (y_dimout>0) || (z_dimout>0) ){
		/* -x , -y and/or -z */
		cubic = 0;
		flag = 1;		
		if( flagk ){
			fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
			fprintf(stderr, "Conflicting instructions provided, you can specify only one of -k on the one hand and -x and/or -y and/or -z on the other hand.\n");
			exit(1);
		}
	}

	if( (a_handle=fopen(inputFilename, "r")) == NULL ){
		fprintf(stderr, "%s : could not open input coordinates file '%s'.\n", argv[0], inputFilename);
		exit(1);
	}
	/* count how many items of data */
	for(numCoordinates=0;;numCoordinates++){
		fscanf(a_handle, "%d", &dummy);
		if( feof(a_handle) ) break;
	}
	if( (numCoordinates%3) != 0 ){
		fprintf(stderr, "%s : the input coordinates file '%s' must contain triplets of points (e.g. X,Y,Z), but instead it contains %d coordinates which is not a multiple of 3!\n", argv[0], inputFilename, numCoordinates);
		fclose(a_handle);
		exit(1);
	}
	numCoordinates /= 3;
	rewind(a_handle);
	if( (inputCoordinates=(int **)malloc(3*sizeof(int *))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for inputCoordinates.\n", argv[0], 3*sizeof(int *));
		fclose(a_handle);
		exit(1);
	}
	if( (outputCoordinates=(int **)malloc(3*sizeof(int *))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for outputCoordinates.\n", argv[0], 3*sizeof(int *));
		fclose(a_handle);
		exit(1);
	}
	for(i=0;i<3;i++){
		if( (inputCoordinates[i]=(int *)malloc(numCoordinates*sizeof(int))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for inputCoordinates[%d].\n", argv[0], numCoordinates*sizeof(int), i);
			fclose(a_handle);
			exit(1);
		}
		if( (outputCoordinates[i]=(int *)malloc(numCoordinates*sizeof(int))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for outputCoordinates[%d].\n", argv[0], numCoordinates*sizeof(int), i);
			fclose(a_handle);
			exit(1);
		}
	}
	for(i=0;i<numCoordinates;i++) for(j=0;j<3;j++)
		fscanf(a_handle, "%d", &(inputCoordinates[j][i]));
	fclose(a_handle);

	/* read the parameter file */
	if( !native2registered_AIR_read_air16(airFilename, &airParameters) ){
		fprintf(stderr, "%s : call to native2registered_AIR_read_air16 has failed for AIR parameters filename '%s'.\n", argv[0], airFilename);
		exit(1);
	}
	if( verboseFlag ) native2registered_print_AIR_parameters(stdout, airParameters);

	/* call the reslicer */
	if( native2registered(
			inputCoordinates, outputCoordinates, numCoordinates,
			&airParameters,
			shouldUseAIR3_08Conventions,
			x_dimout, y_dimout, z_dimout,
			x_distance, y_distance, z_distance,
			x_shift, y_shift, z_shift,
			cubic) == FALSE ){
		fprintf(stderr, "%s : call to native2registered has failed.\n", argv[0]);
		exit(1);
	}
	fprintf(stderr, "\n");

	/* write out the results */
	if( (a_handle=fopen(outputFilename, "w")) == NULL ){
		fprintf(stderr, "%s: could not open output filename '%s' for writing.\n", argv[0], outputFilename);
		exit(1);
	}
	if( shouldIncludeInput )
		for(i=0;i<numCoordinates;i++)
			fprintf(a_handle, "%d %d %d\t%d %d %d\n",
				inputCoordinates[0][i], inputCoordinates[1][i], inputCoordinates[2][i],
				outputCoordinates[0][i], outputCoordinates[1][i], outputCoordinates[2][i]);
	else	for(i=0;i<numCoordinates;i++)
			fprintf(a_handle, "%d %d %d\n",
				outputCoordinates[0][i], outputCoordinates[1][i], outputCoordinates[2][i]);
	fclose(a_handle);

	for(i=0;i<3;i++){
		free(inputCoordinates[i]);
		free(outputCoordinates[i]);
	}
	free(inputFilename); free(outputFilename); free(airFilename);
	exit(0);
}

