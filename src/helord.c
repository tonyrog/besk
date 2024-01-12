// BESK helord operations

#include <math.h>

#include "helord.h"

// convert a helord into a double
double helord_to_double(helord_t x)
{
    if (helord_sign_bit(x))
	return -( ((-x) & HELORD_FRAC) / (double) HELORD_FRAC );
    else
	return (x / (double) HELORD_FRAC);
}

helord_t helord_from_double(double y)
{
    helord_t x;
    
    if (y < -1.0)
	y = -1.0;
    else if (y >= 1.0)
	y = 1.0 - pow(2, -39);
    if (y >= 0.0)
	x = ((helord_t)(y*HELORD_SIGN)) & HELORD_MASK;
    else
	x = helord_neg(((helord_t)((-y)*HELORD_SIGN)) & HELORD_MASK);
    return x;
}

helord_t helord_reverse(helord_t x)
{
    int i;
    helord_t y = 0;
    for (i = 0; i < 40; i++) {
	y = (y<<1) | (x & 1);
	x >>= 1;
    }
    return y;
//    return helord_sign_extend(y);
}
