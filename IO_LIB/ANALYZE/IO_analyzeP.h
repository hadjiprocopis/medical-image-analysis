#ifndef	IO_ANALYZE_P_H
#define	IO_ANALYZE_P_H
/* this file contains three private arrays which describe the
   length and size (in bytes) of each entry of the analyze header structure.
   This information will be used in automating byte-swapping an analyze header,
   if needed.
*/
typedef	struct	_ANALYZE_HEADER_ENTRY {
	int	number_of_items;	/* how many items (e.g. char xx[8] gives 8 items) */
	size_t	size_of_each_item;	/* how many bytes each item (e.g. above is sizeof(char), 1) */
	size_t	preset_size_of_each_item; /* the size_of_each_item above is dependent of the system
					    architecture since it comes from sizeof(...),
					    however maybe two architectures have different sizes
					    for an integer or short int, so this size is what other people
					    are using */
	char	type[10];		/* the type e.g. 'int' */
} _analyze_header_entry;	

#define	_analyze_header_entries_header_key_SIZE	7
_analyze_header_entry	_analyze_header_entries_header_key[] = {
        {1, sizeof(int), 4, "int\0"},
	{10, sizeof(char), 1, "char\0"},
	{18, sizeof(char), 1, "char\0"},
	{1, sizeof(int), 4, "int\0"},
	{1, sizeof(short int), 2, "short int\0"},
	{1, sizeof(char), 1, "char\0"},
	{1, sizeof(char), 1, "char\0"}
};

#define	_analyze_header_entries_image_dimension_SIZE	18
_analyze_header_entry	_analyze_header_entries_image_dimension[] = {
	{8, sizeof(short int), 2, "short int\0"},
	{4, sizeof(char), 1, "char\0"},
	{8, sizeof(char), 1, "char\0"},
	{1, sizeof(short int), 2, "short int\0"},
	{1, sizeof(short int), 2, "short int\0"},
	{1, sizeof(short int), 2, "short int\0"},
	{1, sizeof(short int), 2, "short int\0"},
	{8, sizeof(float), 4, "float\0"},
	{1, sizeof(float), 4, "float\0"},
	{1, sizeof(float), 4, "float\0"},
	{1, sizeof(float), 4, "float\0"},
	{1, sizeof(float), 4, "float\0"},
	{1, sizeof(float), 4, "float\0"},
	{1, sizeof(float), 4, "float\0"},
	{1, sizeof(int), 4, "int\0"},
	{1, sizeof(int), 4, "int\0"},
	{1, sizeof(int), 4, "int\0"},
	{1, sizeof(int), 4, "int\0"}
};

#define	_analyze_header_entries_data_history_SIZE	18
_analyze_header_entry	_analyze_header_entries_data_history[] = {
	{80, sizeof(char), 1, "char\0"},
	{24, sizeof(char), 1, "char\0"},
	{1, sizeof(char), 1, "char\0"},
	{10, sizeof(char), 1, "char\0"},
	{10, sizeof(char), 1, "char\0"},
	{10, sizeof(char), 1, "char\0"},
	{10, sizeof(char), 1, "char\0"},
	{10, sizeof(char), 1, "char\0"},
	{10, sizeof(char), 1, "char\0"},
	{3, sizeof(char), 1, "char\0"},
	{1, sizeof(int), 4, "int\0"},
	{1, sizeof(int), 4, "int\0"},
	{1, sizeof(int), 4, "int\0"},
	{1, sizeof(int), 4, "int\0"},
	{1, sizeof(int), 4, "int\0"},
	{1, sizeof(int), 4, "int\0"},
	{1, sizeof(int), 4, "int\0"},
	{1, sizeof(int), 4, "int\0"}
};
#endif

