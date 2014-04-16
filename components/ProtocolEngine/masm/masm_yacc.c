
# line 2 "masm_yacc.y"

#include <strings.h>
#include <stdio.h>
#include "masm.h"

#define YYMAXDEPTH 300		/* Piranha needs more... */


# line 13 "masm_yacc.y"
typedef union  ytoken { unsigned long BOOL; char *SYM; struct ins_tab *INS;} YYSTYPE;
# define CON 257
# define IF 258
# define ELSEIF 259
# define ELSE 260
# define GOTO 261
# define TERM 262
# define OPAR 263
# define CPAR 264
# define END 265
# define STRT_BLK 266
# define END_BLK 267
# define WHILE 268
# define HALT 269
# define LABEL 270
# define SYMBOL 271
# define ENTRY_PNT 272
# define OR 273
# define AND 274
# define NOT 275
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
typedef int yytabelem;
# define YYERRCODE 256

# line 82 "masm_yacc.y"


#include "masm_lex.c"

static const yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 33
# define YYLAST 98
static const yytabelem yyact[]={

    36,    56,    45,    41,    46,    45,    33,     3,    42,    51,
    46,    45,    49,     9,    35,    46,    45,    28,    34,    24,
    17,    13,    22,    21,     9,    41,    10,     4,    50,    25,
    42,    31,    29,    39,    30,    58,    15,    14,     9,     7,
     6,    32,     2,    12,    52,    40,    11,    38,    26,    20,
    16,     8,    19,     5,    23,     1,     0,    18,     0,     0,
     0,     0,    27,     0,     0,     0,     0,    37,     0,     0,
     0,     0,     0,    43,     0,    47,    48,     0,    44,     0,
     0,     0,     0,     0,     0,     0,     0,    54,    55,    53,
     0,     0,     0,    57,     0,     0,     0,    11 };
static const yytabelem yypact[]={

  -265, -1000,  -238, -1000, -1000,  -218,  -243, -1000,  -249,  -225,
  -226, -1000,  -251,  -249, -1000, -1000,  -239, -1000, -1000,  -265,
 -1000, -1000,  -254,  -228,  -257,  -257, -1000, -1000,  -229,  -236,
 -1000,  -257,  -258,  -257,  -257, -1000, -1000,  -269,  -259, -1000,
  -234,  -262, -1000,  -258, -1000,  -257,  -257,  -263, -1000, -1000,
 -1000, -1000,  -218, -1000, -1000,  -272, -1000,  -232, -1000 };
static const yytabelem yypgo[]={

     0,    55,    41,    54,    45,    42,    53,    40,    52,    39,
    51,    43,    50,    49,    48,    47,    44 };
static const yytabelem yyr1[]={

     0,     1,     5,     6,     8,     5,     7,     7,    10,     9,
     9,    11,    11,    14,    12,    15,    15,    13,    13,    13,
    13,     3,     3,     3,     4,    16,     4,     2,     2,     2,
     2,     2,     2 };
static const yytabelem yyr2[]={

     0,     4,     0,     1,     1,    14,     2,     4,     1,     9,
     5,     0,     5,     1,     6,     0,     5,     3,     7,     9,
     5,     7,     9,     5,     5,     1,     9,     7,     7,     7,
     5,     3,     2 };
static const yytabelem yychk[]={

 -1000,    -1,    -5,   272,   265,    -6,    -7,    -9,   -10,   256,
   269,    -9,   -11,   270,   262,   262,   -12,   271,   -11,    -8,
   -13,   262,   261,    -3,   258,   268,   -14,    -5,   271,   260,
   262,   259,    -2,   263,   275,   271,   257,    -2,   -15,   262,
    -4,   261,   266,    -2,    -4,   274,   273,    -2,    -2,   271,
   262,   271,   -16,    -4,    -2,    -2,   264,    -7,   267 };
static const yytabelem yydef[]={

     2,    -2,     0,     3,     1,     8,     8,     6,    11,     0,
     0,     7,     0,    11,    10,     4,     0,    13,    12,     2,
     9,    17,     0,     0,     0,     0,    15,     5,     0,     0,
    20,     0,     0,     0,     0,    31,    32,    23,    14,    18,
     0,     0,    25,     0,    21,     0,     0,     0,    30,    16,
    19,    24,     8,    22,    28,    29,    27,     8,    26 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

yytoktype yytoks[] =
{
	"CON",	257,
	"IF",	258,
	"ELSEIF",	259,
	"ELSE",	260,
	"GOTO",	261,
	"TERM",	262,
	"OPAR",	263,
	"CPAR",	264,
	"END",	265,
	"STRT_BLK",	266,
	"END_BLK",	267,
	"WHILE",	268,
	"HALT",	269,
	"LABEL",	270,
	"SYMBOL",	271,
	"ENTRY_PNT",	272,
	"OR",	273,
	"AND",	274,
	"NOT",	275,
	"-unknown-",	-1	/* ends search */
};

char * yyreds[] =
{
	"-no such reduction-",
      "micro : programs END",
      "programs : /* empty */",
      "programs : ENTRY_PNT",
      "programs : ENTRY_PNT program HALT TERM",
      "programs : ENTRY_PNT program HALT TERM programs",
      "program : instruction",
      "program : program instruction",
      "instruction : /* empty */",
      "instruction : labels body next",
      "instruction : error TERM",
      "labels : /* empty */",
      "labels : LABEL labels",
      "body : SYMBOL",
      "body : SYMBOL args",
      "args : /* empty */",
      "args : args SYMBOL",
      "next : TERM",
      "next : GOTO SYMBOL TERM",
      "next : cond ELSE target TERM",
      "next : cond TERM",
      "cond : IF expr target",
      "cond : cond ELSEIF expr target",
      "cond : WHILE expr",
      "target : GOTO SYMBOL",
      "target : STRT_BLK",
      "target : STRT_BLK program END_BLK",
      "expr : OPAR expr CPAR",
      "expr : expr AND expr",
      "expr : expr OR expr",
      "expr : NOT expr",
      "expr : SYMBOL",
      "expr : CON",
};
#endif /* YYDEBUG */
/*
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
/*
 * OSF/1 Release 1.0
 */
/* @(#)yaccpar	1.3  com/cmd/lang/yacc,3.1, 9/7/89 18:46:37 */
/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#ifdef YYSPLIT
#   define YYERROR	return(-2)
#else
#   define YYERROR	goto yyerrlab
#endif

#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( "syntax error - cannot backup" );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-1000)

#ifdef YYSPLIT
#   define YYSCODE { \
			extern int (*yyf[])(); \
			register int yyret; \
			if (yyf[yytmp]) \
			    if ((yyret=(*yyf[yytmp])()) == -2) \
				    goto yyerrlab; \
				else if (yyret>=0) return(yyret); \
		   }
#endif

/*
** local variables used by the parser
 * these should be static in order to support
 * multiple parsers in a single executable program. POSIX 1003.2-1993
 */
static YYSTYPE yyv[ YYMAXDEPTH ];	/* value stack */
static int yys[ YYMAXDEPTH ];		/* state stack */

static YYSTYPE *yypv;			/* top of value stack */
static YYSTYPE *yypvt;			/* top of value stack for $vars */
static int *yyps;			/* top of state stack */

static int yystate;			/* current state */
static int yytmp;			/* extra var (lasts between blocks) */

/*
** global variables used by the parser - renamed as a result of -p
*/
int yynerrs;			/* number of errors */
int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */

/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
int
yyparse()
{
	/*
	** Initialize externals - yyparse may be called more than once
	*/
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

	goto yystack;
	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ YYMAXDEPTH ] )	/* room on stack? */
		{
			yyerror( "yacc stack overflow" );
			YYABORT;
		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register const int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( "syntax error" );
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
				yynerrs++;
			skip_init:
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/

		switch(yytmp){

case 3:
# line 30 "masm_yacc.y"
{bgn_sequence(yypvt[-0].SYM);} /*NOTREACHED*/ break;
case 4:
# line 32 "masm_yacc.y"
{end_sequence();} /*NOTREACHED*/ break;
case 8:
# line 40 "masm_yacc.y"
{bgn_instr ();} /*NOTREACHED*/ break;
case 9:
# line 41 "masm_yacc.y"
{end_instr ();} /*NOTREACHED*/ break;
case 10:
# line 42 "masm_yacc.y"
{yyerror ("syntax error");} /*NOTREACHED*/ break;
case 12:
# line 46 "masm_yacc.y"
{add_label (yypvt[-1].SYM);} /*NOTREACHED*/ break;
case 13:
# line 49 "masm_yacc.y"
{def_op_code (yypvt[-0].SYM);} /*NOTREACHED*/ break;
case 16:
# line 54 "masm_yacc.y"
{def_op_arg (yypvt[-0].SYM);} /*NOTREACHED*/ break;
case 17:
# line 57 "masm_yacc.y"
{add_next (anyway, 0);} /*NOTREACHED*/ break;
case 18:
# line 58 "masm_yacc.y"
{add_next (anyway, yypvt[-1].SYM);} /*NOTREACHED*/ break;
case 19:
# line 59 "masm_yacc.y"
{add_next (anyway & ~yypvt[-3].BOOL, yypvt[-1].SYM);} /*NOTREACHED*/ break;
case 20:
# line 60 "masm_yacc.y"
{add_next (anyway & ~yypvt[-1].BOOL, 0);} /*NOTREACHED*/ break;
case 21:
# line 63 "masm_yacc.y"
{add_next (yypvt[-1].BOOL, yypvt[-0].SYM); yyval.BOOL = yypvt[-1].BOOL;} /*NOTREACHED*/ break;
case 22:
# line 64 "masm_yacc.y"
{add_next (yypvt[-1].BOOL & ~yypvt[-3].BOOL, yypvt[-0].SYM); yyval.BOOL = yypvt[-1].BOOL|yypvt[-3].BOOL;} /*NOTREACHED*/ break;
case 23:
# line 65 "masm_yacc.y"
{add_self (yypvt[-0].BOOL); yyval.BOOL = yypvt[-0].BOOL;} /*NOTREACHED*/ break;
case 24:
# line 68 "masm_yacc.y"
{yyval.SYM = yypvt[-0].SYM;} /*NOTREACHED*/ break;
case 25:
# line 69 "masm_yacc.y"
{push_ins();} /*NOTREACHED*/ break;
case 26:
# line 71 "masm_yacc.y"
{yyval.SYM = pop_ins();} /*NOTREACHED*/ break;
case 27:
# line 74 "masm_yacc.y"
{yyval.BOOL = yypvt[-1].BOOL;} /*NOTREACHED*/ break;
case 28:
# line 75 "masm_yacc.y"
{yyval.BOOL = yypvt[-2].BOOL & yypvt[-0].BOOL;} /*NOTREACHED*/ break;
case 29:
# line 76 "masm_yacc.y"
{yyval.BOOL = yypvt[-2].BOOL | yypvt[-0].BOOL;} /*NOTREACHED*/ break;
case 30:
# line 77 "masm_yacc.y"
{yyval.BOOL = ~yypvt[-0].BOOL;} /*NOTREACHED*/ break;
case 31:
# line 78 "masm_yacc.y"
{yyval.BOOL = get_cond(yypvt[-0].SYM);} /*NOTREACHED*/ break;
}


	goto yystack;		/* reset registers in driver code */
}
