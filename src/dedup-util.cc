#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif


#ifndef _WIN32
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "dedup-util.h"

#ifdef _WIN32
#define REPORT_ERROR(context, isFatal) \
  reportError(__FILE__, __LINE__, context, isFatal);

void reportError(const char *filename, int lineNo,
		 const char *context, bool die=false) {
  LPVOID message_buffer;
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, GetLastError(), 0, (LPTSTR) &message_buffer, 0, NULL);
  printf("%s:%d Error %d on %s: %s\n", filename, lineNo,
	 GetLastError(), context, message_buffer);
  LocalFree(message_buffer);
  if (die) ExitProcess(1);
}
#endif


double timeInSeconds() {
#ifdef _WIN32
  LARGE_INTEGER counter, freq;
  if (!QueryPerformanceFrequency(&freq))
    REPORT_ERROR("High-resolution performance counter not supported.", true);
  if (!QueryPerformanceCounter(&counter))
    REPORT_ERROR("calling QueryPerformanceCounter", true);

  return counter.QuadPart / (double) freq.QuadPart;
#else
	struct timespec t;
	#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
		clock_serv_t cclock;
		mach_timespec_t mts;
		host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
		clock_get_time(cclock, &mts);
		mach_port_deallocate(mach_task_self(), cclock);
		t.tv_sec = mts.tv_sec;
		t.tv_nsec = mts.tv_nsec;
	#else
		if (clock_gettime(CLOCK_MONOTONIC, &t)) {
		fprintf(stderr, "Error calling clock_gettime: %s\n", strerror(errno));
		exit(1);
		}
	#endif
  return t.tv_sec + 1e-9 * t.tv_nsec;
#endif // _WIN32
}

bool fileExists(const char *name) {
  struct stat stats;
  // check that the file exists
  if (stat(name, &stats)) return false;
  // check that it's not a directory
  // if (S_ISDIR(stats.st_mode)) return false;
  if (stats.st_mode & S_IFDIR) return false;

  /* _WIN32
  struct __stat64 stats;
  if (_stat64(name, &stats)) return false;
  if (stats.st_mode & _S_IFDIR) return false;
  */

  return true;
}


u64 getFileSize(FILE *f) {
  struct stat stats;
  int fd;
#ifdef _WIN32
  fd = _fileno(f);
#else
  fd = fileno(f);
#endif
  if (fstat(fd, &stats)) {
    fprintf(stderr, "Error getting size of input file: %s\n", strerror(errno));
    return 0;
  } else {
    return (u64) stats.st_size;
  }
}

u64 getFileSize(const char *filename) {
#ifdef _WIN32
  struct __stat64 stats;
#else
   struct stat stats;
#endif

   if (
#ifdef _WIN32
     _stat64
#else
     stat
#endif
     (filename, &stats)) {
    fprintf(stderr, "Error getting size of \"%s\": %s\n",
	    filename, strerror(errno));
    return 0;
  } else {
    return (u64) stats.st_size;
  }
}


static void reverseString(char *head, int len) {
  char *tail = head+len-1;
  while (head < tail) {
    char c = *head;
    *head = *tail;
    *tail = c;
    head++;
    tail--;
  }
}
  

const char *commafy(char buf[14], unsigned x) {
  if (x == 0) {
    strcpy(buf, "0");
  } else {
    int len = 0;
    while (x) {
      buf[len++] = '0' + (x%10);
      x /= 10;
      if ((len+1) % 4 == 0 && x) buf[len++] = ',';
    }
    reverseString(buf, len);
    buf[len] = '\0';
  }
  return buf;
}

#if !defined(__CYGWIN__) && !defined(_WIN32)
const char *commafy(char buf[27], size_t x) {
  return commafy(buf, (u64)x);
}
#endif

const char *commafy(char buf[27], u64 x) {
  if (x == 0) {
    strcpy(buf, "0");
  } else {
    int len = 0;
    while (x) {
      buf[len++] = '0' + (x%10);
      x /= 10;
      if ((len+1) % 4 == 0 && x) buf[len++] = ',';
    }
    reverseString(buf, len);
    buf[len] = '\0';
  }
  return buf;
}


MemoryMappedFile::MemoryMappedFile() {
#ifdef _WIN32
  fileHandle = mappingHandle = 0;
#else
  fileDescriptor = 0;
#endif
  length = 0;
  address = NULL;
}

MemoryMappedFile::~MemoryMappedFile() {
  if (address) this->close();
}


#ifndef _WIN32

const void *MemoryMappedFile::mapFile(const char *filename) {
  if (address) this->close();

  length = getFileSize(filename);
  if (length == 0) return NULL;

  fileDescriptor = open(filename, O_RDONLY);
  if (fileDescriptor == -1) {
    fprintf(stderr, "Cannot open \"%s\": %s\n", filename, strerror(errno));
    return NULL;
  }

  if (sizeof(size_t) == 4 && length > (1ull<<32)) {
    fprintf(stderr, "Failed to mmap %s: too large for 32-bit code.\n",
	    filename);
    return NULL;
  }

  address = (char*) mmap(NULL, length, PROT_READ, MAP_SHARED,
			 fileDescriptor, 0);
  if (address == MAP_FAILED) {
    fprintf(stderr, "Failed to mmap %s: %s\n", filename, strerror(errno));
    address = NULL;
    return NULL;
  }
  return address;
}


void MemoryMappedFile::close() {
  if (address) {
    munmap(address, length);
    ::close(fileDescriptor);
  }
  fileDescriptor = 0;
  length = 0;
  address = NULL;
}

#else  // _WIN32

const void *MemoryMappedFile::mapFile(const char *filename) {
  if (address) this->close();

  length = getFileSize(filename);
  if (length == 0) return NULL;
  if ((sizeof(SIZE_T) < 8 || sizeof(void*) < 8)
      && length > 0xFFFFFFFF) {
    fprintf(stderr, "Files larger than 4GB not supported: %s (%llu bytes)",
      filename, length);
    return NULL;
  }

  // open the file
  fileHandle = CreateFile(filename, GENERIC_READ,
			  FILE_SHARE_WRITE | FILE_SHARE_READ,
			  NULL, OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL, 0);
  if (fileHandle == INVALID_HANDLE_VALUE) {
    REPORT_ERROR("opening file for memory mapping", false);
    return NULL;
  }

  // set it up for mapping
  mappingHandle = CreateFileMapping(fileHandle, NULL, PAGE_READONLY,
				    0, 0, NULL);
  if (mappingHandle == NULL) {
    REPORT_ERROR("memory-mapping file", false);
    CloseHandle(fileHandle);
    return NULL;
  }

  // map the whole file to an address
  address = MapViewOfFile(mappingHandle, FILE_MAP_READ, 0, 0, (SIZE_T) length);
  if (address == NULL) {
    REPORT_ERROR("getting mmap address", false);
    CloseHandle(mappingHandle);
    CloseHandle(fileHandle);
    return NULL;
  }

  return address;
}


void MemoryMappedFile::close() {
  if (address) {
    UnmapViewOfFile(address);
    CloseHandle(mappingHandle);
    CloseHandle(fileHandle);
  }
  length = 0;
  address = NULL;
}

#endif  // _WIN32
