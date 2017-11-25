typedef	struct _CA_SYMBOL		symbol;
typedef	struct _CA_CELL			cell;
typedef	struct _CA_CELLULAR_AUTOMATON	cellular_automaton;

#ifndef _CELLULAR_AUTOMATA_LIB_INTERNAL
/* these are not included during the compilation of the library, these are for users of the library
   AND after the library has been compiled and installed */
#include <CAsymbol.h>
#include <CAcell.h>
#include <CAcellular_automaton.h>
#include <CAIO.h>
#else
/* private includes */
#include "CAsymbol.h"
#include "CAcell.h"
#include "CAcellular_automaton.h"
#include "CAIO.h"
#endif
