// Test telex functions

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "besk.h"
#include "telex.h"


void usage()
{
    fprintf(stderr, "usage: telex [options] <text>\n");
    fprintf(stderr, "OPTIONS\n");
    fprintf(stderr, "  -e  encode to itu2\n");
    fprintf(stderr, "  -d  decode from itu2\n");
    fprintf(stderr, "  -l  latin1\n");
    fprintf(stderr, "  -a  ascii\n");
    exit(1);
}

int main(int argc, char** argv)
{
    int encode = 1;
    int latin1 = 0;
    int opt;

    while ((opt = getopt(argc, argv, "ed")) != -1) {
	switch(opt) {
	case 'e': encode = 1; break;
	case 'd': encode = 0; break;
	case 'l': latin1 = 1; break;
	case 'a': latin1 = 0; break;
	default: usage();
	}
    }

    if (encode) {
	char* str = argv[optind];
	int page = 0;
	int dpage = 0;
	int c;
	
	while((c = *str++) != 0) {
	    uint8_t buf[2];
	    uint16_t ochar;
	    
	    switch(telex_encode(c, &page, buf)) {
	    case 0: break;
	    case 1:
		telex_write_remsa(stdout, buf[0]);
		printf("ITA2: %02X", buf[0]);		
		if (telex_decode(buf[0], &dpage, &ochar) == 1)
		    printf(" => [%c]\n", ochar);
		else
		    printf("\n");
		break;		
	    case 2:
		telex_write_remsa(stdout, buf[0]);
		telex_write_remsa(stdout, buf[1]);
		printf("ITA2: %02X,%02X", buf[0], buf[1]);		
		if (telex_decode(buf[0], &dpage, &ochar) == 1)
		    printf(" => [%c] ", ochar);
		else
		    ;
		if (telex_decode(buf[1], &dpage, &ochar) == 1)
		    printf(" => [%c]\n", ochar);
		else
		    printf("\n");
		break;
	    default:
		printf("ERROR\n");
		exit(1);
	    }
	}
    }
    exit(0);
}
