#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <time.h>

const	char	Examples[] = "\
	-r \"/usr/tmp/\" -o \".txt\"\
\
will produce the string '/usr/tmp/1827717171.txt'. If\
the same command is run for a second time, the number\
portion of the output string above will be different\
because it is ... random!\
\
	-r \"/usr/tmp/\" -o \".txt\" -S 1974\
\
will produce the same string as above BUT, if you\
run the same command again, the same string will\
be produced, including the same number (provided the\
same seed (-S 1974) was used. Why do you want to have\
a program to do that? Perhaps because you want\
reproducible results. See example below as well.\
\
	-r \"/usr/tmp/\" -o \".txt\" -S 1974 -n 15	\
\
will output 15 strings of the form '/usr/tmp/XXX.txt'\
where XXX is a number which will vary from string to string.\
Every time you run the command with the same seed, the\
same sequence of random names will be produced.\
\
	-r \"/usr/tmp/\" -o \".txt\" -S 1974 -n 15 -s 2\
\
will output 15 strings as before but the first filename of\
this list will be the same as the third number of the\
list of the previous example -- that is, two filenames\
have been skipped.\
\
consider the following c-shell script:\
\
-------\
#!/bin/csh -f\
\
set index = 1\
while( $index <= 15 )\
	set filename = `ProduceRandomFilename -r '/usr/tmp/' -o '.txt' -S 1974 -s $index -n 1`\
	echo $filename\
	@ index = $index + 1\
end\
-------\
\
The above script will produce 15 random filenames everytime it is run.\
\
*** NOTE : if you use this program without a seed, then a random filename\
different every time (within the odds of probability) should be produced.\
When no seed is supplied, the current time is used as seed. But because\
the current time resolution is 1 sec, then if you run the command twice\
in 1 second, the same seed will be used and therefore the same filename\
will be used. For example try:\
\
ProduceRandomFilename ; ProduceRandomFilename\
\
If your system is fast, then the same filename will be produced.\
So better do:\
ProduceRandomFilename ; sleep 1 ; ProduceRandomFilename\
";

const	char	Usage[] = "options as follows:\
\t[-r prefix]\
	(string to prefix the random number)\
\t[-o postfix]\
	(string to postfix the random number)\
\t[-n howmany]\
	(how many random number files to give?)\
\t[-s skip_howmany]\
	(how many random number files to skip before start\
	 printing?)\
\t[-S seed]\
	(seed to feed the random number generator with,\
	 if not supplied, current time is used.\
	 If supplied, then it is quaranteed that this\
	 program will give the same sequence of random files\
	 any time the same seed is supplied. By playing\
	 with the '-s' and '-n' options, you can get\
	 predicted sequences of filenames.)\
\
This program will produce a string containing\
a random number every time it is called. It is\
useful in situations when random filenames are\
needed for storing temporary data etc.\
\
Output is dumped to the stdout.";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	char		*postfix = NULL, *prefix = NULL;
	int		howmany = 1, skip_howmany = 0,
			Seed = (int )time(0), i;
	int		optI;

	while( (optI=getopt(argc, argv, "r:o:n:s:S:eh")) != EOF)
		switch( optI ){
			case 'r': prefix = strdup(optarg); break;
			case 'o': postfix= strdup(optarg); break;
			case 'n': howmany= atoi(optarg); break;
			case 's': skip_howmany = atoi(optarg); break;
			case 'S': Seed = atoi(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			case 'h': fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	srand48(Seed);

	if( skip_howmany < 0 ){
		fprintf(stderr, "%s : the parameter to the '-s' option must be a positive integer.\n", argv[0]);
		exit(1);
	}
	if( howmany <= 0 ){
		fprintf(stderr, "%s : the parameter to the '-n' option must be a positive integer.\n", argv[0]);
		exit(1);
	}
	for(i=0;i<skip_howmany;i++) lrand48();

	for(i=0;i<howmany;i++){
		if( prefix != NULL ) fprintf(stdout, "%s", prefix);
		fprintf(stdout, "%ld", lrand48());
		if( postfix!= NULL ) fprintf(stdout, "%s", postfix);
		fprintf(stdout, "\n");
	}
	if( prefix != NULL ) free(prefix);
	if( postfix!= NULL ) free(postfix);	
	exit(0);
}
