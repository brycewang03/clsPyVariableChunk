#ifndef __U64__
#define __U64__
#include <limits.h>

typedef long long unsigned u64;
typedef long long i64;

#define U64_MAX ULLONG_MAX
#define I64_MIN LLONG_MIN
#define I64_MAX LLONG_MAX

#define U64_PRINTF "%llu"
#define U64_PRINTX "%016llx"
#define I64_PRINTF "%lld"

#define to_u64(lo32,hi32) ((lo32) | (u64)(hi32)<<32)

#endif // __U64__
