// Paper tape punch utility
// Create input data files from other formats

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <errno.h>

#include "besk.h"

void usage()
{
    fprintf(stderr, "usage: drum [options]\n");
    fprintf(stderr, "OPTIONS\n");
    fprintf(stderr, "  -d <drum-file>\n");
    fprintf(stderr, "  -c   create if file does not exiit\n");
    fprintf(stderr, "  -z <channel-size>  number of halfwords per channel\n");
    fprintf(stderr, "  -r <channel-num>\n");
    fprintf(stderr, "  -w <channel-num>\n");
    exit(1);
}

size_t write_channel(FILE* drum, char* buf, size_t channel_size)
{
    size_t channel_bytes = (channel_size/2)*5;
    size_t n;

    n = fwrite(buf, 1, channel_bytes, drum);
    printf("write %ld wrote %ld bytes\n", channel_bytes, n);
    return n;
}

size_t read_channel(FILE* drum, char* buf, size_t channel_size)
{
    size_t channel_bytes = (channel_size/2)*5;
    return fread(buf, 1, channel_bytes, drum);
}

// print channel data
void display_channel(FILE* f, char* buf, size_t channel_size)
{
    int i;
    for (i = 0; i < channel_size; i += 2) {  // 40 bits per value
	helord_t x = helord_read(i, (halvord_t*) buf);
	fprintf(f, "%05lX ", (x >> 20) & 0xFFFFF);
	fprintf(f, "%05lX ", x & 0xFFFFF);
    }
    printf("\n");
}


int main(int argc, char** argv)
{
    char* drum_file_name = "DRUM.dat";
    FILE* drum;
    FILE* fin = stdin;
    FILE* fout = stdout;
    long size;    
    int channel_size = DRUM_CHANNEL_SIZE;
    int read_channel_num = -1;
    int write_channel_num = -1;
    int create = 0;
    int opt;
    char buf[1024];
    
    while ((opt = getopt(argc, argv, "cd:z:r:w:")) != -1) {
	switch(opt) {
	case 'd': drum_file_name = optarg; break;
	case 'c': create = 1; break;
	case 'z': channel_size = atoi(optarg); break;
	case 'r': read_channel_num = atoi(optarg); break;
	case 'w': write_channel_num = atoi(optarg); break;
	default: usage();
	}
    }

    if ((drum = fopen(drum_file_name, "r+")) == NULL) {
	if (create) {
	    if ((drum = fopen(drum_file_name, "w+")) == NULL) {
		fprintf(stderr, "unable to create drum file %s (%s)\n",
			drum_file_name, strerror(errno));
		exit(1);
	    }
	}
	else {
	    fprintf(stderr, "unable to create drum file %s (%s)\n",
		    drum_file_name, strerror(errno));
	    exit(1);
	}
    }
    fseek(drum, 0, SEEK_END);
    size = ftell(drum);
    printf("drum size = %ld\n", size);

    if ((size == 0) && create) {
	int i;
	memset(buf, 0, sizeof(buf));
	for (i = 0; i <= DRUM_MAX_CHANNEL_NUMBER; i += 2) {
	printf("write channel %d\n", i);
	    write_channel(drum, buf, channel_size);
	}
	fseek(drum, 0, SEEK_SET);
    }

    if (read_channel_num >= 0) {
	int n = read_channel_num & 0x1FE;  // channel number, even numbered
	long offset = (n>>1)*DRUM_CHANNEL_BYTES;
	fseek(drum, offset, SEEK_SET);
	read_channel(drum, buf, channel_size);
	// display_channel(stdout, buf, channel_size);
    }
    else {  // read channel data from stdin
	int i;
	memset(buf, 0, sizeof(buf));
	for (i = 0; i <= DRUM_MAX_CHANNEL_NUMBER; i += 2) {
	    unsigned long v, h;
	    fscanf(fin, "%05lX", &v);
	    fscanf(fin, "%05lX", &h);
	    helord_write(i, (halvord_t*) buf,
			 ((v & 0xFFFFF) << 20) | (h & 0xFFFFF));
	}
	// display_channel(stdout, buf, channel_size);	
    }
    
    if (write_channel_num >= 0) {
	int n = write_channel_num & 0x1FE;  // channel number, even numbered
	long offset = (n>>1)*DRUM_CHANNEL_BYTES;
	fseek(drum, offset, SEEK_SET);
	write_channel(drum, buf, channel_size);
    }
    else {
	display_channel(fout, buf, channel_size);	
    }
    
    fclose(drum);
    exit(0);
}
