#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> /* required in SOLARIS */
#include <sys/types.h>
#include <sys/stat.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>


/* format is 8 (greyscale), depth is 2 (pixel size) */

const	char	Examples[] = "\n\
	-i data -o output.unc -h 512 -p 2 -W 512 -H 512 -S 1\n\
\n\
	will read a raw file with 512 bytes header,\n\
	two bytes per pixel value (these will be\n\
	read as big-endian which is the default)\n\
	Output UNC file should be 512x512 and contain\n\
	1 slice.\n\
\n\
	For little-endian interpretation of the data\n\
	use:\n\
\n\
	-i data -o output.unc -h 512 -p 2 -W 512 -H 512 -S 1 -L\n\
\n\
	If you want to rotate each slice of the image by 90 degrees\n\
	clockwise use:\n\
	-i data -o output.unc -h 512 -p 2 -W 512 -H 512 -S 1 -r 90\n\
\n\
	If you have a UNC text header you want to load to the output UNC\n\
	do:\n\
	-i data -o output.unc -h 512 -p 2 -W 512 -H 512 -S 1 -r 90 -d header.txt\n\
\n\
";

const	char	Usage[] = "options as follows:\n\
-i inputFilename\n\
	(UNC image file with one or more slices)\n\
\n\
-o outputFilename\n\
	(Output filename)\n\
\n\
-p P\n\
	(specify the number of bytes per pixel value, for UNC files,\n\
	 this is usually 2)\n\
	 \n\
-W width\n\
	(The width of the output UNC file)\n\
-H height\n\
	(The height of the output UNC file)\n\
-S slices\n\
	(The number of slices of the output UNC file)\n\
\n\
[-d headerFilename\n\
	(Optionally specify a file containing header information\n\
	 in the format TAG=VALUE (e.g. weight_of_patient=70000).\n\
	 This file will be read and the output UNC image will be\n\
	 modified accordingly.\n\
	 This option is useful in the case when you want to convert\n\
	 a DICOM image to a UNC. Firstly run the command:\n\
	     dctoraw image.dcm image.raw\n\
	 (dctoraw belongs to the dicom3tools package) then\n\
	     dicom2unc_headers.pl -i image.dcm -o header.txt\n\
	 which will convert the DICOM header to a UNC header\n\
	 with loss of fields (if you want to do this conversion\n\
	 manually, then run dicomDumpHeader.pl -i image.dcm -o header.txt\n\
	 and modify the header file to suit your needs\n\
	 then run this command with the following options:\n\
	     -i image.raw -o image.unc -d header.txt -p 2 -W 256 -H 256 -S 1\n\
	 if you have a 256x256, 16-bit single slice.\n\
	 If you have a volume, then you need to run this command for each slice\n\
	 and then merge all UNC images using UNCmake -i .. -i .. ...)]\n\
[-L\n\
	(Depending on which computing system you use, you will\n\
	 have to consider the byte order in which multibyte numbers are\n\
	 stored, particularly when you are writing those numbers to a\n\
	 file. The two orders are called 'Little Endian' and 'Big Endian'.\n\
	 'Big Endian' describes a computer architecture in which, within a\n\
	 given multi-byte numeric representation, the most significant byte\n\
	 has the lowest address (the word is stored `big-end-first').\n\
\n\
	 Eric Raymond observes that Internet domain name addresses and e-mail\n\
	 addresses are little-endian.\n\
\n\
	 Default operation is big-endian. Using this flag will switch\n\
	 to LITTLE-ENDIAN operation.\n\
\n\
	 If you get negative pixels or a grained image, then most likely\n\
	 you need to change the data interpretation. Try with and without\n\
	 this switch.)]\n\
\n\
[-r n]\n\
	(Rotate each slice of the image by n degrees.\n\
         'n' can only be 90, 180 or 270.)\n\
[-h bytes\n\
	(Specify the number of bytes in the header. These will be ignored\n\
	 and image data will be read from there on. Default is 0.)]\n\
";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	int		inputHandle;
	unsigned char	*buffer = NULL;
	DATATYPE	***dataOut, ***tmpData, ***pData;
	char		*inputFilename = NULL, *outputFilename = NULL,
			*headerFilename = NULL;
	int		W = -1, H = -1, numSlices = 0,
			format = 8, s,
			optI, slices[1000], allSlices = 0, x, y, i, j;
	char		bigEndianFlag = TRUE;
	int		numBytesHeader = 0, numBytesPerPixel = 0,
			numBytesToRead, value, m, rotate = 0;
	struct stat	fileStats;

	while( (optI=getopt(argc, argv, "i:o:eW:H:S:h:p:Lr:d:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 'd': headerFilename = strdup(optarg); break;
			case 'h': numBytesHeader = atoi(optarg); break;
			case 'p': numBytesPerPixel = atoi(optarg); break;
			case 'L': bigEndianFlag = FALSE; break;
			case 'W': W = atoi(optarg); break;
			case 'H': H= atoi(optarg); break;
			case 'S': numSlices = atoi(optarg); break;
			case 'r': rotate = atoi(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( (rotate!=90) && (rotate!=180) && (rotate!=270) ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "Only orthogonal rotation is supported - call it flip. The argument to the '-r' option can be one of 90, 180 or 270 only.\n");
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
	if( W <= 0 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "The width of the output UNC file must be specified as a positive integer.\n");
		exit(1);
	}
	if( H <= 0 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "The height of the output UNC file must be specified as a positive integer.\n");
		exit(1);
	}
	if( numSlices <= 0 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "The number of slices of the output UNC file must be specified as a positive integer.\n");
		exit(1);
	}
	if( numBytesPerPixel <= 0 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "The number of bytes per pixel must be specified as a positive integer. For UNC files this number is usually 2.\n");
		exit(1);
	}
	/* open the input file */
	if( (inputHandle=open(inputFilename, O_RDONLY)) == -1 ){
		fprintf(stderr, "%s : could not open file '%s' for reading.\n", argv[0], inputFilename);
		exit(1);
	}

	/* find the size of the input file */
	if( fstat(inputHandle, &fileStats) == -1 ){
		fprintf(stderr, "%s : could not stat file '%s'.\n", argv[0], inputFilename);
		exit(1);
	}

	if( (numBytesToRead=(fileStats.st_size-numBytesHeader)) <= 0 ){
		fprintf(stderr, "%s : file size is %d bytes, header size is %d bytes, the number of bytes to read (%d) is illegal.\n", argv[0], (int )(fileStats.st_size), numBytesHeader, numBytesToRead);
		exit(1);
	}
	if( numBytesToRead < (numSlices*W*H*numBytesPerPixel) ){
		fprintf(stderr, "%s : the number of bytes to read calculated as: numSlices (%d) x Width (%d) x Height (%d) x number of bytes per pixel (%d) = %d, is more than the bytes left (%d) in the file (%s, size=%d) after removing the header (%d)\n", argv[0], numSlices, W, H, numBytesPerPixel, numSlices*W*H*numBytesPerPixel, numBytesToRead, inputFilename, (int )(fileStats.st_size), numBytesHeader);
		exit(1);
	}
	if( numBytesToRead > (numSlices*W*H*numBytesPerPixel) ){
		fprintf(stderr, "%s : *warning* discarding %d bytes from the end of the file.\n", argv[0], (numSlices*W*H*numBytesPerPixel)-numBytesToRead);
	}
		
	if( (buffer=(unsigned char *)malloc(fileStats.st_size*sizeof(unsigned char))) == NULL ){
		fprintf(stderr, "%s : could not allocate %d bytes for file buffer.\n", argv[0], (int )(fileStats.st_size*sizeof(unsigned char)));
		exit(1);
	}
	if( read(inputHandle, buffer, fileStats.st_size) != fileStats.st_size ){
		fprintf(stderr, "%s : could not read %d bytes from input file '%s'.\n", argv[0], (int )(fileStats.st_size), inputFilename);
		free(buffer);
		exit(1);
	}
	close(inputHandle);

	if( (dataOut=callocDATATYPE3D(numSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
		free(inputFilename); free(outputFilename);
		exit(1);
	}

	s = x = y = 0;
	if( bigEndianFlag ){
		for(i=numBytesHeader;i<fileStats.st_size;){
			for(j=0,value=0,m=1;j<numBytesPerPixel;j++){
				value += buffer[i+j] * m;
				m *= 256;
			}
			i += numBytesPerPixel;
			dataOut[s][x][y] = value;
			if( ++y >= H ){
				y = 0;
				if( ++x >= W ){
					x = 0;
					if( ++s >= numSlices ) break;
				}
			}
		}
	} else {
		for(i=numBytesHeader;i<fileStats.st_size;){
			for(j=0,value=0,m=1;j<numBytesPerPixel;j++){
				value += buffer[i+numBytesPerPixel-j-1] * m;
				m *= 256;
			}
			i += numBytesPerPixel;
			dataOut[s][x][y] = value;

			if( ++y >= H ){
				y = 0;
				if( ++x >= W ){
					x = 0;
					if( ++s >= numSlices ) break;
				}
			}
		}
	}

	pData = dataOut;
	if( rotate == 90 ){
		/* clockwise always */
		if( (tmpData=callocDATATYPE3D(numSlices, H, W)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(dataOut, numSlices, W);
			exit(1);
		}
		for(s=0;s<numSlices;s++)
			for(i=0;i<W;i++) for(j=0;j<H;j++)
				tmpData[s][H-j][i] = dataOut[s][i][j];
		pData = tmpData;
		freeDATATYPE3D(dataOut, numSlices, W);
		i = W; W = H ; H = i; /* switch sizes */
	} else if( rotate == 180 ){
		/* clockwise always */
		if( (tmpData=callocDATATYPE3D(numSlices, W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(dataOut, numSlices, W);
			exit(1);
		}
		for(s=0;s<numSlices;s++)
			for(i=0;i<W;i++) for(j=0;j<H;j++)
				tmpData[s][W-i][H-j] = dataOut[s][i][j];
		pData = tmpData;
		freeDATATYPE3D(dataOut, numSlices, W);
	} else if( rotate == 270 ){
		/* clockwise always */
		if( (tmpData=callocDATATYPE3D(numSlices, H, W)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(dataOut, numSlices, W);
			exit(1);
		}
		for(s=0;s<numSlices;s++)
			for(i=0;i<W;i++) for(j=0;j<H;j++)
				tmpData[s][j][W-i] = dataOut[s][i][j];
		pData = tmpData;
		freeDATATYPE3D(dataOut, numSlices, W);
		i = W; W = H ; H = i; /* switch sizes */
	}

	if( !writeUNCSlices3D(outputFilename, pData, W, H, 0, 0, W, H, (allSlices==0) ? slices : NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(pData, numSlices, W);
		exit(1);
	}
	freeDATATYPE3D(pData, numSlices, W);
	free(buffer);

	/* if there is a text file with header info, then load it and set it */
	if( headerFilename != NULL ){
		FILE	*fp;
		IMAGE	*im;
		int	numEntries; /* number of entries of the header file */
		if( (fp=fopen(headerFilename, "r")) == NULL ){
			fprintf(stderr, "%s : could not open header file '%s' for reading, output UNC file has been created successfully but failed while setting this particular header you have requested, nonetheless, it has a default header of its own.\n", argv[0], headerFilename);
			free(inputFilename); free(outputFilename);
			exit(1);
		}
		if( (im=imopen(outputFilename, UPDATE)) == NULL ){
			fprintf(stderr, "%s : call to imopen has failed for UNC image file '%s'.\n", argv[0], outputFilename);
			free(inputFilename); free(outputFilename);
			exit(1);
		}
		if( (im->file_info=readUNCInfoFromTextFile(fp, NULL, NULL, &numEntries)) < 0 ){
			fprintf(stderr, "%s : call to readUNCInfoFromTextFile has failed for header file '%s'. Output UNC file has been created successfully but failed while setting this particular header you have requested, nonetheless, it has a default header of its own.\n", argv[0], headerFilename);
			free(inputFilename); free(outputFilename);
			fclose(fp);
			exit(1);
		}
		fclose(fp);
		if( numEntries == 0 ){
			fprintf(stderr, "%s : something wrong with header file '%s' - read 0 entries! Output UNC file has been created successfully but failed while setting this particular header you have requested, nonetheless, it has a default header of its own.\n", argv[0], headerFilename);
			free(inputFilename); free(outputFilename);
			exit(1);
		}
		imclose(im);
		fprintf(stdout, "%s : read %d header entries from file '%s' and modified output UNC ('%s') accordingly.\n", argv[0], numEntries, headerFilename, outputFilename);
		free(headerFilename);
	}
	free(inputFilename); free(outputFilename);
	exit(0);
}

