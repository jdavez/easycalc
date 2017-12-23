#pragma once

#ifndef FLOAT_MANAGER_H
#define FLOAT_MANAGER_H 1

#include "PalmOS.h"

// Inferring from definitions in Palm OS FloatMgr.h as, and the fact that Windows CE
// supports only little endian, even on ARM platforms which are big-endian.
typedef struct {
    UInt32 manL;
    UInt32 manH : 20;
    Int32  exp : 11;
    UInt32 sign : 1;
} FlpDoubleBits;

typedef unsigned int SDWord;
typedef struct {
    SDWord low;
    SDWord high;
} _sfpe_64_bits;
typedef _sfpe_64_bits FlpDouble;

typedef union {
    double        d;
    FlpDouble     fd;
    UInt32        ul[2];
    FlpDoubleBits fdb;
} FlpCompDouble;

// Inferring those ...
#define __HI32(x) (x)->fd.high
#define __LO32(x) (x)->fd.low
#define __HIX 1

// Defined in Palm OS FloatMgr.h as:
#define FlpIsZero(x) (((__HI32((FlpCompDouble *)&x) & 0x7fffffff) | (__LO32((FlpCompDouble *)&x))) == 0)
#define FlpIsZero1(x) ((_fpclass(x) & (_FPCLASS_NZ | _FPCLASS_PZ)) != 0)
#define FlpGetSign(x) ((__HI32((FlpCompDouble *)&x) & 0x80000000) != 0)
#define FlpGetSign1(x) ((_fpclass(x) & (_FPCLASS_NINF | _FPCLASS_NN | _FPCLASS_ND | _FPCLASS_NZ)) != 0)
#define FlpGetExponent(x) (((__HI32((FlpCompDouble *)&x) & 0x7ff00000) >> 20) - 1023)
// Can be replaced by a function using frexp()
#define FlpNegate(x) (((FlpCompDouble *)&x)->ul[__HIX] ^= 0x80000000)
#define FlpNegate1(x) (x = _chgsign(x))
#define FlpSetNegative(x) (((FlpCompDouble *)&x)->ul[__HIX] |= 0x80000000)
#define FlpSetNegative1(x) (x = -fabs(x))
#define FlpSetPositive(x) (((FlpCompDouble *)&x)->ul[__HIX] &= ~0x80000000)
#define FlpSetPositive1(x) (x = fabs(x))

#endif