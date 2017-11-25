#ifndef	_COMMON_HEADER

/* this here is for when linking these library with a C++ program
   using the C++ compiler, and it makes sure that the compiler
   does not complain about unknown symbols.
   NOTE that each function declaration in this file, if you want
   it to be identified by C++ compiler correctly, it must be
   preceded by EXTERNAL_LINKAGE
   e.g.
        EXTERNAL_LINKAGE        IMAGE *imcreat(char *, int, int, int, int *);

   other functions which will not be called from inside the C++ program,
   do not need to have this, but doing so is no harm.

   see http://developers.sun.com/solaris/articles/external_linkage.html
   (author: Giri Mandalika)
*/

/* strdup is not portable, this is our own implementation */
char *dupstr(const char *src);

#ifdef __cplusplus
#ifndef EXTERNAL_LINKAGE
#define EXTERNAL_LINKAGE        extern "C"
#endif
#else
#ifndef EXTERNAL_LINKAGE
#define EXTERNAL_LINKAGE	extern
#endif
#endif

/* this is defined in the system headers but ISO_C99 must be
   set which in turn causes lrand48 to be undefined,
   until this is resolved, this is a good workaround */
#ifndef __cplusplus
double	round(double x);
#endif

#define	DATATYPE	int

#ifndef MIN
#define	MIN(a, b)	((a)<(b) ? (a):(b))
#endif
#ifndef MAX
#define	MAX(a, b)	((a)>(b) ? (a):(b))
#endif
#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif
#ifndef DONTKNOW
#define DONTKNOW 2
#endif
#ifndef	ABS
#define	ABS(a)		((a)<0 ? (-(a)):(a))
#endif
#ifndef SQR
#define	SQR(a)	((a)*(a))
#endif
#ifndef CUBE
#define	CUBE(a)	((a)*(a)*(a))
#endif
#ifndef SQRSQR
#define	SQRSQR(a) ((a)*(a)*(a)*(a))
#endif

/* Sigmoid based on tanh which spans from Ymin to Ymax (for when x->-infinity and +infinity respectively)
   and at xoff, its value is halfway between Ymin and Ymax and has a slope of Slope */
#define SIGMOID(_Ymin, _Ymax, _Slope, _xoff, _x)\
(\
	(0.5 * ((_Ymax)-(_Ymin))) *\
	(\
		tanh(\
			(_Slope) * ((_x)-(_xoff))\
		) +\
		1.0\
	) +\
	(_Ymin)\
)

/* sqrt(2*M_PI) for use with GAUSSIAN, below */
#define SQRT_2PI	2.50662827463100050241
/* Gaussian distribution function, given mean and stdev and a point on the x-axis, it gives frequency */
#define GAUSSIAN(_mean, _stdev, _x) (1.0/(SQRT_2PI*(_stdev)) * exp(-(((_x)-(_mean))*((_x)-(_mean))/(2.0*(_stdev)*(_stdev)))))

typedef	struct	_AREA	area;
struct	_AREA {
	/* a, b, may denote coordinates (x,y) and c, d dimensions (width and height)
	   in some cases you might want to say w1, h1, w2, h2 */
	int	a, b, c, d;
	/* lo, hi are ranges of a property within this area */
	double	lo, hi;
};

/* scales the input (_x) between _Ymin and _Ymax, given that _x spans between _Xmin and _Xmax */
/* if any of the values are integers, please typecast them to float/double before you send them in here */
/* if you expect integer back then simply typecast the result back or use the macro ROUND to round it, below */
/* note that in the case where Xmin==Xmax (e.g. no variation in the input) it returns Ymin, otherwise there is division by zero */
#define	SCALE_OUTPUT(_x, _Ymin, _Ymax, _Xmin, _Xmax) ((((_Xmax)==(_Xmin))?(_Ymin):((((_Ymax)-(_Ymin))/((_Xmax)-(_Xmin)))*((_x)-(_Xmin)))+(_Ymin)))

/* rounds a double (_x) to integer - e.g. 2.3 to 2 and 2.6 to 3 - 2.5 goes to 3 */
#define	ROUND(_x)	( (((_x)-((int )(_x)))<0.5) ? ((int )(_x)) : ((int )(_x+1)) )

/* returns true if number a is between the range low <= a < high
   NOTICE THE <= and < */
/* INSIDE: low <= a < high */
#define IS_WITHIN(a, low, high) ((((a)<(high))&&((a)>=(low))) ? TRUE : FALSE)
/* OUTSIDE: a < low OR a >= high */
#define IS_OUTSIDE(a, low, high) ((((a)<(low))||((a)>=(high))) ? TRUE : FALSE)

#define	_COMMON_HEADER
#endif
