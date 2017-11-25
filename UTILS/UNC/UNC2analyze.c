#include <stdio.h>
/* for strdup */
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
/* for strptime */
#undef _GNU_SOURCE
#define _XOPEN_SOURCE
#include <time.h>
#include <ctype.h>
/* for gcc 4 we get problems with stupid defines */
extern char *strptime(const char *s, const char *format, struct tm *tm);

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

/* what is the max number of specs ? */
#define MAX_SPECS       30


const	char	Examples[] = "\
\n	-i inp.unc -o out.unc -s 0 -s 1 -s 4 -x 10 -y 10 -w 13 -h 13 -p 5:15:100 -p 25:35:200 -f 40:50:500 -p 80:100:-1\
\n\
\nThis example will operate only on slices 0,1 and 4 and the\
\nregion bounded by (10,10,23,23) of file 'inp.unc'.\
\n** It will change the intensities of those pixels whose\
\n   intensity is between 5 (incl) and 15 excl to 100.\
\n** It will change the intensities of those pixels whose\
\n   intensity is between 25 (incl) and 35 excl to 200.\
\n** It will change the intensities of those pixels whose\
\n   frequency of occurence (in the histogram)\
\n   is between 40 (incl) and 50 excl to 500.\
\n** It will *LEAVE UNCHANGED* those pixels whose\
\n   intensity is between 80 (incl) and 100 excl\
\n   (notice the -1 value for 'newColor')\
\n** All other pixels including all pixels falling outside\
\n   the region of interest and slices specified will be\
\n   left unchanged to original intensities.\
\n\
\n	-i inp.unc -o out.unc -s 0 -s 1 -s 4 -x 10 -y 10 -w 13 -h 13 -p 5:15:100 -p 25:35:200 -f 40:50:500 -p 80:100:-1 -d 1000\
\n\
\nAs before, but all other pixels falling within ROI and\
\nselected slices will have their intensities change to 1000.\
\n\
\n	-i inp.unc -o out.unc -s 0 -s 1 -s 4 -x 10 -y 10 -w 13 -h 13 -p 5:15:100 -f 40:50:500 -a 1000\
\n\
\nThe pixels that satisfy the following two criteria and\
\nwithin the ROI will have their pixel values changed to\
\n1000:\
\n** intensity is between 5 (incl) and 15 excl to 100\
\n** frequency of occurence (in the histogram)\
\n   is between 40 (incl) and 50 excl to 500.";

const	char	Usage[] = "options as follows:\
\n -i inputFilename\
\n	(Filename of the input UNC image with one or more slices)\
\n\
\n -o outputFilename\
\n	(Filename of the output ANALYZE image)\
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

int	main(int argc, char **argv){
	FILE		*fp;
	IMAGE		*image;
	DATATYPE	***data, ***dataOut;
	char		*inputFilename = NULL, *outputFilename = NULL;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;
	analyze_header	hdr;
	char		anonymiseOutput = FALSE, tmp_str[10000];
	int		tmp_int, i, j, ii, jj;
	int		systemEndian = check_system_endianess(),
			targetEndian = systemEndian;
	
	while( (optI=getopt(argc, argv, "i:o:s:w:h:x:y:a:e:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'a': anonymiseOutput = TRUE; break;
			case 'e': switch(tolower(optarg[0])){
				/* if this option is not present, target endianess = system endianess */
				case 'b' : targetEndian = IO_RAW_BIG_ENDIAN; break;
				case 'l' : targetEndian = IO_RAW_LITTLE_ENDIAN; break;
				case 's' : targetEndian = (systemEndian == IO_RAW_BIG_ENDIAN) ? IO_RAW_LITTLE_ENDIAN : IO_RAW_BIG_ENDIAN; break;
				default	 : fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
					   fprintf(stderr, "the -e option expects one of b, l or s (for big endian, little endian or byte swap)\n");
					   exit(1);
				  } /* switch optarg[0] */
				break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;

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
	if( (data=getUNCSlices3D_withImage(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format, &image)) == NULL ){
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

	if( (dataOut=callocDATATYPE3D(numSlices, w, h)) == NULL ){
		fprintf(stderr, "%s : call to callocDATATYPE3D has failed for %d x %d x %d.\n", argv[0], numSlices, w, h);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	printf("%s, slice :", inputFilename); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf(" %d", slice+1); fflush(stdout);
		for(i=0,ii=x;i<w;ii++,i++) for(j=0,jj=y;j<h;jj++,j++) dataOut[s][i][j] = data[slice][ii][jj];
	}									
	freeDATATYPE3D(data, actualNumSlices, W);
	printf("\n");

	int	xdim = W, ydim = H, zdim = numSlices;
	DATATYPE	maxmin[2];
	min_maxPixel3D(dataOut, 0, 0, 0, w, h, numSlices, &(maxmin[0]), &(maxmin[1]));
	
	/* Initialise analyze header */
	hdr.hk.sizeof_hdr = sizeof(hdr);
/*	hdr.hk.extents = 16384;*/
	hdr.hk.extents = xdim*ydim;	/* changed GJMP 22 4 99 to make same as signa2analyze - may not make any difference */
	/* recommended by Reference Manaul V.7.5 */
	hdr.hk.regular = 'r';
	hdr.dime.dim[0] = 4;
	hdr.dime.dim[1] = xdim;
	hdr.dime.dim[2] = ydim;
	hdr.dime.dim[3] = zdim;
	hdr.dime.dim[4] = 1;
	hdr.dime.datatype = DT_SIGNED_SHORT;	/* always GREY TYPE */
	hdr.dime.bitpix = sizeof(GREYTYPE) * 8;
	hdr.dime.glmin = maxmin[0];
	hdr.dime.glmax = maxmin[1];
	/* CMW 20050601 - roi_scale is spm2 friendly */
	hdr.dime.roi_scale = 1;

	strcpy(hdr.dime.vox_units, "mm");
	strcpy(hdr.dime.cal_units, "");

	hdr.dime.vox_offset = 0;
	hdr.dime.cal_max = 0.0;
	hdr.dime.cal_min = 0.0;

	/* Update info fields */
	if( anonymiseOutput ){
		strcpy(hdr.hist.scannum, "00000_000");
		strcpy(hdr.hist.patient_id, "anonymous");
		strcpy(hdr.hist.descrip, "anonymous");
		strcpy(hdr.hist.exp_date, "00-00-00");
		strcpy(hdr.hist.exp_time, "00:00:00");
	} else {
		imgetinfo(image, "exam_number", tmp_str);
		sscanf(tmp_str, "%d", &tmp_int);
		sprintf(tmp_str, "%05d", tmp_int);
		strcpy(hdr.hist.scannum, tmp_str);
		strcat(hdr.hist.scannum, "_");
		imgetinfo(image, "series_number", tmp_str);
		sscanf(tmp_str, "%d", &tmp_int);
		sprintf(tmp_str, "%03d", tmp_int);
		strcat(hdr.hist.scannum, tmp_str);

		imgetinfo(image, "patient_id", tmp_str);
		strncpy(hdr.hist.patient_id, tmp_str, 10);
		hdr.hist.patient_id[9] = '\0';

		imgetinfo(image, "exam_description", tmp_str);
		strcpy(hdr.hist.descrip, tmp_str);


		/* date and time */
		{
			struct tm      *time_struct;

			/* read back the separate date and time and reformat them as
			   signa2unc would have */
			time_struct = (struct tm *) malloc(sizeof(struct tm));
			imgetinfo(image, "actual_series_date-time", tmp_str);
			strptime(tmp_str, "%a %b %e %T %Y", time_struct);	/* This is the format
										   used by signa2unc */

			strftime(hdr.hist.exp_date, 10, "%d-%m-%y", time_struct);
			hdr.hist.exp_date[8] = '\0';	/* These are the formats used
							   by signa2analyze */


			strftime(hdr.hist.exp_time, 10, "%T", time_struct);
			hdr.hist.exp_time[8] = '\0';
		}

	}

	imgetinfo(image, "most_like_plane", tmp_str);

/*
	if (sscanf(tmp_str, "%d", &tmp_int) == 1)
	{
		switch (tmp_int)
		{
		case (2):
			hdr.hist.orient = 0;
			break;
		case (4):
			hdr.hist.orient = 2;
			break;
		case (8):
			hdr.hist.orient = 1;
			break;
		default:
			hdr.hist.orient = 0;
			break;
		}
	}
	else
		hdr.hist.orient = 0;

*/
/* changed GJMP 22 4 99. Above uses Signa numerical code for orientation - should be the strings "Axial" "Coronal" "Sagittal" as below */

/* Updated to use strstr instead of strcmp, to catch both, eg, "Axial" and "Oblique Axial" /09/08/2004 GJB */
	if (strstr(tmp_str,"Axial") != NULL)
			hdr.hist.orient = 0;
	else if (strstr(tmp_str,"Sagittal") != NULL)
			hdr.hist.orient = 2;
	else if (strstr(tmp_str,"Coronal") != NULL)
			hdr.hist.orient = 1;
	else 
	{
		/* Fall back to looking what the start location for 
		the prescription was, and work out orientation from there */
		imgetinfo(image, "FirstScanRAS", tmp_str);
		/*imgetinfo(image, "LastScanRAS", tmp_str2);*/ /* Redundent? */
		if (strspn(tmp_str, "AP") > 0)
			 hdr.hist.orient = 1;
		else if (strspn(tmp_str, "LR") > 0)
                         hdr.hist.orient = 2;
		else if (strspn(tmp_str, "SI") > 0)
                         hdr.hist.orient = 0;
		else
			hdr.hist.orient = 0;	/* default */
	}

	tmp_str[0] = '\0';
	imgetinfo(image, "pixel_x_size", tmp_str);
	if( tmp_str[0] == '\0' ){
		fprintf(stderr, "%s : pixel_x_size not set. Set it with update_unc_info and then re-run.", argv[0]);
		exit(1);
	}
	hdr.dime.pixdim[1] = atof(tmp_str);

	tmp_str[0] = '\0';
	imgetinfo(image, "pixel_y_size", tmp_str);
	if( tmp_str[0] == '\0' ){
		fprintf(stderr, "%s : pixel_y_size not set. Set it with update_unc_info and then re-run.", argv[0]);
		exit(1);
	}
	hdr.dime.pixdim[2] = atof(tmp_str);

	tmp_str[0] = '\0';
	imgetinfo(image, "pixel_z_size", tmp_str);
	if( tmp_str[0] == '\0' ){
		fprintf(stderr, "%s : pixel_z_size not set. Set it with update_unc_info and then re-run.", argv[0]);
		exit(1);
	}
	hdr.dime.pixdim[3] = atof(tmp_str);

	/* This set as in signa2analyze GJMP 26/4/99 */
	hdr.dime.pixdim[0] = 4;

	imclose(image);

	/* Create new analyze header file */
	sprintf(tmp_str, "%s.hdr", outputFilename);
	if( (fp=fopen(tmp_str, "wb")) == NULL ){
		fprintf(stderr, "%s : could not open output header file '%s' for writing.\n", argv[0], tmp_str);
		freeDATATYPE3D(dataOut, numSlices, w);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	/* write the header and do 'byte swapping' only if system and target endian do not match */
	if( writeANALYZEHeader(fp, &hdr, systemEndian != targetEndian) == FALSE ){
		fprintf(stderr, "%s : call to writeANALYZEHeader has failed for output header file '%s'.\n", argv[0], tmp_str);
		freeDATATYPE3D(dataOut, numSlices, w);
		free(inputFilename); free(outputFilename);
		fclose(fp);
		exit(1);
	}
	fclose(fp);

	/* write the data */
	sprintf(tmp_str, "%s.img", outputFilename);
	if( writeDATATYPE3DasRAW(tmp_str, dataOut, w, h, 0, 0, 0, w, h, numSlices, NULL, 2, targetEndian) == FALSE ){
		fprintf(stderr, "%s : call to writeDATATYPE3DasRAW has failed for output image file '%s'.\n", argv[0], tmp_str);
		freeDATATYPE3D(dataOut, numSlices, w);
		free(inputFilename); free(outputFilename);
		exit(1);
	}

	freeDATATYPE3D(dataOut, numSlices, w);
	free(inputFilename); free(outputFilename);
	exit(0);
}
