%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define YYSTYPE double

/* defined in the lexer, line number */
extern	int	lex_lineNumber;

/* the input file handle, property of the lexer */
extern	FILE	*yyin;

/* routines to open and close a program file */
FILE	*openInputfile(char *);
void	closeInput(FILE *);
YYSTYPE	*parse_histogram_data(char *, int *);

/* var to store data */
YYSTYPE	*dataRead;

/* malloc and remalloc will be done for MALLOC_SIZE numbers at a time, when these are read, another malloc will be done */
#define	MALLOC_SIZE	50

int	numbersReadSinceLastMalloc,	/* how many numbers since last malloc */
	mallocBlocks,			/* how many times have we remalloced (including initial malloc) ? */
	totalNumbersRead;		/* how many numbers read so far in total */

void		yyexit(int);
int		yyerror(char *s);
extern	int	yylex(void);
%}

%token	H_NUMBER
%token	H_COMMENT

%%

numbers:
	number {
		/* first time */
		if( (dataRead=(YYSTYPE *)malloc(MALLOC_SIZE*sizeof(YYSTYPE))) == NULL ){
			fprintf(stderr, "readers/histogram/parse_histogram_data : could not allocate %lu bytes.\n",  MALLOC_SIZE*sizeof(YYSTYPE));
			closeInput(yyin);
			/* this should return an error, so make sure whoever calls this gets 1=error */
			return 1;
		}
		mallocBlocks = 1;
		totalNumbersRead = numbersReadSinceLastMalloc = 1;
		dataRead[totalNumbersRead] = $1;
		$$ = $1;
	}
	|
	numbers number {
		if( ++numbersReadSinceLastMalloc >= MALLOC_SIZE ){
			mallocBlocks++;
			/* we have exhausted this block, we will remalloc */
			if( (dataRead=(YYSTYPE *)realloc(dataRead, mallocBlocks * MALLOC_SIZE * sizeof(YYSTYPE))) == NULL ){
				fprintf(stderr, "readers/histogram/parse_histogram_data : could not re-allocate for %lu bytes - %d numbers read so far.\n", mallocBlocks * MALLOC_SIZE * sizeof(YYSTYPE), totalNumbersRead);
				return(1);
			}
			numbersReadSinceLastMalloc = 0;
		}
		dataRead[totalNumbersRead++] = $2;
		$$ = $2;
	}
	;

number:
	H_NUMBER	{ $$ = $1; }
	;
%%
int yyerror(char *s) {
	printf("%s at line %d\n", s, lex_lineNumber);
	exit(1);
}
int yywrap(){ return 1; }

/* main file is no longer here, this is going to be a library,
   so link any code you have to this library and call parse_histogram_data(char *filename) */

FILE	*openInputfile(char *name){ return fopen(name, "r"); }
void	closeInput(FILE *fh){ fclose(fh); }

/* main function for other programs need to do parsing of files of the oingo language */
/* filename can be null, then it will read from stdin */
/* returns 0 on success, else failure */
YYSTYPE	*parse_histogram_data(char *filename, int *numNumbers){
	int	exitCode;

	/* update global variables */
	if( filename == NULL ){ yyin = stdin; return 0; }
	if( (yyin=openInputfile(filename)) == NULL ){
		fprintf(stderr, "readers/histogram/parse_histogram_data : could not open input file '%s'.\n", filename);
		return NULL;
	}
	if( (exitCode=yyparse()) != 0 ){
		fprintf(stderr, "readers/histogram/parse_histogram_data : parse failed for '%s'.\n", filename);
		closeInput(yyin);
		return NULL;
	}
	closeInput(yyin);
	*numNumbers = totalNumbersRead;
	if( (dataRead=(YYSTYPE *)realloc(dataRead, totalNumbersRead * sizeof(YYSTYPE))) == NULL ){
		fprintf(stderr, "readers/histogram/parse_histogram_data : could not readjust data pointer to fit %lu bytes.\n", totalNumbersRead * sizeof(YYSTYPE));
		return NULL;
	}
	return(dataRead);
}
void	yyexit(int code){ exit(code); }
