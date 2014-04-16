# include "stdio.h"
# define U(x) ((x)&0377)
# define NLSTATE yyprevious=YYNEWLINE
# define BEGIN yybgin = yysvec + 1 +
# define INITIAL 0
# define YYLERR yysvec
# define YYSTATE (yyestate-yysvec-1)
# define YYOPTIM 1
# define YYLMAX 200
# define output(c) putc(c,yyout)
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
# define unput(c) {yytchar= (c);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;}
# define yymore() (yymorfg=1)
# define ECHO fprintf(yyout, "%s",yytext)
# define REJECT { nstr = yyreject(); goto yyfussy;}
int yyleng; extern unsigned char yytext[];
int yymorfg;
extern unsigned char *yysptr, yysbuf[];
int yytchar;
FILE *yyin = 0, *yyout = 0;
extern int yylineno;
struct yysvf { 
	struct yywork *yystoff;
	struct yysvf *yyother;
	int *yystops;};
struct yysvf *yyestate;
extern struct yysvf yysvec[], *yybgin;
# define YYNEWLINE 10
yylex(){
int nstr; extern int yyprevious;
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:
		;
break;
case 2:
		{if (!no_comment) add_comment (&(yytext[2]));}
break;
case 3:
			{if (!no_comment) add_comment (&(yytext[2]));}
break;
case 4:
		{if (2 != sscanf (&yytext[1],"%d%s",
					&yylineno, src_file)) {
				     fprintf (stderr, 
					"Please use: /lib/cpp <src-file> | masm\n");
				     exit (1);
				     }
				 no_comment = 1;
				}
break;
case 5:
			{no_comment = 1;}
break;
case 6:
			{return AND;}
break;
case 7:
			{return OR;}
break;
case 8:
			{return NOT;}
break;
case 9:
			{return OPAR;}
break;
case 10:
			{return CPAR;}
break;
case 11:
			{no_comment = 0; return TERM;}
break;
case 12:
			{return STRT_BLK;}
break;
case 13:
			{return END_BLK;}
break;
case 14:
			{return IF;}
break;
case 15:
			{return ELSE;}
break;
case 16:
			{return WHILE;}
break;
case 17:
			{return ELSEIF;}
break;
case 18:
			{return HALT;}
break;
case 19:
			{return END;}
break;
case 20:
			{return GOTO;}
break;
case 21:
{yylval.SYM = (char *) malloc (yyleng);
				 strncpy (yylval.SYM, yytext, yyleng);
				 yylval.SYM[yyleng - 2] = 0;
				 return ENTRY_PNT;}
break;
case 22:
{yylval.SYM = (char *) malloc (yyleng);
				 strncpy (yylval.SYM, yytext, yyleng);
				 yylval.SYM[yyleng - 1] = 0;
				 return LABEL;}
break;
case 23:
	{yylval.SYM = (char *) malloc (yyleng + 1);
				 strcpy (yylval.SYM, yytext);
				 return SYMBOL;}
break;
case 24:
			{yylval.BOOL = 0; return CON;}
break;
case 25:
			{yylval.BOOL = anyway; return CON;}
break;
case 26:
			{return yytext[0];}
break;
case -1:
break;
default:
fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */
int yyvstop[] = {
0,

1,
0,

1,
0,

26,
0,

1,
26,
0,

5,
0,

8,
26,
0,

26,
0,

26,
0,

9,
26,
0,

10,
26,
0,

26,
0,

24,
26,
0,

25,
26,
0,

11,
26,
0,

23,
26,
0,

23,
26,
0,

23,
26,
0,

23,
26,
0,

23,
26,
0,

23,
26,
0,

12,
26,
0,

26,
0,

13,
26,
0,

26,
0,

1,
0,

6,
0,

-3,
0,

23,
0,

22,
0,

23,
0,

23,
0,

23,
0,

14,
23,
0,

23,
0,

7,
0,

4,
0,

3,
0,

21,
0,

23,
0,

23,
0,

23,
0,

23,
0,

19,
0,

2,
0,

15,
23,
0,

20,
23,
0,

18,
23,
0,

23,
0,

23,
0,

16,
23,
0,

17,
23,
0,
0};
# define YYTYPE unsigned char
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	1,3,	0,0,	
0,0,	0,0,	0,0,	0,0,	
24,38,	0,0,	1,4,	1,5,	
4,25,	49,0,	0,0,	4,25,	
24,38,	24,39,	28,28,	0,0,	
29,29,	0,0,	0,0,	0,0,	
0,0,	41,0,	28,28,	28,0,	
29,29,	29,42,	0,0,	0,0,	
0,0,	0,0,	1,6,	4,25,	
0,0,	1,3,	1,7,	1,8,	
8,27,	1,9,	1,10,	24,38,	
0,0,	49,41,	11,28,	0,0,	
1,11,	1,12,	1,13,	11,29,	
0,0,	28,28,	0,0,	29,29,	
0,0,	41,41,	0,0,	28,41,	
1,14,	31,43,	41,49,	2,6,	
0,0,	2,24,	1,15,	2,7,	
2,8,	0,0,	2,9,	2,10,	
24,38,	0,0,	0,0,	0,0,	
0,0,	2,11,	2,12,	2,13,	
0,0,	0,0,	28,28,	0,0,	
29,29,	0,0,	0,0,	0,0,	
0,0,	2,14,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	18,34,	0,0,	
0,0,	40,48,	1,16,	7,26,	
1,17,	1,18,	1,19,	19,35,	
20,36,	16,32,	34,46,	26,40,	
17,33,	36,47,	44,50,	45,51,	
32,44,	33,45,	46,52,	47,53,	
1,20,	50,54,	53,55,	54,56,	
1,21,	1,22,	1,23,	22,37,	
0,0,	0,0,	0,0,	2,16,	
0,0,	2,17,	2,18,	2,19,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	15,30,	0,0,	
0,0,	2,20,	0,0,	0,0,	
0,0,	2,21,	2,22,	2,23,	
15,30,	0,0,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,31,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	0,0,	0,0,	0,0,	
0,0,	15,30,	0,0,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	15,30,	15,30,	15,30,	
15,30,	0,0,	0,0,	0,0,	
0,0};
struct yysvf yysvec[] = {
0,	0,	0,
yycrank+-1,	0,		yyvstop+1,
yycrank+-30,	yysvec+1,	yyvstop+3,
yycrank+0,	0,		yyvstop+5,
yycrank+3,	0,		yyvstop+7,
yycrank+0,	0,		yyvstop+10,
yycrank+0,	0,		yyvstop+12,
yycrank+2,	0,		yyvstop+15,
yycrank+2,	0,		yyvstop+17,
yycrank+0,	0,		yyvstop+19,
yycrank+0,	0,		yyvstop+22,
yycrank+4,	0,		yyvstop+25,
yycrank+0,	0,		yyvstop+27,
yycrank+0,	0,		yyvstop+30,
yycrank+0,	0,		yyvstop+33,
yycrank+110,	0,		yyvstop+36,
yycrank+1,	yysvec+15,	yyvstop+39,
yycrank+1,	yysvec+15,	yyvstop+42,
yycrank+1,	yysvec+15,	yyvstop+45,
yycrank+5,	yysvec+15,	yyvstop+48,
yycrank+4,	yysvec+15,	yyvstop+51,
yycrank+0,	0,		yyvstop+54,
yycrank+3,	0,		yyvstop+57,
yycrank+0,	0,		yyvstop+59,
yycrank+-7,	0,		yyvstop+62,
yycrank+0,	yysvec+4,	yyvstop+64,
yycrank+1,	0,		0,	
yycrank+0,	0,		yyvstop+66,
yycrank+-17,	0,		0,	
yycrank+-19,	0,		yyvstop+68,
yycrank+0,	yysvec+15,	yyvstop+70,
yycrank+3,	0,		yyvstop+72,
yycrank+1,	yysvec+15,	yyvstop+74,
yycrank+1,	yysvec+15,	yyvstop+76,
yycrank+2,	yysvec+15,	yyvstop+78,
yycrank+0,	yysvec+15,	yyvstop+80,
yycrank+8,	yysvec+15,	yyvstop+83,
yycrank+0,	0,		yyvstop+85,
yycrank+0,	yysvec+24,	0,	
yycrank+0,	0,		yyvstop+87,
yycrank+1,	0,		0,	
yycrank+-15,	yysvec+28,	0,	
yycrank+0,	0,		yyvstop+89,
yycrank+0,	0,		yyvstop+91,
yycrank+13,	yysvec+15,	yyvstop+93,
yycrank+4,	yysvec+15,	yyvstop+95,
yycrank+2,	yysvec+15,	yyvstop+97,
yycrank+11,	yysvec+15,	yyvstop+99,
yycrank+0,	0,		yyvstop+101,
yycrank+-3,	yysvec+28,	yyvstop+103,
yycrank+16,	yysvec+15,	yyvstop+105,
yycrank+0,	yysvec+15,	yyvstop+108,
yycrank+0,	yysvec+15,	yyvstop+111,
yycrank+21,	yysvec+15,	yyvstop+114,
yycrank+21,	yysvec+15,	yyvstop+116,
yycrank+0,	yysvec+15,	yyvstop+118,
yycrank+0,	yysvec+15,	yyvstop+121,
0,	0,	0};
struct yywork *yytop = yycrank+232;
struct yysvf *yybgin = yysvec+1;
unsigned char yymatch[] = {
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,012 ,01  ,011 ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
011 ,01  ,01  ,01  ,'$' ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,'$' ,01  ,
'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,
'$' ,'$' ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,01  ,01  ,01  ,01  ,'$' ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
0};
unsigned char yyextra[] = {
0,0,0,1,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0};
/*
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
/*
 * OSF/1 Release 1.0
*/
/*
#
# IBM CONFIDENTIAL
# Copyright International Business Machines Corp. 1989
# Unpublished Work
# All Rights Reserved
# Licensed Material - Property of IBM
#
#
# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
# 
*/
/* @(#)ncform	1.3  com/lib/l,3.1,8951 9/7/89 18:48:47 */
int yylineno =1;
# define YYU(x) x
# define NLSTATE yyprevious=YYNEWLINE
unsigned char yytext[YYLMAX];
struct yysvf *yylstate [YYLMAX], **yylsp, **yyolsp;
unsigned char yysbuf[YYLMAX];
unsigned char *yysptr = yysbuf;
int *yyfnd;
extern struct yysvf *yyestate;
int yyprevious = YYNEWLINE;
yylook(){
	register struct yysvf *yystate, **lsp;
	register struct yywork *yyt;
	struct yysvf *yyz;
	int yych, yyfirst;
	struct yywork *yyr;
# ifdef LEXDEBUG
	int debug;
# endif
	unsigned char *yylastch;
	/* start off machines */
# ifdef LEXDEBUG
	debug = 0;
# endif
	yyfirst=1;
	if (!yymorfg)
		yylastch = yytext;
	else {
		yymorfg=0;
		yylastch = yytext+yyleng;
		}
	for(;;){
		lsp = yylstate;
		yyestate = yystate = yybgin;
		if (yyprevious==YYNEWLINE) yystate++;
		for (;;){
# ifdef LEXDEBUG
			if(debug)fprintf(yyout,"state %d\n",yystate-yysvec-1);
# endif
			yyt = yystate->yystoff;
			if(yyt == yycrank && !yyfirst){  /* may not be any transitions */
				yyz = yystate->yyother;
				if(yyz == 0)break;
				if(yyz->yystoff == yycrank)break;
				}
			*yylastch++ = yych = input();
            if (yylastch >= yytext + YYLMAX) {
                fprintf(yyout, "Maximum token length exceeded\n");
                yytext[YYLMAX] = 0;
                return 0;
            }
			yyfirst=0;
		tryagain:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"char ");
				allprint(yych);
				putchar('\n');
				}
# endif
			yyr = yyt;
			if ( yyt > yycrank){
				yyt = yyr + yych;
				if (yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
# ifdef YYOPTIM
			else if(yyt < yycrank) {		/* r < yycrank */
				yyt = yyr = yycrank+(yycrank-yyt);
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"compressed state\n");
# endif
				yyt = yyt + yych;
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				yyt = yyr + YYU(yymatch[yych]);
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"try fall back character ");
					allprint(YYU(yymatch[yych]));
					putchar('\n');
					}
# endif
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transition */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
			if ((yystate = yystate->yyother) && (yyt= yystate->yystoff) != yycrank){
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"fall back to state %d\n",yystate-yysvec-1);
# endif
				goto tryagain;
				}
# endif
			else
				{unput(*--yylastch);break;}
		contin:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"state %d char ",yystate-yysvec-1);
				allprint(yych);
				putchar('\n');
				}
# endif
			;
			}
# ifdef LEXDEBUG
		if(debug){
			fprintf(yyout,"stopped at %d with ",*(lsp-1)-yysvec-1);
			allprint(yych);
			putchar('\n');
			}
# endif
		while (lsp-- > yylstate){
			*yylastch-- = 0;
			if (*lsp != 0 && (yyfnd= (*lsp)->yystops) && *yyfnd > 0){
				yyolsp = lsp;
				if(yyextra[*yyfnd]){		/* must backup */
					while(yyback((*lsp)->yystops,-*yyfnd) != 1 && lsp > yylstate){
						lsp--;
						unput(*yylastch--);
						}
					}
				yyprevious = YYU(*yylastch);
				yylsp = lsp;
				yyleng = yylastch-yytext+1;
                if (yyleng >= YYLMAX) {
                    fprintf(yyout, "Maximum token length exceeded\n");
                    yytext[YYLMAX] = 0;
                    return 0;
                }
				yytext[yyleng] = 0;
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"\nmatch ");
					sprint(yytext);
					fprintf(yyout," action %d\n",*yyfnd);
					}
# endif
				return(*yyfnd++);
				}
			unput(*yylastch);
			}
		if (yytext[0] == 0  /* && feof(yyin) */)
			{
			yysptr=yysbuf;
			return(0);
			}
		yyprevious = yytext[0] = input();
		if (yyprevious>0)
			output(yyprevious);
		yylastch=yytext;
# ifdef LEXDEBUG
		if(debug)putchar('\n');
# endif
		}
	}
yyback(p, m)
	int *p;
{
if (p==0) return(0);
while (*p)
	{
	if (*p++ == m)
		return(1);
	}
return(0);
}
	/* the following are only used in the lex library */
yyinput(){
	return(input());
	}
yyoutput(c)
  int c; {
	output(c);
	}
yyunput(c)
   int c; {
	unput(c);
	}
