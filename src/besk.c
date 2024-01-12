//
//  BESK (Binär Elektronisk Sekvens Kalkylator) computer emulator
//
//
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <memory.h>
#include <math.h>

#include "besk.h"
#include "telex.h"

#ifdef SIMULATOR
extern void simulator_init(int argc, char** argv, besk_t* st);
extern void simulator_run(besk_t* st);
#define SIMULATOR_INIT(ac,av,st) simulator_init((ac),(av),(st))
#define SIMULATOR_RUN(st)  simulator_run(st)
#else
#define SIMULATOR_INIT(ac,av,st)
#define SIMULATOR_RUN(st)
#endif

#define FMT_IND  0x0001   // indirect addressing mode
#define FMT_DEC  0x0002   // immediate addressing mode
#define FMT_HEX  0x0004   // immediate addressing mode
#define FMT_ADDR 0x0008   // label address or immediate x3
#define FMT_H    0x0010   // H bit set
#define FMT_Z    0x0020   // Z bit set
#define FMT_HZ   (FMT_H|FMT_Z)  // H and Z bits set
#define FMT_X    0x0100  // x coordinate in AR
#define FMT_Y    0x0200  // y coordinate in AR
#define FMT_C    0x0400  // circle
#define FMT_D    0x0800  // dot
#define FMT_XYCD (FMT_X|FMT_Y|FMT_C|FMT_D) 

static struct {
    uint8_t op;
    const char* mnem;
    const uint16_t fmt;
} op_table[] =
{
    // Note the order of the operations is important!
    // Larger numbered (more specfic) first
    { .op=OP_BAND,  .mnem = "band",  .fmt=FMT_IND },
    { .op=OP_MOVMR, .mnem = "movmr", .fmt=FMT_IND },
    { .op=OP_MUL,   .mnem = "mul",   .fmt=FMT_IND|FMT_HZ },
    { .op=OP_MULR,  .mnem = "mulr",  .fmt=FMT_IND|FMT_HZ },
    { .op=OP_SHR,   .mnem = "shr",   .fmt=FMT_DEC },   // 0x44    
    { .op=OP_ASHR,  .mnem = "ashr",  .fmt=FMT_DEC },   // 0x04
    { .op=OP_SHL40, .mnem = "shl40", .fmt=FMT_DEC },   // 0x45
    { .op=OP_SHL,   .mnem = "shl",   .fmt=FMT_DEC },   // 0x05
    { .op=OP_INCST, .mnem = "incst", .fmt=FMT_IND|FMT_H },  // 0x46
    { .op=OP_ADDST, .mnem = "addst", .fmt=FMT_IND|FMT_HZ }, // 0x06
    { .op=OP_STORA, .mnem = "stora", .fmt=FMT_IND|FMT_HZ },
    { .op=OP_ADDMR, .mnem = "addmr", .fmt=FMT_IND|FMT_HZ },
    { .op=OP_SUBMR, .mnem = "submr", .fmt=FMT_IND|FMT_HZ },
    { .op=OP_JC,    .mnem = "jc",    .fmt=FMT_ADDR|FMT_H },
    { .op=OP_NEG,   .mnem = "neg",   .fmt=FMT_IND|FMT_H },  // 0x4B
    { .op=OP_SUB,   .mnem = "sub",   .fmt=FMT_IND|FMT_HZ }, // 0x0B
    { .op=OP_JMP,   .mnem = "jmp",   .fmt=FMT_ADDR|FMT_H },
    { .op=OP_AADD,  .mnem = "aadd",  .fmt=FMT_IND|FMT_HZ },
    { .op=OP_JLT,   .mnem = "jlt",   .fmt=FMT_ADDR|FMT_H },   // 0x4E
    { .op=OP_JGE,   .mnem = "jge",   .fmt=FMT_ADDR|FMT_H },   // 0x0E
    { .op=OP_ASUB,  .mnem = "asub",  .fmt=FMT_IND|FMT_HZ },
    { .op=OP_LOAD,  .mnem = "load",  .fmt=FMT_IND|FMT_H },  // 0x70
    { .op=OP_ADD,   .mnem = "add",   .fmt=FMT_IND|FMT_HZ }, // 0x10
    { .op=OP_STORE, .mnem = "store", .fmt=FMT_IND|FMT_HZ },
    { .op=OP_DIV,   .mnem = "div",   .fmt=FMT_IND },
    { .op=OP_REV,   .mnem = "rev",   .fmt=0 },
    { .op=OP_READ5, .mnem = "read5", .fmt=FMT_IND },
    { .op=OP_NORM40, .mnem = "norm40", .fmt=0 },  // 0x35
    { .op=OP_NORM,  .mnem = "norm", .fmt=0 },     // 0x15
    { .op=OP_FUNC,  .mnem = "f",    .fmt=FMT_Z|FMT_XYCD }, // 0x18|0x58
    { .op=OP_READ4x1, .mnem = "read4x1", .fmt=FMT_IND },   // 0x39    
    { .op=OP_READ4x10, .mnem = "read4x10", .fmt=FMT_IND }, // 0x19
    { .op=OP_RD,     .mnem = "rd", .fmt=FMT_IND },
    { .op=OP_WRITE4, .mnem = "write4", .fmt=0 },
    { .op=OP_WRITE,  .mnem = "write", .fmt=0 },
    { .op=OP_WD,     .mnem = "wd", .fmt=FMT_IND },
    { .op=0xFF,      .mnem = NULL, .fmt=0 }
};


// Helord layout - 64 bit ord as 2 32-bit half words
// using 20 bit in each half only, Vs,Hs are only used
// as sign extension when possible.
// 
//        even                 odd
// +-------------------+-------------------+
// | Vs:12,Vw:12,Vop:8 | Hs:12,Hw:12,Hop:8 |
// +-------------------+-------------------+
// when return as helord (and as stored in registers)
// +---------------------------------------+
// |      s:24,Vw:12,Vop:8,Hw:12,Hop:8     |
// +---------------------------------------+
//

// reading left halvord (even address) is reading into upper helord,
// while lower (right) is set to zero.
// read right halvord (odd address) is reading into lower helord
// while upper (left) s set to zero
// 
helord_t halvord_read(unsigned addr, halvord_t* mem)
{
     if (addr & 1) // read hhao (right half)
	 return (helord_t) (mem[addr] & HALVORD_MASK);
     else  // read vhao (left half)
	 return ((helord_t) ((mem[addr] & HALVORD_MASK))) << 20;
}

// read halv ord (H=0) | or hel ord (H=1)
helord_t ord_read(int H, unsigned addr, halvord_t* mem)
{
    helord_t value;
    addr &= 0x7ff; // ignore top bit! (cyclic memory!)
    if (H)
	value = helord_read(addr, mem);
    else
	value = halvord_read(addr, mem);
    return value;
}


void halvord_write(unsigned addr, halvord_t* mem, helord_t value)
{
    addr &= 0x7ff; // ignore top bit! (cyclic memory!)
    if (addr & 1) { // hhao only to hhac
	mem[addr] = value & HALVORD_MASK;
    }
    else { // vhao only to vhac! ?
	// mem[addr] = value & HALVORD_MASK;
	mem[addr] = (value >> 20);
    }
}

// write halv ord (H=0) or hel ord (H=1)
void ord_write(int H, unsigned addr, halvord_t* mem, helord_t value)
{
    if (H)
	helord_write(addr, mem, value);
    else
	halvord_write(addr, mem, value);
}

// write address part of addr
void addr_write(int H, unsigned addr, halvord_t* mem, helord_t value)
{
    addr &= 0x7ff; // ignore top bit! (cyclic memory!)
    if (H) {  // both cells
	mem[addr]   = (mem[addr] & ~HALVORD_OP)|((value >> 20) & HALVORD_ADDR);
	mem[addr+1] = (mem[addr+1] & ~HALVORD_OP) | (value & HALVORD_ADDR);

    }
    else if (addr & 1) { // write hha0 (right half)
    	mem[addr] = (mem[addr] & ~HALVORD_OP) | (value & HALVORD_ADDR);


    }
    else {
	// mem[addr] = (mem[addr] & ~0x000FF) | (value & 0xFFF00);
	mem[addr] = (mem[addr] & ~HALVORD_OP) | ((value >> 20) & HALVORD_ADDR);
    }
}

//int xdigit(int c)
//{
//    return (c & 0x10) ? (c & 0xF) : (c & 0xF) + 9;
//}

// Load code from file into memory
// Syntax is:
//          "org" <addr>
//   [<label> | addr3x | blank] [ins5x]
//   [<label> | addr3x | blank] [<opcode> [w3x] | [w3x op2x]
//
// label = <name>':'
// name  = (a-z | A-Z | 0-9 | '_')+
// addr3x = <hex digit> <hex digit> <hex digit>
// ins5x = <hex digit> <hex digit> <hex digit> <hex digit> <hex digit>
// w3x   = <hex digit> <hex digit> <hex digit>
//
#define MAX_NUM_LABELS  (1024)
#define MAX_NUM_PATCHES (1024)
#define UNRESOLVED (-1)

static int num_labels = 0;
static struct {
    char* name;  // label name (without ':')
    int   len;   // length of label name
    halvord_t addr;  // address (0..2047)
} label_table[MAX_NUM_LABELS];

static int num_patches = 0;
static struct {
    int lbl;  // label index with unresolved label
    int addr; // address of unresolved label
} patch_table[MAX_NUM_PATCHES];

int find_label(char* name, size_t nl)
{
    int i;
    for (i = 0; i < num_labels; i++) {
	if ((label_table[i].len == nl) &&
	    (strncmp(label_table[i].name, name, nl) == 0))
	    return i;
    }
    return -1;
}

int add_label(char* name, size_t nl, halvord_t addr)
{
    int i;
    if ((i=num_labels) >= MAX_NUM_LABELS) {
	fprintf(stderr, "Too many labels\n");
	return -1;
    }
    label_table[i].name = malloc(nl+1);
    label_table[i].len = nl;
    memcpy(label_table[i].name, name, nl);
    label_table[i].name[nl] = '\0';
    label_table[i].addr = addr;
    printf("add label[%d] '%s' addr=%03X\n",
	   i, label_table[i].name, label_table[i].addr);
    num_labels++;
    return i;
}

int add_patch(int lbl, int addr)
{
    int i;
    if ((i=num_patches) >= MAX_NUM_PATCHES) {
	fprintf(stderr, "Too many patches\n");
	return -1;
    }
    patch_table[i].lbl = lbl;
    patch_table[i].addr = addr;
    printf("add patch[%d] lbl=%d addr=%03X\n",
	   i, patch_table[i].lbl, patch_table[i].addr);
    num_patches++;    
    return i;
}

halvord_t find_or_patch_address(char* name, size_t nl, halvord_t addr)
{
    int ix;
    if ((ix = find_label(name, nl)) >= 0) {
	if (label_table[ix].addr == UNRESOLVED) {
	    if (add_patch(ix, addr) < 0)
		return -1;
	    return 0;
	}
	return label_table[ix].addr;
    }
    else {
	if ((ix = add_label(name, nl, UNRESOLVED)) < 0)
	    return -1;
	if (add_patch(ix, addr) < 0)
	    return -1;
	return 0;
    }
}

// find opcode from name and return index to opcode entry
// return the modified opcode in the opcode return value
int lookup_opcode(char* name, size_t nl, oktet_t* op, uint16_t* fmt)
{
    int i;
    for (i = 0; op_table[i].op != 0xFF; i++) {
	int len = strlen(op_table[i].mnem);
	if (len <= nl) {
	    if (strncmp(op_table[i].mnem, name, len) == 0) {
		if ((nl == len) ||
		    (name[len] == '.')) {
		    *op = op_table[i].op;
		    *fmt = op_table[i].fmt;
		    return i;
		}
	    }
	}
    }
    return -1;
}

#define MAX_TOKENS 10  // max tokens per line
#define MAX_LINE   128 // max line length
#define T_DEC 256  // 0-9  ($[-][0-9]+)
#define T_HEX 257  // 0-9A-Fa-f ($[-]0x[0-9A-Fa-f]+)
#define T_ID  258  // A-Za-z0-9_.  ($[A-Za-z0-9_]+) label value
#define T_TAB 259  // tab | blank

#define IS_NUM(t) (((t) == T_DEC) || ((t) == T_HEX))
#define IS_ID(t)  (((t) == T_ID) || ((t) == T_HEX))
#define IS_TAB(t) ((t) == T_TAB)

// b >= 2, b =< 16
halvord_t digits_to_halvord(char* ds, int n, int b)
{
    halvord_t val = 0;
    int i;
    for (i = 0; i < n; i++) {
	int d = ds[i];
	val *= b;
	if (d >= '0' && d <= '9')        val += (d-'0');
	else if (d >= 'A' && d <= 'F')   val += (d-'A'+10);
	else if (d >= 'a' && d <= 'f')   val += (d-'a'+10);
	else return -1;
    }
    return val;
}

halvord_t load_code(FILE* f, char* filename, int ln, halvord_t addr,
		    halvord_t* mem)
{
    char line[MAX_LINE+1];
    char* ts[MAX_TOKENS];   // token start
    int   tl[MAX_TOKENS];   // token length
    int   tt[MAX_TOKENS];   // token type
    char* ptr;
    halvord_t addr0 = -1;
    int i;

    
    while((ptr = fgets(line, sizeof(line), f)) != NULL) {
	char* ptr0 = ptr;
	int j;
	ln++;
	i = 0;  // number of tokens
	while(*ptr && (i < MAX_TOKENS)) {
	    while(isblank(*ptr)) ptr++;
	    if ((i == 0) && (ptr != ptr0)) {
		ts[0] = "\\t"; tt[0] = T_TAB; tl[0] = 2;
		i++;
	    }
	    while(isspace(*ptr)) ptr++;
	    ts[i] = ptr; tt[i] = 0; tl[i] = 0;

	    switch(*ptr) {
	    case '#': goto done;  // comment to end of line
	    case '\0': goto done;  // end of line
	    case '[': ts[i]="["; tt[i]='['; tl[i]=1; ptr++; break;
	    case ']': ts[i]="]"; tt[i]=']'; tl[i]=1; ptr++; break;
	    case ':': ts[i]=":"; tt[i]=':'; tl[i]=1; ptr++; break;
	    case '-': ts[i]="-"; tt[i]='-'; tl[i]=1; ptr++; break;
	    case '+': ts[i]="+"; tt[i]='+'; tl[i]=1; ptr++; break;
	    case '0': if (ptr[1] == 'x') { ptr += 2; ts[i]=ptr; goto hex; }
	    case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		tt[i] = T_DEC;
		while(isdigit(*ptr)) ptr++;
		if (isxdigit(*ptr)) {
		    tt[i] = T_HEX;
		    while(isxdigit(*ptr)) ptr++;
		}
		tl[i] = ptr-ts[i];
		break;
	    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	    hex:
	        tt[i] = T_HEX;
		while(isxdigit(*ptr)) ptr++;
		if (isalnum(*ptr) || (*ptr == '_') || (*ptr == '.')) {
		    tt[i] = T_ID;
		    while(isalnum(*ptr) || (*ptr == '_') || (*ptr == '.'))
			ptr++;
		}
		tl[i] = ptr-ts[i];		
		break;
	    case '_':
	    case '.':
	    default:
		tt[i] = T_ID;
		while(isalnum(*ptr) || (*ptr == '_') || (*ptr == '.'))
		    ptr++;
		tl[i] = ptr-ts[i];
		break;
	    }
	    i++;
	}
    done:
	tt[i] = 0;  // last token is always 0

	// debug print tokens
/*	
	printf("%d tokens: ", i);
	for (j = 0; j < i; j++) {
	    printf("'%.*s':%d/%d ", tl[j], ts[j], tl[j], tt[j]);
	}
	printf("\n");
*/

	j = 0;
	if ((i == 0) || ((i==1) && (tt[j] == T_TAB)))
	    continue;  // empty line
	if (IS_ID(tt[j]) && (tt[j+1] == ':')) { // label
	    int ix;
	    if ((ix = find_label(ts[j], tl[j])) >= 0) {
		if (label_table[ix].addr != UNRESOLVED) {
		    fprintf(stderr, "%s:%d: label '%.*s' already defined\n",
			    filename, ln, tl[j], ts[j]);
		    return -1;
		}
		fprintf(stderr, "%s:%d: label '%.*s' = %03X resolved\n",
			filename, ln, tl[j], ts[j], addr);
		label_table[ix].addr = addr;  // resolved!
	    }
	    else if (add_label(ts[j], tl[j], addr) < 0) {
		fprintf(stderr, "%s:%d: too many labels\n", filename, ln);
		return -1;
	    }
	    j += 2;
	}
	else if (IS_NUM(tt[j]) && (tl[j] == 3)) { // addr3x
	    addr = digits_to_halvord(ts[j], 3, 16);
	    if (addr0 == -1) addr0 = addr;
	    j++;
	}
	else if (IS_TAB(tt[j])) { // tab
	    j++;
	}
	else
	    goto syntax_error;

	if (IS_NUM(tt[j]) && (tl[j] == 5)) { // ins5x
	    halvord_t word = digits_to_halvord(ts[j], 5, 16);
	    mem[addr & 0x7ff] = word;
	    addr++;
	}
	else if (IS_ID(tt[j]) && (tl[j] >= 1)) {
	    char* nptr = ts[j];
	    int nlen = tl[j];
	    uint16_t fmt;
	    oktet_t op;
	    int ix;
	    // lookup opcode from name
	    printf("lookup '%.*s' len=%d\n", nlen, nptr, nlen);
		   
	    if ((ix = lookup_opcode(nptr, nlen, &op, &fmt)) >= 0) {
		halvord_t ins = 0;
		j++;
		ins = op;  // opcode with H and Z flags set
		nptr += strlen(op_table[ix].mnem);
		nlen -= strlen(op_table[ix].mnem);

		printf("#op index = %d, opcode=%02x, fmt=%04X name=%.*s\n",
		       ix, op, fmt, nlen, nptr);

		// check for .h | .z suffix
		if (fmt & FMT_H) {
		    if ((nptr[0] == '.') &&
			((nptr[1] == 'h') || (nptr[2] == 'h')))
			ins |= HELORD_BIT;
		}
		if (fmt & FMT_Z) {
		    if ((nptr[0] == '.') &&
			((nptr[1] == 'z') || (nptr[2] == 'z')))
			ins |= ARZERO_BIT;
		}

		if (fmt & FMT_IND) {  // [3x] | [id]  argument 
		    if ((tt[j] == '[') &&
			(IS_NUM(tt[j+1]) && (tl[j+1]==3)) &&
			tt[j+2] == ']') {
			halvord_t as;
			as = digits_to_halvord(ts[j+1], 3, 16);
			ins |= (as << 8);
		    }
		    else if ((tt[j] == '[') &&
			     (IS_ID(tt[j+1]) && (tl[j+1] >= 1)) &&
			     tt[j+2] == ']') {
			halvord_t as;
			printf("lookup '%.*s' len=%d\n",tl[j+1],ts[j+1],tl[j+1]);
			if ((as = find_or_patch_address(ts[j+1],tl[j+1],addr)) >= 0)
			    ins |= ((as & 0x7ff) << 8);
			else
			    goto syntax_error;
			
		    }		    
		    else
			goto syntax_error;
		    j += 3;		    
		}
		if (fmt & FMT_DEC) {  // decimal argument
		    if (tt[j] == T_DEC) { // decimal argument
			halvord_t d = digits_to_halvord(ts[j], tl[j], 10);
			ins |= ((d & 0xfff) << 8);			    
		    }
		}
		else if (fmt & FMT_HEX) { // hex argument
		    if (IS_NUM(tt[j])) {
			halvord_t d = digits_to_halvord(ts[j], tl[j], 16);
			ins |= ((d & 0xfff) << 8);
		    }
		    else
			goto syntax_error;		    
		}
		else if (fmt & FMT_ADDR) {
		    if (IS_NUM(tt[j])) {
			halvord_t d = digits_to_halvord(ts[j], tl[j], 16);
			ins |= ((d & 0xfff) << 8);
		    }
		    else if (IS_ID(tt[j])) {
			char* nptr = ts[j];
			int nlen = tl[j];
			halvord_t as;
			printf("lookup '%.*s' len=%d\n", nlen, nptr, nlen);
			if ((as = find_or_patch_address(nptr, nlen, addr)) >= 0)
			    ins |= ((as & 0x7ff) << 8);
			else
			    goto syntax_error;
		    }
		    else
			goto syntax_error;		    
		}
		else if (fmt & FMT_XYCD) {
		    halvord_t as = 0;
		    if (IS_ID(tt[j])) { // x|xc|xd|y|yc|yd
			nptr = ts[j];
			nlen = tl[j];
		    }
		    else
			goto syntax_error;
		    while(nlen > 0) {
			// printf("F code %c\n", *nptr);
			switch(*nptr++) {
			case '.': break;
			case 'x': as |= (0 << 3); break;
			case 'y': as |= (1 << 3); break;
			case 'd': as |= 0x4; break;  // Punkt
			case 'c': as |= 0x6; break;  // Cirkel
			default: goto syntax_error;
			}
			nlen--;
		    }
		    // 0x0 form is used! not 0x2 form
		    // if ((as & 0x6) == 0)
		    //   as |= 0x2;
		    ins |= ((as & 0x7ff) << 8);
		}
		
		mem[addr & 0x7ff] = ins;
		addr++;
	    }
	    else if (strncmp(".org", ts[j], tl[j]) == 0) {
		j++;
		printf("org %d %.*s\n", tt[j], tl[j], ts[j]);
		if (IS_NUM(tt[j]) && (tl[j] == 3)) {
		    addr = digits_to_halvord(ts[j], 3, 16);
		    if (addr0 == -1) addr0 = addr;
		    continue;
		}
		else
		    goto syntax_error;
	    }
	    else
		goto syntax_error;
	}
	fprintf(stdout, "> %03X %05X\n", addr-1, mem[addr-1]);
    }
    for (i = 0; i < num_patches; i++) {
	mem[patch_table[i].addr] |= (label_table[patch_table[i].lbl].addr<<8);
	fprintf(stdout, ">> %03X %05X\n", patch_table[i].addr, mem[patch_table[i].addr]);
    }
    return addr0;

syntax_error:
    fprintf(stderr, "%s:%d: syntax error\n", filename, ln);
    return -1;
}

helord_t read_drum_memory(besk_t* st, halvord_t INS, helord_t MR)
{
    long offset;
    uint8_t data[DRUM_CHANNEL_BYTES];  // bytes per channel
    uint8_t* ptr;
    helord_t AR;
    int i, n, addr;

    n = W(MR) & 0x1FE;  // channel number, even numbered
    offset = (n>>1)*DRUM_CHANNEL_BYTES;
    fseek(st->drum, offset, SEEK_SET);
    fread(data, 1, DRUM_CHANNEL_BYTES, st->drum);

    // unpack the data helord into memory
    addr = W(INS) & 0x7FE;
    ptr  = data;
    for (i = 0; i < (DRUM_CHANNEL_SIZE/2); i++) {
	// read helord from channel data 40 bits (big endian)
	AR = ((helord_t)ptr[4]<<32) | ((helord_t)ptr[3]<<24) |
	    ((helord_t)ptr[2]<<16) | ((helord_t)ptr[1]<<8) | (helord_t)ptr[0];
	ord_write(H(INS), addr, st->MEM, AR);
	addr += 2;
	ptr  += 5;  // 40 bit helord
    }
    return AR;
}

helord_t write_drum_memory(besk_t* st, halvord_t INS, helord_t MR)
{
    long offset;
    uint8_t data[DRUM_CHANNEL_BYTES];  // bytes per channel
    uint8_t* ptr;
    helord_t AR;
    int  n, i, addr;

    n = W(MR) & 0x1FE;  // channel number, even numbered
    offset = (n>>1)*DRUM_CHANNEL_BYTES;
    fseek(st->drum, offset, SEEK_SET);
    // unpack the data helord into memory
    addr = W(INS) & 0x7FE;
    ptr  = data;
    for (i = 0; i < (DRUM_CHANNEL_SIZE/2); i++) {
	AR = ord_read(H(INS), addr, st->MEM);
	ptr[4] = AR>>32;
	ptr[3] = AR>>24;
	ptr[2] = AR>>16;
	ptr[1] = AR>>8;
	ptr[0] = AR>>0;	
	addr += 2;
	ptr  += 5;  // 40 bit helord
    }
    fwrite(data, 1, DRUM_CHANNEL_BYTES, st->drum);
    return AR;
}


// read n rows from 4 channel data
helord_t read_4_channel_remsa(besk_t* st, int n)
{
    char line[81];
    char* ptr;    
    helord_t x = 0;

    while(n--) {
	helord_t y;	
    next:
	y = 0;
	memset(line, ' ', 5);
	ptr = fgets(line, sizeof(line), st->in);
	if (ptr == NULL) return 0; // fixme: error?
	if (ptr[4] != 'o') goto next; // skip blank in position 5
	// read hex digit
	y |= ((ptr[0] == 'o')<<3);  // pos3
	y |= ((ptr[1] == 'o')<<2);  // pos2
	y |= ((ptr[2] == 'o')<<1);  // pos1
	y |= ((ptr[3] == 'o')<<0);  // pos0
	x = (x << 4) | y;
    }
    return x;
}

void write_4_channel_remsa(besk_t* st, uint8_t code)
{
    if (st->ut == NULL)
	return;
    fprintf(st->ut, "%c%c%c%co\n", 
	    ((code>>3) & 1) ? 'o' : '-',
	    ((code>>2) & 1) ? 'o' : '-',
	    ((code>>1) & 1) ? 'o' : '-',
	    ((code>>0) & 1) ? 'o' : '-');
}

void write_tecken_remsa(besk_t* st, uint8_t code)
{
    if (st->ut == NULL)
	return;
    fprintf(st->ut, "%c%c%c%c-\n", 
	    ((code>>3) & 1) ? 'o' : '-',
	    ((code>>2) & 1) ? 'o' : '-',
	    ((code>>1) & 1) ? 'o' : '-',
	    ((code>>0) & 1) ? 'o' : '-');
}

// format helord/halvord format. left halvord is ".L" and right halvord is ."H"
const char* format_vh(halvord_t I)
{
    switch(I & (ARZERO_BIT|HELORD_BIT)) {
    case (ARZERO_BIT|HELORD_BIT): return ".Z";
    case HELORD_BIT: return "";
    case ARZERO_BIT:
	if (W(I) & 1) return ".ZH";
	return ".ZL";
    case 0x00:
	if (W(I) & 1) return ".H";
	return ".L";
    default:
	return ".?";
    }
}

char* stradd(char* buf, const char* s, size_t* buflen)
{
    size_t len = *buflen;
    
    while(len && *s) {
	*buf++ = *s++;
	len--;
    }
    *buflen = len;
    if (len)
	*buf = '\0';
    return buf;
}

char* format_instruction(oktet_t OP, halvord_t AS, char* buf, size_t buflen)
{
    char tbuf[16];
    uint16_t fmt;    

    int i = 0;
    while(op_table[i].op != 0xFF) {
	oktet_t op = op_table[i].op;
	if (((OP & 0x7f) == op) ||  // 0111 1111
	    ((OP & 0x5f) == op) ||  // 0101 1111
	    ((OP & 0x3f) == op) ||  // 0011 1111
	    ((OP & 0x1f) == op))    // 0001 1111
	    break;
	i++;
    }
    if (op_table[i].op == 0xFF) {
	buf = stradd(buf, "undef", &buflen);
	return buf;
    }
    fmt = op_table[i].fmt;
    buf = stradd(buf, op_table[i].mnem, &buflen);
    if (fmt & FMT_HZ) {
	int h = (fmt & FMT_H) && (OP & HELORD_BIT);
	if (h)
	    buf = stradd(buf, ".h", &buflen);
	if ((fmt & FMT_Z) && (OP & ARZERO_BIT))
	    buf = stradd(buf, h ? "z" : ".z", &buflen);
    }
    if (fmt & FMT_IND) {
	snprintf(tbuf, sizeof(tbuf), " [%03X]", AS);
	buf = stradd(buf, tbuf, &buflen);
    }
    else if (fmt & FMT_ADDR) {
	snprintf(tbuf, sizeof(tbuf), " %03X", AS);
	buf = stradd(buf, tbuf, &buflen);
    }
    else if (fmt & FMT_DEC) {
	snprintf(tbuf, sizeof(tbuf), " %d", AS);
	buf = stradd(buf, tbuf, &buflen);
    }
    else if (fmt & FMT_HEX) {
	snprintf(tbuf, sizeof(tbuf), " %03X", AS);
	buf = stradd(buf, tbuf, &buflen);
    }
    else if (fmt & FMT_XYCD) {
	switch(AS) {
	case 0x000:
	case 0x002: buf = stradd(buf, " x", &buflen); break;
	case 0x004: buf = stradd(buf, " xd", &buflen); break;  // dot
	case 0x006: buf = stradd(buf, " xc", &buflen); break;  // circle
	case 0x008:
	case 0x00A: buf = stradd(buf, " y", &buflen); break;
	case 0x00C: buf = stradd(buf, " yd", &buflen); break;  // dot
	case 0x00E: buf = stradd(buf, " yc", &buflen); break;  // circle
	default: break;
	}
    }
    return buf;
}

int besk_emit_instruction(FILE* f, oktet_t OP, halvord_t AS)
{
    char buf[80];
    format_instruction(OP, AS, buf, sizeof(buf));
    return fprintf(f, "%s", buf);
}

// 1- OP & 0x7F  0111 1111
// 2- OP & 0x3F  0011 1111
// 3  OP & 0x1F  0001 1111

void besk_trace(FILE* f, besk_t* state)
{
    halvord_t INS;
    halvord_t AS; 
    oktet_t OP;
    
    INS = state->INS;
    AS = W(INS);
    OP = O(INS);
    
    fprintf(f, "%03X | %05X | ",  state->KR, INS);
    besk_emit_instruction(f, OP, AS);
    fprintf(f, "\n");
}

static void trace_addr(FILE* f, halvord_t addr, halvord_t INS, halvord_t* mem)
{
    if (H(INS))
	fprintf(f, "    || sta [%03X] %05X%05X\n",
		addr, mem[addr], mem[addr+1]);
    else if (addr & 1)
	fprintf(f, "    || sta.h [%03X] %05X\n",
		addr, mem[addr]);
    else
	fprintf(f, "    || sta.l [%03X] %05X\n",
		addr, mem[addr]);
}

static void trace_read(FILE* f, halvord_t addr, helord_t value)
{
    fprintf(f, "    || rd [%03X] = %010lX (%f)\n",
	    addr, value, helord_to_double(value));
}

static void trace_write(FILE* f, halvord_t addr, halvord_t INS, helord_t value)
{
    if (H(INS))
	fprintf(f, "    || wr %03X %010lX (%f)\n",
		addr, value, helord_to_double(value));
    else if (addr & 1)
	fprintf(f,"    || wr.h %03X %05lX\n",
		addr, (value & HALVORD_MASK));
    else
	fprintf(f, "    || wr.l %03X %05lX\n", addr, (value >> 20));
}


// dump 1, 5
// ----- 11111
// 22222 33333
// 44444 55555
//
// dump 1, 6
// ----- 11111
// 22222 33333
// 44444 55555
// 66666 -----
//
void dump_mem(FILE* f, halvord_t addr0, halvord_t addr1, halvord_t* mem)
{
    if (addr0 == -1) addr0 = 0;
    if (addr1 == -1) addr1 = NUM_HALF_CELLS-1;
    addr0 &= 0x7FE;  // ignore top bit and make even
    while(addr0 <= addr1) { // 
	fprintf(f, "%03X %05X %05X\n", addr0, mem[addr0], mem[addr0+1]);
	addr0 += 2;
    }
}

void dump_prog(FILE* f, halvord_t addr0, halvord_t addr1, halvord_t* mem)
{
    if (addr0 == -1) addr0 = 0;
    if (addr1 == -1) addr1 = NUM_HALF_CELLS-1;
    addr0 &= 0x7FE;  // ignore top bit and make even
    while(addr0 <= addr1) { //
	halvord_t ins = mem[addr0];
	char buf[80];
	format_instruction(O(ins), W(ins), buf, sizeof(buf));	
	fprintf(f, "%03X %05X : %s\n", addr0, ins, buf);
	addr0 += 1;
    }
}

void dump_registers(FILE* f, besk_t* besk)
{
    fprintf(f, "MD=%010lX\n", besk->MD);
    fprintf(f, "MR=%010lX\n", besk->MR);
    fprintf(f, "AR00=%d,AR=%010lX,AR40=%d\n", besk->AR00,besk->AR,besk->AR40);
    fprintf(f, "AS=%03X, OP=%02X\n", W(besk->INS), O(besk->INS));
    fprintf(f, "KR=%03X\n", besk->KR);
}

void dump_state(FILE* f, besk_t* besk)
{
    dump_registers(f, besk);
    dump_mem(f, 0, NUM_HALF_CELLS-1, besk->MEM);
}


// initailize step
// load INS and check for STOP condition
void besk_step0(besk_t* state)
{
    halvord_t INS = state->MEM[state->KR];

    if (H(INS) && (W(INS) & 1)) { // odd address and helord operation = STOP
	// FIXME: OP must be updated to halvord operation in case of RE-START!
	INS ^= HELORD_BIT;  // make halvord operation
	state->running = 0; // force stop
    }
    state->INS  = INS;
    state->ARP  = state->AR;
    if (Z(INS)) { state->AR00=0; state->AR40=0; state->AR=0; state->SI=0; }
}

void besk_step(besk_t* state)
{
    helord_t MD;
    helord_t MR;
    helord_t AR, ARP;
    oktet_t  AR00;
    oktet_t  AR40;
    halvord_t KR;
    halvord_t INS;
    halvord_t AS;
    oktet_t   OP;
    oktet_t   SI;

    // swap in
    MD   = state->MD;
    MR   = state->MR;
    AR   = state->AR;
    AR00 = state->AR00;
    AR40 = state->AR40;
    KR   = state->KR;
    INS  = state->INS;
    ARP  = state->ARP; // previous AR (befor zero) when for operations
    SI   = state->SI;
    
    AS = W(INS);
    OP = O(INS);

    if ((OP & KONTROLL_BIT) &&
	(state->kontroll_utskrift_pos!=KONTROLL_UTSKRIFT_OFF)) {
	// FIXME: kontrol utskrift
    }
    
    switch(N(INS)) { // 00 - 1F
    case OP_BAND:  // 0x00 - Bitwise AND. AR = (MD+AR) & MR
	MD = ord_read(H(INS), AS, state->MEM);
	if (state->trace) trace_read(stdout, INS, MD);
	AR = (MD+AR) & MR;
	SI = 0;
	break;
	
    case OP_MOVMR:  // 0x01 - AR=MR, MR=0
	AR = MR;
	MR = 0;
	SI = 0;	
	break;
	
    case OP_MUL: {  // 0x02 (AR,MR) = W*MR + AR*2^-39
	helord_t H, L;
	MD = ord_read(H(INS), AS, state->MEM);
	if (state->trace) trace_read(stdout, INS, MD);	
	H = helord_muladd(MD, MR, 0, helord_sign_bit(AR), &L);

	/*
	printf("MD=%f * MR=%f = (H=%f, L=%f)\n",
	       helord_to_double(MD), helord_to_double(MR),
	       helord_to_double(H), helord_to_double(L));
	*/
	AR = H;
	AR40 = helord_sign_bit(L);
	MR = L >> 1; // & HELORD_FRAC;
	SI = 0;
	break;
    }

    case OP_MULR: {  // 0x03: (AR,MR) = W*MR + 2^-40
	helord_t H, L;
	MD = ord_read(H(INS), AS, state->MEM);
	if (state->trace) trace_read(stdout, INS, MD);
	H = helord_muladd(MD, MR, 0, HELORD_SIGN, &L);

	/*
	printf("MD=%f * MR=%f + 2^-40 = (H=%f, L=%f)\n",
	       helord_to_double(MD), helord_to_double(MR),
	       helord_to_double(H), helord_to_double(L));
	*/
	
	AR = H;
	AR40 = helord_sign_bit(L);
	MR = L >> 1; // & HELORD_FRAC;
	SI = 0;
	break;
    }
	
    case OP_ASHR: // 0x04 | 0x44 OP_SHR
	if (AS <= 0x3F) {
	    if (Z(INS)) // 0x44 MUST use copy of AR  (since AR=0)
		AR = helord_shr(ARP, AS);
	    else
		AR = helord_ashr(AR, AS);
	}
	SI = 0;
	break;

    case OP_SHL:  // 0x05 | (0x45 OP_SHL40) Shift bits left (with AR40)
	if (AS <= 0x3F) {
	    int k = AS;
	    if (Z(INS)) { // 45 MUST use copy of AR  (since AR=0)
		if (k >= 1) {
		    AR = helord_shl(ARP,1) | AR40;
		    k--;
		    AR40 = 0;
		}
	    }
	    if (k > 0)
		AR = helord_shl(AR, k);
	}
	SI = 0;
	break;

    case OP_ADDST:  // 0x06|0x26|0x46 OP_INCST (FIXME: write before STOP?)
	MD = ord_read(H(INS), AS, state->MEM);
	if (state->trace) trace_read(stdout, INS, MD);	
	if (Z(INS))
	    SI = helord_add_oflw(MD, 0x0020000200, &AR);
	else
	    SI = helord_add_oflw(MD, AR, &AR);
	// SI = AR00 != helord_sign_bit(AR);
	ord_write(H(INS), AS, state->MEM, AR);
	if (state->trace) trace_write(stdout, AS, INS, AR);
	break;
	
    case OP_STORA:  // 0x07 [AS] &= addr(AR) ...
	addr_write(H(INS), AS, state->MEM, AR);
	if (state->trace)
	    trace_addr(stdout, AS, INS, state->MEM);
	break;

    case OP_ADDMR:  // 0x08 AR += [AS]; MR = AR;
	MD = ord_read(H(INS), AS, state->MEM);
	if (state->trace) trace_read(stdout, INS, MD);	
	SI = helord_add_oflw(MD, AR, &AR);
	// SI = AR00 != helord_sign_bit(AR);	
	MR = AR;
	break;
	
    case OP_SUBMR:  // 0x09 AR -= [AS]; MR = AR;
	MD = ord_read(H(INS), AS, state->MEM);
	if (state->trace) trace_read(stdout, INS, MD);
	SI = helord_add_oflw(helord_neg(MD), AR, &AR);
	// SI = AR00 != helord_sign_bit(AR);
	MR = AR;
	break;

    case OP_JC: // 0x0A jump on spill (carry)
	if (SI) {
	    KR = AS;
	    goto swapout;
	}
	break;

    case OP_SUB:  // 0x0B AR -= W(AS)
	MD = ord_read(H(INS), AS, state->MEM);
	if (state->trace) trace_read(stdout, INS, MD);
	SI = helord_add_oflw(helord_neg(MD), AR, &AR);
	// SI = AR00 != helord_sign_bit(AR);
	break;
	
    case OP_JMP:  // 0x0C
	KR = AS;
	goto swapout;
	
    case OP_AADD:  // 0x0D AR += |[AS]|
	MD = ord_read(H(INS), AS, state->MEM);
	if (state->trace) trace_read(stdout, INS, MD);
	SI = helord_add_oflw(helord_abs(MD), AR, &AR);
	// SI = AR00 != helord_sign_bit(AR);	
	break;	

    case OP_JGE:  // 0x0E | 0x4E, JGE | JLT AS
	if (Z(INS)) {
	    AR = ARP;
	    if (AR < 0) {
		KR = AS;
		goto swapout;
	    }
	}
	else {
	    if (AR >= 0) {
		KR = AS;
		goto swapout;
	    }
	}
	break;

    case OP_ASUB:  // 0x0F, AR -= |[AS]|
	MD = ord_read(H(INS), AS, state->MEM);
	if (state->trace) trace_read(stdout, INS, MD);	
	SI = helord_add_oflw(helord_neg(helord_abs(MD)), AR, &AR);
	// SI = AR00 != helord_sign_bit(AR);	
	break;	

    case OP_ADD:  // 0x10, AR += [AS]
	MD = ord_read(H(INS), AS, state->MEM);
	if (state->trace) trace_read(stdout, INS, MD);
	SI = helord_add_oflw(MD, AR, &AR);
	// SI = AR00 != helord_sign_bit(AR);	
	break;

    case OP_STORE:  // 0x11, [AS] = AR
	ord_write(H(INS), AS, state->MEM, AR);
	if (state->trace) trace_write(stdout, AS, INS, AR);
	break;

    case OP_DIV: {   // 0x12, AR/[AS] Q=rev(MR), R=AR
	helord_t q;
	MD = ord_read(H(INS), AS, state->MEM);
	if (state->trace) trace_read(stdout, INS, MD);
	q = helord_divrem(AR, MD, &AR);
	MR = helord_reverse(q);
	break;
    }

    case OP_REV:    // 0x13: AR=rev(ME); MR=0
	AR = helord_reverse(MR);
	MR = 0;
	SI = 0;
	break;
	
    case OP_READ5: // ONLY 0x74!!!: Read 5 channel paper tape
	MD = telex_read_remsa(state->in, 1);
	AR = MD;
	ord_write(H(INS), AS, state->MEM, AR);
	if (state->trace) trace_write(stdout, AS, INS, AR);
	SI = 0;
	break;
	
    case OP_NORM:  // NORM|NORM40 0x15|0x35  Normalize AR 
	if (Z(INS)) {
	    AR = ARP;  // restore (FIXME)
	}
    next:
	if (AR != 0) {
	    switch((AR >> 38) & 0x3) {
	    case 1: case 2: break;  // done AR0 != AR1
	    case 0: case 3:
		AR <<= 1;
		if (Z(INS)) {
		    AR |= AR40;
		    AR40 = 0;
		}
		goto next; // AR0 == AR1
	    }
	}
	SI = 0;
	break;

    case OP_FUNC:  // 0x18  Function value display
	switch(AS) {
	case 0x000:  // 0000
	    state->Fx = AR; state->Fop=0;  // Ej skriv
	    break;
	case 0x002:  // 0010
	    state->Fx = AR; state->Fop=0;  // Ej skriv
	    break;
	case 0x004:  // 0100
	    state->Fx = AR; state->Fop=1;  // Punkt
	    break;
	case 0x006:  // 0110
	    state->Fx = AR; state->Fop=2;  // Cirkel
	    break;
	case 0x008:  // 1000
	    state->Fy = AR; state->Fop=0;  // Ej skriv
	    break;
	case 0x00A:  // 1010
	    state->Fy = AR; state->Fop=0;  // Ej skriv
	    break;
	case 0x00C:  // 1100
	    state->Fy = AR; state->Fop=1;  // Punkt
	    break;
	case 0x00E:  // 1110
	    state->Fy = AR; state->Fop=2;  // Cirkel
	    break;
	}
	break;
	
    case  OP_READ4x10: // 0x19|0x39 OP_READ4x1, read 10 hex rows
	if (Z(INS))
	    MD = read_4_channel_remsa(state, 1);
	else
	    MD = read_4_channel_remsa(state, 10);
	AR = MD;
	SI = 0;
	ord_write(H(INS), AS, state->MEM, AR);
	if (state->trace) trace_write(stdout, AS, INS, AR);
	break;

    case OP_RD:      // 0x1B read from drum memory
	AR = read_drum_memory(state, INS, MR);
	break;

    case OP_WRITE4:  // 0x1C write hexadecimal digit to papper tape
	write_4_channel_remsa(state, AR & 0xF);
	break;
	
    case OP_WRITE:  // 0x1D: write 4 bit 
	write_tecken_remsa(state, AR & 0xF);
	break;

    case OP_WD:     // 0x1F: write to drum memory
	AR = write_drum_memory(state, INS, MR);
	break;
	
    default:
	printf("%03X | ERROR %02X not implemented\n", KR, OP);
	state->running = 0;
	break;
	// STOP?
    }
    KR++;
swapout:
    // swapout
    state->MD   = MD;
    state->MR   = MR;
    state->AR   = AR;
    state->AR00 = AR00;
    state->AR40 = AR40;
    state->KR   = KR;
    state->SI   = SI;
}

void usage()
{
    fprintf(stderr, "usage: besk [options] [file]\n");
    fprintf(stderr, "OPTIONS\n");
    fprintf(stderr, "  -a <addr>  start address\n");
    fprintf(stderr, "  -e <addr>  end address\n");    
    fprintf(stderr, "  -i <filename> of input  (INREMSA)\n");
    fprintf(stderr, "  -u <filename> of output (UTREMSA)\n");
    fprintf(stderr, "  -d <filename> of drum   (DRUM.dat)\n");    
    fprintf(stderr, "  -s         single step\n");
    fprintf(stderr, "  -t         trace instruction\n");    
    fprintf(stderr, "  -S         Simulator\n");
    fprintf(stderr, "  -q         do not run\n");
    fprintf(stderr, "  -m r       dump registers\n");    
    fprintf(stderr, "  -m m       dump memory\n");
    fprintf(stderr, "  -m p       dump program\n");    
    exit(1);
}

int clamp(int x, int a, int b)
{
    if (x < a) return a;
    if (x > b) return b;
    return x;
}

int main(int argc, char** argv)
{
    besk_t state;
    FILE* f;
    FILE* fin;
    FILE* fut;
    FILE* fdrum;
    char* filename = "*stdin*";
    char* utremsa_name = "UTREMSA";
    char* inremsa_name = "INREMSA";
    char* drum_name = "DRUM.dat";
    halvord_t addr = 0x008;
    halvord_t start = -1;
    halvord_t end = -1;
    int sim = 0;
    int step = 0;
    int quit = 0;
    char* mdump = "";
    int trace = 0;
    int opt;
    int xpos = 1, ypos = 1;
    
    while ((opt = getopt(argc, argv, "tSsqi:u:d:a:e:x:y:m:")) != -1) {
	switch(opt) {
	case 'a': {
	    char* eptr;
	    start = strtol(optarg, &eptr, 0);
	    if (*eptr != '\0') usage();
	    break;
	}
	case 'e': {
	    char* eptr;
	    end = strtol(optarg, &eptr, 0);
	    if (*eptr != '\0') usage();
	    break;
	}	    
	case 'S':
	    sim = 1;
	    break;
	case 's':
	    step = 1;
	    break;
	case 'm':
	    mdump = optarg;
	    break;
	case 't':
	    trace = 1;
	    break;
	case 'i': // set inremsa
	    inremsa_name = optarg;
	    break;
	case 'u': // set utremsa
	    utremsa_name = optarg;
	    break;
	case 'd': // set drum memory file name
	    drum_name = optarg;
	    break;	    
	case 'x':
	    xpos = atoi(optarg);
	    xpos = clamp(xpos, 1, 8);
	    break;
	case 'y':
	    ypos = atoi(optarg);
	    ypos = clamp(ypos, 1, 8);
	    break;
	case 'q':
	    quit = 1;
	    break;
	default:
	    usage();
	}
    }

    if (optind >= argc) {
	f = stdin;
	filename = "*stdin*";
    }
    else {
	if ((f = fopen(argv[optind], "r")) == NULL) {
	    fprintf(stderr, "unable to open file %s\n", argv[optind]);
	    usage();
	}
	filename = argv[optind];
    }
    if ((fin = fopen(inremsa_name, "r")) == NULL) {
	fprintf(stderr, "unable to open input paper tape file %s\n",
		inremsa_name);
	exit(1);
    }
    if ((fut = fopen(utremsa_name, "w")) == NULL) {
	fprintf(stderr, "unable to open output paper tape file %s\n",
		utremsa_name);
	exit(1);
    }
    if ((fdrum = fopen(drum_name, "rw")) == NULL) {
	fprintf(stderr, "unable to open output drum file %s\n",
		drum_name);
	exit(1);
    }
    
    memset(&state, 0, sizeof(state));
    state.Fpos_x = xpos;
    state.Fpos_y = ypos;
    state.quit = quit;
    // state.page = TELEX_PAGE_LTR; // do we start here?
    // load constants (should be from paper tape? | drum memory?)
    helord_write(0x000, state.MEM, 0x0020000200);
    helord_write(0x002, state.MEM, 0x0010000100);
    helord_write(0x004, state.MEM, 0x8000000001);
    helord_write(0x006, state.MEM, 0x0000000839);

    addr = load_code(f, filename, 0, addr, state.MEM);
    if ((addr < 0) && (start < 0)) {
	fprintf(stderr, "neither program or start address is given\n");
	usage();
    }
    if (f != stdin)
	fclose(f);
    state.in = fin;
    state.ut = fut;
    state.drum = fdrum;

    if (sim) {
	SIMULATOR_INIT(argc, argv, &state);
    }
    if (step)
	state.gang_pos = GANG_STEP;
    else
	state.gang_pos = GANG_RUN;
    state.kontroll_utskrift_pos = KONTROLL_UTSKRIFT_OFF;

    state.trace = trace;
    
    state.running = 1;
    state.KR = (start<0) ? addr : start;

    while(!state.quit) {
	if (state.running) {
	    besk_step0(&state);
	    if (abs(state.gang_pos) == GANG_STEP) {
		if (sim) { SIMULATOR_RUN(&state); }
		if (abs(state.kontroll_utskrift_pos) == KONTROLL_UTSKRIFT_STEGVIS) {
		    state.trace = 1; // memory trace as well
		    besk_trace(stdout, &state);
		}
		besk_step(&state);
		state.running = 0;
		state.trace = 0;		
	    }
	    else {
		besk_step(&state);
		if (sim) { SIMULATOR_RUN(&state); }
	    }
	}
	else {
	    if (sim) { SIMULATOR_RUN(&state); }
	    else { state.quit = 1; }
	}
    }
    if (mdump) {
	while(*mdump) {
	    switch(*mdump) {
	    case 'r': dump_registers(stdout, &state); break;
	    case 'm': dump_mem(stdout, start, end, state.MEM); break;
	    case 'p': dump_prog(stdout, start, end, state.MEM); break;
	    }
	    mdump++;
	}
    }
    exit(0);
}
