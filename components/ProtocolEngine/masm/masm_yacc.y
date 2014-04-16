%{

#include <strings.h>
#include <stdio.h>
#include "masm.h"

#define YYMAXDEPTH 300		/* Piranha needs more... */

%}

%start micro			/* Grammar entry		 */

%union ytoken { unsigned long BOOL; char *SYM; struct ins_tab *INS;}
%type <BOOL> expr cond
%type <SYM> target
%token <BOOL> CON
%token IF ELSEIF ELSE GOTO TERM OPAR CPAR END STRT_BLK END_BLK WHILE HALT
%token <SYM> LABEL SYMBOL ENTRY_PNT

%left <BOOL> OR
%left <BOOL> AND
%left <BOOL> NOT

%%

micro	: programs END
	;

programs:
	| ENTRY_PNT			{bgn_sequence($1);}
	  program
	  HALT TERM			{end_sequence();}
	  programs
	;

program : instruction
	| program instruction
	;

instruction : 				{bgn_instr ();}
	  labels body next	        {end_instr ();}
	| error TERM			{yyerror ("syntax error");}
	;

labels	:				
	| LABEL labels			{add_label ($1);}
	;

body	: SYMBOL			{def_op_code ($1);}
	  args
	;

args	:
	| args SYMBOL			{def_op_arg ($2);}
	;

next	: TERM				{add_next (anyway, 0);}
	| GOTO SYMBOL TERM		{add_next (anyway, $2);}
	| cond ELSE target TERM		{add_next (anyway & ~$1, $3);}
	| cond TERM			{add_next (anyway & ~$1, 0);}
	;

cond	: IF expr target		{add_next ($2, $3); $$ = $2;}
	| cond ELSEIF expr target	{add_next ($3 & ~$1, $4); $$ = $3|$1;}
	| WHILE expr			{add_self ($2); $$ = $2;}
	;

target	: GOTO SYMBOL			{$$ = $2;}
	| STRT_BLK			{push_ins();}
	  program
	  END_BLK			{$$ = pop_ins();}
	;

expr	: OPAR expr CPAR		{$$ = $2;}
	| expr AND expr			{$$ = $1 & $3;}
	| expr OR expr			{$$ = $1 | $3;}
	| NOT expr			{$$ = ~$2;}
	| SYMBOL			{$$ = get_cond($1);}
	| CON
	;

%%

#include "masm_lex.c"

