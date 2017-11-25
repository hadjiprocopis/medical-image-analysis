#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "filters.h"
#include "combinatorics.h"

int	combinations(int n, int k){
	/* combinations of n objects drawn k at a time */
	/* shortcut, count down on your factorials to 1,
	   = first k factors of n! / last k factors of n!
	*/
	int	i;
	float	ret = 1.0;

	for(i=(k+1);i<=n;i++) ret *= i;
	for(i=2;i<=(n-k);i++) ret /= i;

	return ret;
}
int	factorial(int x)
{
	int	i, y = 1;

	if( x <= 0 ) return 1;

	for(i=1;i<=x;i++) y *= i;

	return y;
}

