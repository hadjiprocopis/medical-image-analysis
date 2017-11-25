#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>
#include <LinkedList.h>
#include <LinkedListIterator.h>

#include <cellular_automata.h>

void	SignalHandler(int);
int	SetSignalHandler(void);

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
\n\t -c numClusters\
\n	(the number of clusters)\
\n\
\n\
\n\t[-n min:max\
\n	(normalise input pixels within min and max (floats).\
\n	 If the '-u' flag is present, normalisation\
\n	 will be done uniformly over all pixels of\
\n	 all features (slices). The default, without\
\n	 the '-u' switch, is to normalise each feature's\
\n	 pixels independently. By default, no normalisation\
\n         is done.)]\
\n\t[-t min:max\
\n	(input pixels NOT falling within the threshold\
\n	 range [min, max) will be ignored and not\
\n	 participate in the clustering.)]\
\n\t[-u\
\n	(when present, normalisation will be done uniformly\
\n	 over all pixels of all features (slices). The default,\
\n	 without the '-u' switch, is to normalise each feature's\
\n         pixels independently.)]\
\n\t[-r min:max\
\n	(the pixel range of the output UNC file containing\
\n	 the clusters.)]\
\n\t[-R min:max\
\n	(the pixel range of the output UNC file containing\
\n	 the probabilities. Note SPM uses 0:32767 which is\
\n         the default here too.)]\
\n\t[-S seed\
\n	(the seed to the random number generator.\
\n	 Use the same seed in order to achieve the\
\n	 same results with the same data files.\
\n	 If not present, a random seed will be used.\
\n	 The current seed is displayed at the end\
\n	 of the program.)]\
\n\t[-N iters:percentage\
\n	(Clustering will be done 'iters' times. If\
\n	 'percentage' is > 0.0, then every time after\
\n	 the first clustering,\
\n	 the clusters will be shaked a little bit\
\n	 from their centroid found during the first\
\n	 run. If 'percentage' is <= 0.0, then every\
\n	 time the clustering will be done afresh with\
\n	 a different seed leading to different initial\
\n 	 conditions. The best run (e.g. lowest entropy)\
\n	 will be saved. The default is that only 1\
\n	 time clustering is done.)]\
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
\n\t[-s sliceNumber [-s s...]]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

/* global so it can be accessed by signal handler */
int			numIterations = 1;
cellular_automaton	*aca = NULL;

int	main(int argc, char **argv){
	int		x = 0, y = 0, w = -1, h = -1, numSlices = 0,
			optI, slices[1000];

	char		*inputFilenames[500], copyHeaderFlag = FALSE,
			*outputBasename = NULL, dummy[1000];
	int		numClusters = -1,
			seed = (int )time(0);
	float		K = 1.0, T = 10000.0, V = 4.0;
	int		bestSeed = seed, iters = 0,
			numInputFiles = 0, i, numNearestNeighbours = 25, numFurthestNeighbours = 25;
	char		*initialClustersString = NULL,
			*pidFilename = NULL, *s, verboseFlag = FALSE;
	
	/* normalise all features within this range */
	float		normaliseRangeMin = 0.0, normaliseRangeMax = 0.0;
	/* do not count pixels outside this range */
	int		thresholdRangeMin = 1, thresholdRangeMax = 65535;
	/* during normalisation, will the min and max pixel value be for each feature or over all features ? */
	int		normaliseUniformlyFlag = FALSE;	
	/* output pixel range for clusters */
	DATATYPE	outputClusterPixelRangeMin = -1, outputClusterPixelRangeMax = -1;
	/* output pixel range for probabilities */
	DATATYPE	outputProbsPixelRangeMin = 0, outputProbsPixelRangeMax = 32767;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:c:n:t:ur:R:S:N:C:p:K:T:V:g:v9")) != EOF)
		switch( optI ){
			case 'i': inputFilenames[numInputFiles++] = strdup(optarg); break;
			case 'o': outputBasename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'n': sscanf(optarg, "%f:%f", &normaliseRangeMin, &normaliseRangeMax); break;
			case 't': sscanf(optarg, "%d:%d", &thresholdRangeMin, &thresholdRangeMax); break;
			case 'u': normaliseUniformlyFlag = TRUE; break;
			case 'c': numClusters = atoi(optarg); break;
			case 'r': sscanf(optarg, "%d:%d", &outputClusterPixelRangeMin, &outputClusterPixelRangeMax); break;
			case 'R': sscanf(optarg, "%d:%d", &outputProbsPixelRangeMin, &outputProbsPixelRangeMax); break;
			case 'S': seed = atoi(optarg); break;
			case 'N': numIterations = atoi(optarg); break;
			case 'K': K = atof(optarg); break;
			case 'T': T = atof(optarg); break;
			case 'V': V = atof(optarg); break;
			case 'v': verboseFlag = TRUE; break;
			case 'g': sscanf(optarg, "%d:%d", &numNearestNeighbours, &numFurthestNeighbours); break;
			case 'p': pidFilename = strdup(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);

			case '9': copyHeaderFlag = TRUE; break;
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}

	srand48(seed);

	if( inputFilenames[0] == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		if( outputBasename != NULL ) free(outputBasename);
		if( initialClustersString != NULL ) free(initialClustersString);
		exit(1);
	}
	if( outputBasename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		if( initialClustersString != NULL ) free(initialClustersString);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]);
		exit(1);
	}
	if( numClusters < 2 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "The number of clusters must be greater than 1.\n");
		if( initialClustersString != NULL ) free(initialClustersString);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename);
		exit(1);
	}

	if( ca_read_data_from_UNC_file(inputFilenames, numInputFiles, x, y, &w, &h,
				    numClusters, slices, &numSlices,
				    1.0, /* slice separation in mm */
				    1, 100000, /* min and max range of pixels to be considered in */
				    &aca) == FALSE ){
		fprintf(stderr, "%s : call to read_data_from_UNC_file has failed for files '%s' .. .\n", argv[0], inputFilenames[0]);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename);
		if( initialClustersString != NULL ) free(initialClustersString);
		exit(1);
	}

	if( SetSignalHandler() == FALSE ){
		fprintf(stderr, "%s : call to SetSignalHandler has failed.\n", argv[0]);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
	}

	if( normaliseRangeMin < normaliseRangeMax )
		ca_normalise_points(aca, normaliseRangeMin, normaliseRangeMax, normaliseUniformlyFlag);

	aca->kT = K * T;
	ca_print_cellular_automaton(stdout, aca);
	ca_initialise_to_random_state(aca);

	printf("%s : nearest neighbours: %d, furthest: %d\n",  argv[0], numNearestNeighbours, numFurthestNeighbours);
	ca_find_neighbours(aca, numNearestNeighbours, numFurthestNeighbours);
	if( verboseFlag ){
		ca_print_cells_and_neighbours(stdout, aca);
		printf("*****************************\n");
	}
	for(iters=0;(iters<numIterations)&&(aca->interruptFlag==FALSE);iters++){
		ca_calculate_next_state(aca);	/* make decisions */
		ca_finalise_next_state(aca);	/* make these decisions effective */
		ca_calculate_statistics(aca);	/* calculate homogeneities and entropy */
		printf("iter %d ... entropy = %f, ", iters, aca->totalEntropy);
		for(i=0;i<aca->nS;i++){
			s = toString_vector(aca->clusters[i]->centroid->f);
			printf("[%d=%s,%d pts] ", aca->clusters[i]->id, s, aca->clusters[i]->n);
			free(s);
		}
		printf("\n");

		if( verboseFlag ) ca_print_cells_and_neighbours(stdout, aca);
	}

/*	for(i=0;i<aca->nC;i++){
		for(j=0;j<aca->c[i]->nNN;j++) printf("%d", aca->c[i]->nn[j]->cs->id);
		printf(" | %d (%d)\n", aca->c[i]->cs->id, aca->c[i]->ps->id);
	}
*/

	sprintf(dummy, "%s_masks.unc", outputBasename);
	if( ca_write_masks_to_UNC_file(dummy, aca, outputProbsPixelRangeMin, outputProbsPixelRangeMax) == FALSE ){
		fprintf(stderr, "%s : call to ca_write_masks_to_UNC_file has failed for file '%s'.\n", argv[0], dummy);
		ca_destroy_cellular_automaton(aca);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]);
		free(outputBasename);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilenames[0], dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilenames[0], dummy);
		exit(1);
	}
	printf("%s : masks written to '%s'\n", argv[0], dummy);

	sprintf(dummy, "%s_probs.unc", outputBasename);
	if( ca_write_probs_to_UNC_file(dummy, aca, outputProbsPixelRangeMin, outputProbsPixelRangeMax) == FALSE ){
		fprintf(stderr, "%s : call to ca_write_probs_to_UNC_file has failed for file '%s'.\n", argv[0], dummy);
		ca_destroy_cellular_automaton(aca);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]);
		free(outputBasename);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilenames[0], dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilenames[0], dummy);
		exit(1);
	}
	printf("%s : probs written to '%s'\n", argv[0], dummy);

	sprintf(dummy, "%s_counts.unc", outputBasename);
	if( ca_write_counts_to_UNC_file(dummy, aca, outputProbsPixelRangeMin, outputProbsPixelRangeMax) == FALSE ){
		fprintf(stderr, "%s : call to ca_write_counts_to_UNC_file has failed for file '%s'.\n", argv[0], dummy);
		ca_destroy_cellular_automaton(aca);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]);
		free(outputBasename);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilenames[0], dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilenames[0], dummy);
		exit(1);
	}
	printf("%s : counts written to '%s'\n", argv[0], dummy);

	sprintf(dummy, "%s_states.unc", outputBasename);
	if( ca_write_states_to_UNC_file(dummy, aca, outputProbsPixelRangeMin, outputProbsPixelRangeMax) == FALSE ){
		fprintf(stderr, "%s : call to ca_write_states_to_UNC_file has failed for file '%s'.\n", argv[0], dummy);
		ca_destroy_cellular_automaton(aca);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]);
		free(outputBasename);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilenames[0], dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilenames[0], dummy);
		exit(1);
	}
	printf("%s : states written to '%s'\n", argv[0], dummy);

	sprintf(dummy, "%s_fitness.unc", outputBasename);
	if( ca_write_fitness_to_UNC_file(dummy, aca, outputProbsPixelRangeMin, outputProbsPixelRangeMax) == FALSE ){
		fprintf(stderr, "%s : call to ca_write_fitness_to_UNC_file has failed for file '%s'.\n", argv[0], dummy);
		ca_destroy_cellular_automaton(aca);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]);
		free(outputBasename);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilenames[0], dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilenames[0], dummy);
		exit(1);
	}
	printf("%s : fitness written to '%s'\n", argv[0], dummy);

	ca_destroy_cellular_automaton(aca);
	for(i=0;i<numInputFiles;i++) free(inputFilenames[i]);
	free(outputBasename);
	printf("Seed = %d\n", bestSeed);
	exit(0);
}

void    SignalHandler(int signal_number){
	printf("SignalHandler : caught signal %d\n\n", signal_number);
		if( signal(signal_number, SignalHandler) == SIG_ERR ){
			fprintf(stderr, "SignalHandler : warning!, could not re-set signal number %d\n", signal_number);
			return;
		}
		switch( signal_number ){
		case    SIGINT:
		case    SIGQUIT:
		case    SIGTERM:	numIterations = -1;
					fprintf(stderr, "SignalHandler : I will quit now (after this iteration is finished ...).\n");
					ca_interrupt_cellular_automaton(aca, TRUE);
					break;
		}
}
int     SetSignalHandler(void){
	if( signal(SIGINT, SignalHandler) == SIG_ERR ){
		fprintf(stderr, "SetSignalHandler : could not set signal handler for SIGINT\n");
		return FALSE;
	}
	if( signal(SIGQUIT, SignalHandler) == SIG_ERR ){
		fprintf(stderr, "SetSignalHandler : could not set signal handler for SIGQUITT\n");
		return FALSE;
	}
	if( signal(SIGTERM, SignalHandler) == SIG_ERR ){
		fprintf(stderr, "SetSignalHandler : could not set signal handler for SIGTERM\n");
		return FALSE;
	}
	return TRUE;
}

