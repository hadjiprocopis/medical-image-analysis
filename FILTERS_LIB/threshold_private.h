/* only Maps.c needs this file */
/* tables which tell us, given 2 changemap_entry values and a logical operation,
   what the result should be.
   [NOP/AND/OR/NOT/XOR][v1 value][v2 value] v1 and v2 can be unchanged or changed
   In the case of NOT, the result will be NOT(v1), v2 will be ignored.
   DO NOT CHANGE THE ORDER OF THE AND/OR/... they depend on the enum values of logicalOperation in threshold.h
   DO NOT CHANGE THE ORDER OF THE changed/unchanged etc. they depend on the enum values of change map_entry
*/
changemap_entry	thresholdTruthTable[5][2][2] = {
	{ /* NOP, v1 values are left unaffected */
		{changed, unchanged},	/* v1=c, v2=c */  /* v1=c, v2=u */
		{unchanged, unchanged}	/* v1=u, v2=c */  /* v1=u, v2=u */
	},
	{ /* AND */
		{changed, unchanged},	/* v1=c, v2=c */  /* v1=c, v2=u */
		{unchanged, unchanged}	/* v1=u, v2=c */  /* v1=u, v2=u */
	},
	{ /* OR */
		{changed, changed},	/* v1=c, v2=c */  /* v1=c, v2=u */
		{unchanged, unchanged}	/* v1=u, v2=c */  /* v1=u, v2=u */
	},
	{ /* NOT(v1), v2 is ignored */
		{unchanged, changed},	/* v1=c, v2=c */  /* v1=c, v2=u */
		{unchanged, changed}	/* v1=u, v2=c */  /* v1=u, v2=u */
	},
	{ /* XOR */
		{unchanged, changed},	/* v1=c, v2=c */  /* v1=c, v2=u */
		{changed, unchanged}	/* v1=u, v2=c */  /* v1=u, v2=u */
	}
};
