#include <stdio.h>
#include <stdlib.h>
#include <math.h>


double	factorial(int x);

int	main(int argc, char **argv)
{
	int	x, y;
	double	comb;

	if( argc != 3 ){
		fprintf(stderr, "Usage: %s X items taking Y at a time\n", argv[0]);
		exit(-1);
	}

	
	x = atoi(argv[1]);
	y = atoi(argv[2]);

	if( x < y ){
		fprintf(stderr, "X must not be less than Y.\n");
		exit(-1);
	}

	comb = factorial(x) / (factorial(x-y)*factorial(y));

	printf("%1.lf\n", comb);
	exit(0);
}

double	factorial(int x)
{
	double	i, y = 1.0;

	if( x <= 0 ) return(1.0);

	for(i=1;i<=(double )x;i++)
		y *= i;

	return(y);
}
