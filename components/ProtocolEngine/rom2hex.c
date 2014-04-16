#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char* argv[]){

	unsigned int addr, opcode, arg1, arg2, next;
	char buffer[256];
	FILE* romfile;
	FILE* hexfile;

	if(argc != 3){
	    fprintf(stderr,"USAGE:\n\trom2hex <file.rom> <file.hex>");
	    exit(1);
	}
	if((romfile = fopen(argv[1],"r")) == NULL){
	    fprintf(stderr,"%s: cannot open %s for reading\n",argv[0], argv[1]);
	    exit(2);
	}
	if((hexfile = fopen(argv[2],"w")) == NULL){
	    fprintf(stderr,"%s: cannot open %s for writing\n",argv[0], argv[2]);
	    exit(3);
	}

	while(fgets(buffer,255,romfile)){
	    if(sscanf(buffer,"%d %d %d %d %d" ,&addr, &opcode, &arg1, &arg2, &next) == 5){
		fprintf(hexfile,"@%03x %05x\n", addr, (opcode<<17)|(arg1<<13)|(arg2<<9)|next);
	    }
	}

	exit(0);
}
    
