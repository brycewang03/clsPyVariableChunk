#ifndef __HASH_ALGS_H__
#define __HASH_ALGS_H__

#include <climits>
#include "RollingHash.h"
#include "u64.h"
#include "u128.h"
#include "u160.h"
//#include "cucheck.h"


#define HASH_ALG_XXHASH 0
#define HASH_ALG_FNV32 1
#define HASH_ALG_FNV64 2
#define HASH_ALG_MURMUR64 3
#define HASH_ALG_ROLLING32 4
#define HASH_ALG_ROLLING64 5


#define HASH_ALGORITHM HASH_ALG_ROLLING64


#if HASH_ALGORITHM == HASH_ALG_XXHASH
#define HASH_LEN 32
#define DEFAULT_HASH_FN HashAlgs::xxHash

#elif HASH_ALGORITHM == HASH_ALG_FNV32
#define HASH_LEN 32
#define DEFAULT_HASH_FN HashAlgs::fnvHash

#elif HASH_ALGORITHM == HASH_ALG_FNV64
#define HASH_LEN 64
#define DEFAULT_HASH_FN HashAlgs::fnvHash64

#elif HASH_ALGORITHM == HASH_ALG_MURMUR64
#define HASH_LEN 64
#define DEFAULT_HASH_FN HashAlgs::murmurHash64

#elif HASH_ALGORITHM == HASH_ALG_ROLLING32
#define HASH_LEN 32
#define DEFAULT_HASH_FN rollingHash32
#define HASH_ALGORITHM_IS_ROLLING

#elif HASH_ALGORITHM == HASH_ALG_ROLLING64
#define HASH_LEN 64
#define DEFAULT_HASH_FN rollingHash64
#define HASH_ALGORITHM_IS_ROLLING

#else
#error No hash algorithm defined.
#endif


#if HASH_LEN == 32
typedef unsigned hash_t;
#define HASH_PRINTF "%x"
#elif HASH_LEN == 64
typedef u64 hash_t;
#define HASH_PRINTF "%llx"
#elif HASH_LEN == 128
typedef u128 hash_t;
#elif HASH_LEN == 160
typedef u160 hash_t;
#else
#error Bad hash length
#endif

/*
  Performance comparison
  Speed of creating index for 4.9GB byte file, 4kB blocks, 32-bit hash
            MB/sec   # of hash collisions
    FNV:       628     185
    Murmur:   1210     201
    xxHash:   1642     193
 */


// See HASH_ALG_...
int getDefaultHashFunctionId();
const char *getHashFunctionName(int hashFnId);
int getHashFunctionResultBits(int hashFnId);


// Fowler-Noll-Vo-1a algoritm
// http://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
//unsigned fnvHash(const void *data, unsigned len);
//HD u64 fnvHash64(const void *data, unsigned len);
//
//#define FNV_HASH_INIT 2166136261u
//#define FNV_HASH_PERMUTE 16777619
//
//#define FNV_HASH_INIT64 14695981039346656037ULL
//#define FNV_HASH_PERMUTE64 1099511628211ULL

// Murmur3
// http://en.wikipedia.org/wiki/MurmurHash
//unsigned murmurHash   (const void *data, unsigned len);
//u64      murmurHash64 (const void *data, unsigned len);
//u128     murmurHash128(const void *data, unsigned len);

// xxhash
// https://code.google.com/p/xxhash/
//unsigned xxHash(const void *data, unsigned len);
//#define PRIME32_1   2654435761U
//#define PRIME32_2   2246822519U
//#define PRIME32_3   3266489917U
//#define PRIME32_4    668265263U
//#define PRIME32_5    374761393U


// len should be a multiple of 64.  If not, a padded buffer will be allocated.
//unsigned sha1Hash32 (const void *data, unsigned len);
//u64      sha1Hash64 (const void *data, unsigned len);
//u128     sha1Hash128(const void *data, unsigned len);
//u160     sha1Hash   (const void *data, unsigned len);


unsigned rollingHash32(const void *data, unsigned len);
u64      rollingHash64(const void *data, unsigned len);

unsigned cityHash32 (const void *data, unsigned len);
u64      cityHash64 (const void *data, unsigned len);
u128     cityHash128(const void *data, unsigned len);


class HashAlgs {
 public:
  //HD static unsigned fnvHash(const void *data_v, unsigned len) {
  //  const unsigned char *data = (const unsigned char *) data_v;
  //  unsigned hash = FNV_HASH_INIT;
  //  for (unsigned i=0; i < len; i++) {
  //    hash = hash ^ data[i];
  //    hash = hash * FNV_HASH_PERMUTE;
  //  }
  //  return hash;
  //}

  //HD static u64 fnvHash64(const void *data_v, unsigned len) {
  //  const unsigned char *data = (const unsigned char *) data_v;
  //  u64 hash = FNV_HASH_INIT64;
  //  for (unsigned i=0; i < len; i++) {
  //    hash = hash ^ data[i];
  //    hash = hash * FNV_HASH_PERMUTE64;
  //  }
  //  return hash;
  //}

 // HD static unsigned xxHash(const void *data_v, unsigned len) {
 //   const unsigned char* p = (const unsigned char*)data_v;
 //   const unsigned char* const bEnd = p + len;
 //   unsigned int h32;

 //   if (len>=16)
 //     {
	//const unsigned char* const limit = bEnd - 16;
	//unsigned int v1 = PRIME32_1 + PRIME32_2;
	//unsigned int v2 = PRIME32_2;
	//unsigned int v3 = 0;
	//unsigned int v4 = 0 - PRIME32_1;

	//do
	//  {
	//    v1 += XXH_LE32(p) * PRIME32_2; v1 = XXH_rotl32(v1, 13); v1 *= PRIME32_1; p+=4;
	//    v2 += XXH_LE32(p) * PRIME32_2; v2 = XXH_rotl32(v2, 13); v2 *= PRIME32_1; p+=4;
	//    v3 += XXH_LE32(p) * PRIME32_2; v3 = XXH_rotl32(v3, 13); v3 *= PRIME32_1; p+=4;
	//    v4 += XXH_LE32(p) * PRIME32_2; v4 = XXH_rotl32(v4, 13); v4 *= PRIME32_1; p+=4;
	//  } while (p<=limit) ;

	//h32 = XXH_rotl32(v1, 1) + XXH_rotl32(v2, 7) + XXH_rotl32(v3, 12) + XXH_rotl32(v4, 18);
 //     }
 //   else
 //     {
	//h32  = PRIME32_5;
 //     }

 //   h32 += (unsigned int) len;
	//
 //   while (p<=bEnd-4)
 //     {
	//h32 += XXH_LE32(p) * PRIME32_3;
	//h32 = XXH_rotl32(h32, 17) * PRIME32_4 ;
	//p+=4;
 //     }

 //   while (p<bEnd)
 //     {
	//h32 += (*p) * PRIME32_5;
	//h32 = XXH_rotl32(h32, 11) * PRIME32_1 ;
	//p++;
 //     }

 //   h32 ^= h32 >> 15;
 //   h32 *= PRIME32_2;
 //   h32 ^= h32 >> 13;
 //   h32 *= PRIME32_3;
 //   h32 ^= h32 >> 16;

 //   return h32;
 // }


  //HD static u64 murmurHash64(const void *data, unsigned len) {
  //  u128 result;
  //  MurmurHash3_32bit_128(data, len, 0, &result);
  //  return result.lo;
  //}

  //static const unsigned murmurC1 = 0x239b961b; 
  //static const unsigned murmurC2 = 0xab0e9789;
  //static const unsigned murmurC3 = 0x38b34ae5; 
  //static const unsigned murmurC4 = 0xa1e38b93;

  //HD static unsigned rotl32 ( unsigned x, int r ) {
  //  return (x << r) | (x >> (32 - r));
  //}

  //HD static u64 rotl64 ( u64 x, int r ) {
  //  return (x << r) | (x >> (64 - r));
  //}

  //HD static unsigned fmix ( unsigned h ) {
  //  h ^= h >> 16;
  //  h *= 0x85ebca6b;
  //  h ^= h >> 13;
  //  h *= 0xc2b2ae35;
  //  h ^= h >> 16;

  //  return h;
  //}

  //HD static u64 fmix ( u64 k ) {
  //  k ^= k >> 33;
  //  k *= 0xff51afd7ed558ccdULL;
  //  k ^= k >> 33;
  //  k *= 0xc4ceb9fe1a85ec53ULL;
  //  k ^= k >> 33;

  //  return k;
  //}
  
 private:
  //HD static unsigned XXH_LE32(const unsigned char *p) {
  //  return *(unsigned int*)p;
  //}

  //HD static unsigned XXH_rotl32(unsigned x, int r) {
  //  return (x << r) | (x >> (32 - r));
  //}


  HD static unsigned getblock ( const unsigned * p, int i ) {
    return p[i];
  }

  HD static u64 getblock ( const u64 * p, int i ) {
    return p[i];
  }
  
  
 // HD static void MurmurHash3_32bit_128 (const void * key, const int len,
	//				unsigned seed, void *out)
 // {
 //   const unsigned char * data = (const unsigned char*)key;
 //   const int nblocks = len / 16;

 //   unsigned h1 = seed;
 //   unsigned h2 = seed;
 //   unsigned h3 = seed;
 //   unsigned h4 = seed;

 //   const unsigned c1 = 0x239b961b; 
 //   const unsigned c2 = 0xab0e9789;
 //   const unsigned c3 = 0x38b34ae5; 
 //   const unsigned c4 = 0xa1e38b93;

 //   //----------
 //   // body

 //   const unsigned * blocks = (const unsigned *)(data + nblocks*16);

 //   for(int i = -nblocks; i; i++)
 //     {
	//unsigned k1 = getblock(blocks,i*4+0);
	//unsigned k2 = getblock(blocks,i*4+1);
	//unsigned k3 = getblock(blocks,i*4+2);
	//unsigned k4 = getblock(blocks,i*4+3);

	//k1 *= c1; k1  = rotl32(k1,15); k1 *= c2; h1 ^= k1;

	//h1 = rotl32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b;

	//k2 *= c2; k2  = rotl32(k2,16); k2 *= c3; h2 ^= k2;

	//h2 = rotl32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747;

	//k3 *= c3; k3  = rotl32(k3,17); k3 *= c4; h3 ^= k3;

	//h3 = rotl32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35;

	//k4 *= c4; k4  = rotl32(k4,18); k4 *= c1; h4 ^= k4;

	//h4 = rotl32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17;
 //     }

 //   //----------
 //   // tail

 //   const unsigned char * tail = (const unsigned char*)(data + nblocks*16);

 //   unsigned k1 = 0;
 //   unsigned k2 = 0;
 //   unsigned k3 = 0;
 //   unsigned k4 = 0;

 //   switch(len & 15)
 //     {
 //     case 15: k4 ^= tail[14] << 16;
 //     case 14: k4 ^= tail[13] << 8;
 //     case 13: k4 ^= tail[12] << 0;
	//k4 *= c4; k4  = rotl32(k4,18); k4 *= c1; h4 ^= k4;

 //     case 12: k3 ^= tail[11] << 24;
 //     case 11: k3 ^= tail[10] << 16;
 //     case 10: k3 ^= tail[ 9] << 8;
 //     case  9: k3 ^= tail[ 8] << 0;
	//k3 *= c3; k3  = rotl32(k3,17); k3 *= c4; h3 ^= k3;

 //     case  8: k2 ^= tail[ 7] << 24;
 //     case  7: k2 ^= tail[ 6] << 16;
 //     case  6: k2 ^= tail[ 5] << 8;
 //     case  5: k2 ^= tail[ 4] << 0;
	//k2 *= c2; k2  = rotl32(k2,16); k2 *= c3; h2 ^= k2;

 //     case  4: k1 ^= tail[ 3] << 24;
 //     case  3: k1 ^= tail[ 2] << 16;
 //     case  2: k1 ^= tail[ 1] << 8;
 //     case  1: k1 ^= tail[ 0] << 0;
	//k1 *= c1; k1  = rotl32(k1,15); k1 *= c2; h1 ^= k1;
 //     };

 //   //----------
 //   // finalization

 //   h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;

 //   h1 += h2; h1 += h3; h1 += h4;
 //   h2 += h1; h3 += h1; h4 += h1;

 //   h1 = fmix(h1);
 //   h2 = fmix(h2);
 //   h3 = fmix(h3);
 //   h4 = fmix(h4);

 //   h1 += h2; h1 += h3; h1 += h4;
 //   h2 += h1; h3 += h1; h4 += h1;

 //   ((unsigned*)out)[0] = h1;
 //   ((unsigned*)out)[1] = h2;
 //   ((unsigned*)out)[2] = h3;
 //   ((unsigned*)out)[3] = h4;
 // }
};


// Use FNV algorithm to accumulate data into a hash value.
//class AccumulatingHash {
//  unsigned state;
//
// public:
//  AccumulatingHash() {reset();}
//
//  unsigned getHash() {return state;}
//  void reset() {state = FNV_HASH_INIT;}
//  
//  unsigned addByte(unsigned char c) {
//    state = state ^ c;
//    state = state * FNV_HASH_PERMUTE;
//    return state;
//  }
//
//  unsigned addBytes(const void *data_v, unsigned len) {
//    const unsigned char *data = (const unsigned char *)data_v;
//    for (unsigned i=0; i < len; i++) {
//      state = state ^ data[i];
//      state = state * FNV_HASH_PERMUTE;
//    }
//    return state;
//  }
//};


//class AccumulatingHashRolling {
//  RollingHash<unsigned> hasher;
//
// public:
//  AccumulatingHashRolling() {reset();}
//
//  unsigned getHash() {return hasher.getHash();}
//  void reset() {hasher.reset();}
//  
//  unsigned addByte(unsigned char c) {return hasher.addChar(c);}
//
//  unsigned addBytes(const void *data_v, unsigned len) {
//    return hasher.addChars((const unsigned char*)data_v, len);
//  }
//};

// Use rolling hash algorithm to slide window data into a hash value.
typedef RollingHash<unsigned> SlidingWindowHash;
/*
class SlidingWindowHash {
  unsigned state;
  RollingHash<unsigned> hash;

 public:
  SlidingWindowHash() {hash.reset();}

  unsigned getHash() {return hash.getHash();}
  void reset() {hash.reset();}
  
  unsigned addByte(unsigned char c) {
    return hash.addChar(c);
	//return state;
  }
  
  unsigned moveByte(unsigned char addc,
	  unsigned char removec ) {
    return hash.moveChar(addc, removec);
	//return state;
  }

  unsigned addBytes(const void *data_v, unsigned len) {
    const unsigned char *data = (const unsigned char *)data_v;
    for (unsigned i=0; i < len; i++) {
      hash.addChar(data[i]);
    }
    return state;
  }
};
*/


/* Given a memory-mapped file, provide hashes of blocks. */
class MappedFileHash {

 protected:
  const unsigned char * const data;
  const u64 length;
  const unsigned blockSize;

 public:
  MappedFileHash(const unsigned char *data_,
		  u64 length_,
		  unsigned blockSize_)
     : data(data_), length(length_), blockSize(blockSize_) {}
  virtual ~MappedFileHash() {}

  // returns a pointer to the data
  const unsigned char *getData() {return data;}

  // returns the length of the file
  u64 getLength() {return length;}

  // returns the hash of bytes [offset..offset+blockSize)
  virtual hash_t getHash(u64 offset) = 0;
};

class MappedFileHashRolling : public MappedFileHash {
  RollingHash<hash_t> rollingHash;
  i64 prevOffset;

 public:
  MappedFileHashRolling(const unsigned char *data_,
			u64 length_,
			unsigned blockSize_)
    : MappedFileHash(data_, length_, blockSize_) {
    prevOffset = I64_MIN;
  }

  // results undefined if offset+blockSize >= length
  hash_t getHash(u64 offset) {
    // if (offset >= length) return 0;

    // best case: move one char
    if ((i64)offset == prevOffset+1) {
      rollingHash.moveChar(data[prevOffset+blockSize], data[prevOffset]);
      prevOffset = (i64)offset;
      return rollingHash.getHash();
    }

    // otherwise rebuild the rolling hash window
    rollingHash.reset();
    for (unsigned i=0; i < blockSize; i++)
      rollingHash.addChar(data[offset+i]);
    
    prevOffset = (i64)offset;
    return rollingHash.getHash();
  }
};
    



#endif // __HASH_ALGS_H__
