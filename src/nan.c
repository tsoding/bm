#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define EXP_MASK (((1LL << 11LL) - 1LL) << 52LL)
#define FRACTION_MASK ((1LL << 52LL) - 1LL)
#define SIGN_MASK (1LL << 63LL)
#define TYPE_MASK (((1LL << 4LL) - 1LL) << 48LL)
#define VALUE_MASK ((1LL << 48LL) - 1LL)

#define TYPE(n) ((1LL << 3LL) + n)

static double mk_inf(void)
{
    uint64_t y = EXP_MASK;
    return *(double*)&y;
}

static double set_type(double x, uint64_t type)
{
    uint64_t y = *(uint64_t *)&x;
    y = (y & (~TYPE_MASK)) | (((TYPE_MASK >> 48LL) & type) << 48LL);
    return *(double*)&y;
}

static uint64_t get_type(double x)
{
    uint64_t y = (*(uint64_t*)&x);
    return (y & TYPE_MASK) >> 48LL;
}

static double set_value(double x, uint64_t value)
{
    uint64_t y = *(uint64_t *)&x;
    y = (y & (~VALUE_MASK)) | (value & VALUE_MASK);
    return *(double*)&y;
}

static uint64_t get_value(double x)
{
    uint64_t y = (*(uint64_t*)&x);
    return (y & VALUE_MASK);
}

#define INTEGER_TYPE 1
#define POINTER_TYPE 2

static int is_double(double x)
{
    return !isnan(x);
}

static int is_integer(double x)
{
    return isnan(x) && get_type(x) == TYPE(INTEGER_TYPE);
}

static int is_pointer(double x)
{
    return isnan(x) && get_type(x) == TYPE(POINTER_TYPE);
}

static double as_double(double x)
{
    return x;
}

static uint64_t as_integer(double x)
{
    return get_value(x);
}

static void *as_pointer(double x)
{
    return (void*) get_value(x);
}

static double box_double(double x)
{
    return x;
}

static double box_integer(uint64_t x)
{
    return set_value(set_type(mk_inf(), TYPE(INTEGER_TYPE)), x);
}

static double box_pointer(void* x)
{
    return set_value(set_type(mk_inf(), TYPE(POINTER_TYPE)), (uint64_t) x);
}

int main(void)
{
    double pi = 3.14159265359;

    assert(pi         == as_double(box_double(pi)));
    assert(12345678LL == as_integer(box_integer(12345678LL)));
    assert(&pi        == as_pointer(box_pointer(&pi)));

    printf("OK\n");

    return 0;
}
