/*
 * Shuffle the lines in a file. See shuffle.1 for manual.
 * (Original: http://www.w3.org/People/Bos/Shuffle)
 *
 * Reads all lines into memory, then writes them out in arbitrary order.
 * Tries to use mmap() first, but if that fails, uses read().
 * The idea is that mmap() should be faster than read() + realloc()...
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 12 April 1999
 * Version: $Id: shuffle.c,v 1.4 1999/04/13 11:47:49 bbos Exp $
 *
 * This is Open Source software, see http://www.w3.org/Consortium/Legal/
 *
 * Copyright © 1995-1999 World Wide Web Consortium, (Massachusetts
 * Institute of Technology, Institut National de Recherche en Informatique
 * et en Automatique, Keio University). All Rights Reserved.
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#ifndef MAP_FAILED
#define	MAP_FAILED ((void *) -1)
#endif

#define BUFINC 8096


/* fatal -- print message and exit */
static void fatal(char *msg1, char *msg2)
{
  (void) write(2, msg1, strlen(msg1));
  (void) write(2, ": ", 2);
  (void) write(2, msg2, strlen(msg2));
  (void) write(1, "\n", 1);
  exit(1);
}

/* syserr -- print system error and exit */
static void syserr(char *msg)
{
  fatal(msg, strerror(errno));
}

/* swap -- swap two integers */
static void swap(int *a, int *b)
{
  int h;
  h = *a;
  *a = *b;
  *b = h;
}

/* main -- read lines, shuffle and write them */
int main(int argc, char *argv[])
{
  int in = 0, out = 1;
  int nlines, i, j, ismmap;
  off_t len;
  char *buf;
  int *lines, *shuffle;

	char	*inputFilename = NULL, *outputFilename = NULL;
  	int	optI, seed = (int )time(0);
	while( (optI=getopt(argc, argv, "i:o:s:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': seed = atoi(optarg); break;
			default: fprintf(stderr, "Unknown option '-%c'.\n", optI); exit(1);
		}
	if( inputFilename != NULL ) if( (in=open(inputFilename, O_RDONLY)) < 0) syserr(inputFilename);
	if( outputFilename != NULL ) if( (out=open(outputFilename, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) syserr(outputFilename);

  /* Map file into memory, either via mmap or read */
  ismmap = (len = lseek(in, 0, SEEK_END)) >= 0;
  if (ismmap) {					/* Real file, so try mmap */
    buf = mmap(NULL, len, PROT_READ, MAP_PRIVATE, in, 0);
    if (buf == MAP_FAILED) syserr(argv[0]);
  } else {					/* Cannot seek, try read */
    if (! (buf = malloc(BUFINC))) syserr(argv[0]);
    for (len = 0; (i = read(in, buf + len, BUFINC)) > 0; len += i)
      if (! (buf = realloc(buf, len + i + BUFINC))) syserr(argv[0]);
    if (i < 0) syserr(argv[0]);
  }

  /* Find the # of lines */
  for (i = 0, nlines = 0; i < len; i++) if (buf[i] == '\n') nlines++;
  if (len > 0 && buf[len-1] != '\n') fatal(argv[0],"Missing newline at eof\n");

  /* Allocate arrays of indexes into buf and indexes into index */
  if (! (lines = malloc((nlines + 1) * sizeof(lines[0])))) syserr(argv[0]);
  if (! (shuffle = malloc((nlines + 1) * sizeof(shuffle[0])))) syserr(argv[0]);

  /* Find the start & end of each line again */
  lines[0] = 0;
  for (i = 0, j = 1; i < len; i++) if (buf[i] == '\n') lines[j++] = i + 1;

  /* Shuffle the shuffle array */
  for (i = 0; i < nlines; i++) shuffle[i] = i;
  srandom(seed);
  for (i = 0; i < nlines; i++) swap(&shuffle[i], &shuffle[random() % nlines]);

  /* Write out the shuffled lines */
  for (i = 0; i < nlines; i++) {
    j = shuffle[i];
    if (write(out, buf + lines[j], lines[j+1] - lines[j]) < 0) syserr(argv[0]);
  }

  /* Let's be good and explicitly unmap, free, close, etc. everything */
  if (ismmap && munmap(buf, len) < 0) syserr(argv[0]);
  if (!ismmap) free(buf);
  if (close(in) < 0) syserr(argv[0]);
  if (close(out) < 0) syserr(argv[0]);
  free(shuffle);
  free(lines);

  return 0;
}
