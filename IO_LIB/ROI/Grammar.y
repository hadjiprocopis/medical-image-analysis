%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Common_IMMA.h>
#include "IO_roi.h"

/* defined in the lexer, line number and current token */
extern	int	lex_lineNumber;
extern	char	*lex_currentToken;

/* the input file handle, property of the lexer */
extern	FILE	*yyin;

/* the name of the executable or function routine if not running it from command line*/
char		*myAppName;

/* routines to open and close a roi file */
FILE	*openInputfile(char *);
void	closeInput(FILE *);

#define	YYSTYPE		char *

roi			**myRois;
int			numRois;
char			errorMsg[1000];

roiRegionElliptical	*anEllipticalRegion;
roiRegionIrregular	*anIrregularRegion;
roiRegionRectangular	*aRectangularRegion;
roiRegionType		aRegionType;

/* allocate ROI_ALLOC_STEP rois at each time more data space is needed */
#define			ROI_ALLOC_STEP	10

void	yyexit(int);
int	yyerror(char *s);
int	yylex();
%}

%token	IRREGULAR ELLIPTICAL RECTANGULAR END_REGION NAME IMAGE POINTS X0 Y0 WIDTH HEIGHT EX0 EY0 EA EB ROT
%token	ASSIGN
%token	STRING
%token	NUMBER
%token	COMMENT
%right	ASSIGN

%start	roi_file

%%
 
roi_file:
	rois
	;
rois:
	region
	|
	region rois
	;
region:
	start_region
	data
	end_region { numRois++; }
	;
start_region:
	region_type {
		/* allocate space for one more roi - if num of rois exceeds already allocated space then reallocate */
		if( ((numRois % ROI_ALLOC_STEP)==0) && (numRois >= ROI_ALLOC_STEP) ){
			/* reallocate */
			int	n = numRois / ROI_ALLOC_STEP + 1;
			if( (myRois=(roi **)realloc(myRois, n * ROI_ALLOC_STEP * sizeof(roi *))) == NULL ){
				sprintf(errorMsg, "%s : could not reallocate %zd bytes for myRois.\n", myAppName, n * ROI_ALLOC_STEP * sizeof(roi *));
				yyerror(errorMsg);
			}
		}
		if( (myRois[numRois]=(roi *)malloc(sizeof(roi))) == NULL ){
			sprintf(errorMsg, "%s : could not allocate %zd bytes for myRois[%d].\n", myAppName, sizeof(roi), numRois);
			yyerror(errorMsg);
		}
		myRois[numRois]->type = aRegionType;
		myRois[numRois]->centroid_x =
		myRois[numRois]->centroid_y =
		myRois[numRois]->x0 = myRois[numRois]->y0 =
		myRois[numRois]->width = myRois[numRois]->height =
		myRois[numRois]->minPixel = myRois[numRois]->maxPixel =
		myRois[numRois]->meanPixel = myRois[numRois]->stdevPixel = 0.0;
		myRois[numRois]->num_points_inside = 0;
		myRois[numRois]->points_inside = NULL;

		switch( aRegionType ){
			case IRREGULAR_ROI_REGION:
				if( (myRois[numRois]->roi_region=(void *)malloc(sizeof(roiRegionIrregular))) == NULL ){
					sprintf(errorMsg, "%s : could not allocate  %zd bytes for roiRegionIrregular roi_region of myRois[%d].\n", myAppName, sizeof(roiRegionIrregular), numRois);
					yyerror(errorMsg);
				}
				anIrregularRegion = (roiRegionIrregular *)(myRois[numRois]->roi_region);
				anIrregularRegion->num_points = 0;
				anIrregularRegion->p = myRois[numRois];
				break;
			case RECTANGULAR_ROI_REGION:
				if( (myRois[numRois]->roi_region=(void *)malloc(sizeof(roiRegionRectangular))) == NULL ){
					sprintf(errorMsg, "%s : could not allocate %zd bytes for roiRegionRectangular roi_region of myRois[%d].\n", myAppName, sizeof(roiRegionRectangular), numRois);
					yyerror(errorMsg);
				}
				aRectangularRegion = (roiRegionRectangular *)(myRois[numRois]->roi_region);
				aRectangularRegion->p = myRois[numRois];
				break;
			case ELLIPTICAL_ROI_REGION:
				if( (myRois[numRois]->roi_region=(void *)malloc(sizeof(roiRegionElliptical))) == NULL ){
					sprintf(errorMsg, "%s : could not allocate %zd bytes for roiRegionElliptical roi_region of myRois[%d].\n", myAppName, sizeof(roiRegionElliptical), numRois);
					yyerror(errorMsg);
				}
				anEllipticalRegion = (roiRegionElliptical *)(myRois[numRois]->roi_region);
				anEllipticalRegion->p = myRois[numRois];
				break;
			default:
				sprintf(errorMsg, "%s : type %d not yet implemented.\n", myAppName, myRois[numRois]->type=aRegionType);
				yyerror(errorMsg);
				break;
		}
	}
	name
	image
	;
region_type:
	IRREGULAR { aRegionType = IRREGULAR_ROI_REGION; }
	|
	RECTANGULAR { aRegionType = RECTANGULAR_ROI_REGION; }
	|
	ELLIPTICAL { aRegionType = ELLIPTICAL_ROI_REGION; }
	;
name:
	NAME ASSIGN string {
		myRois[numRois]->name = $3; /* strdup already done, thanks */
	}
	;
image:
	IMAGE ASSIGN string {
		/* find the last number in the 'image' string - it is the slice number (minus 1), quite crap data structures from amateurs */
		char	*f = $3;
		int	i = strlen(f) - 1, j, k;
		char	n[100];

		/* image ends in '...slice xxx' where xxx is the slice number.
		   the roi->image will hold all the string except from the number 'xxx'
		   this is because we might need to alter slice numbers.
		   This means that when saving in roi/dispunc format, the 'image' field
		   should be followed by the slice number e.g.
		   printf("%s %d", roi->image, roi->slice+1);
		   */
		while( isdigit((int )(f[i])) ) i--;
		for(j=i+1,k=0;j<strlen(f);j++) n[k++] = f[j];
		f[i] = '\0';
		myRois[numRois]->slice = atoi(n) - 1;
		myRois[numRois]->image = strdup(f); free(f);
	}
	;
data:
	irregular_region_data
	|
	elliptical_region_data
	|
	rectangular_region_data
	;
irregular_region_data:
	POINTS ASSIGN number {
		int	n = atoi($3);
		if( (anIrregularRegion->points=(roiPoint **)malloc(n * sizeof(roiPoint *))) == NULL ){
			sprintf(errorMsg, "%s : could not allocate %zd bytes for anIrregularRegion->points of myRois[%d].\n", myAppName, n * sizeof(roiPoint *), numRois);
			yyerror(errorMsg);
		}
	}
	pairs_of_points
	;
	pairs_of_points:
		pair_of_points
		|
		pair_of_points pairs_of_points
		;
	pair_of_points:
		number number {
			if( (anIrregularRegion->points[anIrregularRegion->num_points]=(roiPoint *)malloc(sizeof(roiPoint))) == NULL ){
				sprintf(errorMsg, "%s : could not allocate %zd bytes for anIrregularRegion->points[%d] of myRois[%d].\n", myAppName, sizeof(roiPoint), anIrregularRegion->num_points, numRois);
				yyerror(errorMsg);
			}
			anIrregularRegion->points[anIrregularRegion->num_points]->x = atof($1);
			anIrregularRegion->points[anIrregularRegion->num_points]->y = atof($2);
			anIrregularRegion->points[anIrregularRegion->num_points]->z = myRois[numRois]->slice;

			anIrregularRegion->points[anIrregularRegion->num_points]->X = (int )ROUND(anIrregularRegion->points[anIrregularRegion->num_points]->x);
			anIrregularRegion->points[anIrregularRegion->num_points]->Y = (int )ROUND(anIrregularRegion->points[anIrregularRegion->num_points]->y);
			anIrregularRegion->points[anIrregularRegion->num_points]->Z = myRois[numRois]->slice;

			/* these will be calculated later - using rois_calculate() */
			anIrregularRegion->points[anIrregularRegion->num_points]->dx	=
			anIrregularRegion->points[anIrregularRegion->num_points]->dy	=
			anIrregularRegion->points[anIrregularRegion->num_points]->slope	=
			anIrregularRegion->points[anIrregularRegion->num_points]->r	=
			anIrregularRegion->points[anIrregularRegion->num_points]->theta =
			anIrregularRegion->points[anIrregularRegion->num_points]->dr	=
			anIrregularRegion->points[anIrregularRegion->num_points]->dtheta= 0.0;

			anIrregularRegion->points[anIrregularRegion->num_points]->p = myRois[numRois];

			anIrregularRegion->num_points++;
		}
		;
elliptical_region_data:
	/* assuming they come in these order in the input file */
	EX0 ASSIGN number
	EY0 ASSIGN number
	EA  ASSIGN number
	EB  ASSIGN number
	ROT ASSIGN number
	{
		anEllipticalRegion->ex0 = atof($3);
		anEllipticalRegion->ey0 = atof($6);
		anEllipticalRegion->ea  = atof($9);
		anEllipticalRegion->eb  = atof($12);
		anEllipticalRegion->rot = atof($15);
	}
	;
rectangular_region_data:
	/* assuming they come in these order in the input file */
	X0 ASSIGN number
	Y0 ASSIGN number
	WIDTH ASSIGN number
	HEIGHT ASSIGN number
	{
		aRectangularRegion->x0 = atof($3);
		aRectangularRegion->y0 = atof($6);
		aRectangularRegion->width = atof($9);
		aRectangularRegion->height = atof($12);
	}
	;
number:
	NUMBER { $$ = strdup($1); }
	;
end_region:
	END_REGION
	;
	
string:
	STRING { $$ = strdup($1); }
	;

%%
int yyerror(char *s) {
	printf("%s at line %d, at/before '%s'\n",s, lex_lineNumber, lex_currentToken);
	exit(1);
}
int yywrap(){ return 1; }

/* main file is no longer here, this is going to be a library,
   so link any code you have to this library and call doParse(char *filename) */
/*
int main(int argc, char **argv) {
	int	exitCode;
	char	*inputFilename = "program.txt";

	myAppName = strdup(argv[0]);

	if( (yyin=openInputfile(inputFilename)) == NULL ){
		fprintf(stderr, "%s : could not open input file '%s'.\n", argv[0], inputFilename);
		exit(1);
	}

	if( (exitCode=yyparse()) != 0 ){
		fprintf(stderr, "%s : parse failed.\n", argv[0]);
		closeInput(yyin);
		exit(exitCode);
	}
	closeInput(yyin);
	exit(exitCode);
}
*/
FILE	*openInputfile(char *name){ return fopen(name, "r"); }
void	closeInput(FILE *fh){ fclose(fh); }

/* main function for other programs need to do parsing of files of the oingo language */
/* filename can be null, then it will read from stdin */
/* returns 0 on success, else failure */
int	read_rois_from_file(char *filename, roi ***ret_myRois, int *ret_numRois){
	int	exitCode;

	myAppName = strdup("read_rois_from_file (Grammar.y)");
	if( filename == NULL ){ yyin = stdin; return 0; }
	if( (yyin=openInputfile(filename)) == NULL ){
		fprintf(stderr, "%s : could not open input file '%s'.\n", myAppName, filename);
		return(1);
	}

	if( (myRois=(roi **)malloc(ROI_ALLOC_STEP * sizeof(roi *))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for myRois.\n", myAppName, ROI_ALLOC_STEP * sizeof(roi *));
		closeInput(yyin);
		return(1);
	}
	numRois = 0;
	if( (exitCode=yyparse()) != 0 ){
		fprintf(stderr, "%s : parse failed for '%s'.\n", myAppName, filename);
		closeInput(yyin);
		return(exitCode);
	}

	*ret_myRois = myRois;
	*ret_numRois = numRois;

	closeInput(yyin);
	return(exitCode);
}
void	yyexit(int code){ exit(code); }
