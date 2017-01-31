#include <cstdlib>
#include <cassert>
#include <cstring>
#include "HashAlgs.h"
//#include "MurmurHash3.h"
//#include "xxhash.h"
// #include "intel_sha1.h"
//#include "polarssl_sha1.h"
#include "RollingHash.h"
#include "city.h"
//#include "stdafx.h"

int getDefaultHashFunctionId() {
  return HASH_ALGORITHM;
}


const char *getHashFunctionName(int hashFnId) {
  switch (hashFnId) {
  case 0: return "xxHash";
  case 1: return "FNV32";
  case 2: return "FNV64";
  case 3: return "Murmur64";
  case 4: return "Rolling32";
  case 5: return "Rolling64";
  default: return "unknown";
  }
}


int getHashFunctionResultBits(int hashFnId) {
  switch (hashFnId) {
  case 0: case 1: case 4:
    return 32;
  case 2: case 3: case 5:
    return 64;
  default:
    return 0;
  }
}


// Fowler-Noll-Vo-1a algoritm
// http://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
//unsigned fnvHash(const void *data_v, unsigned len) {
//  const unsigned char *data = (const unsigned char *) data_v;
//  unsigned hash = FNV_HASH_INIT;
//  for (unsigned i=0; i < len; i++) {
//    hash = hash ^ data[i];
//    hash = hash * FNV_HASH_PERMUTE;
//  }
//  return hash;
//}

//u64 fnvHash64(const void *data_v, unsigned len) {
//  const unsigned char *data = (const unsigned char *) data_v;
//  u64 hash = FNV_HASH_INIT64;
//  for (unsigned i=0; i < len; i++) {
//    hash = hash ^ data[i];
//    hash = hash * FNV_HASH_PERMUTE64;
//  }
//  return hash;
//}

// Murmur3
// http://en.wikipedia.org/wiki/MurmurHash
//unsigned murmurHash(const void *data, unsigned len) {
//  unsigned result;
//  MurmurHash3_x86_32(data, len, 0, &result);
//  return result;
//}

//u64 murmurHash64(const void *data, unsigned len) {
//  u128 result;
//  MurmurHash3_x86_128(data, len, 0, &result);
//  /*
//  if (sizeof(void*) == 4) {
//    MurmurHash3_x86_128(data, len, 0, &result);
//  } else {
//    MurmurHash3_x64_128(data, len, 0, &result);
//  }
//  */
//  return result.lo;
//}


//u128 murmurHash128(const void *data, unsigned len) {
//  u128 result;
//  MurmurHash3_x86_128(data, len, 0, &result);
//  /*
//  if (sizeof(void*) == 4) {
//    MurmurHash3_x86_128(data, len, 0, &result);
//  } else {
//    MurmurHash3_x64_128(data, len, 0, &result);
//  }
//  */
//  return result;
//}


//// xxhash
//// https://code.google.com/p/xxhash/
//unsigned xxHash(const void *data, unsigned len) {
//  return XXH32(data, len, 0);
//}

//unsigned sha1Hash32(const void *data, unsigned len) {
//  u160 result;
//  sha1((const unsigned char*)data, len, (unsigned char*)result.data);
//  return result.data[0];
//}
//
//u64 sha1Hash64(const void *data, unsigned len) {
//  u160 result;
//  sha1((const unsigned char*)data, len, (unsigned char*)result.data);
//  return to_u64(result.data[0], result.data[1]);
//}
//
//u128 sha1Hash128(const void *data, unsigned len) {
//  u160 result;
//  sha1((const unsigned char*)data, len, (unsigned char*)result.data);
//  return u128::to_u128(result.data[0], result.data[1], 
//		       result.data[2], result.data[3]);
//}
//
//u160 sha1Hash(const void *data, unsigned len) {
//  u160 result;
//  sha1((const unsigned char*)data, len, (unsigned char*)result.data);
//  return result;
//}


unsigned rollingHash32(const void *data_v, unsigned len) {
  RollingHash<unsigned> roll;
  const unsigned char *data = (const unsigned char *)data_v;
  for (unsigned i=0; i < len; i++)
    roll.addChar(data[i]);
  return roll.getHash();
}

u64 rollingHash64(const void *data_v, unsigned len) {
  RollingHash<u64> roll;
  const unsigned char *data = (const unsigned char *)data_v;
  for (unsigned i=0; i < len; i++)
    roll.addChar(data[i]);
  return roll.getHash();
}

unsigned cityHash32 (const void *data, unsigned len) {
  return CityHash32((const char *)data, len);
}

u64      cityHash64 (const void *data, unsigned len) {
  return CityHash64((const char *)data, len);
}

u128     cityHash128(const void *data, unsigned len) {
  uint128 result = CityHash128((const char *)data, len);
  u128 r;
  r.lo = result.first;
  r.hi = result.second;
  return r;
}

