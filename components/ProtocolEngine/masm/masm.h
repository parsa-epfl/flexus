#define anyway ~0			/* universe for conditions	 */
#define n_succ 4			/* lg2(number of successor instr) */
#define N_succ 16			/*     number of successor instr */
#define mem_max 2048			/* target m-memory size		 */
#define lg_mem_max 11			/* lg2(mem_max) Note: mem_max must be a power of 2 */
#define maxdepth 20			/* max nesting deth of {}-blocks */
#define maxargs  4			/* max number of arguments to an instruction */

#define plt_max_col	20		/* max # of columns in flow plot */
#define plt_col_width	108		/* width: 108 pt = 1.5"		 */
#define plt_page_width  540		/* 7.5" (72 pt/inc)		 */
#define plt_page_height	720		/* 10" (72 pt/inch)		 */

typedef unsigned long long instruction;	/* instructions are assmbled in 64bit words */

#ifdef  MAIN_PRGM			/* main program ?		 */
#define cdext				/* define data struct.		 */
#define dinit(x) = x			/* initialize data		 */
#else
#define cdext extern			/* declare data struct. 	 */
#define dinit(x)			/* is done in main sect		 */
#endif

#define stb_max 4831			/* symbol table size		 */
#define ins_max 50			/* max # of instructions	 */
#define arg_max 500			/* max number of arguments	 */
#define ept_max 50			/* max number of entry points	 */

struct refer {				/* reference list element	 */
	int		src_line;	/* used in source line #	 */
	struct refer	*next;		/* next referenced in...	 */
};

struct sym_tab {			/* symbol table			 */
	char		*mnemo;		/* name				 */
	int		type;		/* type of symbol		 */
	int		value;		/* value of symbol (eg. table index) */
	struct sym_tab	*alias;		/* pointer to alias		 */
	struct refer	*ref;		/* reference pointer		 */
	int		src_line;	/* defined on line ...		 */
	unsigned	defined:1;	/* set to 1 if defined		 */
	unsigned	target_cnt:2;	/* 0 if no predecessor, 1 if one */
					/* and 2 if more than one        */
};

struct ins_tab {			/* instruction table (instances) */
	struct sym_tab	*sym_ptr;	/* label(s)			 */
	struct sym_tab	*next[N_succ];	/* successor instr. or 0	 */
	instruction	body;		/* the meat			 */
	instruction	defined;	/* bits that were explicitly defined */

	int		loc;		/* location of one instance	 */
	int		next_addr;	/* jump to ...			 */

	int		ins_def;	/* instruction definition	 */
	int		src_ln;		/* defined on line ...		 */
	char		*comment;	/* comment line			 */
};

struct flow_dag {
	enum {FD_LABEL, FD_ENTRY_PT, FD_GOTO, FD_INSTR, FD_TEST, FD_SLEEP} type;
	struct sym_tab	*st_ptr;	/* sym_tab ptr (for user defined labales) */
	struct ins_tab	*it_ptr;	/* pointer to intruction tab	*/
	int		succ[N_succ];	/* which condition goes where	*/
	struct flow_dag	*fd_next[N_succ]; /* successor pointers		*/
	short		n_next;		/* # of successors		*/
	short		patch_pos;	/* goto: info to restore tree	*/

	struct flow_dag	*lst_ptr;	/* link all FD entries		*/

	short		depth;		/* #of instructions away from root */
	short		dist;		/* #of nodes away from terminal node */
	int		x, y;		/* position of top entry point	*/
	int		ycl;		/* y connecting length */
	int		height;		/* vertical extend		*/
	int		sa_ind;		/* index into sorted array ... */
};

struct ins_def {			/* instruction definitions	 */
	struct sym_tab	*name;		/* name of this instruction	 */

	unsigned	rtn_vls:N_succ;	/* bit vector of possible return values */
	struct sym_tab	*rtn_sym[N_succ]; /* pts's to return value symbols */

	instruction	value;		/* code				 */
	instruction	mask;		/* mask that delineates code	 */
};

struct arg_def {			/* argument definitions		 */
	struct sym_tab	*name;		/* name of this argument	 */

	instruction	value;		/* code				 */
	instruction	mask;		/* mask that delineates code	 */
};

struct ept_def {			/* entry point definitions	 */
	struct sym_tab	*name;		/* name of this entry point	 */
	int		instr;		/* first instruction		 */
	int		position;	/* position in micro code memory */
	struct flow_dag	*fd_ptr;	/* pointer to flow dag (for plot option) */
};

#define SYM_LABEL	0		/* label symbols		 */
#define SYM_INSTR	1		/* instruction definitions	 */
#define SYM_ENTRY	2		/* entry points			 */
#define SYM_ARG_BGN	10		/* argument class range		 */
#define SYM_ARG_END	(SYM_ARG_BGN + ins_max)
#define SYM_RTV_BGN	SYM_ARG_END	/* return value range		 */
#define SYM_RTV_END	(SYM_RTV_BGN + ins_max)

struct patch_lst {			/* patch list			 */
    struct patch_lst *nxt;		/* ptr to next			 */
    int ploc;				/* location to be patched	 */
};

struct ins_stk {			/* instruction stack		 */
    struct ins_tab	proto;		/* instruction prototype	 */

    struct patch_lst	*PLST;		/* list of old instr to be patched	 */
    struct patch_lst	*RLST;		/* list of new instr to be patched	 */
    char		*blk_nm;	/* name of block (label)	 */
};

/*
 * Misc. global variables
 */

extern	int	yylineno;		/* number in source file (yacc)	 */
cdext	char	src_file[80] dinit("<stdin>"); 	/* string of file name	 */
cdext	int	magic dinit(-1);	/* magic number to be inserted into micro code file */
cdext	int	depth dinit(0);		/* nesting depth		 */
cdext	int	new_blk dinit(0);	/* 1: entering new block	 */
cdext	int	new_seq dinit(0);	/* 1: first instruction after EP */
cdext	struct sym_tab	*halt_lbl;	/* jump to this terminates sequence */
cdext	int	curr_entry_pnt;		/* current entry point		 */
cdext	int	next_addr_field dinit(-1); /* where to put the next address */
cdext	FILE	*list_out;		/* where the listing goes	 */
cdext	FILE	*code_out;		/* where the code goes		 */
cdext	FILE	*plot_out dinit(0);	/* plot output file		 */
cdext	int	no_comment dinit(1);	/* 1: ignore comments		 */
cdext	int	com_instr dinit (-1);	/* where to put comments	 */

/*
 * Tables
 */

cdext	struct sym_tab	ST[stb_max];	/* symbol table			 */
cdext	int		n_ST dinit(0);

cdext	struct ins_tab	IT[mem_max];	/* instruction instance table	 */
cdext	int		n_IT dinit(0);

cdext	struct ins_def	ID[ins_max];	/* instruction definition table	 */
cdext	int		n_ID dinit(0);

cdext	struct arg_def	AR[arg_max];	/* argument definition table	 */
cdext	int		n_AR dinit(0);

cdext	struct ept_def	EP[ept_max];	/* argument definition table	 */
cdext	int		n_EP dinit(0);

cdext	struct ins_stk	ISTK[maxdepth];	/* instruction nesting stack	 */

cdext	struct sym_tab	*MEM[mem_max];	/* micro code memory		 */
cdext	int		m_used dinit(0);

/*
 * function decls.
 */
char	*pop_ins();			/* pop instruction from stack	 */
