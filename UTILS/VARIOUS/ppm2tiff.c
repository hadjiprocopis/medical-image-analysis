/* $Header: /usr/people/sam/tiff/tools/RCS/ppm2tiff.c,v 1.24 1996/01/10 19:35:29 sam Exp $ */

/*
 * Copyright (c) 1991-1996 Sam Leffler
 * Copyright (c) 1991-1996 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "tiffio.h"

#define	streq(a,b)	(strcmp(a,b) == 0)
#define	strneq(a,b,n)	(strncmp(a,b,n) == 0)

static	uint16 compression = (uint16) -1;
static	uint16 predictor = 0;
static	int quality = 75;	/* JPEG quality */
static	int jpegcolormode = JPEGCOLORMODE_RGB;

static	void usage(void);
static	int processCompressOptions(char*);

static void
BadPPM(char* file)
{
	fprintf(stderr, "%s: Not a PPM file.\n", file);
	exit(-2);
}

int
main(int argc, char* argv[])
{
	uint16 photometric;
	uint32 rowsperstrip = (uint32) -1;
	double resolution = -1;
	unsigned char *buf = NULL;
	char	*infile = NULL, *outfile = NULL;
	uint32 row;
	tsize_t linebytes;
	uint16 spp;
	TIFF *out;
	FILE *in;
	int w, h;
	int prec;
	int c;
	extern int optind;
	extern char* optarg;

	compression =  COMPRESSION_NONE;
	while ((c = getopt(argc, argv, "c:r:R:i:o:")) != -1)
		switch (c) {
		case 'i': infile = strdup(optarg); break;
		case 'o': outfile = strdup(optarg); break;
		case 'c':		/* compression scheme */
			if (!processCompressOptions(optarg))
				usage();
			break;
		case 'r':		/* rows/strip */
			rowsperstrip = atoi(optarg);
			break;
		case 'R':		/* resolution */
			resolution = atof(optarg);
			break;
		case '?':
			usage();
			/*NOTREACHED*/
		}

	if( outfile == NULL ){
		usage();
		fprintf(stderr, "an output filename must be specified with the '-i' option.\n");
		exit(1);
	}

	if( infile != NULL ){
		if( (in=fopen(infile, "r")) == NULL ){
			fprintf(stderr, "%s: Can not open input file '%s'.\n", argv[0], infile);
			exit(-1);
		}
	} else in = stdin;
		
	if (getc(in) != 'P')
		BadPPM(infile);
	switch (getc(in)) {
	case '5':			/* it's a PGM file */
		spp = 1;
		photometric = PHOTOMETRIC_MINISBLACK;
		break;
	case '6':			/* it's a PPM file */
		spp = 3;
		photometric = PHOTOMETRIC_RGB;
		if (compression == COMPRESSION_JPEG &&
		    jpegcolormode == JPEGCOLORMODE_RGB)
			photometric = PHOTOMETRIC_YCBCR;
		break;
	default:
		BadPPM(infile);
	}
	if (fscanf(in, " %d %d %d", &w, &h, &prec) != 3)
		BadPPM(infile);
	if (getc(in) != '\n' || w <= 0 || h <= 0 || prec != 255)
		BadPPM(infile);

	if( (out=TIFFOpen(outfile, "w")) == NULL ){
		fprintf(stderr,"%s : could not open output tiff file '%s' for writing.\n", argv[0], outfile);
		exit(-4);
	}

	TIFFSetField(out, TIFFTAG_IMAGEWIDTH,  w);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, h);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
	switch (compression) {
	case COMPRESSION_JPEG:
		TIFFSetField(out, TIFFTAG_JPEGQUALITY, quality);
		TIFFSetField(out, TIFFTAG_JPEGCOLORMODE, jpegcolormode);
		break;
	case COMPRESSION_LZW:
	case COMPRESSION_DEFLATE:
		if (predictor != 0)
			TIFFSetField(out, TIFFTAG_PREDICTOR, predictor);
		break;
	}
	linebytes = spp * w;
	if (TIFFScanlineSize(out) > linebytes)
		buf = (unsigned char *)_TIFFmalloc(linebytes);
	else
		buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(out));
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
	    TIFFDefaultStripSize(out, rowsperstrip));
	if (resolution > 0) {
		TIFFSetField(out, TIFFTAG_XRESOLUTION, resolution);
		TIFFSetField(out, TIFFTAG_YRESOLUTION, resolution);
		TIFFSetField(out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	}
	for (row = 0; row < h; row++) {
		if (fread(buf, linebytes, 1, in) != 1) {
			fprintf(stderr, "%s: scanline %lu: Read error.\n",
			    infile, (unsigned long) row);
			break;
		}
		if (TIFFWriteScanline(out, buf, row, 0) < 0)
			break;
	}
	(void) TIFFClose(out);
	if (buf)
		_TIFFfree(buf);
	return (0);
}

static int
processCompressOptions(char* opt)
{
	if (streq(opt, "none"))
		compression = COMPRESSION_NONE;
	else if (streq(opt, "packbits"))
		compression = COMPRESSION_PACKBITS;
	else if (strneq(opt, "jpeg", 4)) {
		char* cp = strchr(opt, ':');
		if (cp && isdigit((int )(cp[1])))
			quality = atoi(cp+1);
		if (cp && strchr(cp, 'r'))
			jpegcolormode = JPEGCOLORMODE_RAW;
		compression = COMPRESSION_JPEG;
	} else if (strneq(opt, "lzw", 3)) {
		char* cp = strchr(opt, ':');
		if (cp)
			predictor = atoi(cp+1);
		compression = COMPRESSION_LZW;
	} else if (strneq(opt, "zip", 3)) {
		char* cp = strchr(opt, ':');
		if (cp)
			predictor = atoi(cp+1);
		compression = COMPRESSION_DEFLATE;
	} else
		return (0);
	return (1);
}

char* stuff[] = {
"usage: ppm2tiff [options] input.ppm output.tif",
"where options are:",
" -i input      'input' is a ppm input filename",
" -o output     'output' is the tiff output filename",
" -r #		make each strip have no more than # rows",
" -R #		set x&y resolution (dpi)",
"",
" -c jpeg[:opts]  compress output with JPEG encoding",
" -c lzw[:opts]	compress output with Lempel-Ziv & Welch encoding",
" -c zip[:opts]	compress output with deflate encoding",
" -c packbits	compress output with packbits encoding",
" -c none	use no compression algorithm on output",
"",
"JPEG options:",
" #		set compression quality level (0-100, default 75)",
" r		output color image as RGB rather than YCbCr",
"LZW and deflate options:",
" #		set predictor value",
"For example, -c lzw:2 to get LZW-encoded data with horizontal differencing",
NULL
};

static void
usage(void)
{
	char buf[BUFSIZ];
	int i;

	setbuf(stderr, buf);
	for (i = 0; stuff[i] != NULL; i++)
		fprintf(stderr, "%s\n", stuff[i]);
	exit(-1);
}
