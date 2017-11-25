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

#include <clustering.h>

//#define	FITNESS_FUNCTION(_cle) ( ((_cle)->stats->total_entropy * (_cle)->stats->total_distance) )
#define	FITNESS_FUNCTION(_cle) ( ((_cle)->stats->total_entropy) )

void	SignalHandler(int);
int	SetSignalHandler(void);
int	save_to_files(clengine */*_cle*/,
	char */*_outputBasename*/,
	int /*_outputProbsPixelRangeMin*/,
	int /*_outputProbsPixelRangeMax*/,
	int /*_outputClusterPixelRangeMin*/,
	int /*_outputClusterPixelRangeMax*/,
	char */*_copyHeaderFromFile*/);

const	char	Examples[] = "\
\n	-i input1.unc -i input2.unc -o outputs -c 3 -n 0.0:1.0 -T 1000.0:1.0:1.0 -N 2000:2.0:0.0 -S 1974\
\n\
\n	Segment images input1.unc and input2.unc, dump output to files starting with\
\n	the prefix `outputs' (e.g. outputs_masks.unc etc.) with 3 clusters,\
\n	random number generator seed is 1974 (use the same seed to reproduce the results exactly),\
\n	Initial temperature is 1000 and final is 1, cooling scheme `a' parameter is 1\
\n	(which means that temperature decreases nicely exponentially with the number of iterations,\
\n	you can plot the cooling scheme by doing `gnuplot outputs_cooling.gplot')\
\n	The number of iterations is 2000 and the cluster positions will be shaken with gaussian\
\n	noise of mean 0.0 and half-width of 2.0. The locality term will be unchanged.\
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
\n\t -T initial:final:a\
\n	(The temperature of the simulated annealing process\
\n	 will decrease from the `initial' value to the `final'\
\n	 value using the following expression:\
\n\
\n	 T(X) = Tinitial - a * (exp(K * X)-1)\
\n\
\n	 T(X) is the temperature at iteration X, K is\
\n	 K = log(1 + (Tinitial-Tfinal)/a) / numIterations\
\n\
\n	 The above expression represents a negative exponential\
\n	 curve (i.e. it has a vertical asymptote at X = numIterations\
\n	 and a horizontal asymptote at T = Tinitial).\
\n	 When a is less than 1 (and the smaller it is),\
\n	 the temperature drops slowly at first and then when X\
\n	 approaches the total number of iterations, it drops sharply\
\n	 to the final temperature.\
\n	 On the other hand, when a is more than 1 (and the larger it is),\
\n	 the curve loses its sharpness and gradually resembles the\
\n	 shape of a straight line joining (0,Tinitial) with\
\n	 (numIterations, Tfinal) - which means the Temperature T(X)\
\n	 decreases linearly with X.\
\n	 \
\n	 It is safe to set `a' to 10*numIterations - e.g. T decreases linearly.\
\n	 or set `a' to 1/numIterations to get a moderately sharp decrease\
\n	 at about 4/5 to the total number of iterations.)\
\n	 \
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
\n\t[-N iters:cluster_noise_range:beta_noise_range\
\n	(The process will be repeated 'iters' times.\
\n	 Every iteration, the clusters will be shaked\
\n	 using a gaussian noise source with amplitude\
\n	 (width) of 2 * cluster_noise_range.\
\n	 The `beta_noise_range' - if not zero - refers\
\n	 to the noise added to the locality term, beta,\
\n	 which affects the transformed point-to-cluster\
\n	 distance metric. You are safer to set this to zero.)]\
\n\t[-v\
\n        (When the best solution is found, save it to files\
\n         just as it will be done witht the final solutions.\
\n         The files have the same name as the final files and\
\n         will be overwritten by them at the end. Use this\
\n         option to check the best solution found so far\
\n         when the program takes too long to terminate.)]\
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
int	numIterations = 1;
clengine	*cle = NULL;

int	main(int argc, char **argv){
	int		x = 0, y = 0, w = -1, h = -1, numSlices = 0,
			optI, slices[1000];

	FILE		*pidHandle, *reportHandle;
	char		*inputFilenames[500], copyHeaderFlag = FALSE,
			*outputBasename = NULL, dummy[1000];
	int		numClusters = -1,
			seed = (int )time(0);
	float		currentFitness, previousFitness, bestFitness, a = 1.0;
	vector		**currentClusters, **previousClusters, **bestClusters;
	int		*currentClustersID, *previousClustersID, *bestClustersID;
	float		cluster_noise_range = 0.0, beta_noise_range = 0.0,
			K = 1.0, T = 1.0, dE, dET,
			Tfinal = -1, Tinitial = -1, prob;
	int		iters = 0, numAccepted = 0,
			numInputFiles = 0, i, f, bestFitnessAtIter = 1,
			shouldSaveBestSolutions = FALSE, bestSolutionsCount = 1;
	pid_t		myPID = 0;
	char		*pidFilename = NULL;
	vector		**initialClusters = NULL;
	
	/* normalise all features within this range */
	float		normaliseRangeMin = 0.0, normaliseRangeMax = 0.0;
	/* do not count pixels outside this range */
	int		thresholdRangeMin = 1, thresholdRangeMax = 32768;
	/* during normalisation, will the min and max pixel value be for each feature or over all features ? */
	int		normaliseUniformlyFlag = FALSE;	
	/* output pixel range for clusters */
	DATATYPE	outputClusterPixelRangeMin = -1, outputClusterPixelRangeMax = -1;
	/* output pixel range for probabilities */
	DATATYPE	outputProbsPixelRangeMin = 0, outputProbsPixelRangeMax = 32767;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:c:n:t:ur:R:S:N:C:T:p:9v")) != EOF)
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
			case 'T': sscanf(optarg, "%f:%f:%f", &Tinitial, &Tfinal, &a); break; /* initial and final temperature and cooling scheme parameter */
			case 'N': sscanf(optarg, "%d:%f:%f", &numIterations, &cluster_noise_range, &beta_noise_range); break;
			case 'p': pidFilename = strdup(optarg); break;
			case 'v': shouldSaveBestSolutions = TRUE; break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);

			case '9': copyHeaderFlag = TRUE; break;
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}

	if( (Tfinal<=0) ||  (Tinitial<=0) ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "The initial and final temperatures (-T) must be given.\n");
		if( outputBasename != NULL ) free(outputBasename);
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
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]);
		exit(1);
	}
	if( numClusters < 2 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "The number of clusters must be greater than 1.\n");
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename);
		exit(1);
	}

	if( read_data_from_UNC_file(inputFilenames, numInputFiles, x, y, &w, &h,
				    numClusters, slices, &numSlices,
				    1.0, /* slice separation in mm */
				    /* min and max range of pixels to be considered in */
				    thresholdRangeMin, thresholdRangeMax,
				    NULL, NULL, NULL,
				    &cle) == FALSE ){
		fprintf(stderr, "%s : call to read_data_from_UNC_file has failed for files '%s' .. .\n", argv[0], inputFilenames[0]);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename);
		exit(1);
	}

	myPID = getpid();
	if( pidFilename != NULL ){
		if( (pidHandle=fopen(pidFilename, "w")) == NULL ){
			fprintf(stderr, "%s : could not open pid file '%s' for writing the pid.\n", argv[0], pidFilename);
			for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename);
			exit(1);
		}
		fprintf(pidHandle, "%d", (int )myPID);
		fclose(pidHandle);
	}

	if( (currentClusters=new_vectors(cle->nF, cle->nC)) == NULL ){
		fprintf(stderr, "%s : call to new_vectors has failed for %d features and %d clusters - currentClusters.\n", argv[0], cle->nC, cle->nF);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		exit(1);
	}
	if( (bestClusters=new_vectors(cle->nF, cle->nC)) == NULL ){
		fprintf(stderr, "%s : call to new_vectors has failed for %d features and %d clusters - bestClusters.\n", argv[0], cle->nC, cle->nF);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		exit(1);
	}
	if( (previousClusters=new_vectors(cle->nF, cle->nC)) == NULL ){
		fprintf(stderr, "%s : call to new_vectors has failed for %d features and %d clusters - previousClusters.\n", argv[0], cle->nC, cle->nF);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		exit(1);
	}
	if( (currentClustersID=(int *)malloc(cle->nC*sizeof(int))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for currentClustersID.\n", argv[0], cle->nC*sizeof(int));
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		exit(1);
	}		
	if( (bestClustersID=(int *)malloc(cle->nC*sizeof(int))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for bestClustersID.\n", argv[0], cle->nC*sizeof(int));
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		exit(1);
	}
	if( (previousClustersID=(int *)malloc(cle->nC*sizeof(int))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for previousClustersID.\n", argv[0], cle->nC*sizeof(int));
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		exit(1);
	}
	if( normaliseRangeMin < normaliseRangeMax )
		clengine_normalise_points(cle, normaliseRangeMin, normaliseRangeMax, normaliseUniformlyFlag);

	if( SetSignalHandler() == FALSE ){
		fprintf(stderr, "%s : call to SetSignalHandler has failed.\n", argv[0]);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		exit(1);
	}

	srand48(seed);
	printf("Seed = %d, num iters = %d, temp=%.2f to %.2f, noise: clu=%.2f, beta=%.2f\n", seed, numIterations, Tinitial, Tfinal, cluster_noise_range, beta_noise_range);
	reset_clengine(cle);

	clengine_initialise_clusters(cle, NULL);
	calculate_clengine(cle);
	calculate_clengine_stats(cle);
	clengine_assign_points_to_clusters_without_relocations(cle);

	clengine_save_clusters(cle, bestClusters, &bestClustersID);
	bestFitness = 1000000000.0;
	currentFitness = FITNESS_FUNCTION(cle);
	T = Tinitial;
	K = log(1 + (Tinitial-Tfinal)/a) / numIterations;
	while( (iters++ < numIterations) && (cle->interruptFlag==FALSE) ){
		/* save the current state */
		clengine_save_clusters(cle, previousClusters, &previousClustersID);
		previousFitness = currentFitness;

		/* propose a new state - the following function does the update of stats, allocation of points etc */
		clengine_do_simulated_annealing_once(cle, cluster_noise_range, beta_noise_range);
		clengine_save_clusters(cle, currentClusters, &currentClustersID);
		currentFitness = FITNESS_FUNCTION(cle);

		dE = currentFitness - previousFitness;
		dET = - (dE / T);
		if( dET >= 0.0 ) prob = 1.0;
		else prob = exp( dET );

		if( drand48() <= prob ){
			/* accept */
			if( bestFitness > currentFitness ){
				bestFitness = currentFitness;
				copy_vectors(currentClusters, bestClusters, cle->nC);
				for(i=0;i<cle->nC;i++) bestClustersID[i] = currentClustersID[i];
				bestFitnessAtIter = iters;
				clengine_unnormalise_clusters(cle);
				printf("**** best solution found at iter %d temp=%f (%f->%f): current=%f, best=%f at iter %d, a=%f, dE=%f, exp=%f\n", iters, T, Tinitial, Tfinal, currentFitness, bestFitness, bestFitnessAtIter, prob, dE, dET);
				for(i=0;i<cle->nC;i++){
					printf("%d (%d pts) b=%f [", cle->c[i]->id, cle->c[cle->c[i]->id]->n, cle->c[cle->c[i]->id]->beta);
					for(f=0;f<cle->nF;f++) printf("%f ", cle->c[cle->c[i]->id]->centroid->v->c[f]);
					printf("][");
						for(f=0;f<cle->nF;f++) printf("%f ", cle->c[cle->c[i]->id]->centroid->f->c[f]);
					printf("]\n");
				}
				print_clengine(stdout, cle);
				printf("++++\n");

				if( shouldSaveBestSolutions ){
					clengine_sort_clusters_wrt_normalisedpixelvalue_ascending(cle);
					if( save_to_files(cle, outputBasename, outputProbsPixelRangeMin, outputProbsPixelRangeMax, outputClusterPixelRangeMin, outputClusterPixelRangeMax, copyHeaderFlag ? inputFilenames[0] : NULL) == FALSE ){
						fprintf(stderr, "%s : call to save_to_files has failed.\n", argv[0]);
						destroy_clengine(cle);
						for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
						exit(1);
					}
					sprintf(dummy, "%s_stats_%d.txt", outputBasename, bestSolutionsCount);
					if( (reportHandle=fopen(dummy, "w")) == NULL ){
						fprintf(stderr, "%s : could not open file '%s' for writing stats.\n", argv[0], dummy);
						destroy_clengine(cle); for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
						if( initialClusters != NULL ) destroy_vectors(initialClusters, cle->nC);
						exit(1);
					}
					fprintf(reportHandle, "best solution found at iter %d temp=%f (%f->%f): current=%f, best=%f at iter %d, a=%f, dE=%f, exp=%f\n", iters, T, Tinitial, Tfinal, currentFitness, bestFitness, bestFitnessAtIter, prob, dE, dET);
					for(i=0;i<cle->nC;i++) print_cluster_brief(reportHandle, cle->c[i]);
					print_clengine(reportHandle, cle);
					fclose(reportHandle);
					printf("%s : stats written to '%s'.\n", argv[0], dummy);
				}
				bestSolutionsCount++;
			}
			numAccepted++;
		} else {
			/* reject -- put the old clusters back to the clengine (current=previous) */
			copy_vectors(previousClusters, currentClusters, cle->nC);
			for(i=0;i<cle->nC;i++) currentClustersID[i] = previousClustersID[i];
			clengine_load_clusters(cle, currentClusters, currentClustersID);
			currentFitness = previousFitness;
			calculate_clengine(cle);
			calculate_clengine_stats(cle);
			clengine_assign_points_to_clusters_without_relocations(cle);
		}

		if( (iters%50) == 0 ){
			printf("iter : %d, sol.acc.=%d, acc.ratio=%.1f%%, temp=%.2f, best fitness=%.2f at iter %d)\n",
				iters, numAccepted,
				100.0*(1.0-((float )iters-(float )numAccepted)/((float )iters)), T,
				bestFitness, bestFitnessAtIter);
			print_clengine(stdout, cle);
		}

		/* cooling */
		T = Tinitial - a * (exp(K * iters)-1);
	}

	/* ok, load the best clusters and exit */
	clengine_load_clusters(cle, bestClusters, bestClustersID);
	calculate_clengine(cle);
	calculate_clengine_stats(cle);
	clengine_assign_points_to_clusters_without_relocations(cle);
			
	clengine_sort_clusters_wrt_normalisedpixelvalue_ascending(cle); /* e.g. put an order according to intensity - usally csf is last */

	printf("Final:\tcurrrent seed = %d, final entropy = %f (at iter %d) after %d iters.\n", seed, FITNESS_FUNCTION(cle), bestFitnessAtIter, iters);
	printf("clusters:\n");
	for(i=0;i<cle->nC;i++){
		printf("%d (%d pts) b=%f [", cle->c[i]->id, cle->c[cle->c[i]->id]->n, cle->c[cle->c[i]->id]->beta);
		for(f=0;f<cle->nF;f++) printf("%f ", cle->c[cle->c[i]->id]->centroid->v->c[f]);
		printf("][");
		for(f=0;f<cle->nF;f++) printf("%f ", cle->c[cle->c[i]->id]->centroid->f->c[f]);
		printf("]\n");
	}

	destroy_vectors(currentClusters, cle->nC); free(currentClustersID);
	destroy_vectors(previousClusters, cle->nC); free(previousClustersID);
	destroy_vectors(bestClusters, cle->nC); free(bestClustersID);

	if( save_to_files(cle, outputBasename, outputProbsPixelRangeMin, outputProbsPixelRangeMax, outputClusterPixelRangeMin, outputClusterPixelRangeMax, copyHeaderFlag ? inputFilenames[0] : NULL) == FALSE ){
		fprintf(stderr, "%s : call to save_to_files has failed (2).\n", argv[0]);
		destroy_clengine(cle);
		for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		exit(1);
	}
	sprintf(dummy, "%s_stats.txt", outputBasename);
	if( (reportHandle=fopen(dummy, "w")) == NULL ){
		fprintf(stderr, "%s : could not open file '%s' for writing stats.\n", argv[0], dummy);
		destroy_clengine(cle); for(i=0;i<numInputFiles;i++) free(inputFilenames[i]); free(outputBasename); if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
		if( initialClusters != NULL ) destroy_vectors(initialClusters, cle->nC);
		exit(1);
	}
	fprintf(reportHandle, "best entropy = %f\n", bestFitness);
	for(i=0;i<cle->nC;i++) print_cluster_brief(reportHandle, cle->c[i]);
	print_clengine(reportHandle, cle);
	fclose(reportHandle);
	printf("%s : stats written to '%s'.\n", argv[0], dummy);

	destroy_clengine(cle);
	for(i=0;i<numInputFiles;i++) free(inputFilenames[i]);
	free(outputBasename);
	if( pidFilename != NULL ){ unlink(pidFilename); free(pidFilename); }
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
		case    SIGTERM:	fprintf(stderr, "SignalHandler : I will quit now (after this iteration is finished ...).\n");
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

int	save_to_files(clengine *_cle, char *_outputBasename, int _outputProbsPixelRangeMin, int _outputProbsPixelRangeMax, int _outputClusterPixelRangeMin, int _outputClusterPixelRangeMax, char *_copyHeaderFromFile){
	FILE	*clustersASCIIHandle;
	char	dummy[1000];

	sprintf(dummy, "%s_probs.unc", _outputBasename);
	if( write_probability_maps_to_UNC_file(dummy, _cle, _outputProbsPixelRangeMin, _outputProbsPixelRangeMax) == FALSE ){
		fprintf(stderr, "save_to_files : call to write_probability_maps_to_UNC_file has failed for file '%s'.\n", dummy);
		return FALSE;
	}
	/* now copy the image info/title/header of source to destination */
	if( _copyHeaderFromFile != NULL ) if( !copyUNCInfo(_copyHeaderFromFile, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "save_to_files : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", _copyHeaderFromFile, dummy);
		return FALSE;
	}
	printf("save_to_files : probability map written to '%s'\n", dummy);

	sprintf(dummy, "%s_tprobs.unc", _outputBasename);
	if( write_transformed_probability_maps_to_UNC_file(dummy, _cle, _outputProbsPixelRangeMin, _outputProbsPixelRangeMax) == FALSE ){
		fprintf(stderr, "save_to_files : call to write_transformed_probability_maps_to_UNC_file has failed for file '%s'.\n", dummy);
		return FALSE;
	}
	/* now copy the image info/title/header of source to destination */
	if( _copyHeaderFromFile != NULL ) if( !copyUNCInfo(_copyHeaderFromFile, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "save_to_files : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", _copyHeaderFromFile, dummy);
		return FALSE;
	}
	printf("save_to_files : transformed probability map written to '%s'\n", dummy);

	sprintf(dummy, "%s_confs.unc", _outputBasename);
	if( write_confidence_maps_to_UNC_file(dummy, _cle, _outputProbsPixelRangeMin, _outputProbsPixelRangeMax) == FALSE ){
		fprintf(stderr, "save_to_files : call to write_confidence_maps_to_UNC_file has failed for file '%s'.\n", dummy);
		return FALSE;
	}
	/* now copy the image info/title/header of source to destination */
	if( _copyHeaderFromFile != NULL ) if( !copyUNCInfo(_copyHeaderFromFile, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "save_to_files : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", _copyHeaderFromFile, dummy);
		return FALSE;
	}
	printf("save_to_files : confidence map written to '%s'\n", dummy);

	sprintf(dummy, "%s_masks.unc", _outputBasename);
	if( write_masks_to_UNC_file(dummy, _cle, _outputClusterPixelRangeMin, _outputClusterPixelRangeMax) == FALSE ){
		fprintf(stderr, "save_to_files : call to write_clusters_to_UNC_file has failed for file '%s'.\n", dummy);
		return FALSE;
	}
	/* now copy the image info/title/header of source to destination */
	if( _copyHeaderFromFile != NULL ) if( !copyUNCInfo(_copyHeaderFromFile, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "save_to_files : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", _copyHeaderFromFile, dummy);
		return FALSE;
	}
	printf("save_to_files : masks for segmenting out the clusters written to '%s'\n", dummy);

	sprintf(dummy, "%s_entropy.unc", _outputBasename);
	if( write_pixel_entropy_to_UNC_file(dummy, _cle, _outputProbsPixelRangeMin, _outputProbsPixelRangeMax) == FALSE ){
		fprintf(stderr, "save_to_files : call to write_pixel_entropy_to_UNC_file has failed for file '%s'.\n", dummy);
		return FALSE;
	}
	/* now copy the image info/title/header of source to destination */
	if( _copyHeaderFromFile != NULL ) if( !copyUNCInfo(_copyHeaderFromFile, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "save_to_files : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", _copyHeaderFromFile, dummy);
		return FALSE;
	}
	printf("save_to_files : pixel entropy written to '%s'\n", dummy);

	if( 0 && (_cle->nC == 3) ){
		/* skip for now */
		/* only works with 3 clusters */
		sprintf(dummy, "%s_pv.unc", _outputBasename);
		if( write_partial_volume_pixels_to_UNC_file(dummy, _cle, _outputProbsPixelRangeMin, _outputProbsPixelRangeMax) == FALSE ){
			fprintf(stderr, "save_to_files : call to write_partial_volume_pixels_to_UNC_file has failed for file '%s'.\n", dummy);
			return FALSE;
		}
		/* now copy the image info/title/header of source to destination */
		if( _copyHeaderFromFile != NULL ) if( !copyUNCInfo(_copyHeaderFromFile, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
			fprintf(stderr, "save_to_files : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", _copyHeaderFromFile, dummy);
			return FALSE;
		}
		printf("save_to_files : partial volume metrics written to '%s'\n", dummy);
	}

	sprintf(dummy, "%s_clusters.unc", _outputBasename);
	if( write_clusters_to_UNC_file(dummy, _cle, _outputClusterPixelRangeMin, _outputClusterPixelRangeMax) == FALSE ){
		fprintf(stderr, "save_to_files : call to write_clusters_to_UNC_file has failed for file '%s'.\n", dummy);
		return FALSE;
	}
	/* now copy the image info/title/header of source to destination */
	if( _copyHeaderFromFile != NULL ) if( !copyUNCInfo(_copyHeaderFromFile, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "save_to_files : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", _copyHeaderFromFile, dummy);
		return FALSE;
	}

	printf("save_to_files : clusters written to '%s' in UNC format\n", dummy);
	sprintf(dummy, "%s_clusters.txt", _outputBasename);
	if( (clustersASCIIHandle=fopen(dummy, "w")) == NULL ){
		fprintf(stderr, "save_to_files : could not open file '%s' for writing the clusters.\n", dummy);
		return FALSE;
	}
	printf("save_to_files : clusters written to '%s' in text format\n", dummy);
	write_clusters_to_ASCII_file(clustersASCIIHandle, _cle);	
	fclose(clustersASCIIHandle);

	return TRUE;
}
