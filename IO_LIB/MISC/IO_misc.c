#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "IO_misc.h"
#include "../IO_constants.h"

/* Function to determine the endianess of the system, i.e.
   in interpeting data types > 1 byte (i.e. short, long, int etc)
   if Most Significant Byte is the first one we have Big Endian
   and this function returns IO_RAW_BIG_ENDIAN,
   if MSB is the last byte then we have Little Endian and
   this function returns IO_RAW_LITTLE_ENDIAN

   author: Andreas Hadjiprocopis, NMR, ION, 2001, CING 2005
*/
int	check_system_endianess(){
	long	n = 0xA;
	unsigned char	*p = (unsigned char *)(&n);
	int	i;
	for(i=1;i<sizeof(long);i++) n <<= 8;
	n += 0xB;

	if( (*p == 0xA) && (*(p+sizeof(long)-1) == 0xB) ) return IO_RAW_BIG_ENDIAN;
	else if( (*p == 0xB) && (*(p+sizeof(long)-1) == 0xA) ) return IO_RAW_LITTLE_ENDIAN;
	else fprintf(stderr, "check_system_endianess : failed to recognise system endianess -- this should not be happening.\n");
	exit(1);
}
