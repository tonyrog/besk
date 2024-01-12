// Test telex functions
#ifndef __TELEX_H__
#define __TELEX_H__

#include <stdint.h>
#include <stdio.h>

#define TELEX_PAGE_LTR    0x4000  // 0100
#define TELEX_PAGE_FIG    0x8000  // 1000
#define TELEX_PAGE_SWITCH 0xC000

#define ITA2_LTR 0x1F
#define ITA2_FIG 0x1B

extern uint64_t telex_read_remsa(FILE* f, int n);
extern int telex_write_remsa(FILE* f, uint8_t code);

extern int telex_encode(int c, int* page, uint8_t* outbuf);
extern int telex_decode(uint8_t y, int* page, uint16_t* outbuf);

#endif
