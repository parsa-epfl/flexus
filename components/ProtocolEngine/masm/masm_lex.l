%%
[ \t]*			; /* ignore white space		*/
"/*".*"*/"			{if (!no_comment) add_comment (&(yytext[2]));}
"//".*$				{if (!no_comment) add_comment (&(yytext[2]));}

^"#".*\n			{if (2 != sscanf (&yytext[1],"%d%s",
					&yylineno, src_file)) {
				     fprintf (stderr, 
					"Please use: /lib/cpp <src-file> | masm\n");
				     exit (1);
				     }
				 no_comment = 1;
				}
\n				{no_comment = 1;}

"&&"				{return AND;}
"||"				{return OR;}
"!"				{return NOT;}
"("				{return OPAR;}
")"				{return CPAR;}
";"				{no_comment = 0; return TERM;}
"{"				{return STRT_BLK;}
"}"				{return END_BLK;}
if				{return IF;}
else				{return ELSE;}
while				{return WHILE;}
elseif				{return ELSEIF;}
halt				{return HALT;}
"%end"				{return END;}
goto				{return GOTO;}
[A-Za-z][A-Za-z0-9._$]*"::"	{yylval.SYM = (char *) malloc (yyleng);
				 strncpy (yylval.SYM, yytext, yyleng);
				 yylval.SYM[yyleng - 2] = 0;
				 return ENTRY_PNT;}
[A-Za-z][A-Za-z0-9._$]*":"	{yylval.SYM = (char *) malloc (yyleng);
				 strncpy (yylval.SYM, yytext, yyleng);
				 yylval.SYM[yyleng - 1] = 0;
				 return LABEL;}
[A-Za-z][A-Za-z0-9._$]*		{yylval.SYM = (char *) malloc (yyleng + 1);
				 strcpy (yylval.SYM, yytext);
				 return SYMBOL;}
0				{yylval.BOOL = 0; return CON;}
1				{yylval.BOOL = anyway; return CON;}
.				{return yytext[0];}

%%
