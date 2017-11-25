#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>
#include <LinkedList.h>
#include <LinkedListIterator.h>

#include "cellular_automata.h"

symbol	*ca_new_symbol(int id, char *name){
	symbol	*ret;

	if( (ret=(symbol *)malloc(sizeof(symbol))) == NULL ){
		fprintf(stderr, "ca_new_symbol : could not allocate %zd bytes for symbol.\n", sizeof(symbol));
		return (symbol *)NULL;
	}
	ret->id = id;

	if( name != NULL ) ret->name = strdup(name);
	else ret->name = NULL;

	return ret;
}
void	ca_destroy_symbol(symbol *a_symbol){
	if( a_symbol->name != NULL ) free(a_symbol->name);
	free(a_symbol);
}
char	*ca_toString_symbol(symbol *a_symbol){
	char	*ret, *p;
	if( (ret=(char *)malloc(10+a_symbol->name==NULL?0:strlen(a_symbol->name))) == NULL ){
		fprintf(stderr, "ca_toString_symbol : could not allocate %zd bytes for ret.\n", 10+a_symbol->name==NULL?0:strlen(a_symbol->name));
		return NULL;
	}

	if( a_symbol->name != NULL ) sprintf(ret, "[%d, '%s']", a_symbol->id, a_symbol->name);
	else sprintf(ret, "[%d]", a_symbol->id);

	p = strdup(ret); free(ret); return strdup(p);
}
void	ca_print_symbol(FILE *stream, symbol *a_symbol){
	char	*p = ca_toString_symbol(a_symbol);
	fprintf(stream, "%s", p);
	free(p);
}
