#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>
#include <roi_utils.h>
#include <contour.h>

const	char	Examples[] = "\
\n	-i input.unc -o output.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t -o outputBasename\
\n	(Output basename)\
\n\
\n\t[-8\
\n	(define as a neighbour of a pixel one of its\
\n	 8 immediate neighbours. The alternative, which\
\n	 is also the default, is only 4 neighbours, at\
\n	 the top, bottom, left and right. The '-8' option\
\n	 will likely decrease the number of independent\
\n	 objects because it is not as a strict neighbour\
\n	 criterion as the 4 neighbours. On the other\
\n	 hand, the ROI's algorithm does not work well with\
\n	 the '-8' option.)]\
\n\
\n\t[-t min:max\
\n	(treat as active pixels only those within the range\
\n	 of 'min' and 'max', the rest will be zero and ignored.)]\
\n\
\n\t[-c\
\n	(find only connected structures - do not write out\
\n	 the roi of these structures (which is very time consuming)\
\n	 but just color the structures and dump them as a UNC file)]\
\n\
\n\t[-9\
\n        (tell the program to copy the header/title information\
\n        from the input file to the output files. If there is\
\n        more than 1 input file, then the information is copied\
\n        from the first file.)]\
\n\
\n** Use these options to declare the pixel size in the X and Y\
\n   direction:\
\n\
\n\t[-X sx\
\n	(pixel size in the X direction - `sx' in millimetres)]\
\n\t[-Y sy\
\n	(pixel size in the Y direction - `sy' in millimetres)]\
\n\
\n** Use these options to select a region of interest\
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
\nThis program will take a UNC image and find structures in\
\neach slice that are not connected to each other. It will\
\ncolour each of this objects with different value and\
\nalso will produce a ROI (region of interest), unless the\
\n'-c' option was used, which describes the periphery of\
\nthat object. All the ROIs are written to 'outputBasename'.roi,\
\nand can be loaded with dispunc. The file 'outputBasename'.data\
\nwill contain statistics (the same output as ROIstats) of\
\nthe all the ROIs.\
\n\
\nFor each slice, the number of connected objects and the\
\nmaximum recursion level are shown (in brackets).\
\n\
\nBEWARE! make sure that all background noise is removed from\
\nyour image (usually around the scalp). Otherwise, it will create\
\nthousands of small connected objects, waste a lot of memory and time\
\nand probably dump core at the end for good measure.\
\n\
\nIf you get a segmentation fault, it is likely that your stack size\
\nneeds to be increased. At the unix csh shell do: 'limit stack 1000'\
\nfor a 1000 kbytes stack, try more and more if you get segmentation fault...\
\n";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut, **scratch, pixel = 0,
			minP = 1, maxP = 32768; /* min inclusive, max exclusive */
	char		*inputFilename = NULL, *outputFilename = NULL,
			*dummyFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	register int	i, j, ii, jj;
	int		dummy, *numObjectsPerSlice, totalNumRois = 0,
			maxRecursionLevel;
	contour		*a_contour;
	roi		**myRois;
	roiRegionIrregular	*irreRegion;
	neighboursType	nType = NEIGHBOURS_4;
	float		pixel_size_x = 1.0, pixel_size_y = 1.0, pixel_size_z = 1.0, *scratch2;
	char		doBoth = TRUE;
			
	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:8X:Y:t:cr9")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'c': doBoth = FALSE; break;
			case 't': sscanf(optarg, "%d:%d", &minP, &maxP); break;
			case 'X': pixel_size_x = atof(optarg); break;
			case 'Y': pixel_size_y = atof(optarg); break;
			case '8': nType = NEIGHBOURS_8; break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);

			case '9': copyHeaderFlag = TRUE; break;
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
	if( minP > maxP ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "Minimum threshold pixel value must be less or equal the maximum (-t option).\n");
		free(inputFilename); free(outputFilename);
		exit(1);
	}
		
	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename);
				free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (dataOut=callocDATATYPE3D(numSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (numObjectsPerSlice=(int *)malloc(numSlices*sizeof(int))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for numObjectsPerSlice.\n", argv[0], numSlices*sizeof(int));
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(dataOut, numSlices, W);
		exit(1);
	}		
	printf("%s, finding connected objects, slice :", inputFilename); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf(" %d", slice+1); fflush(stdout);

		if( find_connected_pixels2D(data[s], 0, 0, slice, W, H, dataOut[s], nType, minP, maxP, &(numObjectsPerSlice[s]), &maxRecursionLevel, NULL) == FALSE ){
			fprintf(stderr, "%s : call to find_connected_pixels2D has failed for output file '%s'.\n", argv[0], outputFilename);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			free(numObjectsPerSlice);
			exit(1);
		} else {
			printf("(%d/%d)  ", numObjectsPerSlice[s], maxRecursionLevel);
		}
	} /* numSlices */
	printf("\n");

	/* write out all the connected objects because we will use the array */
	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(dataOut, numSlices, W);
		free(numObjectsPerSlice);
		exit(1);
	}

	if( doBoth ){
		/* count how many objects in total = number of rois */
		for(s=0;s<numSlices;s++) totalNumRois += numObjectsPerSlice[s];

		if( (myRois=rois_new(totalNumRois)) == NULL ){
			fprintf(stderr, "%s : call to rois_new has failed for %d rois, output file '%s'.\n", argv[0], totalNumRois, outputFilename);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			free(numObjectsPerSlice);free(numObjectsPerSlice);
			exit(1);
		}		

		if( (scratch=callocDATATYPE2D(W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for scratch data.\n", argv[0], W * H * sizeof(DATATYPE));
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			rois_destroy(myRois, totalNumRois);
			free(numObjectsPerSlice);free(numObjectsPerSlice);
			exit(1);
		}

		printf("%s, tracing contour around objects, slice : ", inputFilename); fflush(stdout);
		dummy = strlen(outputFilename) + 50;
		for(s=0,jj=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			printf("%d ", slice+1); fflush(stdout);

			for(ii=0;ii<numObjectsPerSlice[s];ii++,jj++){
				myRois[jj]->slice = s;
				myRois[jj]->name = malloc(10); sprintf(myRois[jj]->name, "%d", jj+1);
				myRois[jj]->image= malloc(dummy); sprintf(myRois[jj]->image, "%s slice %d", outputFilename, s+1);
				myRois[jj]->type = IRREGULAR_ROI_REGION;

				for(i=0;i<W;i++) for(j=0;j<H;j++) if( (pixel=dataOut[s][i][j]) > 0 ){ i = W+10; break; }
				for(i=0;i<W;i++) for(j=0;j<H;j++)
					if( dataOut[s][i][j] == pixel ){
						scratch[i][j] = 500;
						dataOut[s][i][j] = 0;
					} else scratch[i][j] = 0;
				if( (a_contour=find_contour2D(scratch, 0, 0, W, H, -1, -1, -1, 0)) == NULL ){
					fprintf(stderr, "%s : call to find_contour2D has failed for slice %d.\n", argv[0], s);
					free(inputFilename); free(outputFilename);
					freeDATATYPE3D(data, actualNumSlices, W);
					freeDATATYPE2D(scratch, W);
					freeDATATYPE3D(dataOut, numSlices, W);
					rois_destroy(myRois, jj-1);
					free(numObjectsPerSlice);free(numObjectsPerSlice);
					exit(1);
				}
				if( (myRois[jj]->roi_region=roi_region_new(IRREGULAR_ROI_REGION, NULL, a_contour->num_points)) == NULL ){
					fprintf(stderr, "%s : call to roi_region_new has failed for slice %d, roi %d\n", argv[0], s, jj);
					free(inputFilename); free(outputFilename);
					freeDATATYPE3D(data, actualNumSlices, W);
					freeDATATYPE2D(scratch, W);
					freeDATATYPE3D(dataOut, numSlices, W);
					rois_destroy(myRois, jj-1);
					free(numObjectsPerSlice);free(numObjectsPerSlice);
					exit(1);
				}
				irreRegion = (roiRegionIrregular *)(myRois[jj]->roi_region);
				for(i=0;i<a_contour->num_points;i++){
					irreRegion->points[i]->x = a_contour->x[i];
					irreRegion->points[i]->y = a_contour->y[i];
				}
				contour_destroy(a_contour);
			}
		}
		printf("\n");

		if( !rois_calculate(myRois, totalNumRois) ){
			fprintf(stderr, "%s : call to rois_calculate has failed for %d rois.\n", argv[0], totalNumRois);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE2D(scratch, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			rois_destroy(myRois, totalNumRois);
			free(numObjectsPerSlice);free(numObjectsPerSlice);
			exit(1);
		}

		if( (pixel_size_x!=1.0) || (pixel_size_y!=1.0) )
			if( !rois_convert_pixels_to_millimetres(myRois, totalNumRois, pixel_size_x, pixel_size_y, pixel_size_z) ){
			fprintf(stderr, "%s : call to rois_convert_pixels_to_millimetres has failed for %d rois.\n", argv[0], totalNumRois);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE2D(scratch, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			rois_destroy(myRois, totalNumRois);
			free(numObjectsPerSlice);free(numObjectsPerSlice);
			exit(1);
		}
		
		dummyFilename = (char *)malloc(strlen(outputFilename)+50);
		sprintf(dummyFilename, "%s.roi", outputFilename);
		if( !write_rois_to_file(dummyFilename, myRois, totalNumRois, pixel_size_x, pixel_size_y) ){
			fprintf(stderr, "%s : call to write_rois_to_file has failed for %d rois, output filename '%s'.\n", argv[0], totalNumRois, dummyFilename);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE2D(scratch, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			rois_destroy(myRois, totalNumRois);
			free(numObjectsPerSlice);free(numObjectsPerSlice);
			exit(1);
		}

		if( (scratch2=(float *)malloc(W*H*sizeof(float))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for scratch2.\n", argv[0], W*H*sizeof(float));
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			free(numObjectsPerSlice);
			exit(1);
		}
		rois_associate_with_unc_volume(myRois, totalNumRois, data, scratch2);
		free(scratch2);
		dummyFilename = (char *)malloc(strlen(outputFilename)+50);
		sprintf(dummyFilename, "%s.data", outputFilename);
		if( !write_rois_stats_to_file(dummyFilename, myRois, totalNumRois, FALSE) ){
			fprintf(stderr, "%s : call to write_rois_stats_to_file has failed for %d rois, output filename '%s'.\n", argv[0], totalNumRois, dummyFilename);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE2D(scratch, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			rois_destroy(myRois, totalNumRois);
			free(numObjectsPerSlice);
			exit(1);
		}
		freeDATATYPE2D(scratch, W);
		printf("num rois is %d\n", totalNumRois);
		//rois_destroy(myRois, totalNumRois);
		//free(dummyFilename); free(scratch2);
	} /* doBoth */

	freeDATATYPE3D(dataOut, numSlices, W);
	freeDATATYPE3D(data, actualNumSlices, W);
	free(numObjectsPerSlice);
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}

	free(inputFilename); free(outputFilename);
	
	exit(0);
}
