// Paper tape punch utility
// Create input data files from other formats

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <memory.h>
#include <math.h>

#include "besk.h"

void usage()
{
    fprintf(stderr, "usage: tape [options]\n");
    fprintf(stderr, "OPTIONS\n");
    fprintf(stderr, "  -i <input-file>\n");
    fprintf(stderr, "  -u <output-file>\n");
    exit(1);
}

void write_4_channel_remsa(FILE* f, uint8_t code)
{
    fprintf(f, "%c%c%c%co\n",
	    ((code>>3) & 1) ? 'o' : '-',
	    ((code>>2) & 1) ? 'o' : '-',
	    ((code>>1) & 1) ? 'o' : '-',
	    ((code>>0) & 1) ? 'o' : '-');
}

void write_helord_remsa(FILE* f, helord_t value)
{
    int i;
    for (i = 36; i >= 0; i -= 4)
	write_4_channel_remsa(f, (value>>i) & 0xf);
}

int xdigit(int c)
{
    return (c & 0x10) ? (c & 0xF) : (c & 0xF) + 9;
}

int main(int argc, char** argv)
{
    char* input_file_name = NULL;
    char* output_file_name = NULL;
    FILE* fin = stdin;
    FILE* fout = stdout;
    char line[81];
    char* ptr;
    int ln = 0;
    int opt;
    
    while ((opt = getopt(argc, argv, "i:u:")) != -1) {
	switch(opt) {
	case 'i': input_file_name = optarg; break;
	case 'u': output_file_name = optarg; break;
	default: usage();
	}
    }

    if (input_file_name == NULL)
	input_file_name = "*stdin*";
    else {
	if ((fin = fopen(input_file_name, "r")) == NULL) {
	    fprintf(stderr, "unable to open input file %s\n",
		    input_file_name);
	    exit(1);
	}
    }

    if (output_file_name == NULL)
	output_file_name = "*stdout*";
    else {
	if ((fout = fopen(output_file_name, "w")) == NULL) {
	    fprintf(stderr, "unable to open output file %s\n",
		    output_file_name);
	    exit(1);
	}
    }

    // read various formats and write it as paper tape 40bit word format
    // # comment
    // <floating point> => 40 bit BESK value
    // vhex5 hhex5      => 40 bit BESK value
    // hex10            => 40 bit BESK value
    while((ptr = fgets(line, sizeof(line), fin)) != NULL) {
	double df;
	uint64_t v;
	char* endptr = NULL;
	
	ln++;
	while(isblank(*ptr)) ptr++;
	if (*ptr == '#') continue;
	// fprintf(stdout, "LINE = %s", ptr);
	df = strtod(ptr, &endptr);
	if ((endptr != NULL) && ((*endptr == '\0')||(isspace(*endptr)))) {
	    fprintf(fout, "# %f\n", df);
	    if ((df >= -1.0) && (df < 1.0)) {
		if (df >= 0.0)
		    v = df * 0x8000000000;
		else
		    v = -(fabs(df)*0x8000000000);
		write_helord_remsa(fout, (helord_t) v);
		fprintf(fout, "\n");
	    }
	    else {
		fprintf(stderr, "%s:%d: value is out of range\n",
			input_file_name, ln);
	    }
	}
	else {
	    int n;

	    v = 0;
	    n = 0;
	    while(isxdigit(*ptr)) {
		v = (v<<4) + xdigit(*ptr++);
		n++;
	    }
	    while(isblank(*ptr)) ptr++;
	    while(isxdigit(*ptr)) {
		v = (v<<4) + xdigit(*ptr++);
		n++;
	    }
	    // shift to get 40 bits
	    if (n < 10) {
		v <<= (4*(10-n));
	    }
	    fprintf(fout, "# %010lX\n", v);
	    write_helord_remsa(fout, (helord_t) v);
	    fprintf(fout, "\n");
	}
    }
    exit(0);
}
