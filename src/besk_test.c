//
// Besk tests
//
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "helord.h"

#define FIXEPS  1.0E-11

void test_besk_int()
{
    helord_t a, b, c, c1, c0;
    int co;

    a = helord_from_int64(12);
    b = helord_from_int64(98);    
    co = helord_add(a, b, &c);

    assert(c == 110);
    assert(co == 0);

    a = helord_from_int64(0x7FFFFFFFFF);
    b = helord_from_int64(1);
    co = helord_add(a, b, &c);

    assert(0 == helord_to_int64(c));


    a = helord_from_int64(0x7FFFFFFFFF);
    b = helord_from_int64(0x7FFFFFFFFF);
    co = helord_add(a, b, &c);

    assert(-2 == helord_to_int64(c));
    assert(co == 0);

    a = helord_from_int64(0xFFFFFFFFFF);
    b = helord_from_int64(0xFFFFFFFFFF);
    co = helord_add(a, b, &c);

    assert(-2 == helord_to_int64(c));
    assert(co == 1);

    // test multiplication
    a = helord_from_int64(123);
    b = helord_from_int64(456);
    c1 = helord_mul(a, b, &c0);

    assert(c1 == 0);
    assert(c0 == 56088);

    a = helord_from_int64(0x4876543210);
    b = helord_from_int64(0x2234567890);
    c1 = helord_mul(a, b, &c0);
    printf("c1 = 0x%010lX, c0 = 0x%010lX\n", c1, c0);
    assert(c0 == 0xCE1833A900);
    assert((c1>>1) == 0x09AE87B1A0);	

    a = helord_from_int64(0xF876543210);
    b = helord_from_int64(0xF234567890);
    c1 = helord_mul(a, b, &c0);
    printf("c1 = 0x%010lX, c0 = 0x%010lX\n", c1, c0);
    assert(c0 == 0xCE1833A900);
    assert((c1>>1) == 0xF29C535B20);
}

int fix_equal(helord_t x, double y)
{
    double y0;
    double delta;
    y0 = helord_to_double(x);
    delta = fabs(y0 - y);
    // printf("y0 = %f, delta = %f\n", y0, delta);
    return delta < FIXEPS;
}

void test_besk_fix()
{
    helord_t a, b, c, c1, c0;
    helord_t q, r;
    int si;

    a = helord_from_double(0.5);
    c0 = helord_neg(a);
    if (!fix_equal(c0, -0.5)) {
	printf("FAIL -%f == %f expect %f\n",
	       helord_to_double(a), 
	       helord_to_double(c0), -0.5);
    }

    a = helord_from_double(-0.5);
    c0 = helord_neg(a);
    if (!fix_equal(c0, 0.5)) {
	printf("FAIL -%f == %f expect %f\n",
	       helord_to_double(a), 
	       helord_to_double(c0), 0.5);
    }    

    a = helord_from_double(0.5);
    b = helord_from_double(0.4);
    si = helord_add_oflw(a, b, &c);

    assert(fix_equal(c, 0.9));
    assert(si == 0);

    a = helord_from_double(0.5);
    b = helord_from_double(0.6);
    si = helord_add_oflw(a, b, &c);
    printf("c = %f (si=%d)\n", helord_to_double(c), si);
    assert(si == 1);

    a = helord_from_double(0.5);
    b = helord_from_double(0.4);
    c1 = helord_mul(a, b, &c0);

    if (!fix_equal(c1, 0.2)) {
	printf("FAIL %f*%f == %f expect %f\n",
	       helord_to_double(a), helord_to_double(b),
	       helord_to_double(c1), 0.2);
	printf("a = 0x%010lX\n", a);
	printf("b = 0x%010lX\n", b);
	printf("c1 = 0x%010lX, c0 = 0x%010lX\n", c1, c0);
	printf("c0 = %f\n", helord_to_double(c0));
    }

    a = helord_from_double(-0.5);
    b = helord_from_double(-0.4);
    c1 = helord_mul(a, b, &c0);
    
    if (!fix_equal(c1, 0.2)) {
	printf("%d: FAIL %g*%g == %f expect %g\n",
	       __LINE__,
	       helord_to_double(a), helord_to_double(b),
	       helord_to_double(c1), 0.2);
	printf("a = 0x%010lX\n", a);
	printf("b = 0x%010lX\n", b);
	printf("c1 = 0x%010lX, c0 = 0x%010lX\n", c1, c0);
	printf("c0 = %f\n", helord_to_double(c0));
    }
	

    a = helord_from_double(0.5);
    b = helord_from_double(-0.4);
    c1 = helord_mul(a, b, &c0);
    if (!fix_equal(c1, -0.2)) {
	printf("%d: FAIL %f*%f == %f expect %f\n",
	       __LINE__,
	       helord_to_double(a), helord_to_double(b),
	       helord_to_double(c1), -0.2);
	printf("a = 0x%010lX\n", a);
	printf("b = 0x%010lX\n", b);
	printf("c1 = 0x%010lX, c0 = 0x%010lX\n", c1, c0);
	printf("c0 = %f\n", helord_to_double(c0));

    }
	
    a = helord_from_double(-0.5);
    b = helord_from_double(0.4);
    c1 = helord_mul(a, b, &c0);
    if (!fix_equal(c1, -0.2)) {
	printf("%d: FAIL %f*%f == %f expect %f\n",
	       __LINE__,
	       helord_to_double(a), helord_to_double(b),
	       helord_to_double(c1), -0.2);
	printf("a = 0x%010lX\n", a);
	printf("b = 0x%010lX\n", b);
	printf("c1 = 0x%010lX, c0 = 0x%010lX\n", c1, c0);
	printf("c0 = %f\n", helord_to_double(c0));

    }    

    a = helord_from_double(0.1);
    b = helord_from_double(0.8);
    q = helord_divrem(a, b, &r);
    // q = helord_reverse(q);
    printf("a = %f, b = %f, q = %f, r = %f)\n",
	   helord_to_double(a), helord_to_double(b),
	   helord_to_double(q), helord_to_double(r));
    assert(fix_equal(q, 0.125));
}

int main(int argc, char** argv)
{
    test_besk_int();
    test_besk_fix();
    exit(0);
}
