// BESK halvord operations 

#include "halvord.h"

// works on both sign exteded and non sign extened
halvord_t halvord_reverse(halvord_t x)
{
    int i;
    halvord_t y = 0;
    for (i = 0; i < 20; i++) {
	y = (y<<1) | (x & 1);
	x >>= 1;
    }
    return y;
}
