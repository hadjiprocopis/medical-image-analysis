/* @(#)iminfo.h	1.2 7/27/90 */
#ifndef	_IMINFO_HEADER

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
#ifdef __cplusplus
#ifndef EXTERNAL_LINKAGE
#define EXTERNAL_LINKAGE        extern "C"
#endif
#else
#ifndef EXTERNAL_LINKAGE
#define EXTERNAL_LINKAGE
#endif
#endif

/* Info types used by data dictionaries */
#define	INT		1
#define	FLOAT	2
#define	DOUBLE	3
#define	STRING	4
#define	BOOL	5

struct	info_rec{
			struct info_rec	*next;
			char			nameval[132];
		};

typedef	struct info_rec		Info;
typedef	struct info_rec		*Dim_info;

struct	data_dic_rec{
			char	name[24];
			short	type;
			short	access;
			char	description[80];
		};

typedef struct	data_dic_rec	Dic_rec;

EXTERNAL_LINKAGE	void	create_info();
EXTERNAL_LINKAGE	void	read_info();
EXTERNAL_LINKAGE	void	write_info();
EXTERNAL_LINKAGE	void	get_list();
EXTERNAL_LINKAGE	void	put_list();
EXTERNAL_LINKAGE	int	imgetinfo();
EXTERNAL_LINKAGE	int	imputinfo();
EXTERNAL_LINKAGE	int	imgetdiminfo();
EXTERNAL_LINKAGE	int	imputdiminfo();
EXTERNAL_LINKAGE	int	get_info();
EXTERNAL_LINKAGE	int	put_info();
EXTERNAL_LINKAGE	void	read_name();
EXTERNAL_LINKAGE	void	read_val();
EXTERNAL_LINKAGE	int	read_name_value(char *, char **, char **);
EXTERNAL_LINKAGE	int	get_info_pairs_into_array(Info *, char ***, char ***);
EXTERNAL_LINKAGE	void	write_name();
EXTERNAL_LINKAGE	void	write_val();
EXTERNAL_LINKAGE	void	printinfo();
EXTERNAL_LINKAGE	void	copyinfo();
EXTERNAL_LINKAGE	void	copy_list();
EXTERNAL_LINKAGE	void	imcopyinfo();
EXTERNAL_LINKAGE	Dic_rec *open_dic();
EXTERNAL_LINKAGE	Dic_rec *check_dic();
EXTERNAL_LINKAGE	char	*trunc_str();
EXTERNAL_LINKAGE	void	read_odf();

#define	_IMINFO_HEADER
#endif
