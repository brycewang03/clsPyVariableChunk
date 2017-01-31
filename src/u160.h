#ifndef __U160_H__
#define __U160_H__

#include <cctype>
#include "stdio.h"

struct u160 {
  unsigned data[5]; // 20 bytes 160 total bits

  // return the data as an array of 5 unsigned integers
  // unsigned* intArray() {return (unsigned*)data;}

  static bool isHexByte(const char *p) {
    return isxdigit(p[0]) && isxdigit(p[1]);
  }

  static unsigned toHexDigit(char c) {
    if (isdigit(c))
      return c - '0';
    else
      return tolower(c)-'a'+10;
  }

  // big-endian
  static unsigned char toHexByte(const char *p) {
    return (toHexDigit(p[0]) << 4) | toHexDigit(p[1]);
  }

  // each byte is big-endian, but the sequence of bytes is little-endian
  bool fromHex(const char *p) {
    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    for (int i=0; i < 5; i++) {
      unsigned shift = 0;
      for (int j=0; j < 4; j++) {
	if (!isHexByte(p)) return true;
	data[i] |= toHexByte(p) << shift;
	p += 2;
	shift += 8;
      }
    }	
    return true;
  }
  

  static unsigned reverseWord(unsigned x) {
    return (x<<24) | 
      ((x & 0xff00) << 8) |
      ((x & 0xff0000) >> 8) |
      (x >> 24);
  }

  const char *toHex(char buf[41]) const {
    for (int i=0; i < 5; i++) {
      // sprintf(buf+i*8, "%08x", data[4-i]);
#ifdef _WIN32
      //sprintf_s(buf+i*8, 9, "%08x", reverseWord(data[i]));
#else
      sprintf(buf+i*8, "%08x", reverseWord(data[i]));
#endif
    }
    return buf;
  }

  size_t hash() const {
    return (size_t)7*data[0]
      ^13*data[1]
      ^17*data[2]
      ^37*data[3]
      ^101*data[4];
  }

  bool operator < (const u160 &other) const {
    for (int i=4; i >= 0; i--) {
      if (data[i] != other.data[i]) return data[i] < other.data[i];
    }
    return false;
  }

};

/*
namespace std {
  template<>
  class hash<u160> {
  public:
    size_t operator()(const u160 &x) const {
      return x.hash();
    }
  };
}
*/
class Hash_u160 {
 public:
  size_t operator()(const u160 &x) const {
    return x.hash();
  }
};

#endif // __U160_H__
