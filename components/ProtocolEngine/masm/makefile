#
# TMC micro code assembler
#
CFLAGS = -g
CC = cc

.KEEP_STATE:
all: masm anno

masm:   masm.o masm_yacc.c masm_lex.c
	$(CC) $(CFLAGS) -o masm masm.o masm_yacc.c -lfl

anno:  anno.c
	$(CC) $(CFLAGS) -o anno anno.c

install:
	cp masm	anno /wrl/proj/piranha/bin



