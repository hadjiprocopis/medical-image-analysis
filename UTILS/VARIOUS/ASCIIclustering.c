#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <Common_IMMA.h>
#include <image.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>
#include <LinkedList.h>
#include <LinkedListIterator.h>

#include <clustering.h>

void	SignalHandler(int);
int	SetSignalHandler(void);

const	char	Examples[] = "\
	-i input.unc -o output.unc\
";

const	char	Usage[] = "options as follows:\n\
\t -i inputFilename\n\
	(ASCII data file)\n\
\n\
\t -o outputBasename\n\
	(Output basename)\n\
\t[-n min:max\n\
	(normalise input pixels within min and max (floats).\n\
	 If the '-u' flag is present, normalisation\n\
	 will be done uniformly over all pixels of\n\
	 all features (slices). The default, without\n\
	 the '-u' switch, is to normalise each feature's\n\
	 pixels independently. By default, no normalisation\n\
         is done.)]\n\
\t[-t min:max\n\
	(input pixels NOT falling within the threshold\n\
	 range [min, max) will be ignored and not\n\
	 participate in the clustering.)]\n\
\t[-u\n\
	(when present, normalisation will be done uniformly\n\
	 over all pixels of all features (slices). The default,\n\
	 without the '-u' switch, is to normalise each feature's\n\
         pixels independently.)]\n\
\t[-r min:max\n\
	(the pixel range of the output UNC file containing\n\
	 the clusters.)]\n\
\t[-R min:max\n\
	(the pixel range of the output UNC file containing\n\
	 the probabilities. Note SPM uses 0:32767 which is\n\
         the default here too.)]\n\
\t[-S seed\n\
	(the seed to the random number generator.\n\
	 Use the same seed in order to achieve the\n\
	 same results with the same data files.\n\
	 If not present, a random seed will be used.\n\
	 The current seed is displayed at the end\n\
	 of the program.)]\n\
\t[-N iters:percentage\n\
	(Clustering will be done 'iters' times. If\n\
	 'percentage' is > 0.0, then every time after\n\
	 the first clustering,\n\
	 the clusters will be shaked a little bit\n\
	 from their centroid found during the first\n\
	 run. If 'percentage' is <= 0.0, then every\n\
	 time the clustering will be done afresh with\n\
	 a different seed leading to different initial\n\
 	 conditions. The best run (e.g. lowest entropy)\n\
	 will be saved. The default is that only 1\n\
	 time clustering is done.)]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

/* global so it can be accessed by signal handler */
int	numIterations = 1;
clengine	*cle = NULL;

int	main(int argc, char **argv){
	FILE		*clustersASCIIHandle, *pidHandle, *reportHandle;
	char		*inputFilename = NULL,
			*outputBasename = NULL;
	int		numClusters = -1, optI, seed = (int )time(0),
			numFeatures, numPoints;
	float		minTotalEntropy, noisePercentage = 0.0;
	int		bestSeed = seed, iters = 0, bestIter = 0,
			i, c, f;
	pid_t		myPID = 0;
	char		*bestStatsString = strdup("fo"),
			*initialClustersString = NULL,
			*pidFilename = NULL, dummy[1000];
	vector		**initialClusters = NULL, **bestClusters;
	int		*bestClustersID;
	
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

	while( (optI=getopt(argc, argv, "i:o:e:c:n:t:ur:R:S:N:C:p:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputBasename = strdup(optarg); break;
			case 'n': sscanf(optarg, "%f:%f", &normaliseRangeMin, &normaliseRangeMax); break;
			case 't': sscanf(optarg, "%d:%d", &thresholdRangeMin, &thresholdRangeMax); break;
			case 'u': normaliseUniformlyFlag = TRUE; break;
			case 'c': numClusters = atoi(optarg); break;
			case 'r': sscanf(optarg, "%d:%d", &outputClusterPixelRangeMin, &outputClusterPixelRangeMax); break;
			case 'R': sscanf(optarg, "%d:%d", &outputProbsPixelRangeMin, &outputProbsPixelRangeMax); break;
			case 'S': seed = atoi(optarg); break;
			case 'N': sscanf(optarg, "%d:%f", &numIterations, &noisePercentage); break;
			case 'C': initialClustersString = strdup(optarg); break;
			case 'p': pidFilename = strdup(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilename == NULL ){
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
		free(inputFilename);
		exit(1);
	}
	if( numClusters < 2 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "The number of clusters must be greater than 1.\n");
		if( initialClustersString != NULL ) free(initialClustersString);
		free(inputFilename); free(outputBasename);
		exit(1);
	}

	numFeatures = numPoints = -1; /* let it find these numbers itself */
	if( read_data_from_ASCII_file(inputFilename, numClusters, &numFeatures, &numPoints, &cle) == FALSE ){
		fprintf(stderr, "%s : call to read_data_from_UNC_file has failed for file '%s' .. .\n", argv[0], inputFilename);
		free(inputFilename); free(outputBasename);
		if( initialClustersString != NULL ) free(initialClustersString);
		exit(1);
	}
	fprintf(stdout, "%s : num points is %d and num features is %d.\n", argv[0], numPoints, numFeatures);

	myPID = getpid();
	if( pidFilename != NULL ){
		if( (pidHandle=fopen(pidFilename, "w")) == NULL ){
			fprintf(stderr, "%s : could not open pid file '%s' for writing the pid.\n", argv[0], pidFilename);
			free(inputFilename); free(outputBasename);
			if( initialClustersString != NULL ) free(initialClustersString);
			exit(1);
		}
		fprintf(pidHandle, "%d", (int )myPID);
		fclose(pidHandle);
	}

	if( initialClustersString != NULL ){
		char	*dummyS;
		if( (initialClusters=new_vectors(cle->nF, cle->nC)) == NULL ){
			fprintf(stderr, "%s : call to new_vectors has failed for %d features and %d clusters.\n", argv[0], cle->nC, cle->nF);
			free(inputFilename); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
			free(initialClustersString);
		}
		
		for(c=0;c<cle->nC;c++) for(f=0;f<cle->nF;f++){
			if( (c==0) && (f==0) ){
				if( (dummyS = strtok(initialClustersString, " ")) == NULL ){
					fprintf(stderr, "%s : something wrong with the initial clusters parameter string (-C) option while reading the first feature of the first cluster. Total number of features was %d and total number of clusters was %d.\n", argv[0], cle->nF, cle->nC);
					free(inputFilename); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
					destroy_vectors(initialClusters, cle->nC);
					free(initialClustersString);
					exit(1);
				}
				sscanf(dummyS, "%f", &(initialClusters[c]->c[f]));
			} else {
				if( (dummyS = strtok((char *)NULL, " ")) == NULL ){
					fprintf(stderr, "%s : something wrong with the initial clusters parameter string (-C) option while reading feature %d of cluster %d. Total number of features was %d and total number of clusters was %d.\n", argv[0], f+1, c+1, cle->nF, cle->nC);
					free(inputFilename); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
					destroy_vectors(initialClusters, cle->nC);
					free(initialClustersString);
					exit(1);
				}
				sscanf(dummyS, "%f", &(initialClusters[c]->c[f]));
			}
		}
		printf("initial clusters : ");
		for(c=0;c<cle->nC;c++){
			printf("(");
			for(f=0;f<cle->nF;f++) printf("%f ", initialClusters[c]->c[f]);
			printf(")");
		}
		printf("\n");
		free(initialClustersString);
	}

	if( SetSignalHandler() == FALSE ){
		fprintf(stderr, "%s : call to SetSignalHandler has failed.\n", argv[0]);
		free(inputFilename); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		exit(1);
	}
	if( (bestClusters=new_vectors(cle->nF, cle->nC)) == NULL ){
		fprintf(stderr, "%s : call to new_vectors has failed for %d features and %d clusters.\n", argv[0], cle->nC, cle->nF);
		free(inputFilename); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		exit(1);
	}
	if( (bestClustersID=(int *)malloc(cle->nC*sizeof(int))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for bestClustersID.\n", argv[0], cle->nC*sizeof(int));
		free(inputFilename); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		exit(1);
	}		
	if( normaliseRangeMin < normaliseRangeMax )
		clengine_normalise_points(cle, normaliseRangeMin, normaliseRangeMax, normaliseUniformlyFlag);

	if( noisePercentage <= 0.0 ){
		minTotalEntropy = 1000000000.0;
		while( iters++ < numIterations ){
			srand48(seed);
			printf("Seed = %d\n", seed);
			reset_clengine(cle);
			clengine_initialise_clusters(cle, initialClusters);
			calculate_clengine(cle);
			clengine_assign_points_to_clusters(cle);
			if( (cle->stats->total_entropy < minTotalEntropy) && (numIterations > 0) ){
				minTotalEntropy = cle->stats->total_entropy;
				bestSeed = seed;
				bestIter = iters;
				free(bestStatsString);
				bestStatsString = toString_clestats(cle->stats);
			}
			printf("iter %d:\tcurrent seed = %d, current entropy = %f\n\t\tbest seed = %d, best entropy = %f at iter %d\n\t%s\n\n", iters, seed, cle->stats->total_entropy, bestSeed, minTotalEntropy, bestIter, bestStatsString);
			seed = lrand48();
		}
		/* repeat with the best seed settings */
		clengine_interrupt(cle, FALSE);
		srand48(bestSeed);
		reset_clengine(cle);
		clengine_initialise_clusters(cle, initialClusters);
		calculate_clengine(cle);
		clengine_assign_points_to_clusters(cle);

		printf("Final results: best seed = %d,\n%s\n", bestSeed, bestStatsString);
	} else {
		srand48(seed);
		printf("Seed = %d (i am shaking clusters)\n", seed);
		minTotalEntropy = 1000000000.0;
		reset_clengine(cle);
		clengine_initialise_clusters(cle, initialClusters);
		clengine_save_clusters(cle, bestClusters, &bestClustersID);
		while( iters++ < numIterations ){
			clengine_shake_clusters(cle, noisePercentage);
			calculate_clengine(cle);
			clengine_assign_points_to_clusters(cle);
			calculate_clengine(cle);
			if( cle->stats->total_entropy < minTotalEntropy ){
				minTotalEntropy = cle->stats->total_entropy;
				clengine_save_clusters(cle, bestClusters, &bestClustersID);
				bestIter = iters;
				free(bestStatsString);
				bestStatsString = toString_clestats(cle->stats);
			}
			printf("iter %d:\tcurrent seed = %d, current entropy = %f\n\t\tbest entropy = %f at iter %d\nbest stats: %s\n\n", iters, seed, cle->stats->total_entropy, minTotalEntropy, bestIter, bestStatsString);
		}
		clengine_interrupt(cle, FALSE);
		clengine_load_clusters(cle, bestClusters, bestClustersID);
		calculate_clengine(cle);
		clengine_assign_points_to_clusters(cle);
		calculate_clengine(cle);
		printf("Final stats (i am shaking clusters):\n%s\n", bestStatsString);
		for(i=0;i<cle->nC;i++) print_cluster_brief(stdout, cle->c[i]);
		print_clengine(stdout, cle);
	}
	destroy_vectors(bestClusters, cle->nC);
	free(bestClustersID);

	clengine_sort_clusters_wrt_pixelvalue_ascending(cle);

	sprintf(dummy, "%s_clusters.txt", outputBasename);
	if( (clustersASCIIHandle=fopen(dummy, "w")) == NULL ){
		fprintf(stderr, "%s : could not open file '%s' for writing the clusters.\n", argv[0], dummy);
		destroy_clengine(cle); free(inputFilename); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		if( initialClusters != NULL ) destroy_vectors(initialClusters, cle->nC);
		exit(1);
	}
	printf("%s : clusters written to '%s' in text format\n", argv[0], dummy);
	write_clusters_to_ASCII_file(clustersASCIIHandle, cle);	
	fclose(clustersASCIIHandle);

	sprintf(dummy, "%s_points.txt", outputBasename);
	if( write_points_to_ASCII_file(dummy, cle) == FALSE ){
		fprintf(stderr, "%s : call to write_points_to_ASCII_file has failed.\n", argv[0]);
		destroy_clengine(cle); free(inputFilename); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		if( initialClusters != NULL ) destroy_vectors(initialClusters, cle->nC);
		exit(1);
	}
	printf("%s : points and their distances from clusters (etc.) written to '%s' in text format\n", argv[0], dummy);

	sprintf(dummy, "%s_stats.txt", outputBasename);
	if( (reportHandle=fopen(dummy, "w")) == NULL ){
		fprintf(stderr, "%s : could not open file '%s' for writing stats.\n", argv[0], dummy);
		destroy_clengine(cle); free(inputFilename); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		if( initialClusters != NULL ) destroy_vectors(initialClusters, cle->nC);
		exit(1);
	}
	fprintf(reportHandle, "best entropy = %f at iter %d\nbest stats: %s\n", minTotalEntropy, bestIter, bestStatsString);
	for(i=0;i<cle->nC;i++) print_cluster_brief(reportHandle, cle->c[i]);
	print_clengine(reportHandle, cle);
	fclose(reportHandle);
	printf("%s : stats written to '%s'.\n", argv[0], dummy);

	destroy_clengine(cle); free(inputFilename); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
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
					clengine_interrupt(cle, TRUE);
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

