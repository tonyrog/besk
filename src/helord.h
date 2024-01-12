//
// BESK helord
// A helord is a 40-bit signed fixed point number with 39 bits of
// fraction and one sign bit.  The fraction is stored in the low 39
// bits of the helord.  The sign bit is stored in the high bit of the
// helord.  The sign bit is 1 for negative numbers and 0 for positive
// numbers.  The fraction is stored in two's complement form.
// The helord_t is stored in a 64 bit word with the high 24 bits unused.
//
#ifndef __HELORD_H__
#define __HELORD_H__

#include <assert.h>
#include <stdint.h>

#include "halvord.h"

#ifndef LOCAL
#define LOCAL static
#endif

#if defined(__WIN32__) || defined(_WIN32)
#ifndef LOCAL_API
#define LOCAL_API
#endif
#else
#ifndef LOCAL_API
#define LOCAL_API __attribute__ ((unused))
#endif
#endif

typedef uint64_t helord_t;   // 40-bit vhao:20, hhao (packed in 64 bit)

#define HELORD_MASK 0xFFFFFFFFFFL
#define HELORD_FRAC 0x7FFFFFFFFFL
#define HELORD_SIGN 0x8000000000L

LOCAL inline helord_t helord_read(unsigned addr, halvord_t* mem) LOCAL_API;
LOCAL void helord_write(unsigned addr, halvord_t* mem, helord_t value) LOCAL_API;

extern double   helord_to_double(helord_t x);
extern helord_t helord_from_double(double y);
extern helord_t helord_reverse(helord_t x);

LOCAL inline int64_t helord_to_int64(helord_t x) LOCAL_API;
LOCAL inline helord_t helord_from_int64(int64_t x) LOCAL_API;

LOCAL inline int is_helord(helord_t x) LOCAL_API;
LOCAL inline int helord_sign_bit(helord_t x) LOCAL_API;
LOCAL inline int64_t helord_sign_extend(helord_t x) LOCAL_API;
LOCAL inline helord_t helord_neg(helord_t a) LOCAL_API;
LOCAL inline helord_t helord_abs(helord_t a) LOCAL_API;
LOCAL inline helord_t helord_shl(helord_t a, unsigned s) LOCAL_API;
LOCAL inline helord_t helord_shr(helord_t a, unsigned s) LOCAL_API;
LOCAL inline helord_t helord_ashr(helord_t a, unsigned s) LOCAL_API;
LOCAL inline int helord_add(helord_t a, helord_t b, helord_t* rp) LOCAL_API;
LOCAL inline int helord_add_oflw(helord_t a, helord_t b, helord_t* rp) LOCAL_API; 
LOCAL inline helord_t helord_mul(helord_t a, helord_t b, helord_t* lwp) LOCAL_API;
LOCAL inline helord_t helord_divrem(helord_t a, helord_t b, helord_t* rp) LOCAL_API;
LOCAL inline helord_t helord_muladd(helord_t a, helord_t b,
				    helord_t c1, helord_t c0, helord_t* lwp) LOCAL_API;

static helord_t helord_read(unsigned addr, halvord_t* mem)
{
    return ((((helord_t)mem[addr]) << 20) & ~((helord_t)0xFFFFF)) |
	(mem[addr+1] & 0xFFFFF);
}

static void helord_write(unsigned addr, halvord_t* mem, helord_t value)
{
    addr &= 0x7ff; // ignore top bit! (cyclic memory!)
    // value = helord_sign_extend(value);
    mem[addr]     = (value>>20);      // left
    mem[addr+1]   = (value & HALVORD_MASK);  // right (positive)
}


// works for both sign extended and none sign extened data
static inline int helord_sign_bit(helord_t x)
{
    assert(is_helord(x));
    return ((x >> 39) & 1);
}

static inline helord_t helord_from_int64(int64_t x)
{
    return ((x) & HELORD_MASK);
}

static inline int64_t helord_sign_extend(helord_t x)
{
    return ((((int64_t) x) << 24) >> 24);
}

static inline int64_t helord_to_int64(helord_t x)
{
    assert(is_helord(x));
    if (helord_sign_bit(x))
	return -((~x + 1) & HELORD_FRAC);
    else
	return x;
}
    
static inline int is_helord(helord_t x)
{
    return (((x) & ~HELORD_MASK) == 0);
}


// works for sign extended helord
static inline helord_t helord_neg(helord_t a)
{
    assert(is_helord(a));
    return (~a + 1) & HELORD_MASK;
}

static inline helord_t helord_abs(helord_t a)
{
    assert(is_helord(a));
    return helord_sign_bit(a) ? helord_neg(a) : a;
}

static inline helord_t helord_shl(helord_t a, unsigned s)
{
    assert(is_helord(a));
    return (a << s) & HELORD_MASK;
}

static inline helord_t helord_shr(helord_t a, unsigned s)
{
    return (a >> s);
}

static inline helord_t helord_ashr(helord_t a, unsigned s)
{
    assert(is_helord(a));
    return ((a << 24) >> (24+s)) & HELORD_MASK;
}

// add and return carry out
static inline int helord_add(helord_t a, helord_t b, helord_t* rp)
{
    helord_t s;
    assert(is_helord(a));
    assert(is_helord(b));
    s = a + b;
    *rp = s & HELORD_MASK;
    return (s >> 40) & 1; // carry 
}

// add and return overflow out
static inline int helord_add_oflw(helord_t a, helord_t b, helord_t* rp)
{
    helord_t s, r;
    assert(is_helord(a));
    assert(is_helord(b));
    s = a + b;
    r = s & HELORD_MASK;
    *rp = r;
    return ((a>>39) && (b >> 39) && !(r>>39)) ||
	(!(a>>39) && !(b>>39) && (r>>39));
}

// double word add and return carry out
static inline int helord_dadd(helord_t a1, helord_t a0,
			      helord_t b1, helord_t b0,
			      helord_t* rp1, helord_t* rp0)
{
    helord_t c1, c0;
    
    assert(is_helord(a1)); assert(is_helord(a0));
    assert(is_helord(b1)); assert(is_helord(b0));
    
    c0 = a0 + b0;
    c1 = a1 + b1;
    if ((c0 >> 40) & 1) c1++;
    *rp0 = c0 & HELORD_MASK;
    *rp1 = c1 & HELORD_MASK;
    return (c1 >> 40) & 1; // carry
}

static inline void helord_dshl1(helord_t a1, helord_t a0,
				helord_t* rp1, helord_t* rp0)
{
    a1 = (a1 << 1) | helord_sign_bit(a0);
    a0 = (a0 << 1);
    *rp1 = a1 & HELORD_MASK;
    *rp0 = a0 & HELORD_MASK;
}


static inline void helord_dashr1(helord_t a1, helord_t a0,
				helord_t* rp1, helord_t* rp0)
{
    a0 = (a0 >> 1) | ((a1 & 1)<<39);
    a1 = (a1 >> 1) | ((helord_t)helord_sign_bit(a1)<<39); // sign extend
    *rp1 = a1 & HELORD_MASK;
    *rp0 = a0 & HELORD_MASK;
}

static inline void helord_dshr1(helord_t a1, helord_t a0,
				helord_t* rp1, helord_t* rp0)
{
    a0 = (a0 >> 1) | ((a1 & 1)<<39);
    a1 = (a1 >> 1);
    *rp1 = a1 & HELORD_MASK;
    *rp0 = a0 & HELORD_MASK;
}

// Multiply two helords and return the high and low parts
// A   = |xxx|a0..a39|
// B   = |xxx|b0..b39|

static inline helord_t helord_mul(helord_t a, helord_t b, helord_t* lwp)
{
    __int128 p;
    int64_t aa, bb;
    assert(is_helord(a));
    assert(is_helord(b));
    aa = helord_sign_extend(a);
    bb = helord_sign_extend(b);
    p = (__int128)aa * bb;

    *lwp = ((helord_t) p) & HELORD_MASK;
    return ((helord_t) (p >> 39)) & HELORD_MASK;
}

// calculate a*b + (c1,c0)
static inline helord_t helord_muladd(helord_t a, helord_t b,
				     helord_t c1, helord_t c0,
				     helord_t* lwp)
{
    int sp;
    helord_t p1, p0;
    assert(is_helord(a));
    assert(is_helord(b));    
    p1 = helord_mul(a, b, &p0);
    sp = helord_add(p0, c0, lwp);
    // ignore overflow? (or preserve)
    (void) helord_add(p1, sp, &p1);
    (void) helord_add(p1, c1, &p1);
    return p1;
}

// calculate *qq = a*b + *rp
// return quitient and store remainder in *rp
static inline helord_t helord_divrem(helord_t a, helord_t b, helord_t* rp)
{
    __int128 n128; 
    int64_t a64, b64, r, q;
    assert(is_helord(a));
    assert(is_helord(b));
    a64 = helord_sign_extend(a);
    n128 = ((__int128)a64) << 39;
    /* printf("n128 = %016lX.%016lx\n",
	   (uint64_t)(n128>>64),
	   (uint64_t)(n128 & 0xFFFFFFFFFFFFFFFFL)); */
    b64 = helord_sign_extend(b);
    q = n128 / b64;
    // printf("q = %016lX (%f)\n", q, helord_to_double(helord_from_int64(q)));
    r = n128 % b64;
    // printf("r = %016lX\n", r);
    *rp = helord_from_int64(r);
    return helord_from_int64(q);
}

#endif
