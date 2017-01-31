#ifndef __ROLLING_HASH_H__
#define __ROLLING_HASH_H__

#include <cassert>
#include <cstring>

template <class T>
class RollingHash {
  T state, multiplier, factor;
  int size;

  // according to http://blog.teamleadnet.com/2012/10/rabin-karp-rolling-hash-dynamic-sized.html
  // 69069 is a good hash multiplier.

 public:
  static const T defaultMultiplier = 69069;

  RollingHash(T multiplier_ = defaultMultiplier) {
    state = 0;
    size = 0;
    multiplier = multiplier_;
    factor = 1;
  }

  // Add a byte to the rolling hash window.
  // Returns the resulting hash value.
  T addChar(unsigned char c) {
    size++;
    factor = factor * multiplier;
    state = state * multiplier + c;
    // printf("add %d, hash = %u\n", c, state);
    return state;
  }

  T addChars(const unsigned char *p, unsigned len) {
    for (unsigned i=0; i < len; i++)
      addChar(p[i]);
    return state;
  }

  // Add a byte to and remove a byte from the rolling hash window.
  // Returns the resulting hash value.
  T moveChar(unsigned char addc,
	     unsigned char removec) {
	//Add a byte
    state = state * multiplier + addc;
    //Remove a byte
    state -= factor * removec;
    // printf("move %d to %d, hash = %u\n", removec, addc, state);
    return state;
  }

  // Returns the current hash value.
  T getHash() {
    return state;
  }

  void reset() {
    state = 0;
    size = 0;
    factor = 1;
  }
};


/*
  Computes a rolling hash, but maintains an internal copy of the data
  in the window, so the caller need not worry about the byte to be removed.
*/
template <class T>
class RollingHashBuffered {
  T state;
  int size;
  unsigned multiplier, factor;

  unsigned char *buffer;
  int bufferPos;  // position of the next place to add a value

  // return (a^n)%(2^32)
  // yes, this can be optimized
  unsigned intPow(unsigned a, unsigned n) {
    unsigned f = 1;
    for (unsigned i=0; i < n; i++) f *= a;
    return f;
  }

 public:
  RollingHashBuffered(int windowLength,
		      unsigned multiplier_ = RollingHash<unsigned>::defaultMultiplier) {
    state = 0;
    size = windowLength;
    multiplier = multiplier_;
    factor = intPow(multiplier, size-1);

    buffer = new unsigned char[size];
    assert(buffer);
    memset(buffer, 0, size);
    bufferPos = 0;
  }


  ~RollingHashBuffered() {
    delete[] buffer;
  }


  // Add a byte to the rolling hash window.  The byte at the trailing
  // end of the window will be automatically removed.
  // Returns the resulting hash value.
  T addChar(unsigned char c) {
    state -= factor * buffer[bufferPos];
    state = state * multiplier + c;
    buffer[bufferPos] = c;
    bufferPos = (bufferPos+1) % size;

    // printf("move %d to %d, hash = %u\n", removec, addc, state);
    return state;
  }

  // Returns the current hash value.
  T getHash() {
    return state;
  }

  void reset() {
    state = 0;
  }
};

#endif // __ROLLING_HASH_H__
