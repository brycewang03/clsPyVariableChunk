#ifndef __DEDUP_UTIL_H__
#define __DEDUP_UTIL_H__

#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

#include "u64.h"
#include <cmath>
#include <iostream>

double timeInSeconds();

bool fileExists(const char *name);

u64 getFileSize(FILE *handle);
u64 getFileSize(const char *name);

const char *commafy(char buf[14], unsigned x);
#if !defined(__CYGWIN__) && !defined(_WIN32)
const char *commafy(char buf[27], size_t x);
#endif
const char *commafy(char buf[27], u64 x);

class MemoryMappedFile {
  u64 length;
  void *address;

#ifndef _WIN32
  int fileDescriptor;
#else
  HANDLE fileHandle, mappingHandle;
#endif

  const void *mapFileInternal(const char *filename);
  
 public:
  MemoryMappedFile();
  ~MemoryMappedFile();

  // Maps an entire file into memory and return a pointer to the
  // beginning of the mapping.  On error, prints error message and
  // returns NULL.  If a mapping is already open, it is closed first.
  // const void *mapFile(FILE *inf, bool closeFileOnDestroy);
  const void *mapFile(const char *filename);

  // close a mapping.
  void close();

  // Return a pointer to the beginning of the mapping, or NULL if
  // no mapping has been made yet.
  const void *getAddress() {return address;}

  // get length of the mapping
  u64 getLength() {return length;}

};

#endif // __DEDUP_UTIL_H__
