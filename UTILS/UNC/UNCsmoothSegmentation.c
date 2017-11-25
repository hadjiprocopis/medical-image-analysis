#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>
#include <contour.h>

const	char	Examples[] = "\
\n	-i input.unc -o output.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i input1 -i input2 [-i input3 ...]\
\n	(input files must be UNC files with 1 or more slices.\
\n	 All input files must have the same number of slices.\
\n	 Further more, for each pixel (i,j) of the same\
\n	 slice (s) of each file (f) at most 1 must be non-zero.\
\n	 e.g. For all (i,j), for a given s, for all f,\
\n	 number of pixels satisfying (i,j)[s][f] != 0\
\n	 must be 0 or 1.)\
\n\
\n\t -o outputFilename\
\n	(Output filename)\
\n\
\n\t[-D window_width:window_height\
\n	(an area of neighbours is formed around\
\n	 each pixel. The dimensions of this rectangle\
\n	 are (2*window_width+1) and (2*window_height+1).\
\n	 The bigger the width and height of this\
\n	 rectangle, the smoother the result is but\
\n	 also the less accurate the result.\
\n	 Default is a 2:2 (e.g. a window of 5x5)).]\
\n\
\n\t[-3\
\n	(the neighbourhood of each pixel is defined\
\n	 as a cube around it rather than as a\
\n	 rectangle. Thus, the slice above and the\
\n	 slice below are involved.)]	 \
\n\t[-n N\
\n	(a pixel will be moved to the file with the\
\n	 maximum number of neighbours around it if\
\n	 the ratio of the number of neighbours over\
\n	 the number of total neighbours (area of\
\n	 neighbourhood) is NO LESS than 'N' -\
\n	 0.0 <= N <= 1.0)]\
\n\t[-m M\
\n	(a pixel will be moved to the file with the\
\n	 maximum number of neighbours around it if\
\n	 the difference between the mean pixel value\
\n	 of the neighbours and the value of the given\
\n	 pixel is NO MORE than M percent -\
\n	 0.0 <= M <= 1.0 )]\
\n\t[-a A\
\n	(an 'island' of pixels will be moved if the\
\n	 number of its pixels is less than or equal to A,\
\n	 provided all other criteria are also satisfied.)]\
\n\t[-v\
\n	(be vebose - will report which pixels are moved and\
\n	 where.)]\
\n\
\n\t[-9\
\n        (tell the program to copy the header/title information\
\n        from the input file to the output files. If there is\
\n        more than 1 input file, then the information is copied\
\n        from the first file.)]\
\n\
\n** Use this options to select a region of interest\
\n   You may use one or more or all of '-w', '-h', '-x' and\
\n   '-y' once. You may use one or more '-s' options in\
\n   order to specify more slices. Slice numbers start from 1.\
\n   These parameters are optional, if not present then the\
\n   whole image, all slices will be used.\
\n\
\n\t[-w widthOfInterest]\
\n\t[-h heightOfInterest]\
\n\t[-x xCoordOfInterest]\
\n\t[-y yCoordOfInterest]\
\n\t[-s sliceNumber [-s s...]]\
\n\
\nThis program takes 2 or more UNC files which representing\
\nsegmented portions of the same image. They must be mutually\
\nexclusive - e.g. no more than 1 pixel can be non-zero for the\
\nsame location of each input file. So, it will find stray pixels\
\nwhich for some reason they are classified as belonging to one\
\ntissue type but are in the middle of nowhere, most likely belonging\
\nto the other tissue type. Sometimes this might happen because of\
\nnoise during acquisition which distorts a voxel's signal and shifts\
\nit up or down.";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***data[100], ***dataOut[100], ***dataOut2[100],
			thresholdMinPixel = 1, thresholdMaxPixel = 128 * 256;
	char		*inputFilenames[100], *outputBasename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0, ****numNeighbours;

	float		neighboursRatio = 0.0, ****meansNeighbours,
			***meansConnectedObjects,
			meansRatio = 1.0, minMeanDifference, temp;
	int		numinputFilenames = 0,
			windowNeighboursW = 2, windowNeighboursH = 2,
			minMeanDifferenceAt,
			windowMeansW = 6, windowMeansH = 6, movedPixelsTotal, **movedPixels,
			maxNumNeighbours, maxNumNeighboursAt = 0, v,
			**num_connected_objects, max_recursion_level, minArea = 1;
	register int	i, j, f, ff, ii, jj,
			mI1, mI2, mJ1, mJ2,
			X1, X2, Y1, Y2;
	char		dummy[1000], verboseFlag = FALSE,
			isNeighboursRatioOK, isMeansRatioOK;
	connected_objects	***con_obj;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:d:D:t:n:m:a:v9")) != EOF)
		switch( optI ){
			case 'i': inputFilenames[numinputFilenames++] = strdup(optarg); break;
			case 'o': outputBasename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 't': sscanf(optarg, "%d:%d", &thresholdMinPixel, &thresholdMaxPixel); break;
			case 'd': sscanf(optarg, "%d:%d", &windowNeighboursW, &windowNeighboursH); break;
			case 'D': sscanf(optarg, "%d:%d", &windowMeansW, &windowMeansH); break;
			case 'a': minArea = atoi(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			case 'n': neighboursRatio = atof(optarg); break;
			case 'm': meansRatio = atof(optarg); break;
			case 'v': verboseFlag = TRUE; break;

			case '9': copyHeaderFlag = TRUE; break;
			
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilenames[0] == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		if( outputBasename != NULL ) free(outputBasename);
		exit(1);
 	}
	if( outputBasename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		for(i=0;i<numinputFilenames;i++) free(inputFilenames[i]);
		exit(1);
	}
	if( (windowNeighboursW<0) && (windowNeighboursH<0) ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "Window width and height must be positive integers.\n");
		for(i=0;i<numinputFilenames;i++) free(inputFilenames[i]);
		free(outputBasename);
 		exit(1);
	}		
	printf("%s, reading input files :", argv[0]); fflush(stdout);
	for(i=0;i<numinputFilenames;i++){
		if( (data[i]=getUNCSlices3D(inputFilenames[i], 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
			fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilenames[i]);
			for(j=0;j<numinputFilenames;j++) free(inputFilenames[j]); free(outputBasename);
			exit(1);
		}
		printf(" %s", inputFilenames[i]); fflush(stdout);
	}
	printf("\n");

	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in files.\n", argv[0], actualNumSlices);
				for(j=0;j<numinputFilenames;j++) free(inputFilenames[j]); free(outputBasename); for(j=0;j<numinputFilenames;j++) freeDATATYPE3D(data[j], actualNumSlices, W);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				for(j=0;j<numinputFilenames;j++) free(inputFilenames[j]); free(outputBasename); for(j=0;j<numinputFilenames;j++) freeDATATYPE3D(data[j], actualNumSlices, W);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		for(j=0;j<numinputFilenames;j++) free(inputFilenames[j]); free(outputBasename);
		for(j=0;j<numinputFilenames;j++) freeDATATYPE3D(data[j], actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		for(j=0;j<numinputFilenames;j++) free(inputFilenames[j]); free(outputBasename);
		for(j=0;j<numinputFilenames;j++) freeDATATYPE3D(data[j], actualNumSlices, W);
		exit(1);
	}
	for(i=0;i<numinputFilenames;i++)
		if( (dataOut[i]=callocDATATYPE3D(numSlices, W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
			for(j=0;j<numinputFilenames;j++) free(inputFilenames[j]); free(outputBasename);
			for(j=0;j<numinputFilenames;j++) freeDATATYPE3D(data[j], actualNumSlices, W);
			exit(1);
		}

	for(f=0;f<numinputFilenames;f++) for(s=0;s<numSlices;s++) for(i=0;i<W;i++) for(j=0;j<H;j++)
		dataOut[f][s][i][j] = data[f][(allSlices==0) ? slices[s] : s][i][j];

	/* some constants for the for-loops */
	X1 = x + MAX(windowNeighboursW,windowMeansW); X2 = x+w-MAX(windowNeighboursW,windowMeansW);
	Y1 = y + MAX(windowNeighboursH,windowMeansH); Y2 = y+h-MAX(windowNeighboursH,windowMeansH);
	movedPixelsTotal = 0;
	
	if( (con_obj=(connected_objects ***)malloc(numinputFilenames*sizeof(connected_objects **))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for con_obj.\n", argv[0], numinputFilenames*sizeof(connected_objects **));
		exit(1);
	}
	if( (meansNeighbours=(float ****)malloc(numinputFilenames*sizeof(float ***))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for meansNeighbours.\n", argv[0], numinputFilenames*sizeof(float ***));
		exit(1);
	}
	if( (meansConnectedObjects=(float ***)malloc(numinputFilenames*sizeof(float **))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for meansConnectedObjects.\n", argv[0], numinputFilenames*sizeof(float **));
		exit(1);
	}
	if( (numNeighbours=(int ****)malloc(numinputFilenames*sizeof(int ***))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for numNeighbours.\n", argv[0], numinputFilenames*sizeof(int ***));
		exit(1);
	}
	if( (num_connected_objects=(int **)malloc(numinputFilenames*sizeof(int *))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for num_connected_objects.\n", argv[0], numinputFilenames*sizeof(int *));
		exit(1);
	}
	for(f=0;f<numinputFilenames;f++){
		if( (con_obj[f]=(connected_objects **)malloc(numSlices*sizeof(connected_objects *))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for con_obj[%d].\n", argv[0], numSlices*sizeof(connected_objects *), f);
			exit(1);
		}
		if( (meansNeighbours[f]=(float ***)malloc(numinputFilenames*sizeof(float **))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for meansNeighbours[%d].\n", argv[0], numinputFilenames*sizeof(float **), f);
			exit(1);
		}
		if( (numNeighbours[f]=(int ***)malloc(numinputFilenames*sizeof(int **))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for numNeighbours[%d].\n", argv[0], numinputFilenames*sizeof(int **), f);
			exit(1);
		}
		for(ff=0;ff<numinputFilenames;ff++){
			if( (meansNeighbours[f][ff]=(float **)malloc(numSlices*sizeof(float *))) == NULL ){
				fprintf(stderr, "%s : could not allocate %zd bytes for meansNeighbours[%d][%d].\n", argv[0], numSlices*sizeof(float *), f, ff);
				exit(1);
			}
			if( (numNeighbours[f][ff]=(int **)malloc(numSlices*sizeof(int *))) == NULL ){
				fprintf(stderr, "%s : could not allocate %zd bytes for numNeighbours[%d][%d].\n", argv[0], numSlices*sizeof(int *), f, ff);
				exit(1);
			}
		}
		if( (meansConnectedObjects[f]=(float **)malloc(numSlices*sizeof(float *))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for meansConnectedObjects[%d].\n", argv[0], numSlices*sizeof(float *), f);
			exit(1);
		}
		if( (num_connected_objects[f]=(int *)malloc(numSlices*sizeof(int))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for numNeighbours[%d].\n", argv[0], numSlices*sizeof(int), f);
			exit(1);
		}
	}
	/* find connected objects and handle those with small area */
	/* meansNeighbours and numNeighbours :
	[the file where the object is in][the file where you want to count neighbours][slice][connected object id (whose file is [0])] */
	for(f=0;f<numinputFilenames;f++){
		printf("%s, find connected objects, image %s, slice # :", argv[0], inputFilenames[f]); fflush(stdout);

		if( (dataOut2[f]=callocDATATYPE3D(numSlices, W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for output data (2).\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
			for(j=0;j<numinputFilenames;j++){ free(inputFilenames[j]); freeDATATYPE3D(data[j], actualNumSlices, W); freeDATATYPE3D(dataOut[j], numSlices, W); }
			free(outputBasename);
			exit(1);
		}
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			printf(" %d", slice+1); fflush(stdout);
			if( find_connected_pixels2D(dataOut[f][s], 0, 0, slice, W, H, dataOut2[f][s], NEIGHBOURS_4, thresholdMinPixel, thresholdMaxPixel, &(num_connected_objects[f][s]), &max_recursion_level, &(con_obj[f][s])) == FALSE ){
				fprintf(stderr, "%s : call to find_connected_pixels2D has failed for %d image.\n", argv[0], i);
				for(j=0;j<numinputFilenames;j++){ free(inputFilenames[j]); freeDATATYPE3D(data[j], actualNumSlices, W); freeDATATYPE3D(dataOut[j], numSlices, W); freeDATATYPE3D(dataOut2[j], numSlices, W); }
				free(outputBasename);
				exit(1);
			}
			printf("(%d/%d) ", num_connected_objects[f][s], max_recursion_level); fflush(stdout);
			for(ff=0;ff<numinputFilenames;ff++){
				if( (meansNeighbours[f][ff][s]=(float *)malloc(num_connected_objects[f][s]*sizeof(float))) == NULL ){
					fprintf(stderr, "%s : could not allocate %zd bytes for numNeighbours[%d][%d][%d].\n", argv[0], num_connected_objects[f][s]*sizeof(float), f, ff, s);
					exit(1);
				}
				if( (numNeighbours[f][ff][s]=(int *)malloc(num_connected_objects[f][s]*sizeof(int))) == NULL ){
					fprintf(stderr, "%s : could not allocate %zd bytes for numNeighbours[%d][%d][%d].\n", argv[0], num_connected_objects[f][s]*sizeof(int), f, ff, s);
					exit(1);
				}
			}
			if( (meansConnectedObjects[f][s]=(float *)malloc(num_connected_objects[f][s]*sizeof(float))) == NULL ){
				fprintf(stderr, "%s : could not allocate %zd bytes for meansConnectedObjects[%d][%d].\n", argv[0], num_connected_objects[f][s]*sizeof(float), f, s);
				exit(1);
			}
		}
		printf("\n");
	}
	
	for(f=0;f<numinputFilenames;f++){
		printf("%s, process objects, image %s, slice # :", argv[0], inputFilenames[f]); fflush(stdout);
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			printf(" %d", slice+1); fflush(stdout);
			
			/* check the mean intensities around the bounding box of this object over all files */
			for(i=0;i<num_connected_objects[f][s];i++){
				if( con_obj[f][s]->objects[i]->num_points <= minArea ){
					mI1 = MAX(x, con_obj[f][s]->objects[i]->x0-windowMeansW); mI2 = MIN(x+w,con_obj[f][s]->objects[i]->x0+con_obj[f][s]->objects[i]->w+windowMeansW);
					mJ1 = MAX(y, con_obj[f][s]->objects[i]->y0-windowMeansH); mJ2 = MIN(y+h,con_obj[f][s]->objects[i]->y0+con_obj[f][s]->objects[i]->h+windowMeansH);
					/* now find the mean intensity of the connected object */
					meansConnectedObjects[f][s][i] = 0.0;
					for(j=0;j<con_obj[f][s]->objects[i]->num_points;j++)
						meansConnectedObjects[f][s][i] += con_obj[f][s]->objects[i]->v[j];
					/* and the mean intensity of the neighbours (for each input file) */
					for(ff=0;ff<numinputFilenames;ff++){
						/* if this is the input file the connected object belongs to, we
						   have to substract all the pixels of the connected object from the
						   neighbours,
						   we also have to remove from the counted neighbours, those who belong to the connected object */
						if( ff == f ){
							meansNeighbours[f][ff][s][i] = -meansConnectedObjects[f][s][i]; /* sum really (not mean) */
							numNeighbours[f][ff][s][i] = -con_obj[f][s]->objects[i]->num_points;
						} else {
							meansNeighbours[f][ff][s][i] = 0.0;
							numNeighbours[f][ff][s][i] = 0;
						}
						for(ii=mI1;ii<mI2;ii++) for(jj=mJ1;jj<mJ2;jj++)
							if( IS_WITHIN(data[ff][slice][ii][jj], thresholdMinPixel, thresholdMaxPixel) ){
								meansNeighbours[f][ff][s][i] += data[ff][slice][ii][jj];
								numNeighbours[f][ff][s][i]++;
							}
						if( numNeighbours[f][ff][s][i] > 0 ) meansNeighbours[f][ff][s][i] /= numNeighbours[f][ff][s][i];
					}
					meansConnectedObjects[f][s][i] /= con_obj[f][s]->objects[i]->num_points;
				}
			}
		}
		printf("\n");
	}

	movedPixelsTotal = 0;
	if( (movedPixels=(int **)malloc(numinputFilenames*sizeof(int *))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for movedPixels.\n", argv[0], numinputFilenames*sizeof(int *));
		exit(1);
	}
	for(f=0;f<numinputFilenames;f++){
		if( (movedPixels[f]=(int *)malloc(numinputFilenames*sizeof(int))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for movedPixels[%d].\n", argv[0], numinputFilenames*sizeof(int), f);
			exit(1);
		}
		for(ff=0;ff<numinputFilenames;ff++) movedPixels[f][ff] = 0;
	}

	/* ok now we have what we need (mean and num neighbours) - we will start removing pixels */
	for(f=0;f<numinputFilenames;f++){
		printf("%s, filter, image %s, slice # :", argv[0], inputFilenames[f]); fflush(stdout);
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			printf(" %d", slice+1); fflush(stdout);

			for(i=0;i<num_connected_objects[f][s];i++){
				if( con_obj[f][s]->objects[i]->num_points <= minArea ){
					minMeanDifference = 100000000;
					maxNumNeighbours = -1;
					for(ff=0;ff<numinputFilenames;ff++){
						if( f == ff ) continue;
						temp = ABS(meansConnectedObjects[f][s][i] - meansNeighbours[f][ff][s][i]);
						if( temp < minMeanDifference ){
							minMeanDifference = temp;
							minMeanDifferenceAt = ff;
						}
						if( numNeighbours[f][ff][s][i] > maxNumNeighbours ){
							maxNumNeighbours = numNeighbours[f][ff][s][i];
							maxNumNeighboursAt = ff;
						}
					}
					/* check the criteria for moving something */
					if( numNeighbours[f][f][s][i] > 0 )
						isNeighboursRatioOK = ((maxNumNeighboursAt+0.0) / numNeighbours[f][f][s][i]) >= neighboursRatio;
					else isNeighboursRatioOK = TRUE;
					isMeansRatioOK = (minMeanDifference/meansConnectedObjects[f][s][i]) <= meansRatio;
					/* move now the object from f to min..At */
					if( isMeansRatioOK && isNeighboursRatioOK ){
						if( verboseFlag ) printf("moving from file %s (%d) to %s (%d) : bounding box (%d,%d,%d,%d)\n", inputFilenames[f], f, inputFilenames[maxNumNeighboursAt], maxNumNeighboursAt, con_obj[f][s]->objects[i]->x0, con_obj[f][s]->objects[i]->y0, con_obj[f][s]->objects[i]->w, con_obj[f][s]->objects[i]->h);
						for(j=0;j<con_obj[f][s]->objects[i]->num_points;j++){
							ii = con_obj[f][s]->objects[i]->x[j];
							jj = con_obj[f][s]->objects[i]->y[j];
							v  = con_obj[f][s]->objects[i]->v[j];
							for(ff=0;ff<numinputFilenames;ff++) dataOut[f][s][ii][jj] = 0;
							dataOut[maxNumNeighboursAt][s][ii][jj] = v;
							movedPixelsTotal++;
							movedPixels[f][maxNumNeighboursAt]++;
						}
					}
				}
			}
		}
		printf("\n");
	}
	printf("%s : total number of pixels moved is %d\n", argv[0], movedPixelsTotal);
	for(f=0;f<numinputFilenames;f++) printf("\t%d", f+1); printf("\t  total\n");
	for(f=0;f<numinputFilenames;f++){
		printf("%d", f+1);
		v = 0;
		for(ff=0;ff<numinputFilenames;ff++){ if( f == ff ){ printf("\t-"); continue ; } printf("\t%d", movedPixels[f][ff]); v+= movedPixels[f][ff]; }
		printf("\t| %d\n", v);
	}
	printf("total:");
	for(f=0;f<numinputFilenames;f++){
		v = 0;
		for(ff=0;ff<numinputFilenames;ff++) v += movedPixels[ff][f];
		printf("\t%d", v);
	}
	printf("\n\n");
		
			
	/* free memory */
	for(f=0;f<numinputFilenames;f++){
		freeDATATYPE3D(data[f], actualNumSlices, W);
		for(ff=0;ff<numinputFilenames;ff++){
			for(s=0;s<numSlices;s++){
				free(meansNeighbours[f][ff][s]); free(numNeighbours[f][ff][s]);
			}
			free(meansNeighbours[f][ff]); free(numNeighbours[f][ff]);
		}
		free(meansNeighbours[f]); free(numNeighbours[f]);

		for(s=0;s<numSlices;s++){
			free(meansConnectedObjects[f][s]);
			connected_objects_destroy(con_obj[f][s]);
		}
		free(con_obj[f]);
		free(num_connected_objects[f]);
		free(meansConnectedObjects[f]);
		free(movedPixels[f]);
	}
	free(movedPixels); free(con_obj); free(meansNeighbours); free(meansConnectedObjects); free(numNeighbours); free(num_connected_objects);

	printf("%s, writing output files :", argv[0]); fflush(stdout);
	for(j=0;j<numinputFilenames;j++){
		sprintf(dummy, "%s_%d", outputBasename, j+1);
		if( !writeUNCSlices3D(dummy, dataOut[j], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
			fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
			for(j=0;j<numinputFilenames;j++) free(inputFilenames[j]); free(outputBasename);
			for(j=0;j<numinputFilenames;j++) freeDATATYPE3D(dataOut[j], numSlices, W);
			for(j=0;j<numinputFilenames;j++) freeDATATYPE3D(dataOut2[j], numSlices, W);
			exit(1);
		}
		/* now copy the image info/title/header of source to destination */
		if( copyHeaderFlag ) if( !copyUNCInfo(inputFilenames[j], dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
			fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilenames[j], dummy);
			exit(1);
		}

		printf(" %s", dummy); fflush(stdout);
	}
	printf("\n");
	for(j=0;j<numinputFilenames;j++) free(inputFilenames[j]); free(outputBasename);
	for(j=0;j<numinputFilenames;j++) freeDATATYPE3D(dataOut[j], numSlices, W);
	for(j=0;j<numinputFilenames;j++) freeDATATYPE3D(dataOut2[j], numSlices, W);
	exit(0);
}

