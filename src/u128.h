#ifndef __U128_H__
#define __U128_H__

#include "u64.h"
//#include "cucheck.h"
#include "u160.h"
#include "stdio.h"

//#define HD __host__ __device__
#define DV __device__

#define HD
//#define DV

struct u128 {
  u64 lo, hi;

  //HD u128() {}

  HD u128 operator ~ () const {
    u128 result;
    result.lo = ~lo;
    result.hi = ~hi;
    return result;
  }

  HD bool operator == (u64 x) const {
    return hi == 0 && lo == x;
  }

  HD bool operator != (u64 x) const {
    return !(*this == x);
  }

  HD bool operator == (const u128 &x) const {
    return hi == x.hi && lo == x.lo;
  }

  HD bool operator != (const u128 &x) const {
    return !(*this == x);
  }

  HD bool operator < (const u128 &x) const {
    if (hi == x.hi) {
      return lo < x.lo;
    } else {
      return hi < x.hi;
    }
  }

  HD u64 operator & (u64 x) {
    return lo & x;
  }

  // word0 is the least significant
  static u128 to_u128(unsigned word0, unsigned word1, unsigned word2, unsigned word3) {
    u128 x;
    x.lo = to_u64(word0, word1);
    x.hi = to_u64(word2, word3);
    return x;
  }

  bool fromHex(const char *p) {
    // skip leading whitespace
    while (isspace(*p)) p++;

    // skip optional "0x" prefix
    if (p[0] == '0' && tolower(p[1]) == 'x') p += 2;

    if (!isxdigit(*p)) return false;

    const char *pstart = p;

    // find the end
    while (isxdigit(*(p+1))) p++;
    
    lo = hi = 0;

    u64 factor = 1;
    while (p >= pstart && factor) {
      lo |= factor * u160::toHexDigit(*p);
      p--;
      factor <<= 4;
    }

    if (p >= pstart) {
      factor = 1;
      while (p >= pstart && factor) {
	hi |= factor * u160::toHexDigit(*p);
	p--;
	factor <<= 4;
      }      
    }
    return true;
  }


  const char *toHex(char buf[33]) const {
    sprintf(buf, U64_PRINTX, hi);
    sprintf(buf+16, U64_PRINTX, lo);
    return buf;
  }
  
};

#endif // __U128_H__
