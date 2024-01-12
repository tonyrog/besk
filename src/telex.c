// Test telex functions

#include <stdio.h>
#include <stdint.h>
#include <memory.h>

#include "telex.h"

// read n rows from 5 channel data telex

static uint16_t ita2_ltr[32] =
{
    0,    'T', '\r', 'O', ' ',  'H', 'N', 'M',
    '\n', 'L', 'R',  'G', 'I',  'P', 'C', 'V',
    'E',  'Z', 'D',  'B', 'S',  'Y', 'F', 'X',
    'A',  'W', 'J',    1, 'U',  'Q', 'K',   0
};

static uint16_t ita2_fig[32] = {
    0,    '5', '\r', '9', ' ',  '#', ',', '.',
    '\n', ')', '4',  '&', '8',  '0', ':', ';',
    '3',  '"', '$',  '?', '\a', '6', '!', '/',
    '-',  '2', '\'',   1, '7',  '1', '(',   0
};

static uint16_t* ita2_to_utf16[2] = { ita2_ltr, ita2_fig };


static uint16_t utf16_to_ita2[256] =
{
    // Figure & Letter
    ['\r'] = 0x0002,
    ['\n'] = 0x0008,
    [' ']  = 0x0004,
    // Figure    
    ['\a'] = 0x8014,
    ['!']  = 0x8016,  // ALT swedish Å
    ['"']  = 0x8011,  // ALT +
    ['#']  = 0x8005,  // ALT swedish Ö
    ['$']  = 0x8012,
    ['&']  = 0x800B,  // ALT swedish Ä
    ['\''] = 0x801A,
    ['(']  = 0x801E,
    [')']  = 0x8009,
    [',']  = 0x8006,
    ['-']  = 0x8018,
    ['.']  = 0x8007,
    ['/']  = 0x8017,
    ['0']  = 0x800D,
    ['1']  = 0x801D,
    ['2']  = 0x8019,
    ['3']  = 0x8010,
    ['4']  = 0x800A,
    ['5']  = 0x8001,
    ['6']  = 0x8015,
    ['7']  = 0x801C,
    ['8']  = 0x800C,
    ['9']  = 0x8003,
    [':']  = 0x800E,
    [';']  = 0x800F,  // ALT =
    ['?']  = 0x8013,
    // BESK special - swedish
    [197] = 0x8016,  // ALT swedish Å
    ['+'] = 0x8011,
    [196] = 0x8008,
    [214] = 0x8005,
    ['='] = 0x800F,
    // Letter
    ['A'] = 0x4018,
    ['B'] = 0x4013,
    ['C'] = 0x400E,
    ['D'] = 0x4012,
    ['E'] = 0x4010,
    ['F'] = 0x4016,
    ['G'] = 0x400B,
    ['H'] = 0x4005,
    ['I'] = 0x400C,
    ['J'] = 0x401A,
    ['K'] = 0x401E,
    ['L'] = 0x4009,
    ['M'] = 0x4007,
    ['N'] = 0x4006,
    ['O'] = 0x4003,
    ['P'] = 0x400D,
    ['Q'] = 0x401D,
    ['R'] = 0x400A,
    ['S'] = 0x4014,
    ['T'] = 0x4001,
    ['U'] = 0x401C,
    ['V'] = 0x400F,
    ['W'] = 0x4019,
    ['X'] = 0x4017,
    ['Y'] = 0x4015,
    ['Z'] = 0x4011,

    ['a'] = 0x4018,
    ['b'] = 0x4013,
    ['c'] = 0x400e,
    ['d'] = 0x4012,
    ['e'] = 0x4010,
    ['f'] = 0x4016,
    ['g'] = 0x400b,
    ['h'] = 0x4005,
    ['i'] = 0x400c,
    ['j'] = 0x401a,
    ['k'] = 0x401e,
    ['l'] = 0x4009,
    ['m'] = 0x4007,
    ['n'] = 0x4006,
    ['o'] = 0x4003,
    ['p'] = 0x400d,
    ['q'] = 0x401d,
    ['r'] = 0x400a,
    ['s'] = 0x4014,
    ['t'] = 0x4001,
    ['u'] = 0x401c,
    ['v'] = 0x400f,
    ['w'] = 0x4019,
    ['x'] = 0x4017,
    ['y'] = 0x4015,
    ['z'] = 0x4011,
};

// read 5 channel paper tape code
uint64_t telex_read_remsa(FILE* f, int n)
{
    char line[81];
    char* ptr;    
    uint64_t x = 0;
    uint64_t y;	

    while(n--) {
next:
	y = 0;
	memset(line, ' ', 5);
	ptr = fgets(line, sizeof(line), f);
	if (ptr == NULL) return 0; // fixme: error?
	// read hex digit
	y |= ((ptr[0] == 'o')<<4);  // pos4
	y |= ((ptr[1] == 'o')<<3);  // pos3
	y |= ((ptr[2] == 'o')<<2);  // pos2
	y |= ((ptr[3] == 'o')<<1);  // pos1
	y |= ((ptr[4] == 'o')<<0);  // pos0
	if (y == 0) goto next;
	x = (x << 5) | y;
    }
    return x;
}

// write 5 channel code onto paper tape (FILE)
int telex_write_remsa(FILE* f, uint8_t code)
{
    if ((code == 0) || (code > 0x1F)) return -1; // not valid code
    return fprintf(f, "%c%c%c%c%c\n", 
		   ((code>>4) & 1) ? 'o' : '-',
		   ((code>>3) & 1) ? 'o' : '-',
		   ((code>>2) & 1) ? 'o' : '-',
		   ((code>>1) & 1) ? 'o' : '-',
		   ((code>>0) & 1) ? 'o' : '-');
}


// encode return 0,1,2 and data in outbuf at least 2 bytes
int telex_encode(int c, int* page, uint8_t* outbuf)
{
    int n = 0;
    uint16_t code = utf16_to_ita2[c & 0xFF];
    if (code) {
	uint16_t cp = code & 0xC000;
	if (cp != 0) {
	    if (cp != *page) {  // switch to page cp
		*page = cp; // set new page
		if (cp == TELEX_PAGE_FIG)
		    outbuf[n++] = ITA2_FIG;
		else
		    outbuf[n++] = ITA2_LTR;
	    }
	}
	outbuf[n++] = code & 0x1F;
    }
    return n;
}

int telex_decode(uint8_t y, int* page, uint16_t* outbuf)
{
    switch(y) {
    case 0: return -1;  // not used
    case ITA2_FIG: *page = TELEX_PAGE_FIG; break;
    case ITA2_LTR: *page = TELEX_PAGE_LTR; break;
    default:
	*outbuf = ita2_to_utf16[*page>>15][y & 0x1f];
	return 1;
    }
    return 0;
}
