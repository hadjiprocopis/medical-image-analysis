#ifndef DATATYPE
#define DATATYPE        int
#endif

#include "typedefs.h"
#include "constants.h"

#include "type.h"
#include "number.h"
#include "strinG.h"
#include "boolean.h"
#include "point.h"
#include "dimensions.h"
#include "region.h"
#include "imagE.h"

#include "symbol.h"
#include "symboltable.h"

#include "parameter.h"
#include "operator.h"
#include "function.h"
#include "designator.h"
#include "expression.h"
#include "supertype.h"
#include "assignment.h"

#include "ImplementedFunctions.h"

/* defined in private.h */
extern	char *null;
extern	char *empty;

/* main entry point to parsing oingo files */
int	doParse(char */*filename*/, symboltable */*function_templates*/, symboltable */*constant_definitions*/, symboltable */*program_symbols*/, symboltable */*program_functions*/);
