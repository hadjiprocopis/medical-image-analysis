#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Common_IMMA.h"

/* strdup is not portable, here is our own implementation */
char *dupstr(const char *src){
	char *p;
	if( (p=(char *)malloc(strlen(src)+1)) != NULL ) strcpy(p, src);
	return p;
}
