/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Program:  IMAGE.H                                                         */
/*                                                                           */
/* Purpose:  This file contains the constant and type definitions for        */
/*           the routines defined in image.c.                                */
/*                                                                           */
/* Author:   John Gauch - Version 2                                          */
/*           Zimmerman, Entenman, Fitzpatrick, Whang - Version 1             */
/*                                                                           */
/* Date:     February 23, 1987                                               */
/*                                                                           */
/* Revisions:								     */
/*                                                                           */
/*	     Made C++ version, called Image.h.  3 Oct 1990, A. G. Gash.	     */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#ifndef _IMAGE_HEADER_
#define _IMAGE_HEADER_

/* this here is for when linking these library with a C++ program
   using the C++ compiler, and it makes sure that the compiler
   does not complain about unknown symbols.
   NOTE that each function declaration in this file, if you want
   it to be identified by C++ compiler correctly, it must be
   preceded by EXTERNAL_LINKAGE
   e.g.
	EXTERNAL_LINKAGE	IMAGE *imcreat(char *, int, int, int, int *);

   other functions which will not be called from inside the C++ program,
   do not need to have this, but doing so is no harm.

   see http://developers.sun.com/solaris/articles/external_linkage.html
   (author: Giri Mandalika)
*/
#ifdef __cplusplus
#ifndef EXTERNAL_LINKAGE
#define EXTERNAL_LINKAGE        extern "C"
#endif
#else
#ifndef EXTERNAL_LINKAGE
#define EXTERNAL_LINKAGE	extern
#endif
#endif

#ifdef SYSV
#include <sys/types.h>
#ifndef __FCNTL_HEADER__	/* Because system V does not do this */
#define __FCNTL_HEADER__	/* 	in fcntl.h		     */
#include <sys/fcntl.h>
#endif !__FCNTL_HEADER__
#endif

#ifdef SYSV
#ifndef __FILE_HEADER__		/* Because system V does not do this */
#define __FILE_HEADER__		/* 	in file.h		     */
#include <sys/file.h>
#endif !__FILE_HEADER__
#else
#include <sys/file.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iminfo.h"

/* Boolean values */
#define TRUE		1
#define FALSE		0

/* Routine return codes */
#define VALID		1
#define INVALID		0

/* Maximum and minimum GREYTYPE values */
#define MINVAL		-32768
#define MAXVAL		32767

/* Pixel format codes */
#define GREY		0010
#define COLOR		0020
#define COLORPACKED	0040
#define USERPACKED	0200
#define BYTE		0001
#define SHORT		0002
#define LONG		0003
#define REAL		0004
#define COMPLEX		0005

/* Pixel format types */
typedef short GREYTYPE;
typedef short COLORTYPE;
typedef struct { unsigned char r,g,b,a; } CPACKEDTYPE;
typedef int USERTYPE;
typedef unsigned char BYTETYPE;
typedef short SHORTTYPE;
typedef long LONGTYPE;
typedef float REALTYPE;
typedef struct { float re, im; } COMPLEXTYPE;

/* Constants for open calls */
#define READ		(O_RDONLY)
#define UPDATE		(O_RDWR)
#define CREATE		(O_RDWR | O_CREAT | O_EXCL)
 
/* Constants for imgetdesc calls */
#define MINMAX		0
#define HISTO		1

/* Protection modes for imcreat */
#define UOWNER		0600
#define UGROUP		0060
#define RGROUP		0040
#define UOTHER		0006
#define ROTHER		0004
#define DEFAULT		0644

/* Array length constants */
#define nADDRESS	9
#define nTITLE		81
#define nMAXMIN		2
#define nHISTOGRAM	1024
#define nDIMV		10
#define nERROR		200
#define nINFO		100

/* Old names for array length constants (from version 1) */
#define _NDIMS		nDIMV
#define _ERRLEN		nERROR
#define TITLESIZE	nTITLE
#define MAXPIX		(nHISTOGRAM-1)

/* Structure for image information (everything but pixels) */
typedef struct {
   int   Fd;			/* Computed fields */
   int   PixelSize;
   int   PixelCnt;

   int   Address[nADDRESS];	/* Header fields from file */
   char  Title[nTITLE];
   int   ValidMaxMin;
   int   MaxMin[nMAXMIN];
   int   ValidHistogram;
   int   Histogram[nHISTOGRAM];
   int   PixelFormat;
   int   Dimc;
   int   Dimv[nDIMV];

   Info            *file_info;     /* Extended (Queen Square) info fields */
   Dim_info        pDimv[nDIMV];
   } IMAGE;

/* Error string buffer */
//EXTERNAL_LINKAGE	char _imerrbuf[nERROR];

/* Initialization routines */
EXTERNAL_LINKAGE	IMAGE *imcreat(char *, int, int, int, int *);
EXTERNAL_LINKAGE	IMAGE *imcreat_memory(int, int, int *);
EXTERNAL_LINKAGE	IMAGE *imopen(char *, int);
EXTERNAL_LINKAGE	int imclose(IMAGE *);



/* Pixel access routines */
EXTERNAL_LINKAGE	int imread(IMAGE *, int, int, GREYTYPE *);
EXTERNAL_LINKAGE	int imwrite(IMAGE *, int, int, GREYTYPE *);
EXTERNAL_LINKAGE	int imgetpix(IMAGE *, int [][2], int *, GREYTYPE *);
EXTERNAL_LINKAGE	int imputpix(IMAGE *, int [][2], int *, GREYTYPE *);
EXTERNAL_LINKAGE	int GetPut2D(IMAGE *, int [][2], GREYTYPE *, int);
EXTERNAL_LINKAGE	int GetPut3D(IMAGE *, int [][2], GREYTYPE *, int);
EXTERNAL_LINKAGE	int GetPutND(IMAGE *, int [][2], GREYTYPE *, int);

/* Information access routines */
EXTERNAL_LINKAGE	int imheader(IMAGE *, int *, int *, int *, int *, int *, int *);
EXTERNAL_LINKAGE	int imdim(IMAGE *, int *, int *);
EXTERNAL_LINKAGE	int imbounds(IMAGE *, int *);
EXTERNAL_LINKAGE	int imgetdesc(IMAGE *, int, int[]);
EXTERNAL_LINKAGE	int imtest(IMAGE *, int);
EXTERNAL_LINKAGE	int imgettitle(IMAGE *, char *);
EXTERNAL_LINKAGE	int imputtitle(IMAGE *, char *);
/*char *imgetinfo(IMAGE *, char *); Replacement routines in iminfo.c
EXTERNAL_LINKAGE	int imputinfo(IMAGE *, char *, char *);
EXTERNAL_LINKAGE	int imcopyinfo(IMAGE *, IMAGE *);
EXTERNAL_LINKAGE	char **iminfoids(IMAGE *);*/
EXTERNAL_LINKAGE	char *imerror(); 
EXTERNAL_LINKAGE	int im_snap(int, int, int, char *, char *, GREYTYPE *); 

/*---------------------------------------------------------------------------*/
/*                                                                           */
/* File:     PDIM.H                                                          */
/*                                                                           */
/* Purpose:  This file contains declarations used by PDIM.C                  */
/*                                                                           */
/* Author:   John Gauch - Version 2                                          */
/*           Chuck Mosher - Version 1                                        */
/*                                                                           */
/* Date:     July 21, 1986                                                   */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* Global PDIM constants */
#define REC_SIZE 200

/* Types of units possible */
#define MILLIMETER 0
#define CENTIMETER 1

/* Structure for single slice description */
typedef struct {
   float Ox,Oy,Oz;
   float Ux,Uy,Uz;
   float Vx,Vy,Vz;
   float time;
   int   number;
   } SLICEREC;

/* Structure for whole PDIM description */
typedef struct {
   int version;
   int units;
   int machine;
   int slicecnt;
   SLICEREC *patient;
   SLICEREC *table;
   } PDIMREC;
 
/* Forward declarations of PDIM routines */
EXTERNAL_LINKAGE	int pdim_read();
EXTERNAL_LINKAGE	int pdim_write();
EXTERNAL_LINKAGE	int pdim_free();
EXTERNAL_LINKAGE	int pdim_append();
EXTERNAL_LINKAGE	int pdim_window();
EXTERNAL_LINKAGE	int pdim_scale();
EXTERNAL_LINKAGE	int pdim_rotate();
EXTERNAL_LINKAGE	int pdim_translate();
EXTERNAL_LINKAGE	int pdim_map();
#endif
