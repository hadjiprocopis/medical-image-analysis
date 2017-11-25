/* AHP MODS:

some wrote:
tmp = (Dim_info) malloc(sizeof(long) + 1);
or even
tmp = (Dim_info) malloc(sizeof(Dim_info) + 1);

since
Dim_info = pointer to struct info_rec

then it should be:
tmp = (Dim_info) malloc(sizeof(struct info_rec) + 1);

these are serious bugs because they do not show in solaris
*/


/* @(#)iminfo.c	1.2 7/27/90 */
/*
 * File:	iminfo.c
 * 
 * Author	Dave Wicks
 * 
 * Date:	16/8/89
 * 
 * Comments:	Extension to the UNC image processing library format with
 * modified info fields.
 * 
 * Modification history:
 *	dlp 	10 Jan 90  Ran through indent(1) 
 *	dlp 	11 Jan 90  Added status returns to imget[dim]info and removed
 *				error printf
 *	dlp	 1 Feb 90  Fixed bug? in put_info
 *      mah     19 Oct 96  Added check for successful mallocs because
 *                         failed mallocs causing programs to crash in 
 *			   unexpected ways. Fixed memory leak in get_info().
 *			
 *
 * Routines:
 * 
 * IMAGE FILE (DISK) <-> IMAGE DATA STUCTURE (MEMORY) ROUTINES
 * 
 * read_info	:	Reads image file info into the image info
 * datastructure in memory. ( Called by imopen(). )
 * 
 * write_info	:	Writes image info data in memory to info field in
 * image file. ( Called by imclose(). )
 * 
 * get_list	:	Gets list of name-value pairs. ( called by
 * read_info(). )
 * 
 * put_list	:	Puts list of name-value pairs. ( called by
 * write_info(). )
 * 
 * page *
 * IMAGE DATA STRUCTURE ACCESS ROUTINES
 * 
 * imgetinfo	:	Reads general file info.
 * 
 * imputinfo	:	Writes general file info.
 * 
 * imgetdiminfo:	Reads info for an element of a specific dimension.
 * 
 * imputdiminfo:	Writes info for an element of a specific dimension.
 * 
 * get_info	:	Reads info from linked Info list. (called by
 * imgetinfo & imgetdiminfo)
 * 
 * put_info	:	Writes info to linked Info list. (called by imputinfo
 * & imputdiminfo)
 * 
 * read_name	:	Reads name of info from the info string
 * 
 * read_val	:	Reads value of info from the info string
 * 
 * write_name	:	Writes name of info to the info string.
 * 
 * write_val	:	Writes value of info to the info string
 * 
 * printinfo	:	Prints info field contents to standard output.
 * 
 * copyinfo	:	Copies info fields from one image to another. NB. It
 * overwrites any existing info in the destination image.
 * 
 * copy_list	:	Copies an Info field (Called by copyinfo).
 * 
 * page *
 * 
 * DATA DICTIONARY ROUTINES:
 * 
 * open_dic	:	Opens specified dictionary.  This reads the ascii
 * dictionary file, with its include files, and stores the entries in an
 * array for fast access.
 * 
 * check_dic	:	Checks a name is in a data dictionary and returns its
 * dictionary entry.	NB. This is a utility for the applications programmer
 * who is responsible for the type conversion which can carried out with
 * sscanf and sprintf.
 * 
 *
 * ODF Compatability:
 *
 * trunc_str :  Strips off non alphanumeric characters from  the
 * 				end of a string.
 *
 * read_odf :	reads odf for picker scan information and writes it
 *				to the general info field.
 * page * */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include	"image.h"	/* includes iminfo.h */
#define 	FROMBEG		0
#define 	FROMEND		2

void	create_info(im)
	IMAGE          *im;
{
	int             i, j;
	Dim_info        tmp;

	im->file_info = (Info *) malloc(sizeof(Info) + 1);
	if (!im->file_info) {
		fprintf(stderr, "Malloc failed in create_info\n");
		return;
	}
	im->file_info->nameval[0] = '\0';
	im->file_info->next = NULL;

	/* Create info for each element of each Dimension */
	for (i = 0; i < im->Dimc; i++) {
		im->pDimv[i] = (Dim_info) malloc(im->Dimv[i] * sizeof(struct info_rec));
		if (!im->pDimv[i]) {
			fprintf(stderr, "Malloc failed in create_info\n");
			return;
		}
		for (j = 0; j < im->Dimv[i]; j++) {
			/* mods AHP you can't have:
				tmp = (Dim_info) malloc(sizeof(struct info_rec)+1);
				sizeof(long) = 4 bytes and sizeof(struct info_rec) = 136 bytes
				note: Dim_info = struct info_rec *
				printf("%ld %ld\n", sizeof(long), sizeof(struct info_rec));
			*/
			tmp = (Dim_info) malloc(sizeof(struct info_rec) + 1);
			if (!tmp) {
				fprintf(stderr, "Malloc failed in create_info\n");
				return;
		}
			tmp->nameval[0] = '\0';
			tmp->next = NULL;
			*((Dim_info *) im->pDimv[i] + j) = tmp;
		}		/* for j */
	}			/* for i */

}

/* page * */

void	read_info(info_buf, im)
     char           *info_buf;
     IMAGE          *im;
{
  int             length;
  int             i, j;

  /* Read file_info field */
  /* length is the length of the string without a terminator */
  length = strlen(info_buf);
  get_list(info_buf, length, &(im->file_info));
  /* Increment the buffer to the next string, including any terminator */
  info_buf += length + 1;

  /* Read info for each element of each Dimension */
  for (i = 0; i < im->Dimc; i++) {
    im->pDimv[i] = (Dim_info) malloc(im->Dimv[i] * sizeof(struct info_rec));
    if (!im->pDimv[i]) {
	fprintf(stderr, "Malloc failed in read_info\n");
	return;
    }
    for (j = 0; j < im->Dimv[i]; j++) {
      length = strlen(info_buf);
      get_list(info_buf, length, ((Dim_info *) im->pDimv[i] + j));
      if (length != 0) info_buf += length + 1;
    }				/* for j */
  }				/* for i */

}				/* read_info */


/* page * */

void	write_info(im, info_buf, total_length)
	IMAGE          *im;
	char          **info_buf;
	int            *total_length;
{
	/* void	put_list(); */
	char           *buf_start;
	int             buffer_size;
	int             length;
	int             i, j;

	/*
	 * Allocate fileinfo buffer allowing 12 for file_info and 2k for
	 * other fields
	 */
	buffer_size = 12288; /* Increased initial size of buffer from 4096 - GJB 17/10/2000 */
	for (i = 0; i < im->Dimc; i++)
		buffer_size += (im->Dimv[i]) * 2048; /* Doubled this from 1024 - GJB 17/10/2000 */
	buf_start = *info_buf = (char *) malloc(buffer_size);
    	if (!buf_start) {
		fprintf(stderr, "Malloc failed in write_info\n");
		return;
    	}

	/* Write file_info field */
	put_list(im->file_info, *info_buf);
	/*free(im->file_info); Not needed - freed by put_list?? GJB 17/10/2000 */
	im->file_info = NULL;
	*total_length = length = strlen(*info_buf) + 1;
	*info_buf += length;

	/* Write info for each element of each Dimension */
	for (i = 0; i < im->Dimc; i++) {
		for (j = 0; j < im->Dimv[i]; j++) {
			put_list(*((Dim_info *) im->pDimv[i] + j), *info_buf);
			/*free(*((Dim_info *) im->pDimv[i] + j)); Not needed - freed by put_list?? GJB 17/10/2000 */
			length = strlen(*info_buf) + 1;
			*info_buf += length;
			*total_length += length;
			/*
			 * If the buffer is over 90% full then double its
			 * size
			 */
			if (*total_length > buffer_size * 0.9) {
				buf_start = (char *) realloc(buf_start, buffer_size * 2);
				*info_buf = buf_start + *total_length;
			}
		}		/* for j */
/* AHP : mistake */
//		free(im->pDimv[i]);
/* end AHP */
		im->pDimv[i] = NULL;
	}			/* for i */
	*info_buf = buf_start;
}				/* write_info */


/* page * */

void	get_list(info_buf, length, info)
	char           *info_buf;
	int             length;
	Info          **info;
{
  char           *tmp_str;	/* Temporary name-value buffer */
  Info           *ptr, *tmp_ptr; /* Temporary Info pointers */
  int             cnt, size;	/* string character positions */

  if (length == 0) {
    *info = (Info *) malloc(sizeof(Info) + 1);
    if (!*info) {
	fprintf(stderr, "Malloc failed in get_list\n");
	return;
    }
    (*info)->nameval[0] = '\0';
    (*info)->next = NULL;
    return;
  }
  cnt = 0;
  size = 0;
  tmp_str = (char *) malloc(length + 1);
  if (!tmp_str) {
	fprintf(stderr, "Malloc failed in get_list\n");
	return;
  }
  /* Copy name-value string from the 1st line of the buffer */
  while ((tmp_str[size] = info_buf[cnt + size]) != '\n' && size < length)
    size++;
 /* XXXXX is this the bug?*/
 /*size = 1;*/
  tmp_str[size] = '\0';
  /* Allocate space for 1st name-value in list */
  *info = ptr = (Info *) malloc(sizeof(Info) + size + 1);
  if (!*info) {
	fprintf(stderr, "Malloc failed in get_list\n");
	return;
  }
  strcpy(ptr->nameval, tmp_str);
  ptr->next = NULL;
  cnt += size + 1;

  while (cnt < length - 1) {
    size = 0;
    /* Copy name-value string from next line of the buffer */
    while ((tmp_str[size] = info_buf[cnt + size]) != '\n' &&
	   (cnt + size) < length)
      size++;
    tmp_str[size] = '\0';
    /* Allocate space for next name-value in list */
    tmp_ptr = (Info *) malloc(sizeof(Info) + size + 1);
    if (!tmp_ptr) {
	fprintf(stderr, "Malloc failed in get_list\n");
	return;
    }
    strcpy(tmp_ptr->nameval, tmp_str);
    tmp_ptr->next = NULL;
    /* link previous name-value to current one */
    ptr->next = tmp_ptr;
    /* update previous name-value pointer */
    ptr = tmp_ptr;
    cnt += size + 1;
  }				/* while */

free(tmp_str);
}				/* get_list */


/* page * */

void	put_list(info, info_buf)
	Info           *info;
	char           *info_buf;	/* NB Must be big enough for info
					 * list */
{
	Info           *tmp_ptr;/* Temporary Info pointer */
	int             cnt, size;	/* string character positions */

	cnt = 0;
	while (info != NULL) {
		size = 0;
		while ((info_buf[cnt + size] = info->nameval[size]) != '\0')
			size++;
		info_buf[cnt + size] = '\n';
		tmp_ptr = info;
		info = info->next;
		free(tmp_ptr);
		cnt += size + 1;
	}
	info_buf[cnt - 1] = '\0';

}				/* put_list */


/* page * */
int	imgetinfo(im, name, value)
	IMAGE          *im;
	char           *name, *value;
{
	return(get_info(im->file_info, name, value));
}

int	imputinfo(im, name, value)
	IMAGE          *im;
	char           *name, *value;
{
	return	put_info(&(im->file_info), name, value);
}

int
imgetdiminfo(im, dim, n, name, value)
	IMAGE          *im;
	int             dim, n;
	char           *name, *value;
{
	return(get_info(*((Dim_info *) im->pDimv[dim] + n), name, value));
}


int	imputdiminfo(im, dim, n, name, value)
	IMAGE          *im;
	int             dim, n;
	char           *name, *value;
{
	return put_info(((Dim_info *) im->pDimv[dim] + n), name, value);
}

/* page * */

int
get_info(ptr, name, value)
	Info           *ptr;
	char           *name, *value;
{
	char           *checkname;

	while (ptr != NULL) {
		checkname = (char *) malloc(strlen(ptr->nameval));
		if (!checkname) {
			fprintf(stderr, "Malloc failed in get_info\n");
			return(0);
		}
		read_name(ptr->nameval, checkname);
		if (!strcmp(name, checkname)) {
			free(checkname);
			read_val(ptr->nameval, value);
			return (1);
		}
		free(checkname);
		ptr = ptr->next;
	}
/*	fprintf(stderr, "get_info: name not found.\n"); */
	return (0);
}

/* function to read all name/value pairs of an Info and
   copy them onto an array to be returned.
	use:
	char	**names, **values;
	int	numEntries, i;
	if( get_info_pairs(im->info, names, values, &numEntries) == 0 ) exit;
	for(i=0;i<numEntries;i++)
		printf("%s = %s\n", names[i], values[i]);
	// do not forget to free when finished
	for(i=0;i<numEntries;i++){ free(names[i]); free(values[i]); }
	free(names); free(values);

	returns -1 on failure,
		the number of entries (name/value pairs) found on success
	
	Author : Andreas Hadjiprocopis , ION 2003, CING 2005
*/
int
get_info_pairs_into_array(info, names, values)
	Info           *info;
	char           ***names, ***values;
{
	Info		*ptr = info;
	int		numEntries, i;

	for(ptr=info,numEntries=0;ptr!=NULL;ptr=ptr->next,numEntries++);

	if( (*names=(char **)malloc(numEntries*sizeof(char *))) == NULL ){
		fprintf(stderr, "get_info_pairs_into_array : could not allocate %ld bytes for names (%d entries).\n", numEntries*sizeof(char *), numEntries);
		return -1;
	}
	if( (*values=(char **)malloc(numEntries*sizeof(char *))) == NULL ){
		fprintf(stderr, "get_info_pairs_into_array : could not allocate %ld bytes for values (%d entries).\n", numEntries*sizeof(char *), numEntries);
		free(names);
		return -1;
	}
	for(ptr=info,i=0;ptr!=NULL;ptr=ptr->next){
		if( read_name_value(ptr->nameval, &((*names)[i]), &((*values)[i])) == 1 ){
			/* success */
			i++;
		} else
			fprintf(stderr, "get_info_pairs_into_array : ignoring empty entry\n");
	}
	return i;
}

/* page * */

int
put_info(ptr_addr, name, value)
	Info          **ptr_addr;
	char           *name, *value;
{
	char           *checkname;
	Info           *ptr, *new_ptr, *prev;

	ptr = *ptr_addr;
	prev = NULL;

	while (ptr != NULL) {
		/* Allow for the very first time this is called, when 
		nameval will be null.  We must make checklength non-zerolength
		so that read_name can write a 'null' to it.  GJB 17/10/2000 */
		if (strlen(ptr->nameval) > 0)
			checkname = (char *) malloc(strlen(ptr->nameval));
		else
			checkname = (char *) malloc(1);
		if (!checkname) {
			fprintf(stderr, "Malloc failed in put_info\n");
			return(0);
		}
		read_name(ptr->nameval, checkname);
		if (!strcmp(name, checkname) || !strcmp("", checkname)) {
			/*checkval is never actually used?? GJB 17/10/2000 */
			/*checkvalue = (char *) malloc(strlen(ptr->nameval));
			if (!checkvalue) {
				fprintf(stderr, "Malloc failed in put_info\n");
				return(0);
			}
			read_val(ptr->nameval, checkvalue);
			free(checkvalue);*/
			/* dlp 1 feb 90 changed next block */
			if (strlen(value)+strlen(name) < strlen(ptr->nameval)) {
				write_name(name, ptr->nameval);
				write_val(value, ptr->nameval);
			}
			else {
				new_ptr = (Info *) malloc(sizeof(Info) + strlen(name) + 2 + strlen(value) + 1);
				if (!new_ptr) {
					fprintf(stderr, "Malloc failed in put_info\n");
					return(0);
				}
				write_name(name, new_ptr->nameval);
				write_val(value, new_ptr->nameval);
				new_ptr->next = ptr->next;
				if (prev == NULL)
					*ptr_addr = new_ptr;
				else {
					prev->next = new_ptr;
					free(ptr);
				}
			}
			free(checkname);
			return (1);
		}
		prev = ptr;
		ptr = ptr->next;
		free(checkname);
	}
	new_ptr = (Info *) malloc(sizeof(Info) + strlen(name) + 2 + strlen(value) + 1);
	if (!new_ptr) {
		fprintf(stderr, "Malloc failed in put_info\n");
		return(0);
	}
	write_name(name, new_ptr->nameval);
	write_val(value, new_ptr->nameval);
	new_ptr->next = NULL;
	if (prev != NULL)
		prev->next = new_ptr;
	return (2);
}


/* page * */

void	read_name(nameval, name)
	char           *nameval;
	char           *name;
{
	int i;
	i = 0;
	while (nameval[i] != '=' && nameval[i] != '\0') {
		name[i] = nameval[i];
		i++;
	}
	name[i] = '\0';
}
/* function to read name and value from a namevalue string (e.g. "name=value")
   a little bit more efficient.
   You need to free name and value when finished with them

   returns 1 on success or 0 on failure (e.g. no valid header)
   
   Author : Andreas Hadjiprocopis, ION 2003, CING 2005
*/
int	read_name_value(nameval, name, value)
	char           *nameval;
	char           **name;
	char	       **value;
{
	char	*dummy, *p;

	if( nameval[0] == '\0' ) return 0;

	if( (dummy=strdup(nameval)) == NULL ){
		fprintf(stderr, "read_name_value : strdup failed for dummy (nameval='%s')\n", nameval);
		return 0;
	}
	if( (p=strchr(dummy, '=')) == NULL ){
		fprintf(stderr, "read_name_value : malformed UNC header : could not find '=' in nameval (='%s')\n", nameval);
		free(dummy);
		return 0;
	}
	*value = strdup(++p);
	*(--p) = '\0';
	*name = strdup(dummy);
	free(dummy);
//	printf("XX nameval='%s' |%s|, |%s| (%p, %p)\n", nameval, *name, *value, *name, *value);
	return 1;
}


void	read_val(nameval, val)
	char           *nameval;
	char           *val;
{
	int i,j;
	i = j = 0;

	while ((nameval[i]) != '=') {
		if (nameval[i] == '\0') {
			val[0] = '\0';
			return;
		}
		i++;
	}
	i++;
	while ((val[j] = nameval[i]) != '\0') {
		i++;
		j++;
	}
}

/* page * */

void	write_name(name, nameval)
	char           *name;
	char           *nameval;
{
	int             i;
	i = 0;
	while ((nameval[i] = name[i]) != '\0')
		i++;
	nameval[i] = '=';
}


void	write_val(val, nameval)
	char           *val;
	char           *nameval;
{
	int             i, j;
	i = j = 0;

	while ((nameval[i]) != '=')
		i++;
	i++;
	while ((nameval[i] = val[j]) != '\0') {
		i++;
		j++;
	}
}

/* page * */

void	printinfo(im)
	IMAGE          *im;
{
	Info           *ptr;
	int             i, j;
	int             foundinfo;

	/* Print out general file info */
	printf("General file information :");
	ptr = im->file_info;
	if (!strcmp(ptr->nameval, ""))
		printf("\tNone.\n");
	else {
		printf("\n");
		while (ptr != NULL) {
			printf("\t\t\t\t%s\n", ptr->nameval);
			ptr = ptr->next;
		}		/* while */
	}			/* else */

	/* Print out dimension info */
	for (i = 0; i < im->Dimc; i++) {
		printf("\nDimension %d information:", i);
		foundinfo = 0;
		for (j = 0; j < im->Dimv[i]; j++) {
			ptr = *((Dim_info *) im->pDimv[i] + j);
			if (strcmp(ptr->nameval, "") != 0) {
				foundinfo++;
				printf("\nElement %d :", j);
				while (ptr != NULL) {
					printf("\n\t\t\t%s", ptr->nameval);
					ptr = ptr->next;
				}
			}	/* if */
		}		/* for j */
		if (foundinfo == 0)
			printf("\tNone.\n");
		printf("\n");
	}			/* for i */


}				/* printinfo */


/* page * */

void	copyinfo(im1, im2)
	IMAGE          *im1, *im2;
{
	int             i, j;

	/* Check images are the same size */
	if (im1->Dimc != im2->Dimc) {
		printf("copyinfo: Images of different dimensions.\n");
		return;
	}
	copy_list(im1->file_info, &(im2->file_info));

	for (i = 0; i < im1->Dimc; i++)
		if (im1->Dimv[i] != im2->Dimv[i]) {
			printf("copyinfo: Image dimensions are different sizes.\n");
			printf("copyinfo: Cannot copy information for dimension %d.\n", i);
		} else
			for (j = 0; j < im1->Dimv[i]; j++)
				copy_list(*((Dim_info *) im1->pDimv[i] + j), ((Dim_info *) im2->pDimv[i] + j));

}				/* copyinfo */


void	copy_list(ptr1, ptr2_addr)
	Info           *ptr1;
	Info          **ptr2_addr;
{
	Info           *ptr2;

	*ptr2_addr = ptr2 = (Info *) malloc(sizeof(Info) + strlen(ptr1->nameval) +1);
	if (!ptr2) {
		fprintf(stderr, "Malloc failed in copy_list\n");
		return;
	}
	strcpy(ptr2->nameval, ptr1->nameval);
	ptr2->next = NULL;
	ptr1 = ptr1->next;

	while (ptr1 != NULL) {
		ptr2->next = (Info *) malloc(sizeof(Info) + strlen(ptr1->nameval) +1);
		if (!ptr2->next) {
			fprintf(stderr, "Malloc failed in copy_list\n");
			return;
		}
		ptr2 = ptr2->next;
		ptr2->next = NULL;
		strcpy(ptr2->nameval, ptr1->nameval);
		ptr1 = ptr1->next;
	}

}				/* copy_list */

/* imcopyinfo to support UNC programs */

void	imcopyinfo(im1, im2)
IMAGE          *im1, *im2;
{ 
copyinfo(im1,im2);
}

/* page * */


Dic_rec        *
open_dic(dic_file_name)
	char           *dic_file_name;
{
	FILE           *fp;
	char            command[80];
	char            type_str[8], access_str[8], descr_str[80];
	int             cnt;
	Dic_rec        *dic, *first_dic;
	int             dic_size;
	int             i;

	sprintf(command, "/usr/lib/cpp -B %s | grep -v '^#'", dic_file_name);
	if ((fp = (FILE *) popen(command, "r")) == NULL) {
		fprintf(stderr, "open_dic: Cannot open %s.\n", dic_file_name);
		exit(1);
	}
	dic_size = sizeof(Dic_rec) * 64;
	first_dic = dic = (Dic_rec *) malloc(dic_size);
	if (!dic) {
		fprintf(stderr, "Malloc failed in open_dic\n");
		return((Dic_rec *) NULL);
	}

	i = 0;
	while ((cnt = fscanf(fp, "%s\t%s\t%s\t", dic->name, type_str, access_str)) != EOF) {
		if (cnt != 3) {
			fprintf(stderr, "open_dic: Error in %s at line %d.\n", dic_file_name, i + 1);
			exit(1);
		}		/* if */
		/* n read data type */
		if (!strcmp(type_str, "INT"))
			dic->type = 1;
		else if (!strcmp(type_str, "FLOAT"))
			dic->type = 2;
		else if (!strcmp(type_str, "DOUBLE"))
			dic->type = 3;
		else if (!strcmp(type_str, "STRING"))
			dic->type = 4;
		else if (!strcmp(type_str, "BOOL"))
			dic->type = 5;
		else {
			fprintf(stderr, "open_dic: Error in %s at line %d.\n", dic_file_name, i + 1);
			exit(1);
		}		/* else */

		/* access code to be implemented */

		/* read description string */
		if (fgets(descr_str, 80, fp) == NULL) {
			fprintf(stderr, "open_dic: Error in %s at line %d.\n", dic_file_name, i + 1);
			exit(1);
		}		/* if */
		descr_str[strlen(descr_str) - 1] = '\0';
		strcpy(dic->description, descr_str);

		/* check dic buffer is still big enough */
		if (i == dic_size - 1) {
			dic_size = dic_size * 2;
			dic = (Dic_rec *) realloc(dic, dic_size);
			dic += i;
		}		/* if */
		dic++;
		i++;
	}			/* while */

	/* set up last dic */
	strcpy(dic->name, "END_OF_DIC");

	return (first_dic);

}


Dic_rec        *
check_dic(dic, check_name)
	Dic_rec        *dic;
	char           *check_name;
{
	int             i;

	while (strcmp(dic->name, "END_OF_DIC") != 0) {
		if (!strcmp(dic->name, check_name))
			return (dic);
		dic++;
		i++;
	}			/* while */

	fprintf(stderr, "check_dic: %s not found in dictionary.\n", check_name);
	return (NULL);
}

/* page * */

char    *
trunc_str(str,length)
char    *str;
int     length;
{
int     i = 0;
i=length-1;
while( (i>=0) && !isalnum((int )(str[i])) ) i--;
str[i+1]='\0';
return(str);
}

typedef	struct { 
	char	checkstr[8];
	char	name[24];
	int		dummy[8];
	}	ODF;

typedef struct
{
	char	patient_name[40];
    char	patient_ID[12];
	char	sex[4];
    char	age[4];
    char	consultant[40];
    int		study_number;
    char	operator_ID[12];
	char	date[8];
 	char	time[8];
	char	comments[4][52];
 	char	hospital_name[16];
	int		protocol_number;
	char	protocol_name_1[24];
	char	protocol_name_2[32];
  	int		Tr;
	int		minimum_Tr;
    int		maximum_Tr;
	char	slice_orientation[4];
    int		slice_offset;
    int		repetitions;
    int		number_of_views;
    char	patient_orientation_1[4];
	char	patient_orientation_2[4];
	int		filter_function;
	int		number_of_slices;
	int		field_of_view;			
	char	gating[4];				
	int		gating_delay;
	char	sequence[8];
	char	RF_pulse[8];
	char	sampling[8];
	char	frequency_offset[8];
	char	phase_encoding[8];
	char	read_profile[8];
	char	slice_select[8];
	int		samples_per_view;
	char	phase_enc_direction[4];
	char	two_sided_echo[4];
	int		reconstruction_type;
	int		attenuation;
	int		bandwidth;
	int		image_matrix;
	char	gating_allowed[4];
}	PICKER_SCAN_INFO;

void	read_odf(Buffer, im)
char	*Buffer;
IMAGE	*im;

{
char						*tmp_ptr;
ODF							*odf;
PICKER_SCAN_INFO			*psi;
char						tmp_str[256];

/* Read ODF and check it is valid and of type "pickerinfo" */
tmp_ptr=Buffer+4; /* jump odfCnt */
odf=(ODF *)tmp_ptr;
if (strcmp(odf->checkstr,"~~~~~~~")){
	fprintf(stderr,"Illegal ODF found.\n");
	exit(1);
}
if (strcmp(odf->name,"pickerinfo")){
	fprintf(stderr,"Picker Scan information not found.\n");
	return;
}

/* Write picker ODF to new info structure */
tmp_ptr+=64; /* jump odf header */
psi=(PICKER_SCAN_INFO *)tmp_ptr;
imputinfo(im,"pat_name",trunc_str(psi->patient_name,40));
imputinfo(im,"pat_id",trunc_str(psi->patient_ID,12));
imputinfo(im,"sex",trunc_str(psi->sex,4));
imputinfo(im,"age",trunc_str(psi->age,4));
imputinfo(im,"consultant",trunc_str(psi->consultant,40));
sprintf(tmp_str,"%d",psi->study_number);
imputinfo(im,"study_no",tmp_str);
imputinfo(im,"op_id",trunc_str(psi->operator_ID,12));
imputinfo(im,"date",trunc_str(psi->date,8));
imputinfo(im,"time",trunc_str(psi->time,8));
strcpy(tmp_str,trunc_str(psi->comments[0],52));
strcat(tmp_str," ");
strcat(tmp_str,trunc_str(psi->comments[1],52));
strcat(tmp_str," ");
strcat(tmp_str,trunc_str(psi->comments[2],52));
strcat(tmp_str," ");
strcat(tmp_str,trunc_str(psi->comments[3],52));
imputinfo(im,"comments",tmp_str);
imputinfo(im,"hosp_name",trunc_str(psi->hospital_name,16));
sprintf(tmp_str,"%d",psi->protocol_number);
imputinfo(im,"protocol_no",tmp_str);
imputinfo(im,"prot_name1",trunc_str(psi->protocol_name_1,24));
imputinfo(im,"prot_name2",trunc_str(psi->protocol_name_2,32));
sprintf(tmp_str,"%d",psi->Tr);
imputinfo(im,"TR",tmp_str);
sprintf(tmp_str,"%d",psi->minimum_Tr);
imputinfo(im,"minTR",tmp_str);
sprintf(tmp_str,"%d",psi->maximum_Tr);
imputinfo(im,"maxTR",tmp_str);
imputinfo(im,"slice_orient",trunc_str(psi->slice_orientation,4));
sprintf(tmp_str,"%d",psi->slice_offset);
imputinfo(im,"slice_offset",tmp_str);
sprintf(tmp_str,"%d",psi->repetitions);
imputinfo(im,"reps",tmp_str);
sprintf(tmp_str,"%d",psi->number_of_views);
imputinfo(im,"nviews",tmp_str);
imputinfo(im,"pat_orient1",trunc_str(psi->patient_orientation_1,4));
imputinfo(im,"pat_orient2",trunc_str(psi->patient_orientation_2,4));
sprintf(tmp_str,"%d",psi->filter_function);
imputinfo(im,"filter",tmp_str);
sprintf(tmp_str,"%d",psi->number_of_slices);
imputinfo(im,"nslices",tmp_str);
sprintf(tmp_str,"%d",psi->field_of_view);
imputinfo(im,"FOV",tmp_str);
imputinfo(im,"gating",trunc_str(psi->gating,4));
sprintf(tmp_str,"%d",psi->gating_delay);
imputinfo(im,"gating_delay",tmp_str);
imputinfo(im,"sequence",trunc_str(psi->sequence,8));
imputinfo(im,"RFpulse",trunc_str(psi->RF_pulse,8));
imputinfo(im,"sampling",trunc_str(psi->sampling,8));
imputinfo(im,"freq_offset",trunc_str(psi->frequency_offset,8));
imputinfo(im,"phase_encoding",trunc_str(psi->phase_encoding,8));
imputinfo(im,"read_profile",trunc_str(psi->read_profile,8));
imputinfo(im,"slice_select",trunc_str(psi->slice_select,8));
sprintf(tmp_str,"%d",psi->samples_per_view);
imputinfo(im,"samples",tmp_str);
imputinfo(im,"phase_enc_dir",trunc_str(psi->phase_enc_direction,4));
imputinfo(im,"two_sided_echo",trunc_str(psi->two_sided_echo,4));
sprintf(tmp_str,"%d",psi->reconstruction_type);
imputinfo(im,"recon_type",tmp_str);
sprintf(tmp_str,"%d",psi->attenuation);
imputinfo(im,"attenuation",tmp_str);
sprintf(tmp_str,"%d",psi->bandwidth);
imputinfo(im,"bandwidth",tmp_str);
sprintf(tmp_str,"%d",psi->image_matrix);
imputinfo(im,"image_matrix",tmp_str);
imputinfo(im,"gating_allowed",trunc_str(psi->gating_allowed,4));
/*
The following were added only to the routines using the new info fields:

sprintf(tmp_str,"%5.1f",(float)psi->slice_thickness/10.0);
imputinfo(im,"slice_thickness",tmp_str);
sprintf(tmp_str,"%d",psi->tau);
imputinfo(im,"tau",tmp_str);
sprintf(tmp_str,"%d",psi->Te);
imputinfo(im,"Te",tmp_str);
*/
}
