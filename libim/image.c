/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Program:  IMAGE.C                                                         */
/*                                                                           */
/* Purpose:  This is version 2 of the image processing library.  It is       */
/*           intended to provide a simple interface for reading and          */
/*           writing images created by version 1 of the library.  This       */
/*           collection of routines differs from the original routines       */
/*           in the following ways:                                          */
/*                                                                           */
/*           1)  New types of images are handled.  The set now includes      */
/*               GREY, COLOR, COLORPACKED, BYTE, SHORT, LONG, REAL,          */
/*               COMPLEX and USERPACKED.                                     */
/*                                                                           */
/*           2)  Faster access to pixels is provided.  Special routines      */
/*               are included which are optimal for reading/writing 1D,      */
/*               2D and 3D images.  The N dimensional pixel access routine   */
/*               has also been improved.                                     */
/*                                                                           */
/*           3)  Three new routines imread, imwrite and imheader.            */
/*               See the iman entries for a full description of these        */
/*               new routines.                                               */
/*                                                                           */
/* Contains: imcreat            - Image initialization routines              */
/*           imopen                                                          */
/*           imclose                                                         */
/*                                                                           */
/*           imread             - Pixel access routines                      */
/*           imwrite                                                         */
/*           imgetpix                                                        */
/*           imputpix                                                        */
/*           GetPut2D                                                        */
/*           GetPut3D                                                        */
/*           GetPutND                                                        */
/*                                                                           */
/*           imheader           - Information access routines                */
/*           imdim                                                           */
/*           imbounds                                                        */
/*           imgetdesc                                                       */
/*           imtest                                                         */
/*           imgettitle                                                      */
/*           imputtitle                                                      */
/*           imgetinfo		- REPLACED BY NEW ROUTINES 7/8/89	     */
/*           imputinfo		- REPLACED BY NEW ROUTINES 7/8/89	     */
/*           imcopyinfo		- REPLACED BY NEW ROUTINES 7/8/89	     */
/*           iminfoids		- REPLACED BY NEW ROUTINES 7/8/89	     */
/*           imerror                                                         */
/*           im_snap                                                         */
/*                                                                           */
/* Author:   John Gauch - Version 2                                          */
/*           Zimmerman, Entenman, Fitzpatrick, Whang - Version 1             */
/*                                                                           */
/* Date:     February 23, 1987                                               */
/*                                                                           */
/* Revised:  October 29, 1987 - Fixed one problem with the way histograms    */
/*           are calculated in imgetdesc.  Changes marked with JG1.          */
/*                                                                           */
/* Revised:  June 2, 1989 - Added imtest.  A. G. Gash.			     */
/*	     Sep 20, 1989 - Added im_snap.  A. G. Gash.			     */
/*	     Feb 12, 1991 - Generalized im_snap to work with any type.	     */
/*		 A. G. Gash.						     */
/*           Apr 19, 1991 - Replaced the sizes of the different datatypes    */
/*                          by sizeof calls.  Andre S.E. Koster,	     */
/*                          3D Computer Vision, The Netherlands,	     */
/*                                                                           */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define	TWOPOWER16	65536
#define	TWOPOWER15	32768

#ifndef ABS
#define	ABS(a)	(((a)<0)?(-(a)):(a))
#endif
#ifndef MIN
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif

typedef	unsigned char	UNCBYTE;

#include "image.h"

char _imerrbuf[nERROR];

/* modifications by AHP (15 Jan 2003) to make code machine-independent */
#define	sTITLE		0
#define	sVALIDMAXMIN	1
#define	sMAXMIN		2
#define	sVALIDHISTOGRAM	3
#define	sHISTOGRAM	4
#define	sPIXELFORMAT	5
#define	sDIMC		6
#define	sDIMV		7
/* 256 * 256 = 65536 and 256 * 256 * 256 = 16777216 - converts 4 bytes into an integer using little/big endian? */
/* aa is an array of 4 bytes to form the integer */
#define	CHAR_TO_INT(aa)	( ((aa)[3]) + ((aa)[2]) * 256 + ((aa)[1]) * 65536 + ((aa)[0]) * 16777216 )
/* each number in the array of theInts is broken into its 4 bytes and put into theChars array of 4*numInts bytes */

void	INT_TO_CHAR(int *theInts, int numInts, char *theChars){
	int	dummy, dummy2, i, j;

	for(i=0,j=0;i<numInts;i++,j+=4){
		dummy2 = theInts[i];
		theChars[j]   = dummy = dummy2 / 16777216; dummy2 -= dummy * 16777216;
		theChars[j+1] = dummy = dummy2 / 65536; dummy2 -= dummy * 65536;
		theChars[j+2] = dummy = dummy2 / 256;
		theChars[j+3] = dummy2 - dummy * 256;
	}
}	

#define	SIZEOF_INTEGER	4

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  These declarations are private to this library.                 */
/*           These MACROs copy an error message into the error string.       */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/* Constants for lseek calls */
#define FROMBEG		0
#define FROMHERE	1
#define FROMEND		2

/* Address index constants */
#define aMAXMIN		0
#define aHISTO		1
#define aTITLE		2
#define aPIXFORM	3
#define aDIMC		4
#define aDIMV		5
#define aPIXELS		6
#define aINFO		7
#define aVERNO		8

/* Modes for GetPut routines */
#define READMODE	0
#define WRITEMODE	1

/* Blocking factor for imgetdesc */
#define MAXGET 4096

#define Error(Mesg)\
   {\
   strcpy(_imerrbuf, Mesg);\
   fprintf(stderr, Mesg);\
   return(INVALID);\
   }

#define ErrorNull(Mesg)\
   {\
   strcpy(_imerrbuf, Mesg);\
   return(NULL);\
   }

#define Warn(Mesg)\
   {\
   strcpy(_imerrbuf, Mesg);\
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine creates an image.  The user specified image        */
/*           parameters are stored in the new image record.  For historical  */
/*           reasons, 4092 bytes are unused between the end of the           */
/*           histogram and the start of the pixel format field - gauchj.     */
/*                                                                           */
/*---------------------------------------------------------------------------*/
#define UNUSED 4092
IMAGE *imcreat(Name, Protection, PixForm, Dimc, Dimv)
   char *Name;
   int Protection;
   int PixForm;
   int Dimc;
   int *Dimv;
   {
   IMAGE *Image;
   int Cnt;
   int Fd;
   int i;
   char Null;

   Null = '\0';
   
   /* Check parameters */
   if (Name == NULL) ErrorNull("Null image name");
   if ((PixForm != GREY) && (PixForm != COLOR) && (PixForm != COLORPACKED) && 
       (PixForm != BYTE) && (PixForm != SHORT) && (PixForm != LONG) && 
       (PixForm != REAL) && (PixForm != COMPLEX) && (PixForm != USERPACKED)) 
      ErrorNull("Invalid pixel format");
   if ((Dimc < 1) || (Dimc > nDIMV)) ErrorNull("Illegal number of dimensions");

   /* Create image file */
   Fd = open(Name,CREATE,Protection);
   if (Fd == EOF) ErrorNull("Image already exists");

   /* Allocate image record */
   Image = (IMAGE *)malloc((unsigned)sizeof(IMAGE));
   if (Image == NULL) ErrorNull("Allocation error");

   /* Initialize image record */   
   Image->Title[0] = Null;
   Image->ValidMaxMin = FALSE;
   Image->MaxMin[0] = MAXVAL;
   Image->MaxMin[1] = MINVAL;
   Image->ValidHistogram = FALSE;
   for (i=0; i<nHISTOGRAM; i++)
      Image->Histogram[i] = 0;
   Image->PixelFormat = PixForm;
   Image->Dimc = Dimc;

   /* Determine number of pixels in image */
   Image->PixelCnt = 1;
   for (i=0; i<Image->Dimc; i++) {
      Image->Dimv[i] = Dimv[i];
      Image->PixelCnt = Image->PixelCnt * Dimv[i];
   }

   /* Determine size of each pixel */
   switch (Image->PixelFormat) {
      case GREY         : Image->PixelSize = sizeof (GREYTYPE); break;
      case COLOR        : Image->PixelSize = sizeof (COLORTYPE); break;
      case COLORPACKED  : Image->PixelSize = sizeof (CPACKEDTYPE); break;
      case BYTE         : Image->PixelSize = sizeof (BYTETYPE); break;
      case SHORT        : Image->PixelSize = sizeof (SHORTTYPE); break;
      case LONG         : Image->PixelSize = sizeof (LONGTYPE); break;
      case REAL         : Image->PixelSize = sizeof (REALTYPE); break;
      case COMPLEX      : Image->PixelSize = sizeof (COMPLEXTYPE); break;
      case USERPACKED   : Image->PixelSize = sizeof (USERTYPE); break;
   }

   /* Initialize image addresses (do NOT change this) */
   Image->Address[aTITLE] = sizeof( Image->Address );
   Image->Address[aMAXMIN] = Image->Address[aTITLE] 
      + sizeof( Image->Title );
   Image->Address[aHISTO] = Image->Address[aMAXMIN] 
      + sizeof( Image->ValidMaxMin ) + sizeof( Image->MaxMin );
   Image->Address[aPIXFORM] = Image->Address[aHISTO] + UNUSED
      + sizeof( Image->ValidHistogram ) + sizeof( Image->Histogram );
   Image->Address[aDIMC] = Image->Address[aPIXFORM] 
      + sizeof( Image->PixelFormat );
   Image->Address[aDIMV] = Image->Address[aDIMC] 
      + sizeof( Image->Dimc );
   Image->Address[aPIXELS] = Image->Address[aDIMV] 
      + sizeof( Image->Dimv );
   Image->Address[aINFO] = Image->Address[aPIXELS] 
      + Image->PixelCnt * Image->PixelSize;
   Image->Address[aVERNO] = 1;

   /* Write null information field */
   Cnt = (int)lseek(Fd, (long)Image->Address[aINFO], FROMBEG);
   Cnt = write(Fd, (char *)&Null, sizeof(Null));
   if (Cnt != sizeof(Null)) ErrorNull("Image write failed");
   /* Set up info pointers to blank fields */
   create_info(Image);

   /* Save file pointer */
   Image->Fd = Fd;
   return(Image);
   }
/*
  This function creates an image but only in memory, not to the filesystem,
  no filename is required,
  Author : Andreas Hadjiprocopis, ION 2003, CING 2005
*/
IMAGE *imcreat_memory(PixForm, Dimc, Dimv)
   int PixForm;
   int Dimc;
   int *Dimv;
   {
   IMAGE *Image;
   int i;
   char Null;

   Null = '\0';
   
   /* Check parameters */
   if ((PixForm != GREY) && (PixForm != COLOR) && (PixForm != COLORPACKED) && 
       (PixForm != BYTE) && (PixForm != SHORT) && (PixForm != LONG) && 
       (PixForm != REAL) && (PixForm != COMPLEX) && (PixForm != USERPACKED)) 
      ErrorNull("Invalid pixel format");
   if ((Dimc < 1) || (Dimc > nDIMV)) ErrorNull("Illegal number of dimensions");

   /* Allocate image record */
   Image = (IMAGE *)malloc((unsigned)sizeof(IMAGE));
   if (Image == NULL) ErrorNull("Allocation error");

   /* Initialize image record */   
   Image->Title[0] = Null;
   Image->ValidMaxMin = FALSE;
   Image->MaxMin[0] = MAXVAL;
   Image->MaxMin[1] = MINVAL;
   Image->ValidHistogram = FALSE;
   for (i=0; i<nHISTOGRAM; i++)
      Image->Histogram[i] = 0;
   Image->PixelFormat = PixForm;
   Image->Dimc = Dimc;

   /* Determine number of pixels in image */
   Image->PixelCnt = 1;
   for (i=0; i<Image->Dimc; i++) {
      Image->Dimv[i] = Dimv[i];
      Image->PixelCnt = Image->PixelCnt * Dimv[i];
   }

   /* Determine size of each pixel */
   switch (Image->PixelFormat) {
      case GREY         : Image->PixelSize = sizeof (GREYTYPE); break;
      case COLOR        : Image->PixelSize = sizeof (COLORTYPE); break;
      case COLORPACKED  : Image->PixelSize = sizeof (CPACKEDTYPE); break;
      case BYTE         : Image->PixelSize = sizeof (BYTETYPE); break;
      case SHORT        : Image->PixelSize = sizeof (SHORTTYPE); break;
      case LONG         : Image->PixelSize = sizeof (LONGTYPE); break;
      case REAL         : Image->PixelSize = sizeof (REALTYPE); break;
      case COMPLEX      : Image->PixelSize = sizeof (COMPLEXTYPE); break;
      case USERPACKED   : Image->PixelSize = sizeof (USERTYPE); break;
   }

   /* Initialize image addresses (do NOT change this) */
   Image->Address[aTITLE] = sizeof( Image->Address );
   Image->Address[aMAXMIN] = Image->Address[aTITLE] 
      + sizeof( Image->Title );
   Image->Address[aHISTO] = Image->Address[aMAXMIN] 
      + sizeof( Image->ValidMaxMin ) + sizeof( Image->MaxMin );
   Image->Address[aPIXFORM] = Image->Address[aHISTO] + UNUSED
      + sizeof( Image->ValidHistogram ) + sizeof( Image->Histogram );
   Image->Address[aDIMC] = Image->Address[aPIXFORM] 
      + sizeof( Image->PixelFormat );
   Image->Address[aDIMV] = Image->Address[aDIMC] 
      + sizeof( Image->Dimc );
   Image->Address[aPIXELS] = Image->Address[aDIMV] 
      + sizeof( Image->Dimv );
   Image->Address[aINFO] = Image->Address[aPIXELS] 
      + Image->PixelCnt * Image->PixelSize;
   Image->Address[aVERNO] = 1;

   /* Set up info pointers to blank fields */
   create_info(Image);

   /* Save file pointer */
   Image->Fd = -1;
   return(Image);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine opens an image.  The image parameters are          */
/*           read from the file and stored in the image record.              */
/*                                                                           */
/*---------------------------------------------------------------------------*/


IMAGE *imopen(ImName, Mode)
   char *ImName;
   int Mode;
   {
   IMAGE *Image;
   int Cnt;
   int Fd;
   int i;
   int Length;
   char *Buffer;
   int  *odfCnt;

	/* modifications AHP : code should not be machine dependent !!! */
	/* because of some `coders' the routines for reading and writing
	   UNC files in this library is specific to sun/solaris. This is so because
	   instead of saying, a histogram information is so many bytes in the header
	   of the UNC file, instead they say it is of sizeof(...) bytes - that sizeof
	   bit is very machine specific, e.g. sizeof(int) in dos and sizeof(int)
	   in SUN are two different things. so here i am hardcoding sizes in bytes
	   for each part of the header and hope for the best. */
	int	j;
	unsigned char	myBuffer[16000], *aP;
	size_t	mySizes[nADDRESS];
	mySizes[sTITLE] = 		81;
	mySizes[sVALIDMAXMIN] = 	4;
	mySizes[sMAXMIN] = 		8;
	mySizes[sVALIDHISTOGRAM] =	4;
	mySizes[sHISTOGRAM] = 		4096;
	mySizes[sPIXELFORMAT] = 	4;
	mySizes[sDIMC] = 		4;
	mySizes[sDIMV] = 		40;

   /* Check parameters */
   if (ImName == NULL) ErrorNull("Null image name");
   if ((Mode != READ) && (Mode != UPDATE)) ErrorNull("Invalid open mode");

   /* Open image file */
   Fd = open(ImName,Mode);
   if (Fd == EOF) ErrorNull("Image file not found");

   /* Allocate image record */
   Image = (IMAGE *)malloc((unsigned)sizeof(IMAGE));
   if (Image == NULL) ErrorNull("Allocation error");


   /* AHP : most UNC files have been created with a system whose integer size was 4 bytes.
      That was left to the system, from now on, we will use 4 bytes for each integer.
      Even if future systems have integers of 8 bytes, say, we will write all integers as
      4 bytes into the UNC files and read them likewise. Now that can be shortsighted but
      a) not my mistake, b) ensures compatibility with previous UNC and c) unlikely that
      integer values in UNC files will exceed the 4-byte range (+/- 2 trillion) */
      
	/* Read addresses of image header fields */
	/* first 36 bytes, 4 bytes for each address, 9 addresses */
	if( read(Fd, myBuffer, 36) != 36 ) ErrorNull("imopen : Image read failed : Address");
	for(i=0,aP=&(myBuffer[0]);i<nADDRESS;i++,aP+=4) Image->Address[i] = CHAR_TO_INT(aP);

	/* Read Title field (an array of bytes) */
	lseek(Fd, (long)Image->Address[aTITLE], FROMBEG);
	if( read(Fd, (char *)(&(Image->Title[0])), mySizes[sTITLE]) != mySizes[sTITLE] ) ErrorNull("imopen : Image read failed : Title");

	/* size of int in SUN/solaris was 4 bytes */
	/* Read MaxMin field */
	lseek(Fd, (long)Image->Address[aMAXMIN], FROMBEG);
	if( read(Fd, myBuffer, mySizes[sVALIDMAXMIN]) != mySizes[sVALIDMAXMIN] ) ErrorNull("imopen : Image read failed : ValidMaxMin");
	Image->ValidMaxMin = CHAR_TO_INT(myBuffer);

	if( read(Fd, myBuffer, mySizes[sMAXMIN]) != mySizes[sMAXMIN] ) ErrorNull("imopen : Image read failed : MaxMin");
	Image->MaxMin[0] = CHAR_TO_INT(&(myBuffer[0]));
	Image->MaxMin[1] = CHAR_TO_INT(&(myBuffer[4]));

	/* Read Histogram field */
	lseek(Fd, (long)Image->Address[aHISTO], FROMBEG);
	if( read(Fd, myBuffer, mySizes[sVALIDHISTOGRAM]) != mySizes[sVALIDHISTOGRAM] ) ErrorNull("imopen : Image read failed : ValidHistogram");
	Image->ValidHistogram = CHAR_TO_INT(myBuffer);

	if( read(Fd, myBuffer, mySizes[sHISTOGRAM]) != mySizes[sHISTOGRAM] ) ErrorNull("imopen : Image read failed : Histogram");
	for(i=0,j=0;i<nHISTOGRAM;i++,j+=4) Image->Histogram[i] = CHAR_TO_INT(&(myBuffer[j]));

	/* Read PixelFormat field */
	lseek(Fd, (long)Image->Address[aPIXFORM], FROMBEG);
	if( read(Fd, myBuffer, mySizes[sPIXELFORMAT]) != mySizes[sPIXELFORMAT] ) ErrorNull("imopen : Image read failed : PixelFormat");
	Image->PixelFormat = CHAR_TO_INT(myBuffer);

	/* Determine size of each pixel */
	switch (Image->PixelFormat) {
		/* remember: avoid machine dependencies */
/*		case GREY         : Image->PixelSize = sizeof (GREYTYPE); break;
		case COLOR        : Image->PixelSize = sizeof (COLORTYPE); break;
		case COLORPACKED  : Image->PixelSize = sizeof (CPACKEDTYPE); break;
		case BYTE         : Image->PixelSize = sizeof (BYTETYPE); break;
		case SHORT        : Image->PixelSize = sizeof (SHORTTYPE); break;
		case LONG         : Image->PixelSize = sizeof (LONGTYPE); break;
		case REAL         : Image->PixelSize = sizeof (REALTYPE); break;
		case COMPLEX      : Image->PixelSize = sizeof (COMPLEXTYPE); break;
		case USERPACKED   : Image->PixelSize = sizeof (USERTYPE); break;
*/
		/* so use the numbers from a sun machine to all other machines */
		case GREY         : Image->PixelSize = 2; break;
		case COLOR        : Image->PixelSize = 2; break;
		case COLORPACKED  : Image->PixelSize = 4; break;
		case BYTE         : Image->PixelSize = 1; break;
		case SHORT        : Image->PixelSize = 2; break;
		case LONG         : Image->PixelSize = 4; break;
		case REAL         : Image->PixelSize = 4; break;
		case COMPLEX      : Image->PixelSize = 8; break;
		case USERPACKED   : Image->PixelSize = 4; break;
	}

	/* Read DimC field */
	lseek(Fd, (long)Image->Address[aDIMC], FROMBEG);
	if( read(Fd, myBuffer, mySizes[sDIMC]) != mySizes[sDIMC] ) ErrorNull("imopen : Image read failed : Dimc");
	Image->Dimc = CHAR_TO_INT(myBuffer);

	/* Read DimV field */
	lseek(Fd, (long)Image->Address[aDIMV], FROMBEG);
	if( read(Fd, myBuffer, mySizes[sDIMV]) != mySizes[sDIMV] ) ErrorNull("imopen : Image read failed : Dimv");
	for(i=0,j=0;i<nDIMV;i++,j+=4) Image->Dimv[i] = CHAR_TO_INT(&(myBuffer[j]));

	/* Determine number of pixels in image */
	Image->PixelCnt = 1;
	for(i=0;i<Image->Dimc;i++) Image->PixelCnt = Image->PixelCnt * Image->Dimv[i];

	/* Determine length of information field - e.g. find the size of the whole file */
	if( (Cnt=((int )lseek(Fd, (long)0, FROMEND))) == -1 ) ErrorNull("imopen : Seek EOF failed");
	if( (Length=(Cnt - Image->Address[aINFO])) < 1 ) ErrorNull("imopen : Invalid information field");
	if( (Buffer=(char *)malloc((unsigned)Length)) == NULL ) ErrorNull("imopen : Allocation error");

	/* Read whole information field into a buffer */
	lseek(Fd, (long)Image->Address[aINFO], FROMBEG);
	if( (Cnt=read(Fd, (char *)Buffer, Length)) != Length ) ErrorNull("imopen : Image read failed : Information Field");

   /* Check that the info block exists in the file.  If it does then
          read all info fields into image data structure. Otherwise create
          a new blank info data stucture.
          Also:
          Check for old ODF format by looking for the odfCnt which should
          be set to 1 as most if not all files only had the "pickerinfo"
          ODF. If found then read the info into the new structure.
   */
        if (Length > 1){
                odfCnt=(int *)Buffer;
                if (*odfCnt==1){
                        fprintf(stderr,"Reading and Converting old ODF format.\n");
                        create_info(Image);
                        read_odf(Buffer,Image);
                }
                else read_info(Buffer,Image);
        }
        else
                create_info(Image);
   /* Free buffer used for information string */
   free(Buffer);

   /* Save file pointer */
   Image->Fd = Fd;
   return(Image);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine closes an image.  The image parameters are         */
/*           written to the file from the image record.                      */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	imclose(Image)
   IMAGE *Image;
   {
   int Fd;
   int Cnt;
   int Length;
   char *Buffer;

	/* modifications AHP : code should not be machine dependent !!! */
	/* because of some `coders' the routines for reading and writing
	   UNC files in this library is specific to sun/solaris. This is so because
	   instead of saying, a histogram information is so many bytes in the header
	   of the UNC file, instead they say it is of sizeof(...) bytes - that sizeof
	   bit is very machine specific, e.g. sizeof(int) in dos and sizeof(int)
	   in SUN are two different things. so here i am hardcoding sizes in bytes
	   for each part of the header and hope for the best. */
	char	myBuffer[16000];
	size_t	mySizes[nADDRESS];
	mySizes[sTITLE] = 		81;
	mySizes[sVALIDMAXMIN] = 	4;
	mySizes[sMAXMIN] = 		8;
	mySizes[sVALIDHISTOGRAM] =	4;
	mySizes[sHISTOGRAM] = 		4096;
	mySizes[sPIXELFORMAT] = 	4;
	mySizes[sDIMC] = 		4;
	mySizes[sDIMV] = 		40;

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");

   /* Check that file is open */
   Fd = Image->Fd;
   if (Fd == EOF) Error("Image not open");

	/* write without using machine-dependent code */
	/* Write addresses of image header fields */
	lseek(Fd, (long)0, FROMBEG); /* go to the beginning */
	INT_TO_CHAR(Image->Address, nADDRESS, myBuffer);
	if( write(Fd, myBuffer, 36) != 36 ) Warn("imclose : Image write failed : Address");

	/* Write Title field - it is chars so no problem */
	lseek(Fd, (long)Image->Address[aTITLE], FROMBEG);
	if( write(Fd, Image->Title, mySizes[sTITLE]) != mySizes[sTITLE] ) Warn("imclose : Image write failed : Title");

	/* Write MaxMin field */
	lseek(Fd, (long)Image->Address[aMAXMIN], FROMBEG);
	INT_TO_CHAR(&(Image->ValidMaxMin), 1, myBuffer);
	if( write(Fd, myBuffer, mySizes[sVALIDMAXMIN]) != mySizes[sVALIDMAXMIN] ) Warn("imclose : Image write failed : ValidMaxMin");
	INT_TO_CHAR(Image->MaxMin, nMAXMIN, myBuffer);
	if( write(Fd, myBuffer, mySizes[sMAXMIN]) != mySizes[sMAXMIN] ) Warn("imclose : Image write failed : MaxMin");

	/* Write Histogram field */
	lseek(Fd, (long)Image->Address[aHISTO], FROMBEG);
	INT_TO_CHAR(&(Image->ValidHistogram), 1, myBuffer);
	if( write(Fd, myBuffer, mySizes[sVALIDHISTOGRAM]) != mySizes[sVALIDHISTOGRAM] ) Warn("imclose : Image write failed : ValidHistogram");
	INT_TO_CHAR(Image->Histogram, nHISTOGRAM, myBuffer);
	if( write(Fd, myBuffer, mySizes[sHISTOGRAM]) != mySizes[sHISTOGRAM] ) Warn("imclose : Image write failed : Histogram");

	/* Write PixelFormat field */
	lseek(Fd, (long)Image->Address[aPIXFORM], FROMBEG);
	INT_TO_CHAR(&(Image->PixelFormat), 1, myBuffer);
	if( write(Fd, myBuffer, mySizes[sPIXELFORMAT]) != mySizes[sPIXELFORMAT] ) Warn("imclose : Image write failed : PixelFormat");

	/* Write DimC field */
	lseek(Fd, (long)Image->Address[aDIMC], FROMBEG);
	INT_TO_CHAR(&(Image->Dimc), 1, myBuffer);
	if( write(Fd, myBuffer, mySizes[sDIMC]) != mySizes[sDIMC] ) Warn("imclose : Image write failed : Dimc");

	/* Write DimV field */
	lseek(Fd, (long)Image->Address[aDIMV], FROMBEG);
	INT_TO_CHAR(Image->Dimv, nDIMV, myBuffer);
	if( write(Fd, myBuffer, mySizes[sDIMV]) != mySizes[sDIMV] ) Warn("imclose : Image write failed : Dimc");

	/* Write Info field */
	lseek(Fd, (long)Image->Address[aINFO], FROMBEG);
   
	write_info(Image, &Buffer, &Length);
	if( (Cnt=write(Fd, Buffer, Length)) != Length) Warn("imclose : Image write failed : Information Field");

	/* EOF */
	myBuffer[0] = '\0'; write(Fd, myBuffer, 1);
	/* END AHP */
   
   /* Close file and free image record */
   free((char *)Image);
   close(Fd);
   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine reads pixel data from an image.                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	imread(Image, LoIndex, HiIndex, Buffer)
   IMAGE *Image;
   int LoIndex;
   int HiIndex;
   GREYTYPE *Buffer;
   {
   int Cnt;
   int Length;
   int Offset;   
   /* mods AHP for removing system dependencies */
	register int		i, dummy;
	UNCBYTE			*scratchPad;
	register GREYTYPE	*aP1;	/* it will be a pointer to Buffer (GREYTYPE) */
	register UNCBYTE	*aP2;	/* it will be a pointer to scratchPad */

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");
   if (Buffer == NULL) Error("Null read buffer");
   if ((LoIndex < 0) || (HiIndex > Image->PixelCnt) ||
      (LoIndex > HiIndex)) Error("Invalid pixel index");

   /* Check that file is open */
   if (Image->Fd == EOF) Error("Image not open");

   /* Determine number of bytes to read and the lseek offset */
   Length = (HiIndex - LoIndex +1) * Image->PixelSize;
   Offset = Image->Address[aPIXELS] + LoIndex * Image->PixelSize;

   /* Read pixels into buffer */
   Cnt = (int)lseek(Image->Fd, (long)Offset, FROMBEG);

   /* mods AHP for removing system dependencies */
   /* again - some systems read little-endian and some other way round
      so this is wrong. read into a char array and then convert into longs */

/* WRONG:   Cnt = read(Image->Fd, (char *)Buffer, Length); */
	if( (scratchPad=(UNCBYTE *)malloc(Length*sizeof(UNCBYTE))) == NULL ){
		fprintf(stderr, "imread : could not allocate %ld bytes for scratchPad.\n", Length*sizeof(UNCBYTE));
		return INVALID;
	}
	if( read(Image->Fd, (UNCBYTE *)scratchPad, Length) != Length ) Error("Image pixel read failed");
	switch( Image->PixelSize ){
		case	2:
			Cnt = HiIndex-LoIndex+1;
			for(i=0,aP1=&(Buffer[0]),aP2=&(scratchPad[0]);i<Cnt;i++,aP1++,aP2+=2){
				/* we use 2s complement, big-endian format to write to disk,
				   if we want to read it, we have to follow the opposite procedure:
				   dummy = MSB * 256 + LSB
				   if( dummy >= 2^15 )
				   	we have negative number = - (2^16-dummy)
				   else
				   	we have positive number = dummy
				   endif
				*/
//printf("imread : start with %d and %d\n", *aP2, *(aP2+1));
				/* Gareth J. Barker addition: */
/*				dummy = (*aP2)*256 + (*(aP2+1));*/
/*				*aP1 = ((dummy && 0x00FF) << 8) || ((dummy && 0xFF00) >> 8);*/
				if( (dummy=((*aP2)*256 + (*(aP2+1)))) >= TWOPOWER15 ){
					*aP1 = dummy - TWOPOWER16;
				} else {
					*aP1 = dummy;
				}
//printf("dummy is %d and number is %d\n", dummy, *aP1);
			}
			break;
		default:
			fprintf(stderr, "imread : do not know how to read images with a pixel size other than 2 (it was %d).\n", Image->PixelSize);
			free(scratchPad);
			return INVALID;
	}
	free(scratchPad);

   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine writes pixel data to an image.  For GREY images,   */
/*           the maximum and minimum values are also updated, but the        */
/*           histogram is NOT updated.  The algorithm used to update the     */
/*           maximum and minimum values generates a bound on the image       */
/*           intensities, but not always the tightest bound.  This is the    */
/*           best we can do by looking at pixels as they are written.  To    */
/*           compute the tightest upper bound requires that we look at all   */
/*           pixels in the image.  This is what imgetdesc does.              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/* AHP: it writes from LoIndex to HiIndex INCLUSIVE */
int	imwrite(Image, LoIndex, HiIndex, Buffer)
   IMAGE *Image;
   int LoIndex;
   int HiIndex;
   GREYTYPE *Buffer;
   {
   int Length;
   int Offset;   
   int TempMin;
   int TempMax;
   int PixelCnt;
   int i;
	/* mods AHP */
	UNCBYTE			*scratchPad;
	register int		j, k, twosComplement;
	register GREYTYPE	*aP1;	/* it will be a pointer to Buffer (GREYTYPE) */
	register UNCBYTE	*aP2;	/* it will be a pointer to scratchPad */

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");
   if (Buffer == NULL) Error("Null read buffer");
   if ((LoIndex < 0) || (HiIndex > Image->PixelCnt) ||
      (LoIndex > HiIndex)) Error("Invalid pixel index");

   /* Check that file is open */
   if (Image->Fd == EOF) Error("Image not open");

   /* Determine number of bytes to write and the lseek offset */
   Length = (HiIndex - LoIndex +1) * Image->PixelSize;
   Offset = Image->Address[aPIXELS] + LoIndex * Image->PixelSize;

   /* Write pixels into buffer */
   lseek(Image->Fd, (long)Offset, FROMBEG);
   /* mods AHP */
   /* WRONG, Buffer is an array of GREYTYPE by typecasting it to (char *)
      we force the system to make its own interpretations - thus adding
      system dependencies. Different systems might use different conventions
      (e.g. little and big - endian) and we will have a problem */
	if( (scratchPad=(UNCBYTE *)malloc(Length*sizeof(UNCBYTE))) == NULL ){
		fprintf(stderr, "imwrite : could not allocate %ld bytes for scratchPad.\n", Length*sizeof(UNCBYTE));
		return INVALID;
	}

	switch( Image->PixelSize ){
		case	2:
			k = HiIndex-LoIndex+1;
			for(j=0,i=0,aP1=&(Buffer[0]),aP2=&(scratchPad[0]);i<k;i++,aP1++,j+=2){
				/* we use 2s complement, big-endian format */
				/* 1) convert integer to 2s complement using
					2s com = (2^16-1) - ABS(integer) + 1 = 2^16 - ABS(integer)
				   2) big-endian conversion using:
				   	MSB = (2s com) / 256 e.g. most significant bit
				   	LSB = (2s com) % 256 e.g. least significant bit
				   	MSB goes first
				*/
//printf("write: starting with %d\n", *aP1);
				twosComplement = (TWOPOWER16 + *aP1) % TWOPOWER16; /* in this way we handle both positive and negative without if-statements */
//printf("then %d\n", twosComplement);
				*aP2 = (UNCBYTE )(twosComplement / 256);
//printf("and %d\n", *aP2);
				aP2++;
				*aP2 = (UNCBYTE )(twosComplement % 256);
//printf("and %d\n", *aP2);
				aP2++;
			}
			break;
		default:
			fprintf(stderr, "imwrite : do not know how to write images with a pixel size other than 2 (it was %d).\n", Image->PixelSize);
			free(scratchPad);
			return INVALID;
	} /* switch */

   /* WRONG : Cnt = write(Image->Fd, (char *)Buffer, Length); */
	if( write(Image->Fd, (UNCBYTE *)scratchPad, Length) != Length ) Error("Image pixel write failed");
	free(scratchPad);
   /* END of AHP modifications */

   /* Invalidate the MaxMin and Histogram fields */
   Image->ValidMaxMin = FALSE;
   Image->ValidHistogram = FALSE;

   /* Compute new Maximum and Minimum for GREY images */
   if (Image->PixelFormat == GREY)
      {
      TempMax = Image->MaxMin[1];
      TempMin = Image->MaxMin[0];
      PixelCnt = HiIndex - LoIndex + 1;
      for (i=0; i<PixelCnt; i++)
         {
         if (Buffer[i] > TempMax) TempMax = Buffer[i];
         if (Buffer[i] < TempMin) TempMin = Buffer[i];
         }
      Image->MaxMin[1] = TempMax;
      Image->MaxMin[0] = TempMin;
      }

   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine reads an arbitrary subwindow of an image.  It      */
/*           does optimal I/O for 1D, 2D and 3D images.  More general I/O    */
/*           is used for higher dimension images.                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	imgetpix(Image, Endpts, Coarseness, Pixels)
   IMAGE *Image;
   int Endpts[][2];
   int Coarseness[];
   GREYTYPE *Pixels;
   {
   int i;

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");
   if (Pixels == NULL) Error("Null pixel buffer");
   
   /* Check that file is open */
   if (Image->Fd == EOF) Error("Image not open");

   /* Check endpoints */
   for (i=0; i<Image->Dimc; i++)
      {
      if (Coarseness[i] != 1) Error("Coarseness not implemented");
      if (Endpts[i][0] < 0) Error("Bad endpoints range");
      if (Endpts[i][1] >= Image->Dimv[i]) Error("Bad endpoints range");
      if (Endpts[i][1] < Endpts[i][0]) Error("Bad endpoints order");
      }

   /* Check for image dimensions */
   switch (Image->Dimc) {

      /* Handle 1D images */
		case 1: 
         return(imread(Image, Endpts[0][0], Endpts[0][1], Pixels));
         break;

      /* Handle 2D images */
		case 2:
         return(GetPut2D(Image, Endpts, Pixels, READMODE));
         break;

      /* Handle 3D images */
		case 3:
         return(GetPut3D(Image, Endpts, Pixels, READMODE));
         break;

      /* Handle higher dimension images */
      default:
         return(GetPutND(Image, Endpts, Pixels, READMODE));
         break;
      }

   /* Removed to keep lint/cc happy - Never reached */
   /*return(VALID); */
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine writes an arbitrary subwindow of an image.  It     */
/*           does optimal I/O for 1D, 2D and 3D images.  More general I/O    */
/*           is used for higher dimension images.                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	imputpix(Image, Endpts, Coarseness, Pixels)
   IMAGE *Image;
   int Endpts[][2];
   int Coarseness[];
   GREYTYPE *Pixels;
   {
   int i;
   int PixelCnt;
   int TempMax;
   int TempMin;

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");
   if (Pixels == NULL) Error("Null pixel buffer");
   
   /* Check that file is open */
   if (Image->Fd == EOF) Error("Image not open");

   /* Check endpoints */
   PixelCnt = 1;
   for (i=0; i<Image->Dimc; i++)
      {
      if (Coarseness[i] != 1) Error("Coarseness not implemented");
      if (Endpts[i][0] < 0) Error("Bad endpoints range");
      if (Endpts[i][1] >= Image->Dimv[i]) Error("Bad endpoints range");
      if (Endpts[i][1] < Endpts[i][0]) Error("Bad endpoints order");
      PixelCnt = PixelCnt * (Endpts[i][1] - Endpts[i][0] + 1);
      }

   /* Invalidate the MaxMin and Histogram fields */
   Image->ValidMaxMin = FALSE;
   Image->ValidHistogram = FALSE;

   /* Compute new Maximum and Minimum for GREY images */
   if (Image->PixelFormat == GREY)
      {
      TempMax = Image->MaxMin[1];
      TempMin = Image->MaxMin[0];
      for (i=0; i<PixelCnt; i++)
         {
         if (Pixels[i] > TempMax) TempMax = Pixels[i];
         if (Pixels[i] < TempMin) TempMin = Pixels[i];
         }
      Image->MaxMin[1] = TempMax;
      Image->MaxMin[0] = TempMin;
      }

   /* Check for image dimensions */
   switch (Image->Dimc) {

      /* Handle 1D images */
		case 1: 
         return(imwrite(Image, Endpts[0][0], Endpts[0][1], Pixels));
         break;

      /* Handle 2D images */
		case 2:
         return(GetPut2D(Image, Endpts, Pixels, WRITEMODE));
         break;

      /* Handle 3D images */
		case 3:
         return(GetPut3D(Image, Endpts, Pixels, WRITEMODE));
         break;

      /* Handle higher dimension images */
      default:
         return(GetPutND(Image, Endpts, Pixels, WRITEMODE));
         break;
      }

   /* Removed to keep lint/cc happy - Never reached */
   /*return(VALID); */
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine reads and writes pixels to 2D images.  The         */
/*           minimum number of I/O calls are used.                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	GetPut2D(Image, Endpts, Pixels, Mode)
   IMAGE *Image;
   int Endpts[][2];
   GREYTYPE *Pixels;
   int Mode;
   {
   int Xdim;
   int Ydim;
   int Xlength;
   int Ylength;
   int ReadBytes;
   int Yloop;
   int Yskipcnt;
   int FirstPixel;
   int Cnt;
   int i;
   char *PixelPtr;

   /* The 0th dimension is y; the 1st is x */
   Ydim = 0;
   Xdim = 1;
	    
   /* Determine how many pixels to READ/WRITE in each dimension */
   Ylength = (Endpts[Ydim][1] - Endpts[Ydim][0]) + 1;
   Xlength = (Endpts[Xdim][1] - Endpts[Xdim][0]) + 1;
	    
   /* Determine how many pixels to SKIP in each dimension */
   Yskipcnt = (Image->Dimv[Xdim]-Xlength) * Image->PixelSize;

   /* Determine the maximum number of consecutive pixels that can */
   /* be read in at one time and the number of non-contiguous     */
   /* sections that span the y dimension                          */
   if (Xlength != Image->Dimv[Xdim])
      {
      ReadBytes = Xlength * Image->PixelSize;
      Yloop = Ylength;
      }
   else 
      {
      ReadBytes = Ylength * Xlength * Image->PixelSize;
      Yloop = 1;
      }
 
   /* Calculate position of first pixel */
   FirstPixel = ((Endpts[Ydim][0] * Image->Dimv[Xdim]) 
              + Endpts[Xdim][0]) * Image->PixelSize
              + Image->Address[aPIXELS];

   /* Seek to first pixel in image */
   Cnt = (int)lseek(Image->Fd, (long)FirstPixel, FROMBEG);
   if (Cnt == -1) Error("Seek first pixel failed");

   Warn("Perhaps you should not be using this routine in linux YET!");
   /* Loop reading/writing pixel data in sections */
   PixelPtr = (char *) Pixels;
   for (i=0; i<Yloop; i++)
      {
      /* Read/write data into buffer */
      if (Mode == READMODE)
         Cnt = read(Image->Fd, (char *)PixelPtr, ReadBytes);
      else
         Cnt = write(Image->Fd, (char *)PixelPtr, ReadBytes);
      if (Cnt != ReadBytes) Error("Pixel read/write failed");

      /* Advance buffer pointer */
      PixelPtr += ReadBytes;

      /* Seek to next line of pixels to read/write */
      Cnt = (int)lseek(Image->Fd, (long)Yskipcnt, FROMHERE);
      if (Cnt == -1) Error("Seek next pixel failed");
      }

   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine reads and writes pixels to 3D images.  The         */
/*           minimum number of I/O calls are used.                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	GetPut3D(Image, Endpts, Pixels, Mode)
   IMAGE *Image;
   int Endpts[][2];
   GREYTYPE *Pixels;
   int Mode;
   {
   int Xdim;
   int Ydim;
   int Zdim;
   int Xlength;
   int Ylength;
   int Zlength;
   int ReadBytes;
   int Yloop;
   int Zloop;
   int Yskipcnt;
   int Zskipcnt;
   int FirstPixel;
   int Cnt;
   int i;
   int j;
   char *PixelPtr;

   /* 0th dimension is z; 1st is y; 2nd is x */
   Zdim = 0;
   Ydim = 1;
   Xdim = 2;

   /* Determine how many pixels to READ/WRITE in each dimension*/
   Zlength = (Endpts[Zdim][1] - Endpts[Zdim][0]) + 1;
   Ylength = (Endpts[Ydim][1] - Endpts[Ydim][0]) + 1;
   Xlength = (Endpts[Xdim][1] - Endpts[Xdim][0]) + 1;

   /* Determine how many pixels to SKIP in each dimension*/
   Yskipcnt = (Image->Dimv[Xdim]-Xlength) * Image->PixelSize;
   Zskipcnt = (Image->Dimv[Ydim]-Ylength) 
            * Image->Dimv[Xdim] * Image->PixelSize;

   /* Determine the maximum number of consecutive pixels that can */
   /* be read in at one time and the number of non-contiguous     */
   /* sections that span the y and z dimensions                   */
   if (Xlength != Image->Dimv[Xdim])
      {
      ReadBytes = Xlength * Image->PixelSize;
      Yloop = Ylength;
      Zloop = Zlength;
      }
   else if (Ylength != Image->Dimv[Ydim]) 
      {
      ReadBytes = Ylength * Xlength * Image->PixelSize;
      Yloop = 1;
      Zloop = Zlength;
      }
   else 
      {
      ReadBytes = Zlength * Ylength * Xlength * Image->PixelSize;
      Yloop = 1;
      Zloop = 1;
      }
	    
   /* Calculate position of first pixel in image */
   FirstPixel = ((Endpts[Zdim][0] * Image->Dimv[Xdim] * Image->Dimv[Ydim])
              + (Endpts[Ydim][0] * Image->Dimv[Xdim]) 
              +  Endpts[Xdim][0]) * Image->PixelSize
              + Image->Address[aPIXELS];

   /* Seek to first pixel in image */
   Cnt = (int)lseek(Image->Fd, (long)FirstPixel, FROMBEG);
   if (Cnt == -1) Error("Seek first pixel failed");

   Warn("Perhaps you should not be using this routine in linux YET");
   /* Loop reading/writing pixel data in sections */
   PixelPtr = (char *) Pixels;
   for (i=0; i<Zloop; i++)
      {
      for (j=0; j<Yloop; j++)
         {
         /* Read/write data from file into buffer */
         if (Mode == READMODE)
            Cnt = read(Image->Fd, (char *)PixelPtr, ReadBytes);
         else
            Cnt = write(Image->Fd, (char *)PixelPtr, ReadBytes);
         if (Cnt != ReadBytes) Error("Pixel read/write failed");
   
         /* Advance buffer pointer */
         PixelPtr += ReadBytes;
   
         /* Seek to next line of pixels to read/write */
         Cnt = (int)lseek(Image->Fd, (long)Yskipcnt, FROMHERE);
         if (Cnt == -1) Error("Seek next pixel failed");
         }

      /* Seek to next slice of pixels to read/write */
      Cnt = (int)lseek(Image->Fd, (long)Zskipcnt, FROMHERE);
      if (Cnt == -1) Error("Seek next pixel failed");
      }

   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine reads and writes pixels to N dimensional images.   */
/*           The minimum number of I/O calls are used.                       */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	GetPutND(Image, Endpts, Pixels, Mode)
   IMAGE *Image;
   int Endpts[][2];
   GREYTYPE *Pixels;
   int Mode;
   {
   int SliceSize[nDIMV];
   int ReadCnt[nDIMV];
   int SkipCnt[nDIMV];
   int Index[nDIMV];
   int Dimc;
   int ReadBytes;
   int NextPixel;
   int i;
   int Cnt;
   char *PixelPtr;

   /* Determine size of one "slice" in each dimension */
   Dimc = Image->Dimc;
   SliceSize[Dimc-1] = Image->PixelSize;
   for (i=Dimc-2; i>=0; i--)
      SliceSize[i] = SliceSize[i+1] * Image->Dimv[i+1];

   /* Determine number of "slices" to read and skip in each dimension */
   for (i=0; i<Dimc; i++)
      {
      ReadCnt[i] = Endpts[i][1] - Endpts[i][0] + 1;
      SkipCnt[i] = Image->Dimv[i] - ReadCnt[i];
      }
   ReadBytes = ReadCnt[Dimc-1] * SliceSize[Dimc-1];
    
   /* Seek to beginning of pixels */
   Cnt = (int)lseek(Image->Fd, (long)Image->Address[aPIXELS], FROMBEG);

   /* Find offset to first pixel */
   NextPixel = 0;
   for (i=0; i<Dimc; i++)
      {
      NextPixel = NextPixel + SliceSize[i]*Endpts[i][0];
      Index[i] = Endpts[i][0];
      }

   Warn("Perhaps you should not be using this routine in linux YET");
   /* Loop reading and skipping pixels */
   PixelPtr = (char *) Pixels;
   while (Index[0] <= Endpts[0][1])
      {
      /* Seek to next line of pixels to read/write */
      Cnt = (int)lseek(Image->Fd, (long)NextPixel, FROMHERE);

      /* Read/write data from file into buffer */
      if (Mode == READMODE)
         Cnt = read(Image->Fd, (char *)PixelPtr, ReadBytes);
      else
         Cnt = write(Image->Fd, (char *)PixelPtr, ReadBytes);
      if (Cnt != ReadBytes) Error("Pixel read/write failed");
  
      /* Advance buffer pointer */
      PixelPtr += ReadBytes;
      Index[Dimc-1] = Endpts[Dimc-1][1] + 1;
 
      /* Find offset to next pixel */
      NextPixel = 0;
      for (i=Dimc-1; i>=0; i--)
         {
         if (Index[i] > Endpts[i][1])
            {
            NextPixel = NextPixel + SkipCnt[i] * SliceSize[i];
            if (i > 0) 
               {
               Index[i] = Endpts[i][0];
               Index[i-1]++; /* AHP: gcc 6.4.1 complains about array subscript below array bounds which is obviously wrong (see if(i>0) above) */
               }
            }
         }
      }

   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine reads all useful image information from the        */
/*           image image header.  This routine can be used in place of       */
/*           imdim, imbounds, and imgetdesc.                                 */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	imheader(Image, PixFormat, PixSize, PixCnt, Dimc, Dimv, MaxMin)
   IMAGE *Image;
   int *PixFormat;
   int *PixSize;
   int *PixCnt;
   int *Dimc;
   int *Dimv;
   int *MaxMin;
   {
   int i;

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");
   if (PixFormat == NULL) Error("Null pixformat pointer");
   if (PixSize == NULL) Error("Null pixsize pointer");
   if (PixCnt == NULL) Error("Null pixcnt pointer");
   if (Dimc == NULL) Error("Null dimc pointer");
   if (Dimv == NULL) Error("Null dimv pointer");
   if (MaxMin == NULL) Error("Null maxmin pointer");

   /* Check that file is open */
   if (Image->Fd == EOF) Error("Image not open");

   /* Copy data */
   *PixFormat = Image->PixelFormat;
   *PixSize = Image->PixelSize;
   *PixCnt = Image->PixelCnt;
   *Dimc = Image->Dimc;
   for (i=0; i<*Dimc; i++)
      Dimv[i] = Image->Dimv[i];

   /* Get correct MINMAX field */
   imgetdesc(Image, MINMAX, MaxMin);
   
   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine returns the pixel format and dimension count.      */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	imdim(Image, PixFormat, Dimc)
   IMAGE *Image;
   int *PixFormat;
   int *Dimc;
   {

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");
   if (PixFormat == NULL) Error("Null pixformat pointer");
   if (Dimc == NULL) Error("Null dimc pointer");

   /* Check that file is open */
   if (Image->Fd == EOF) Error("Image not open");

   /* Copy data */
   *PixFormat = Image->PixelFormat;
   *Dimc = Image->Dimc;

   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine returns the image dimension vector.                */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	imbounds(Image, Dimv)
   IMAGE *Image;
   int *Dimv;
   {
   int i;

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");
   if (Dimv == NULL) Error("Null dimv pointer");

   /* Check that file is open */
   if (Image->Fd == EOF) Error("Image not open");

   /* Copy data */
   for (i=0; i<Image->Dimc; i++)
      Dimv[i] = Image->Dimv[i];
   
   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine reads the MAXMIN or HISTO field from the image.    */
/*           The histogram field contains 4096 buckets which contain         */
/*           pixel counts for pixels in the range [MinPixel..MaxPixel].      */
/*           When there are less than 4096 pixel values this technique       */
/*           works perfectly.  When there are more than 4096 pixel values    */
/*           all pixels greater than MinPixel+4095 are included in the       */
/*           4096th bucket.  While this seems like a bit of a hack, it was   */
/*           the best solution given historical contraints.  Ideally, the    */
/*           size of the histogram array should be [MinPixel..MaxPixel].     */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	imgetdesc(Image, Type, Buffer)
   IMAGE *Image;
   int Type;
   int Buffer[];
   {
   GREYTYPE Pixels[MAXGET];
   int Low, High;
   int TempMax, TempMin;
   int PixelCnt;
   int Index;
   int i;

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");
   if (Image->PixelFormat != GREY) Error("Image type is not GREY");

   /* Handle request for MINMAX field */
   if (Type == MINMAX)
      {
      if (Image->ValidMaxMin == TRUE)
         {
         /* Copy current MaxMin field */
         Buffer[0] = Image->MaxMin[0];
         Buffer[1] = Image->MaxMin[1];
         }
      else
         {
         /* Compute new MaxMin field */
         TempMin = MAXVAL;
         TempMax = MINVAL;

         /* Loop reading pixels */
         for (Low=0; Low<Image->PixelCnt; Low=Low+MAXGET)
            {
            /* Compute read range */
            High = Low + MAXGET - 1;
            if (High >= Image->PixelCnt) High = Image->PixelCnt -1;
 
            /* Read pixels */
            if (imread(Image, Low, High, Pixels) == INVALID)
               Error("Could not read pixels");
            
            /* Search for new MaxMin Values */
            PixelCnt = High - Low + 1;
            for (i=0; i<PixelCnt; i++)
               {
               if (Pixels[i]<TempMin) TempMin = Pixels[i];
               if (Pixels[i]>TempMax) TempMax = Pixels[i];
               }
            }

         /* Save new MaxMin field */
         Image->ValidMaxMin = TRUE;
         Image->MaxMin[0] = Buffer[0] = TempMin;
         Image->MaxMin[1] = Buffer[1] = TempMax;
         }
      }

   /* Handle request for HISTOGRAM field */
   else if (Type == HISTO) 
      {
      if (Image->ValidHistogram == TRUE)
         {
         /* Copy current Histogram field */
         for (i=0; i<nHISTOGRAM; i++)
            Buffer[i] = (int) Image->Histogram[i];
         }
      else
         {
         /* Compute MAX and MIN pixel values JG1 */
         if (imgetdesc(Image, MINMAX, Buffer) == INVALID)
            Error("Could not obtain minmax values");
         TempMin = Buffer[0];
         TempMax = Buffer[1];
         
         /* Compute new Histogram field */
         for(i=0; i<nHISTOGRAM; i++)
	    Buffer[i] = 0;

         /* Loop reading pixels */
         for (Low=0; Low<Image->PixelCnt; Low=Low+MAXGET)
            {
            /* Compute read range */
            High = Low + MAXGET - 1;
            if (High >= Image->PixelCnt) High = Image->PixelCnt -1;
 
            /* Read pixels */
            if (imread(Image, Low, High, Pixels) == INVALID)
               Error("Could not read pixels");
            
            /* Update Histogram and handle any pixels out of range */
            for (i=0; i<=(High-Low); i++)
               {
               /* Determine index into histogram map JG1 */
               Index = Pixels[i] - TempMin;
               if (Index < 0)
                  Buffer[ 0 ]++;
               else if (Index >= nHISTOGRAM)
                  Buffer[ nHISTOGRAM-1 ]++;
               else
                  Buffer[ Index ]++;
               }
            }

         /* Save new Histogram field */
         Image->ValidHistogram = TRUE;
         for (i=0; i<nHISTOGRAM; i++)
            Image->Histogram[i] = Buffer[i];
         }
      }

   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine returns the validity of the HISTO or MAXMIN	     */
/*	     fields.							     */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	imtest(Image, Type)
   IMAGE *Image;
   int Type;
   {

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");

   /* Check that file is open */
   if (Image->Fd == EOF) Error("Image not open");

   /* Handle request for MINMAX field */
   if (Type == MINMAX)
      return (Image->ValidMaxMin);

   /* Handle request for HISTOGRAM field */
   else if (Type == HISTO) 
      return (Image->ValidHistogram);

	return FALSE;
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine reads the title field from the image header.       */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	imgettitle(Image, Title)
   IMAGE *Image;
   char *Title;
   {

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");
   if (Title == NULL) Error("Null title string pointer");

   /* Check that file is open */
   if (Image->Fd == EOF) Error("Image not open");

   /* Copy title field */
   strcpy(Title, Image->Title);

   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine writes the title field to the image header.        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	imputtitle(Image, Title)
   IMAGE *Image;
   char *Title;
   {
   int Length;

   /* Check parameters */
   if (Image == NULL) Error("Null image pointer");
   if (Title == NULL) Error("Null title string pointer");

   /* Check that file is open */
   if (Image->Fd == EOF) Error("Image not open");

   /* Copy title field */
   Length = strlen(Title) + 1;
   if (Length > nTITLE) Error("Title too long");
   strcpy(Image->Title, Title);

   return(VALID);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine returns a pointer to a list of information         */
/*           field names.                                                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/
char **iminfoids(Image)
   IMAGE *Image;
   {
   /* No longer supported*/
   fprintf(stderr, "Warning accessed unsupported libim routine 'iminfoids'\n");
   return(NULL);
   }

/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Purpose:  This routine returns the error string from the error buffer.    */
/*                                                                           */
/*---------------------------------------------------------------------------*/
char *imerror()
   {
   char *Message;
   int Length;

   /* Copy error string */
   Length = strlen(_imerrbuf) + 1;
   Message = (char *)malloc((unsigned)Length);
   if (Message == NULL) ErrorNull("Allocation error");
   strcpy(Message, _imerrbuf);

   return(Message);
   }



/*page*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Function:  Im_snap							     */
/*                                                                           */
/* Purpose:  To privide a quick means to writing out a 2D image from a	     */
/*	     program undergoing development.  This is not intended to be     */
/*	     used in finished programs.					     */
/*                                                                           */
/*	     The image may be of any valid type.  Note that no checking	     */
/*	     of the calling parameters is made.	 The programmer must use     */
/*	     due care.							     */
/*                                                                           */
/* Return:  Returns INVALID if there is a failure, VALID if the image is     */
/*	    successfully dumped.					     */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int	im_snap(xdim, ydim, pixformat, name, newtitle, pixel)
int	xdim;
int	ydim;
int	pixformat;
char   *name;
char   *newtitle;
GREYTYPE   *pixel;
{

IMAGE  *image;
int	dimc;
int	dimv[nDIMV];
int	pixcnt;
int	maxmin[nMAXMIN];
int	histo[nHISTOGRAM];


	pixcnt = xdim*ydim;
	dimc = 2;
	dimv[0] = ydim;
	dimv[1] = xdim;

	/* Create new image file */
	if ((image = imcreat(name, DEFAULT, pixformat, dimc, dimv)) ==
		INVALID) Error("Can not create snapshot image");

	/* Write out new image file */
	if (imwrite(image, 0, pixcnt - 1, pixel) == INVALID) {
		Error("Can not write pixels to snapshot image\n");
	}

	/* Update the values of minmax and histo for new image */
	if (pixformat == GREY) {
		if (imgetdesc(image, MINMAX, maxmin) == INVALID) {
			Error("Can not update maxmin field");
		}
		if (imgetdesc(image, HISTO, histo) == INVALID) {
			Error("Can not update histo field");
		}
	}

	if (newtitle != NULL) 
		if (imputtitle(image, newtitle) == INVALID) {
			Error("Can not write title to new image");
		}

	/* Close image files */
	imclose(image);

	return (VALID);
}
