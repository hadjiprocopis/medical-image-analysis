#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#include <Common_IMMA.h>
#include <image.h>

const char *Usage = "\t -i \"inputFilename\"\n\t -o \"outputFilename\"\n\t[-s sliceNumber (DEFAULT is 0, the first slice)]\n\t[-w pixelsWidth (DEFAULT is the selected slice's width)]\n\t[-h pixelsHeight(DEFAULT is the selected slice's height)]\n\t[-d 8|16|24|32 (the depth of the output file, e.g. how many bits for color)]";
const char *Author= "Andreas Hadjiprocopis, NMR, ION, 2001.";
const char *Description = "This program will take a UNC image file and produce a raw image file. The raw image file is just a sequence of bytes with no header etc. The UNC file may consist of a serious of slices of images. The user may select one such slice and define the dimensions of it. A UNC file consists of 16-bit pixel colors, the raw file will use 2 bytes for each pixel (HI-BYTE followed by LO-BYTE -> H*256+L=UNC pixel value).";

int	main(int argc, char **argv){
	IMAGE		*image;
	GREYTYPE	*data;
	unsigned char	*dataOut;
	register int	i;
	int		fileHandle, depth = -1;

	char		inputFilename[250] = "", outputFilename[250] = "";
	int		sliceNumber = 0, optI,
			width = -1, height = -1, imageSize,
			actualWidth, actualHeight, actualImageSize,
			totalNumberOfSlices, offset, outputDataSize;

	/* getopt */
	while( (optI=getopt(argc, argv, "i:o:s:g:w:h:d:")) != EOF)
		switch(optI){
			case 'i': strcpy(inputFilename, optarg); break;
			case 'o': strcpy(outputFilename, optarg); break;
			case 's': sliceNumber = atoi(optarg); break;
			case 'w': width = atoi(optarg); break;
			case 'h': height = atoi(optarg); break;
			case 'd': depth = atoi(optarg); break;
			default : fprintf(stderr, "Usage: %s Options ... as follows\n%s\n\n%s\n\n%s\n", argv[0], Usage, Description, Author);
				  exit(1);
		}

	if( (sliceNumber < 0) || (inputFilename[0] == '\0') || (outputFilename[0] == '\0') ||
	    ((depth!=8) && (depth!=16) && (depth!=24) && (depth!=32)) ){
		fprintf(stderr, "Usage: %s Options ... as follows\n%s\n\n%s\n\n%s\n", argv[0], Usage, Description, Author);
		exit(1);
	}

	if( (image=imopen(inputFilename, READ)) == NULL ){
		fprintf(stderr,"%s : Can not open input image '%s' : ", argv[0], inputFilename); perror("");
		exit(1);
	}
	actualWidth = image->Dimv[image->Dimc-1];
	actualHeight= image->Dimv[image->Dimc-2];

	if( width < 0 ) width = actualWidth;
	if( height < 0 ) height = actualHeight;

	actualImageSize = actualWidth * actualHeight;
	if( image->Dimc < 3 ) totalNumberOfSlices = 0;
	else totalNumberOfSlices = image->Dimv[0];
	offset =  sliceNumber*actualImageSize;
	if( width > actualWidth ){
		fprintf(stderr, "%s : the width you specified is too big - make it less than %d\n", argv[0], actualWidth);
		exit(1);
	}
	if( height > actualHeight ){
		fprintf(stderr, "%s : the height you specified is too big - make it less than %d\n", argv[0], actualHeight);
		exit(1);
	}
	if( sliceNumber > totalNumberOfSlices ){
		fprintf(stderr, "%s : the slice number you specified does not exist - make it less than %d\n", argv[0], totalNumberOfSlices);
		exit(1);
	}
		
	printf("Image dimensions: %d (", image->Dimc);
	for(i=0;i<image->Dimc;i++) printf("%d ", image->Dimv[i]);
	printf(")\n");  
	printf("pixel size=%d, count=%d and format=%d\n", image->PixelSize, image->PixelCnt, image->PixelFormat);

	/* GREYTYPE is of type short = 2 bytes (signed by the way) */
	if( (data=(GREYTYPE *)calloc((imageSize=width*height), sizeof(GREYTYPE))) == NULL ){
		fprintf(stderr, "%s : could not allocate %d of size %zd (GREYTYPE) : ", argv[0], width*height, sizeof(GREYTYPE)); perror("");
		imclose(image);
		exit(1);
	}
	if( imread(image, offset, offset+imageSize, data) == INVALID ){
		fprintf(stderr, "%s : could not read %d pixels with %d pixels offset.\n", argv[0], imageSize, offset);
		imclose(image);
		exit(1);
	}
	imclose(image);
	if( (fileHandle=open(outputFilename, O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0 ){
		fprintf(stderr, "%s : could not create output file '%s' : ", argv[0], outputFilename); perror("");
		exit(1);
	}
	outputDataSize = imageSize * (depth/8);
	if( (dataOut=(unsigned char *)calloc(outputDataSize, sizeof(unsigned char))) == NULL ){
		fprintf(stderr, "%s : could not calloc %d of char : ", argv[0], outputDataSize); perror("");
		close(fileHandle);
		exit(1);
	}
	if( image->PixelSize == 1 ){ /* from 8-bit image to :  */
		if( depth == 8 ){
			for(i=0;i<imageSize;i++)
				dataOut[i] = (unsigned char )data[i];
		} else if( depth == 16 ){
			for(i=0;i<imageSize;i++){
				dataOut[2*i] = 0;
				dataOut[2*i+1] = (unsigned char )data[i];
			}
		} else if( depth == 24 ){
			for(i=0;i<imageSize;i++){
				dataOut[3*i] = 0;
				dataOut[3*i+1] = 0;
				dataOut[3*i+2] = (unsigned char )data[i];
			}
		} else if( depth == 32 ){
			for(i=0;i<imageSize;i++){
				dataOut[4*i] = 0;
				dataOut[4*i+1] = 0;
				dataOut[4*i+2] = 0;
				dataOut[4*i+3] = (unsigned char )data[i];
			}
		}
	} else if( image->PixelSize == 2 ){ /* from 16-bit image to : */
		if( depth == 8 ){
			fprintf(stderr, "%s : warning, possible loss of information when writing 8-bit data.\n", argv[0]);
			for(i=0;i<imageSize;i++) // possible loss of information
				dataOut[i] = (unsigned char )(data[i] % 256);
		} else if( depth == 16 ){
			for(i=0;i<imageSize;i++){
				dataOut[2*i] = (unsigned char )(data[i]>>8);
				dataOut[2*i+1] = (unsigned char )(data[i] % 256);
			}
		} else if( depth == 24 ){
			for(i=0;i<imageSize;i++){
				dataOut[3*i] = 0;
				dataOut[3*i+1] = (unsigned char )(data[i]>>8);
				dataOut[3*i+2] = (unsigned char )(data[i] % 256);
			}
		} else if( depth == 32 )
			for(i=0;i<imageSize;i++){
				dataOut[4*i] = 0;
				dataOut[4*i+1] = 0;
				dataOut[4*i+2] = (unsigned char )(data[i]>>8);
				dataOut[4*i+3] = (unsigned char )(data[i] % 256);
			}
	}
	if( (write(fileHandle, dataOut, outputDataSize)) != outputDataSize ){
		fprintf(stderr, "%s : error writing data of %d bytes to output file '%s' : ", argv[0], outputDataSize, outputFilename); perror("");
		close(fileHandle);
		exit(1);
	}
	close(fileHandle);

	free(data);
	free(dataOut);
	exit(0);
}

