struct	_DESIGNATOR {
	char		*name;
	supertype	*c;

	supertype	*root;
};

designator	*new_designator(char * /*name of designator*/);
designator	*clone_designator(designator *);
void		set_designator(designator * /*destination*/, supertype * /*source*/);
void		unset_designator(designator *);
void		destroy_designator(designator *);
char		*string_designator(designator *);
