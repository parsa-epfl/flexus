#include <stdio.h>
#include <string.h>
#define MAIN_PRGM
#include "masm.h"

extern FILE *yyin, *yyout;


main (argc, argv)
    int argc;
    char *argv[];
{
    if (argc == 5) {			/* plot option			*/
	plot_out = fopen(argv[4], "w");
	argc = 4;
    }

    if (argc != 4 || !(list_out = fopen(argv[3], "w")) ||
	!(code_out = fopen(argv[2], "w"))) {
	fprintf(stderr, "masm <microcode def> <code-output> <list-file> [<plot-file>]\n");
	exit(1);
    }
    init ();				/* initialize tables		 */

    read_def (argv[1]);			/* get machine description	 */

    yyparse ();				/* assemble code		 */

    check ();				/* undefind labels ?		 */

    prnt_ST();				/* print symbols		 */

    m_map ();				/* map to memory		 */

    prnt_map ();			/* print memory map		 */

    code_gen ();			/* write code			 */

    prnt_IT ();				/* print listing of instructions */

    if (plot_out)
	plot_it ();			/* generate postscript plot of code */

    exit(0);
}

yyerror (s)				/* error printer		 */
    char *s;
{
    fprintf(stderr, "%s, line %d: %s\n", src_file, yylineno, s);
    exit(1);
}

hash (s)				/* hash function		 */
    char *s;
{
    int i = 0;
    while (*s) i += 10993 * i + *s++;
    return i ^ (i >> 16);
}

struct sym_tab *sym_tab_lookup (s, type)
	char *s;
	int type;
{
	int i;
	i = hash(s) % stb_max;
	while (ST[i].mnemo && (type != ST[i].type || strcmp(s, ST[i].mnemo)))
		i = (i + 1) % stb_max;

	return &(ST[i]);
}

struct sym_tab *sym_tab_insert (s, type, value)
	char *s;
	int type, value;
{
	struct sym_tab *p;

	p = sym_tab_lookup (s, type);
	if (p->mnemo && p->defined)
		yyerror ("Multiply defined Symbol");
	else if (n_ST > (stb_max - stb_max/10))
		yyerror ("Symbol table overflow (increase stb_max)");
	else {
		if (!(p->mnemo)) {
			p->mnemo = strdup(s);
			p->type = type;
			p->ref = 0;
			n_ST++;
		}
		p->value = value;
		p->defined = 1;
		p->src_line = yylineno;
		p->alias = 0;
		p->target_cnt = 0;
	}

	return p;
}

struct sym_tab *sym_tab_refer (s, type)
	char *s;
	int type;
{
	struct sym_tab *p;
	struct refer *r;

	p = sym_tab_lookup (s, type);
	if (!(p->mnemo)) {
		if (n_ST > (stb_max - stb_max/10))
			yyerror ("Symbol table overflow (increase stb_max)");
		p->mnemo = strdup(s);
		p->type = type;
		p->ref = 0;
		n_ST++;
		p->value = -1;
		p->defined = 0;
		p->src_line = -1;
	}

	r = (struct refer *) malloc(sizeof(struct refer));
	r->next = p->ref;
	p->ref = r;
	r->src_line = yylineno;

	return p;
}

init ()					/* initialize tables		 */
{
    int i, j, k;

    for (i = stb_max; --i; ST[i].mnemo = 0);

    yyin = stdin;			// default
    yyout = stdout;

    yylineno = 0;
    halt_lbl = sym_tab_insert ("stop", SYM_LABEL, -1);
    yylineno = 1;

    depth = 0;
    ISTK[0].blk_nm = 0;
    ISTK[0].PLST = 0;
    ISTK[0].RLST = 0;
}

read_def (s)			/* read machine description	 */
    char *s;
{
    FILE *inf;
    char buf[1000], t[1000];
    static char *defs[7] = {
	"magic",
	"instr",
	"arg",
	"EP",
	"value",
	"//",
	"next"
    };
    int i, j;

    if (!(inf = fopen(s, "r")))
	yyerror ("Failed to open Micro-code definition file");

    yylineno = 0;
    while (fgets (buf, 1000, inf)) {
	yylineno++;

	sscanf(buf, "%s", t);
	for (i = 0; i < 7; i++)
		if (!strcmp(defs[i], t))
			break;
	switch (i) {
	case 0:				/* get *one* magic number */
		if (magic >= 0 || 1 != sscanf(buf, "%*s%d", &magic) || magic < 0)
			yyerror("invalid magic number");
		break;

	case 1:				/* instruction definition */
		if (n_ID >= ins_max)
			yyerror("Too many instruction defs (increase 'ins_max')");
		new_instr(&(buf[5]), &(ID[n_ID]));
		n_ID++;
		break;

	case 2:				/* argument definition */
		if (n_AR >= arg_max)
			yyerror("Too many argument defs (increase 'arg_max')");
		new_arg(&(buf[3]), &(AR[n_AR]));
		n_AR++;
		break;

	case 3:				/* entry point definition */
		if (n_EP >= ept_max)
			yyerror("Too many entry point defs (increase 'arg_max')");
		if (2 != sscanf(buf, "%*s%s%d", t, &i) || i < 0 || i >= mem_max)
			yyerror("Invalid entry point");
		for (j = 0; j < n_EP; j++)
			if (EP[j].position == i)
				yyerror("Entry point is multiply defined");
		EP[n_EP].name = sym_tab_insert (t, SYM_ENTRY, n_EP);
		EP[n_EP].instr = -1;
		EP[n_EP].position = i;
		EP[n_EP].fd_ptr = 0;
		n_EP++;
		break;

	case 4:				/* return value definition */
		if (2 != sscanf(buf, "%*s%s%d", t, &i) || i < 0 || i >= N_succ || n_ID < 1)
			yyerror("Invalid return value");
		if (i && ID[n_ID - 1].rtn_vls & (1 << i))
			yyerror("Multiple definitions for same return value");
		ID[n_ID - 1].rtn_sym[i] = sym_tab_insert (t, SYM_RTV_BGN + (n_ID - 1), i);
		ID[n_ID - 1].rtn_vls |= 1 << i;
		break;

	case 5:				/* ignore comment lines */
		break;

	case 6:				/* locate next address field */
		if (2 != sscanf(buf, "%*s%d%d", &i, &j) || (j - i) != (lg_mem_max - 1) ||
		    j < i || i < 0 || j >= 8*sizeof(instruction) || next_addr_field >= 0)
			yyerror("Invalid next address field descriptor");
		next_addr_field = i;
		break;

	default:
		yyerror("Invalid machine definition");
	}
    }

    if (next_addr_field < 0 || magic < 0)
	yyerror ("next or magic definition missing");

    yylineno = 1;
}

new_instr(s, ip)			/* define a new instruction	 */
	char *s;
	struct ins_def *ip;
{
/*
 * Instruction definition:
 *
 * instr <mnemo> <first bit> <last bit> <value (hex)> <branch mask position / -1>
 */
	char mnemo[1000];
	int fb, lb, v, i;

	if (4 != sscanf(s, "%s%d%d%d", mnemo, &fb, &lb, &v) || fb > lb ||
	    v < 0 || v >= (2 << (lb - fb)) || lb >= (sizeof(instruction) * 8))
		yyerror("malformed instruction definition");
	ip->name = sym_tab_insert (mnemo, SYM_INSTR, n_ID);
	ip->rtn_vls = 1;		/* 0 is allways possible */
	ip->value = v << fb;
	ip->mask = ((2 << (lb - fb)) - 1) << fb;
	for (i = 0; i < N_succ; i++)
		ip->rtn_sym[i] = 0;
}

new_arg (s, ap)				/* define a anew argument	 */
	char *s;
	struct arg_def *ap;
{
/*
 * Argument definition:
 *
 * instr <mnemo> <first bit> <last bit> <value (hex)>
 */
	char mnemo[1000];
	int fb, lb, v;

	if (4 != sscanf(s, "%s%d%d%d", mnemo, &fb, &lb, &v) || fb > lb || n_ID < 1 ||
	    v < 0 || v >= (2 << (lb - fb)) || lb >= (sizeof(instruction) * 8))
		yyerror("malformed argument definition");
	ap->name = sym_tab_insert (mnemo, SYM_ARG_BGN + n_ID - 1, n_AR);
	ap->value = v << fb;
	ap->mask = ((2 << (lb - fb)) - 1) << fb;
}

bgn_instr ()				/* begin a new instruction	 */
{
    int i;

    ISTK[depth].proto.sym_ptr = 0;
    for (i = 0; i < N_succ; i++)
	ISTK[depth].proto.next[i] = 0;
    ISTK[depth].proto.body = 0;
    ISTK[depth].proto.defined = (mem_max - 1) << next_addr_field;

    ISTK[depth].proto.loc = -1;
    ISTK[depth].proto.ins_def = -1;
    ISTK[depth].proto.src_ln = yylineno;
    ISTK[depth].proto.comment = 0;

    if (ISTK[depth].blk_nm && new_blk) {
	    new_blk = 0;
	    add_label(ISTK[depth].blk_nm);
    }
    com_instr = -1;
}

end_instr ()				/* complete instruction		 */
{
    int i, j;
    char buf[100], *s;
    static int lcnt = 0, sub = 0;
    static int lcnt_m = 0, sub_m = 0;
    struct sym_tab *t;
    struct ins_tab *ip;
    struct patch_lst *pp;

    ip = &(ISTK[depth].proto);
    t = ip->sym_ptr;
    if (t == 0) {	/* no label: generate one!	 */
	s = (char *) malloc (sizeof(char) * 20);
	if (ip->src_ln == lcnt)
	    sprintf(s, "Line-%d.%d", ip->src_ln, ++sub);
	else {
	    if (ip->src_ln > lcnt_m) {
		sprintf(s, "Line-%d", ip->src_ln);
		sub = 0;
	    } else {			/* need to step back */
		sub = sub_m;
		sprintf(s, "Line-%d.%d", ip->src_ln, ++sub);
	    }
	}
	lcnt = ip->src_ln;
	add_label(s);
	t = ip->sym_ptr;
    };

    if (lcnt_m < lcnt)
	lcnt_m = lcnt;
    if (sub_m < sub)
	sub_m = sub;

    while(t) {				/* give value to all instances */
	    t->value = n_IT;
	    t = t->alias;
    }

    while (ISTK[depth].PLST) {		/* need to add address to pred.	 */
	for (j = 0; j < N_succ; j++)
	    if (IT[ISTK[depth].PLST->ploc].next[j] == 0)
		IT[ISTK[depth].PLST->ploc].next[j] = ip->sym_ptr;
	ISTK[depth].PLST = ISTK[depth].PLST -> nxt;
    }

    ISTK[depth].PLST = ISTK[depth].RLST;
    ISTK[depth].RLST = 0;

    for(j = 0; j < N_succ; j++)		/* look for unresolved labels	 */
	if (ip->next[j] == 0)
	    break;
    if (j < N_succ) {			/* request to be patched	 */
	pp = (struct patch_lst *) malloc (sizeof(struct patch_lst));
	pp->ploc = n_IT;
	pp->nxt = ISTK[depth].PLST;
	ISTK[depth].PLST = pp;
    }

    if (n_IT >= mem_max)
	yyerror("Too many instructions (mem_max)");
    IT[n_IT] = ISTK[depth].proto;	/* copy it to table		 */

    if (depth == 0 && new_seq) {	/* link up to entry point	 */
	    new_seq = 0;
	    EP[curr_entry_pnt].instr = n_IT;
    }
    com_instr = n_IT++;
}

add_comment (s)				/* add user supplied comments	 */
    char *s;
{
	int i;

	if (com_instr < 0)
		return;
	while (*s == ' ' || *s == '\t') s++;
	i = strlen(s);
	if (s[i - 1] == '/') i -= 2;
	while (i > 0 && (s[i - 1] == ' ' || s[i - 1] == '\t')) i--;
	s[i] = 0;
	if (i > 0)
		IT[com_instr].comment = strdup(s);
	com_instr = -1;
}

add_label (s)
	char *s;
{
	struct sym_tab *t;
	t = sym_tab_insert (s, SYM_LABEL, n_IT);
	if (ISTK[depth].proto.sym_ptr) {
		t->alias = (ISTK[depth].proto.sym_ptr)->alias;
		(ISTK[depth].proto.sym_ptr)->alias = t;
	} else
		ISTK[depth].proto.sym_ptr = t;
}

push_ins()
{
    register char  *s;
    static int      lcnt = 0, sub = 0;

    if (depth >= maxdepth)
	yyerror("Too deeply nested: increase 'maxdepth'\n");

    s = (char *) malloc(20);
    if (yylineno == lcnt)
	sprintf(s, "Block-%d.%d", yylineno, ++sub);
    else {
	sprintf(s, "Block-%d", yylineno);
	sub = 0;
    }
    lcnt = yylineno;

    ISTK[++depth].blk_nm = s;
    ISTK[depth].RLST = 0;
    ISTK[depth].PLST = 0;

    new_blk = 1;
}

char *pop_ins()
{
    register struct patch_lst *t, *q;

    if (new_blk)
	yyerror ("Empty blocks are not supported");
    if (depth <= 0)
	yyerror ("Bug: pop from empty instruction stack");

    for (t = ISTK[depth].PLST; t;) {		/* copy patch list  */
	q = t->nxt;
	t->nxt = ISTK[depth-1].RLST;
	ISTK[depth-1].RLST = t;
	t = q;
    }

    return ISTK[depth--].blk_nm;
}

def_op_code (s)				/* defines an opcode		 */
    char *s;
{
	struct sym_tab *t;
	struct ins_def *ip;
	int i;

	t = sym_tab_refer (s, SYM_INSTR);
	if (!(t->defined))
		yyerror("undefined micro instruction");

	i = t->value;
	ip = &(ID[i]);

	ISTK[depth].proto.body |= ip->value;
	ISTK[depth].proto.defined |= ip->mask;
	ISTK[depth].proto.ins_def = i;
}

def_op_arg (s)				/* defines an argument		 */
    char *s;
{
	struct sym_tab *t;
	struct arg_def *ap;
	int i;

	i = ISTK[depth].proto.ins_def;
	t = sym_tab_refer (s, SYM_ARG_BGN + i);
	if (!(t->defined))
		yyerror("undefined micro instruction argument");

	ap = &(AR[t->value]);

	if (ISTK[depth].proto.defined & ap->mask)
		yyerror("overlapping argument fields");
	ISTK[depth].proto.body |= ap->value;
	ISTK[depth].proto.defined |= ap->mask;
}

add_next (cc, s)			/* add successor instruction	 */
    unsigned long cc;
    char *s;
{
    unsigned long j, k;
    struct sym_tab *t;

    if (!cc) {
	fprintf(stderr, "%s, line %d: Warning - branch never taken\n",
		src_file, yylineno);
	return;
    }

    if (!s) {					/* fall throught to next stmt	*/
	t = 0;
    } else
        t = sym_tab_refer (s, SYM_LABEL);	/* locate instruction		 */

    for (j = 1, k = 0; k < N_succ; k++, j += j)
	if (cc & j)
	    ISTK[depth].proto.next[k] = t;
}

add_self (cc)
    unsigned long cc;
{
    static int      lcnt = 0, sub = 0;
    char           *s;

    s = (char *) malloc(20);
    if (yylineno == lcnt)
	sprintf(s, "While-%d.%d", yylineno, ++sub);
    else {
	sprintf(s, "While-%d", yylineno);
	sub = 0;
    }
    lcnt = yylineno;

    add_label(s);
    add_next(cc, s);
}

unsigned long get_cond(s)		/* find the return condition	 */
	char *s;
{
	struct sym_tab *t;

	t = sym_tab_refer (s, SYM_RTV_BGN + ISTK[depth].proto.ins_def);
	if (!(t->defined))
		yyerror("Continuation condition not defined for this instruction");
	return 1 << (t->value);
}

bgn_sequence (s)			/* begin of code for a particular entry point */
	char *s;
{
	struct sym_tab *t;

	t = sym_tab_refer (s, SYM_ENTRY);
	if (!(t->defined) || EP[t->value].instr >= 0)
		yyerror ("Un/Multiply-defined entry point");
	curr_entry_pnt = t->value;

	new_seq = 1;
}

end_sequence ()				/* end of sequence		*/
{
    int j;

    if (depth != 0)
	yyerror("illegal sequence termination in nested block");

    if (new_seq)
	yyerror("entry point leading to empty sequence");

    while (ISTK[0].PLST) {		/* need halt all threads to exit */
	for (j = 0; j < N_succ; j++)
	    if (IT[ISTK[0].PLST->ploc].next[j] == 0)
		IT[ISTK[0].PLST->ploc].next[j] = halt_lbl;
	ISTK[depth].PLST = ISTK[depth].PLST -> nxt;
    }
}

check ()				/* sanity check on source */
{
    int i, j, k, l;

    for (i = 0; i < n_EP; i++)
	if (EP[i].instr < 0)
	    fprintf(stderr, "Warning: Entry point <%s> not used\n", EP[i].name->mnemo);

    for (i = 0, k = 0; i < n_IT; i++) {	/* for each instruction... */

	if (IT[i].comment == 0) {	/* auto-generate missing comments */
		char buf[80];
		sprintf (buf, "<%s>", IT[i].sym_ptr->mnemo);
		IT[i].comment = strdup(buf);
	}

	for (j = 0, l = 0; j < N_succ; j++) {
	    if (IT[i].next[j] == 0 || IT[i].next[j]->defined == 0) {
		k = 1;
		fprintf(stderr, "Error: Label <%s> not defined\n", IT[i].next[j]->mnemo);
		IT[i].next[j]->defined = 1;	/* prevents multiple error messages */
	    } else if (IT[i].next[j] == halt_lbl)
		l = 1;
	}

	if (l) {
		for (j = 0; j < N_succ; j++)
			if (IT[i].next[j] != halt_lbl) {
				fprintf(stderr, "Error: stop-label must be used unconditionally (line %d)\n", IT[i].src_ln);
				k = 1;
				break;
			}
	}
    }

    if (k)
	exit(1);
}

prnt_ST ()			/* print symbol table		 */
{
    int i;
    struct refer *t;

    fprintf(list_out, "\nSymbol table:\n", n_ST);
    fprintf(list_out, "Name             Defined Used\n");

    for (i = 0; i < stb_max; i++) {
	if (ST[i].mnemo == 0 || ST[i].type != SYM_LABEL)
	    continue;
	fprintf(list_out, "%-16s ", ST[i].mnemo);
	if (ST[i].defined)
	    fprintf(list_out, "%-4d   ", ST[i].src_line);
	else
	    fprintf(list_out, "?undef?");
	for (t = ST[i].ref; t; t = t->next)
	    fprintf(list_out, " %-4d", t->src_line);
	if (ST[i].alias)
	    fprintf(list_out, " = %s", ST[i].alias->mnemo);
	fprintf(list_out, "\n");
    }
}

m_map ()			/* map to memory		 */
{
	int i, j, k;
	unsigned long zero, one, reachable;
	struct ins_tab *ip;
	struct sym_tab *t;

	for (i = 0; i < mem_max; i++)
		MEM[i] = 0;		/* all invalid initially */

	for (i = 0; i < n_EP; i++) {	/* allocate entry points */
		k = EP[i].instr;
		if (k < 0)
			continue;	/* well - no code defined!! */
		j = EP[i].position;
		if (MEM[j])
			yyerror ("Conflicting entry points");
		MEM[j] = IT[k].sym_ptr;
		IT[k].loc = j;
		if (m_used <= j)
			m_used = (j / N_succ + 1) * N_succ;
	}

	for (i = 0; i < n_IT; i++) {	/* for each instruction */
		ip = &(IT[i]);

		if (ip->next[0] == halt_lbl) {	/* no successor instruction */
			ip->next_addr = 0;
			continue;
		}

		reachable = ID[ip->ins_def].rtn_vls;
		align_chk (ip, &zero, &one);

		for (j = 0; j < m_used; j++) { /* try to locate a match */
			if (j & zero)
				continue;	/* improper alignment */

			for (k = 0; k < N_succ; k++) {
				if (!(reachable & (1 << k)))
					continue;
				if (MEM[k | j] != ip->next[k])
					break;	/* mismatch ! */
			}

			if (k >= N_succ) {	/* found a match! */
				ip->next_addr = j;
				break;
			}
		}

		if (j < m_used)
			continue;		/* was matched up! */

		for (j = 0; j < mem_max; j++) {	/* allocate */
			if (j & zero || (~j) & one)
				continue;	/* improper alignment */

			for (k = 0; k < N_succ; k++) {
				if (!(reachable & (1 << k)))
					continue;
				if (MEM[k | j] && MEM[k | j] != ip->next[k])
					break;	/* mismatch ! */
			}

			if (k >= N_succ) {	/* found a match! */
				ip->next_addr = j;
				for (k = 0; k < N_succ; k++)
					if (reachable & (1 << k)) {
						MEM[j | k] = t = ip->next[k];
						IT[t->value].loc = j | k;
					}
				break;
			}
		}

		if (j >= mem_max)
			yyerror("Microcode store too small");

		if (j >= m_used)
			m_used = (j / N_succ + 1) * N_succ;
	}
}

align_chk (ip, mbz, sbo)
	struct ins_tab *ip;
	unsigned long *mbz, *sbo;
{
	unsigned long reachable, msk;
	int i, j, k;

	*mbz = 0;	/* bits of the next address that must be 0 */
	*sbo = 0;	/* bits of the next address that should be 1 to
			   minimize rom need */

	reachable = ID[ip->ins_def].rtn_vls;

	for (i = 0; i < n_succ; i++) {
		msk = 1 << i;
		for (j = 0; j < N_succ; j++) {
			k = j ^ msk;
			if (!(reachable & (1 << j)))
				continue;
			if (!(reachable & (1 << k)))
				continue;
			if (ip->next[j] == ip->next[k])
				*sbo |= msk;
			else
				*mbz |= msk;
		}
	}

	*sbo &= ~(*mbz);
}

prnt_map ()				/* print memory map		 */
{
    int i, j;

    fprintf(list_out, "\nMemory Map (%d of %d used);\n", m_used, mem_max);

    for (i = 0; i < m_used; i += N_succ) {
	for (j = 0; j < N_succ; j++) {
		fprintf(list_out, "%3x:%-10s", i + j, (MEM[i + j]) ? MEM[i + j]->mnemo : "**unused**");
		if (j < (N_succ - 1))
			fprintf(list_out, " | ");
	}
	fprintf(list_out, "\n");
    }
}

prnt_IT ()			/* print instruction table	 */
{
    int i, j, k;
    int s[stb_max], key();
    char buf[80], *cp;
    struct ins_tab *ip;

    fprintf(list_out, "\nInstruction table (%d entries):\n", n_IT);

    for (i = 0, j = 0; i < stb_max; i++)
	if (ST[i].mnemo && ST[i].type == SYM_LABEL && &(ST[i]) != halt_lbl)
		s[j++] = i;
    qsort (s, j, sizeof (int), key);	/* sort labels */

    for (i = 0; i < j; i++) {		/* print in sorted order */
	ip = &(IT[ST[s[i]].value]);

	if (ip->sym_ptr != &(ST[s[i]])) {	/* not primary label */
	    fprintf(list_out, "%-14s: alias for '%s'\n",
		ST[s[i]].mnemo, ip->sym_ptr->mnemo);
	    continue;
	}

	fprintf(list_out, "%-14s: %x %x %x %x %03x >", ip->sym_ptr->mnemo,
	    ip->body & 0xF, (ip->body >> 4) & 0xF, (ip->body >> 8) & 0xF, (ip->body >> 12) & 0xF, ip->next_addr);
	if (ip->next_addr == 0) {
		fprintf(list_out, " stop\n");
		continue;
	}
	for (k = 0; k < N_succ; k++) {
		if (!(ID[ip->ins_def].rtn_vls & (1 << k)))
			fprintf(list_out, " ....");
		else
			fprintf(list_out, " %04x", ip->next_addr | k);
	}
	fprintf(list_out, "\n");
    }
}

key (a, b)			/* sort key		 */
    int *a, *b;
{
    return strcmp (ST[*a].mnemo, ST[*b].mnemo);
}

code_gen ()			/* generate code	 */
{
    char *buf[80], *ctime();
    int i, j;
    struct ins_tab *ip;
    long clock, time();

    for (i = 0; i < n_IT; i++) {	/* complete instructions */
	    IT[i].body |= IT[i].next_addr << next_addr_field;
    }

    clock = time (0L);
    fprintf(code_out, "%d %s %s", magic, src_file, ctime(&clock));

    for (i = 0; i < m_used; i++)
	if (MEM[i]) {
	    ip = &(IT[MEM[i]->value]);
	    j = ip->body;
	    fprintf (code_out, "%d %d %d %d %d %d ;%d: %s\n", i,
		j & 0xF, (j >> 4) & 0xF, (j >> 8) & 0xF, (j >> 12) & 0xF, ip->next_addr,
		ip->src_ln, ip->comment);
	}
}

/*
 * Plot support stuff
 */

static struct flow_dag *fd_root = 0;
static struct flow_dag *fd_free = 0;

struct flow_dag *new_fde()
{
	struct flow_dag *fd;
	int i;

	if (fd_free) {
		fd = fd_free;
		fd_free = fd->lst_ptr;
	} else
		fd = (struct flow_dag *) malloc (sizeof (struct flow_dag));

	fd->lst_ptr = fd_root;
	fd_root = fd;

	fd->n_next = 1;
	fd->st_ptr = 0;
	fd->it_ptr = 0;
	for (i = 0; i < N_succ; i++)
		fd->succ[i] = 0;

	return fd;	
}

free_fde(fd)
	struct flow_dag *fd;
{
	struct flow_dag *t;

	t = fd_root;
	if (t == fd) {
		fd_root = fd->lst_ptr;
	} else {
		for (; t; t = t->lst_ptr)
			if (t->lst_ptr == fd) {
				t->lst_ptr = fd->lst_ptr;
				break;
			}
	}
	fd->lst_ptr = fd_free;
	fd_free = fd;
}

struct flow_dag *exp_flow (ip, md)
	int *md;
	struct ins_tab *ip;
{
	struct flow_dag *fd, *t;
	struct sym_tab *sp, *et[N_succ];
	struct tmp {
		int		src;
		int		dist;
	} T[N_succ], Tx;

	int i, j, k, inv[N_succ];

	fd = new_fde();

	fd->it_ptr = ip;
	fd->st_ptr = ip->sym_ptr;
	fd->type = FD_INSTR;
	if (ID[ip->ins_def].rtn_vls != 1) {
		fd->type = FD_TEST;
		if (!strcmp(ID[ip->ins_def].name->mnemo, "RECEIVE"))
			fd->type = FD_SLEEP;
	}

	T[0].dist = 0;					/* assume that this is a terminal node */
	for (i = 0, j = 0; i < N_succ; i++) {
		sp = ip->next[i];

		if (!sp || sp == halt_lbl) {
			fd->succ[i] = -1;
			continue;	/* skip dead ends */
		}

		for (k = 0; k < j; k++)
			if (sp == et[k])
				break;
		if (k < j)		/* done this one! */
			fd->succ[i] = k;
		else {
			int n_dist = 0;

			et[j] = sp;
			for (t = fd_root; t; t = t->lst_ptr)		/* try to locate label */
				if (t->type == FD_LABEL && sp == t->st_ptr)
					break;
			if (!t) {
				struct ins_tab *x;
				x = &(IT[sp->value]);
				for (t = fd_root; t; t = t->lst_ptr)	/* try to locate instr. */
					if (x == t->it_ptr)
						break;
				if (!t)					/* have not seen this one before */
					t = exp_flow (x, &n_dist);
			} else
				n_dist = t->dist;

			T[j].src = j;
			T[j].dist = n_dist + 1;
			fd->succ[i] = j;
			fd->fd_next[j++] = t;
		}
	}

	for (i = 1; i < j; i++)			/* sort T */
		for (k = 0; k < i; k++)
			if (T[k].dist < T[i].dist) {
				Tx = T[k];
				T[k] = T[i];
				T[i] = Tx;
				t = fd->fd_next[k];
				fd->fd_next[k] = fd->fd_next[i];
				fd->fd_next[i] = t;
			}

	for (i = 0; i < j; i++)			/* invert index */
		inv[T[i].src] = i;
	for (i = 0; i < N_succ; i++) {
		k = fd->succ[i];
		if (k >= 0)
			fd->succ[i] = inv[k];
	}

	fd->n_next = j;				/* # of successor nodes */
	fd->dist = j = T[0].dist;		/* distance to terminal node */

	for (sp = ip->sym_ptr; sp; sp = sp->alias) {	/* process labels */
		char *p = sp->mnemo;
		while (*p != '-' && *p) p++;	/* look for "-" */
		if (*p != '-' || sp->target_cnt == 2) {		/* ignore labels with "-" in them:
						           they are automatically generated */
			t = new_fde();
			t->st_ptr = sp;
			t->it_ptr = ip;
			t->type = FD_LABEL;
			t->dist = ++j;
			t->fd_next[0] = fd;
			fd = t;
		}
	}

	*md = j;				/* return max distance to a terminal node */
	return fd;
}

new_dag (ep_ptr)
	struct ept_def *ep_ptr;
{
	struct flow_dag *fd;
	int max_dist;

	ep_ptr->fd_ptr = fd = new_fde();

	fd->st_ptr = ep_ptr->name;
	fd->type = FD_ENTRY_PT;
	fd->fd_next[0] = exp_flow (&(IT[ep_ptr->instr]), &max_dist);
	fd->dist = max_dist;
}

plot_it()
{
	int i, j, k;
	int bv;

	fprintf(plot_out, "%%!PS-Adobe-2.0 EPSF-2.0\n");
	fprintf(plot_out, "/PBF /Helvetica findfont 8 scalefont def\n");
	fprintf(plot_out, "/UBF /Helvetica findfont 5 scalefont def\n");
	fprintf(plot_out, "/NBF /Helvetica-Bold findfont 8 scalefont def\n");
	fprintf(plot_out, "/LBF /Helvetica-Bold findfont 8 scalefont def\n");
	fprintf(plot_out, "/EPF /Helvetica-Bold findfont 12 scalefont def\n");
	fprintf(plot_out, "/LTXT {moveto setfont dup stringwidth pop 0 exch sub 0 rmoveto show} def\n");
	fprintf(plot_out, "/RTXT {moveto setfont show} def\n");
	fprintf(plot_out, "/UTXT {gsave translate 0 0 moveto 90 rotate 0 0 RTXT grestore} def\n");
	fprintf(plot_out, "/DTXT {gsave translate 0 0 moveto 90 rotate 0 0 LTXT grestore} def\n");

	fprintf(plot_out, "/MPG {/r exch def /c exch def /fg exch def\n");
	fprintf(plot_out, "0 1 r 1 sub {/rc exch def\n");
	fprintf(plot_out, "0 1 c 1 sub {/cc exch def gsave\n");
	fprintf(plot_out, "520 18 moveto PBF setfont cc 20 string cvs show (/) show rc 20 string cvs show\n");
	fprintf(plot_out, "( of ) show c 20 string cvs show (/) show r 20 string cvs show\n");
/*
	fprintf(plot_out, "newpath 36 36 moveto 0 %d rlineto %d 0 rlineto 0 -%d rlineto closepath clip\n",
		plt_page_height, plt_page_width, plt_page_height);
*/
	fprintf(plot_out, "-%d cc mul 36 add -%d rc mul 36 add translate fg gsave showpage grestore grestore\n",
		plt_page_width, plt_page_height);
	fprintf(plot_out, "%%%%Page: 1 1\n");
	fprintf(plot_out, "} for } for } def\n");

	for (i = 0; i < n_IT; i++) {
		bv = 0;
		for (j = 0; j < N_succ; j++)
		    if (!(bv & (1 << j))) {
			switch (IT[i].next[j]->target_cnt) {
			    case 0:
				IT[i].next[j]->target_cnt = 1;
				break;
			    case 1:
				IT[i].next[j]->target_cnt = 2;
				break;
			    case 2:
				break;
			}

			for (k = j + 1; k < N_succ; k++)
			    if (IT[i].next[j] == IT[i].next[k])
				bv |= 1 << k;
		    }
	}

	for (i = 0; i < n_EP; i++) {
		if (EP[i].instr < 0)
			continue;		/* skip undefined entry points */
		new_dag (&(EP[i]));
		clear_flow();
		layout (&(EP[i]));
		gen_ps();
	}

	fprintf(plot_out, "%%EOF\n");
}

clear_flow()
{
	int i;
	struct flow_dag *fd, *t;

	for (fd = fd_root; fd;) {
		fd->x = -1;
		t = fd->lst_ptr;		/* note: we might delete fd, threfore ... */
		if (fd->type == FD_GOTO) {
			(fd->fd_next[0])->fd_next[fd->patch_pos] = fd->fd_next[1];
			free_fde (fd);			
		}
		fd = t;		
	}
}

static int	vert_extend[plt_max_col];
static int	is_goto[plt_max_col];
static int	max_width;
static struct	flow_dag **sa = 0;
static int	n_sa, max_sa = 0;

gen_ps()
{
	struct flow_dag *fd;
	int i, gr_key(), y_min, np_y, np_x, x_cnt;
	static int pagecnt = 1;

	if (sa == 0) {
		max_sa = 300;
		sa = (struct flow_dag **) malloc (max_sa * sizeof (struct flow_dag *));
	}

	for (i = 0, y_min = 0; i < plt_max_col; i++)
		if (y_min > vert_extend[i])
		    y_min = vert_extend[i];
	y_min = -y_min;				/* height of flow graph in points */

	if (max_width > 5 || y_min > plt_page_height) {	/* this is going to be a multi-page gizmo */
		np_x = max_width / 5 + 1;
		np_y = y_min / plt_page_height + 1;
		y_min = plt_page_height * np_y;
		fprintf(plot_out, "/x0 {\n");
		x_cnt = 1;
	} else {
		np_y = 0;
		np_x = 0;
		y_min = plt_page_height;
	}

	for (fd = fd_root, n_sa = 0; fd; fd = fd->lst_ptr)
		if (fd->x >= 0) {
			if (n_sa >= max_sa) {
				max_sa += max_sa + 100;
				sa = (struct flow_dag **) realloc (sa, max_sa * sizeof (struct flow_dag *));
			}
			sa[n_sa++] = fd;
			fd->y += y_min;
		}

	qsort(sa, n_sa, sizeof(struct flow_dag *), gr_key);
	for (i = 0; i < n_sa; i++)
		sa[i]->sa_ind = i;

	fprintf(plot_out, "0.5 setlinewidth\n");

	for (i = 0; i < n_sa; i++) {
		if (np_y && !((i + 1) % 50))
			fprintf (plot_out, "} def /x%d {\n", x_cnt++);
		plt_ins(sa[i]);
	}

	if (np_y) {
		fprintf(plot_out, "} def {x0");
		for (i = 1; i < x_cnt; i++)
			fprintf(plot_out, " x%d", i);
		fprintf(plot_out, "} %d %d MPG\n", np_x, np_y);
	} else
		fprintf(plot_out, "showpage\n%%%%Page: %d %d\n", pagecnt, pagecnt);
	pagecnt++;
}

gr_key(a, b)
	struct flow_dag **a, **b;
{
	if ((*a)->y < (*b)->y) return  1;
	if ((*a)->y > (*b)->y) return -1;
	if ((*a)->x < (*b)->x) return  1;
	if ((*a)->x > (*b)->x) return -1;
	return 0;
}

layout (ep_ptr)
	struct ept_def *ep_ptr;
{
	int i;

	for (i = 0; i < plt_max_col; i++)
		vert_extend[i] = 0;
	max_width = 0;
	left_descent (ep_ptr->fd_ptr, 0);
	right_descent (ep_ptr->fd_ptr, 0);
}

right_descent (fd, col)
	struct flow_dag *fd;
	int col;
{
	int i, j, py, pyi;

	fd->x = plt_col_width/2 + col * plt_col_width + 36;
	if (max_width < col)
	    max_width = col;

	py = pyi = vert_extend[col];
	j = fd->n_next;
	if ((col + j) > plt_max_col) {
		fprintf(stderr, "Sorry: plot is too wide (plt_max_col)\n");
		exit(0);	/* this ain't fatal */
	}
	for (i = 1; i < j; i++)
		if (py > vert_extend[col + i])
		    py = vert_extend[col + i];
	fd->y = py;
	fd->ycl = pyi - py;

	py -= fd->height = b_box_height(fd);
	vert_extend[col] = py;		/* takes cares of goto's */
	for (i = 1; i < j; i++)
		vert_extend[col + i] = py;

	for (i = j; i--;)
		    right_descent (fd->fd_next[i], col + i);
}

left_descent (fd, d)
	struct flow_dag *fd;
	int d;
{
	int i, j;

	fd->x = -2;
	fd->depth = d++;
	j = fd->n_next;

	for (i = 0; i < j; i++) {
	    if (fd->fd_next[i]->x != -2)
		left_descent(fd->fd_next[i], d);
	    else {					/* seen this node before */
		struct flow_dag *new;
		new = new_fde();
		new->type = FD_GOTO;
		new->st_ptr = fd->fd_next[i]->st_ptr;
		new->it_ptr = fd->fd_next[i]->it_ptr;
		new->x = -2;
		new->n_next = 0;

		new->patch_pos = i;			/* remember how to restore things */
		new->fd_next[0] = fd;
		new->fd_next[1] = fd->fd_next[i];

		fd->fd_next[i] = new;
	    }
	}
}

/************** How things look like *******************/

int b_box_height(fd)
	struct flow_dag *fd;
{
	int h;

	switch (fd->type) {
	case FD_ENTRY_PT:
		h = 36;
		break;
	case FD_LABEL:
	case FD_GOTO:
	case FD_INSTR:
		h = 18;
		break;
	case FD_TEST:
	case FD_SLEEP:
		h = 18 * 4;
		break;
	default:
		yyerror("internal bug: undefined flow-chart object");
	}

	return h;
}

#define WD2	((plt_col_width * 4) / 10)		/* 80% of 1/2 withdth - to draw boxes */

int direct_goto(fd)
	struct flow_dag *fd;
{
	struct ins_tab *iptr;
	int i, j, x, x1, y;
	iptr = fd->it_ptr;
	x = fd->x;
	y = fd->y;

	for (i = fd->sa_ind + 1; i < n_sa; i++) {
		if (sa[i]->it_ptr == iptr && sa[i]->type == FD_LABEL)
			break;				/* found target */

		x1 = sa[i]->x;
		j = sa[i]->n_next;
		if (j > 0) j--;
		if (x1 <= x && (x1 + j * plt_col_width) >= x)
			return 0;			/* run into somthing */
	}

	if (i >= n_sa)
		return 0;				/* target not found */

	x1 = sa[i]->x;
	for (j = fd->sa_ind + 1; i < i; i++)		/* look for obstacles */
		if (sa[j]->x > x1 && sa[j]->x < x)
			return 0;

	fprintf(plot_out, "newpath %d %d moveto %d %d lineto %d %d lineto stroke\n",
				x, y, x, sa[i]->y - 9, sa[i]->x + 5, sa[i]->y - 9);
	return 1;
}
plt_ins(fd)
	struct flow_dag *fd;
{
	int i, x, y, x1, y1, h;
	int j, cnt, first, space;
	char *p;

	x = fd->x;		/* top entry point */
	y = fd->y;
	h = fd->height;		/* height of symbol */

	fprintf(plot_out, "%%%% sym: %s  ins: %s\n",
		(fd->st_ptr) ? fd->st_ptr->mnemo : "n/a",
		(fd->it_ptr) ? ID[fd->it_ptr->ins_def].name->mnemo : "n/a");

	if (fd->ycl > 0)	/* connect to previous object */
		fprintf(plot_out, "newpath %d %d moveto %d %d lineto stroke\n", x, y, x, y + fd->ycl);

	switch (fd->type) {
	case FD_LABEL:
		for (p = fd->st_ptr->mnemo; *p && *p != '-'; p++);
		if (*p != '-')
			fprintf(plot_out, "gsave 1 setlinewidth\n");
		fprintf(plot_out, "newpath %d %d 5 0 360 arc stroke\n", x, y - 9);
		if (*p != '-')
			fprintf(plot_out, "grestore\n");
		fprintf(plot_out, "newpath %d %d moveto %d %d lineto stroke\n", x, y, x, y - 4);
		fprintf(plot_out, "newpath %d %d moveto %d %d lineto stroke\n", x, y - 14, x, y - 18);
		fprintf(plot_out, "(%s) LBF %d %d LTXT\n", fd->st_ptr->mnemo, x - 7, y - 12);

		fprintf(plot_out, "(%d) PBF %d %d RTXT\n", fd->st_ptr->src_line, x + 7, y - 7);
		break;

	case FD_ENTRY_PT:
		fprintf(plot_out, "(%s) EPF %d %d RTXT\n", fd->st_ptr->mnemo, x - WD2, y - 14);

		y -= 18;
		fprintf(plot_out, "gsave 1 setlinewidth\n");
		fprintf(plot_out, "newpath %d %d 5 0 360 arc stroke\n", x, y - 9);
		fprintf(plot_out, "newpath %d %d 7 0 360 arc stroke\n", x, y - 9);
		fprintf(plot_out, "grestore\n");
		fprintf(plot_out, "newpath %d %d moveto %d %d lineto stroke\n", x, y - 16, x, y - 18);

		fprintf(plot_out, "(%d) PBF %d %d RTXT\n", fd->st_ptr->src_line, x + 9, y - 8);
		break;

	case FD_GOTO:
		if (direct_goto(fd))
			break;
		fprintf(plot_out, "newpath %d %d moveto %d %d lineto %d %d lineto stroke\n",
			x, y, x, y - 11, x + 9, y - 11);
		fprintf(plot_out, "newpath %d %d moveto %d %d lineto %d %d lineto stroke\n",
			x + 6, y - 8, x + 9, y - 11, x + 6, y - 14);
		fprintf(plot_out, "(%s) PBF %d %d RTXT\n", fd->st_ptr->mnemo, x + 11, y - 13);
		fprintf(plot_out, "(%d) PBF %d %d RTXT\n", fd->st_ptr->src_line, x + 11, y - 6);
		break;

	case FD_INSTR:
		fprintf(plot_out, "newpath %d %d moveto %d %d lineto %d %d lineto %d %d lineto closepath stroke\n",
			x - WD2, y - 4, x + WD2, y - 4, x + WD2, y - 14, x - WD2, y - 14);
		fprintf(plot_out, "newpath %d %d moveto %d %d lineto stroke\n", x, y, x, y - 4);
		fprintf(plot_out, "newpath %d %d moveto %d %d lineto stroke\n", x, y - 14, x, y - 18);
		if (fd->it_ptr->comment)
			fprintf(plot_out, "(%s) UBF %d %d RTXT\n", fd->it_ptr->comment, x + 2, y - 2);

		fprintf(plot_out, "(%s   %x  %x %x) NBF %d %d RTXT\n", ID[fd->it_ptr->ins_def].name->mnemo,
			(fd->it_ptr->body >> 4) & 0xf, (fd->it_ptr->body >> 8) & 0xf, (fd->it_ptr->body >> 12) & 0xf,
			x - WD2 + 2, y - 12);
		fprintf(plot_out, "(%d) PBF %d %d RTXT\n", fd->it_ptr->src_ln, x + WD2 + 2, y - 9);
		if (fd->n_next == 0)
		    fprintf(plot_out, "gsave 2.0 setlinewidth newpath %d %d moveto %d %d lineto stroke grestore\n",
			    x - 5, y - 18, x + 5, y - 18);
		break;

	case FD_SLEEP:
	case FD_TEST:
		fprintf(plot_out, "(%s   %x  %x) NBF %d %d RTXT\n", ID[fd->it_ptr->ins_def].name->mnemo,
			(fd->it_ptr->body >> 4) & 0xf, (fd->it_ptr->body >> 8) & 0xf, (fd->it_ptr->body >> 12) & 0xf,
			x - WD2 + 2, y - 12);
		fprintf(plot_out, "newpath %d %d moveto %d %d lineto stroke\n", x, y, x, y - 4);
		if (fd->it_ptr->comment)
			fprintf(plot_out, "(%s) UBF %d %d RTXT\n", fd->it_ptr->comment, x + 2, y - 2);

		for (i = 0; i < fd->n_next; i++) {
		    for (cnt = 0, j = 0; j < N_succ; j++)
			if (fd->succ[j] == i && ID[fd->it_ptr->ins_def].rtn_sym[j])
			    cnt++;
		    /*
		     * OK, now we have cnt lines to draw in the current
		     * plot column.  Figure out a reasonable spacing...
		     */
		    space = 6;
		    first = x + i * plt_col_width - space * (cnt - 1) / 2;
		    for (cnt = 0, j = 0; j < N_succ; j++)
			if (fd->succ[j] == i && ID[fd->it_ptr->ins_def].rtn_sym[j]) {
			    fprintf(plot_out, "newpath %d %d moveto %d %d lineto stroke\n",
				    first + space * cnt, y - 14,
				    first + space * cnt, y - 72);
			    fprintf(plot_out, "(%s) UBF %d %d DTXT\n",
				    ID[fd->it_ptr->ins_def].rtn_sym[j]->mnemo,
				    first + space * cnt - 1, y - 16);
			    cnt++;
			}
		    fprintf(plot_out, "newpath %d %d moveto %d %d lineto stroke\n",
			    first, y - 72, first + (cnt - 1) * space, y - 72);
		}

		if (fd->type == FD_SLEEP)
		    fprintf(plot_out, "gsave 1.5 setlinewidth\n");
		x1 = x + WD2 + (fd->n_next - 1) * plt_col_width;
		x -= WD2;
		y1 = y - 14;
		y -= 4;
		fprintf(plot_out, "newpath %d %d moveto\n", x, y);
		fprintf(plot_out, "%d %d lineto %d %d lineto %d %d lineto %d %d lineto %d %d lineto closepath stroke\n",
			x1, y, x1 + 5, y - 5, x1, y1, x, y1, x - 5, y - 5);
		if (fd->type == FD_SLEEP)
		    fprintf(plot_out, "grestore\n");

		fprintf(plot_out, "(%d) PBF %d %d RTXT\n", fd->it_ptr->src_ln, x1 + 6, y - 5);
		break;

	default:
		yyerror("internal bug: undefined flow-chart object");
	}
}
