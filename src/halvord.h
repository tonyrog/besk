//
//  BESK halvord
//
#ifndef __HALVORD_H__
#define __HALVORD_H__

#include <stdint.h>
#include <assert.h>

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


typedef int32_t halvord_t;   // 20-bit

#define HALVORD_MASK 0xFFFFF
#define HALVORD_ADDR 0xFFF00
#define HALVORD_OP   0x000FF

extern halvord_t halvord_reverse(halvord_t x);

LOCAL inline halvord_t halvord_abs(halvord_t x) LOCAL_API;
LOCAL inline halvord_t halvord_sign_bit(halvord_t x) LOCAL_API;

// works for both sign extended and none sign extened data
static inline int halvord_sign_bit(halvord_t x)
{
    return ((x >> 19) & 1);
}

static inline int is_halvord(halvord_t x)
{
    return (((x) & ~HALVORD_MASK) == 0);
}


// works for sign extended halvord
static inline halvord_t halvord_neg(halvord_t a)
{
    assert(is_halvord(a));
    return (~a + 1) & HALVORD_MASK;
}

static inline halvord_t halvord_abs(halvord_t a)
{
    assert(is_halvord(a));
    return halvord_sign_bit(a) ? halvord_neg(a) : a;
}

#endif
