#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
// #include <unistd.h>
#include "dedup-table.h"
#include "dedup-util.h"
#include "HashAlgs.h"

// number of bytes in the header for a serialized DedupFile
#define HEADER_SIZE 64

#define CREATE_BUF_SIZE (1024*1024)
#define CREATE_STATUS_INTERVAL 100000000
#define EMPTY_INDEX_SIZE 128

// allocate memory for 'size' entries, complaining if the malloc fails
void DedupTable::allocateEntries(int size) {
  entries = new Entry[size];
  if (!entries) {
    fprintf(stderr, "Failed to allocate %llu bytes for hash table\n",
	    (u64)(size * sizeof(Entry)));
    exit(1);
  }
  memset(entries, 0, size * sizeof(Entry));
}


// Increase the capacity and move all the existing entries to the new table.
void DedupTable::grow(unsigned minimumNewSize) {

  if (allocatedSize == 0x80000000) {
    fprintf(stderr, "FAIL: Hash table is at maximum size, "
	    "%u of %u entries filled.\n",
	    entryCount, allocatedSize);
    exit(1);
  }

  if (minimumNewSize <= allocatedSize) return;

  unsigned oldSize = allocatedSize;

  // save pointers to the beginning and end of the old data array
  Entry *oldEntries = entries;

  // double the size and allocate a new array
  allocatedSize = roundUpSize(minimumNewSize);
  maxCount = (int)(maxLoadFactor * allocatedSize);
  allocateEntries(allocatedSize);

  /*
  double startTime = timeInSeconds();
  printf("Growing from %u to %u...", oldSize, allocatedSize);
  fflush(stdout);
  */

  // move all the old entries to the new array
  unsigned entriesFound = 0;
  int sizeMask = allocatedSize-1;
  for (unsigned i=0; i < oldSize; i++) {
    if (oldEntries[i].hashValue != 0) {
      entriesFound++;

      // find an empty slot
      int newPos = oldEntries[i].hashValue & sizeMask;
      while (entries[newPos].hashValue != 0)
	newPos = (newPos+1) & sizeMask;

      entries[newPos] = oldEntries[i];
    }
  }
  assert(entryCount == entriesFound);
  // fprintf(stderr, "%d of %d entries rehashed\n", entriesFound, size());

  /*
  printf("done. (%.3f sec)\n", timeInSeconds() - startTime);
  fflush(stdout);
  */

  delete[] oldEntries;
}

// Add an entry that has already been hashed.
// This method doesn't check if the table has exceeded maxLoadFactor.
void DedupTable::addHashedEntry(hash_t hashValue, unsigned blockNo) {

  // assuming allocatedSize is a power of 2, this is equivalent to:
  //   entryNo = hashValue % allocatedSize
  int sizeMask = allocatedSize-1;
  int entryNo = hashValue & sizeMask;

  blockHashes.push_back(hashValue);

  // linear probing
  while (entries[entryNo].hashValue != 0) {

    if (entries[entryNo].hashValue == hashValue) {
      // add another block for this hash value
      if (entries[entryNo].count == 1) {
	DuplicateBlocks *db = new DuplicateBlocks;
	db->hashValue = hashValue;
	db->blocks.push_back(entries[entryNo].blockNo);
	db->blocks.push_back(blockNo);
	entries[entryNo].blockNo = duplicateBlocks.size();
	duplicateBlocks.push_back(db);
      } else {
	duplicateBlocks[entries[entryNo].blockNo]->blocks.push_back(blockNo);
      }
      entries[entryNo].count++;
      return;
    }

    entryNo = (entryNo+1) & sizeMask;
  }

  entries[entryNo].hashValue = hashValue;
  entries[entryNo].blockNo = blockNo;
  entries[entryNo].count = 1;
  entryCount++;
}


// round up a value for allocatedSize to the next higher power of 2
unsigned DedupTable::roundUpSize(unsigned n) {
  if (n >= 0x80000000) return 0x80000000;
  // from "Hacker's Delight", 2nd ed., section 3.2
  n--;
  n = n | (n >> 1);
  n = n | (n >> 2);
  n = n | (n >> 4);
  n = n | (n >> 8);
  n = n | (n >> 16);
  return n + 1;
}


/*
  If capacityIsAllocationSize is true, then initialCapacity is the value
  to use for allocatedSize.  Otherwise, it is the expected number of entries,
  or entryCount.
*/
DedupTable::DedupTable(unsigned blockSize_,
		       bool capacityIsAllocationSize,
		       unsigned initialCapacity,
		       float maxLoadFactor_) {
  blockSize = blockSize_;
  maxLoadFactor = maxLoadFactor_;

  if (capacityIsAllocationSize) {
    allocatedSize = initialCapacity;
    // check that allocatedSize is a power of 2
    assert((allocatedSize & (allocatedSize-1)) == 0);
  } else {
    // add a fudge factor of 10 just to insure a roundoff error doesn't cause an
    // expensive resize operation just before the end
    allocatedSize = roundUpSize
      ((unsigned)(initialCapacity / maxLoadFactor) + 10);
    blockHashes.reserve(initialCapacity);
  }

  allocateEntries(allocatedSize);
  entryCount = 0;
  maxCount = (int)(maxLoadFactor * allocatedSize);

  probeCalls = probeIters = 0;

  // simple sanity checks
  assert(maxLoadFactor > 0);
  assert(maxLoadFactor < 1);
  assert(allocatedSize > 1);
  assert(maxCount < allocatedSize);
  assert(maxCount > 0);
}


DedupTable::~DedupTable() {
#ifdef PROFILE_PROBING
  if (probeCalls)
    printf("%llu calls to probeTable(), average iters = %.3f\n",
	   probeCalls, (double)probeIters/probeCalls);
#endif
  delete[] entries;
  for (unsigned i=0; i<duplicateBlocks.size(); i++)
    delete duplicateBlocks[i];
}


void DedupTable::addBlock(const char *data) {
  if (entryCount >= maxCount) grow(allocatedSize*2);

  hash_t hashValue = hash(data, blockSize);

  addHashedEntry(hashValue, size());
}


// If a block with a matching hash value was found, set *blockNo to
// the block index and return true.  Otherwise return false.
bool DedupTable::findFirstMatch(const char *data, unsigned *blockNo) {
  hash_t hashValue = hash(data, blockSize);
  return findHashedFirstMatch(hashValue, blockNo);
}


bool DedupTable::findHashedFirstMatch(hash_t hashValue,
				      unsigned *blockNo) {
  Entry *entry = probeTable(hashValue);
  if (entry) {
    if (entry->count == 1) {
      *blockNo = entry->blockNo;
    } else {
      *blockNo = duplicateBlocks[entry->blockNo]->blocks[0];
    }
    return true;
  } else {
    return false;
  }
}


// If any matching blocks are found, store them in blockNos and return
// true.  Otherwise return false.
bool DedupTable::findAllMatches(const char *data,
				std::vector<unsigned> &blockNos,
				unsigned maxMatchCount) {
  hash_t hashValue = hash(data, blockSize);
  return findHashedAllMatches(hashValue, blockNos, maxMatchCount);
}


bool DedupTable::findHashedAllMatches(hash_t hashValue,
				      std::vector<unsigned> &blockNos,
				      unsigned maxMatchCount) {
  Entry *entry = probeTable(hashValue);
  blockNos.clear();
  if (entry) {
    if (entry->count == 1) {
      blockNos.push_back(entry->blockNo);
    } else {
      DuplicateBlocks *d = duplicateBlocks[entry->blockNo];
      unsigned n = d->blocks.size();
      if (maxMatchCount < n) n = maxMatchCount;
      for (unsigned i=0; i < n; i++) {
	blockNos.push_back(d->blocks[i]);
      }
    }
    return true;
  } else {
    return false;
  }
}


// Returns true iff the given block number is found somewhere
// in the list of blocks with this hash value.
bool DedupTable::findHashedMatch(hash_t hashValue, unsigned blockNo) {
  Entry *entry = probeTable(hashValue);
  if (!entry) return false;

  if (entry->count == 1) 
    return entry->blockNo == blockNo;


  DuplicateBlocks *d = duplicateBlocks[entry->blockNo];
  
  // binary search
  if (blockNo < d->blocks[0]) return false;
  
  unsigned lo = 0, hi = d->blocks.size()-1;
  while (lo < hi) {
    unsigned mid = (lo + hi + 1) / 2;
    if (blockNo < d->blocks[mid])
      hi = mid - 1;
    else
      lo = mid;
  }
  return d->blocks[lo] == blockNo;
}


u64 DedupTable::writeToFile(const char *filename, bool verbose) {
  char header[HEADER_SIZE] = {0};

  FILE *outf = fopen(filename, "wb");
  if (!outf) {
    fprintf(stderr, "Failed to open \"%s\" for writing: %s\n",
	    filename, strerror(errno));
    return false;
  }

  // build the header
  strcpy(header, "ddup");
  *(int*)(header+4) = 0;  // version number
  *(unsigned*)(header+8) = blockSize;
  *(unsigned*)(header+12) = allocatedSize;
  *(unsigned*)(header+16) = entryCount;
  *(unsigned*)(header+20) = blockHashes.size();
  *(unsigned char*)(header+24) = HASH_ALGORITHM;  // primary hash algorithm
  *(unsigned char*)(header+25) = 0;  // secondary hash algorithm

  // write the header
  fwrite(header, HEADER_SIZE, 1, outf);

  char buf[27];

  // write the entries array
  if (verbose) {
    printf("Writing %s entries...\n", commafy(buf, allocatedSize));
    fflush(stdout);
  }
  fwrite(entries, allocatedSize, sizeof(Entry), outf);

  // write the block hashes
  if (verbose) {
    printf("Writing %s hash values...\n",
	   commafy(buf, (unsigned)blockHashes.size()));
    fflush(stdout);
  }
  blockHashes.writeEntries(outf);

  // write the duplicateBlocks entries
  if (verbose) {
    printf("Writing %s duplicate block sequences...\n",
	   commafy(buf, (unsigned)duplicateBlocks.size()));
    fflush(stdout);
  }
  u64 dupBlockSize = 0;
  unsigned dupBlockCount = duplicateBlocks.size();
  dupBlockSize += fwrite(&dupBlockCount, 1, sizeof(unsigned), outf);
  for (unsigned dupNo=0; dupNo < dupBlockCount; dupNo++) {
    DuplicateBlocks *dup = duplicateBlocks[dupNo];
    dupBlockSize += fwrite(&dup->hashValue, 1, sizeof(hash_t), outf);
    unsigned len = dup->blocks.size();
    dupBlockSize += fwrite(&len, 1, sizeof(unsigned), outf);
    dupBlockSize += dup->blocks.writeEntries(outf);
  }

  fclose(outf);
  return HEADER_SIZE +
    (u64)allocatedSize * sizeof(Entry) +
    (u64)size() * sizeof(hash_t) +
    dupBlockSize;
}


DedupTable *DedupTable::createEmpty(unsigned blockSize) {
  return new DedupTable(blockSize, true, EMPTY_INDEX_SIZE);
}


// read a regular file and create a DedupTable index of it
DedupTable *DedupTable::createFromFile(const char *filename,
				       unsigned blockSize) {
  FILE *inf = fopen(filename, "rb");
  if (!inf) {
    fprintf(stderr, "Failed to open \"%s\" for reading: %s\n",
	    filename, strerror(errno));
    return NULL;
  }

  DedupTable *table = createFromFile(inf, blockSize);
  fclose(inf);
  return table;
}


DedupTable *DedupTable::createFromFile(FILE *inf,
				       unsigned blockSize) {
  u64 fileSize = getFileSize(inf);
  if ((fileSize >> 32) > blockSize) {
    fprintf(stderr, "File too large (%llu bytes) for given block size (%u)\n",
	    fileSize, blockSize);
    return NULL;
  }

  // tailor the size for smaller tables
  unsigned nBlocks = (unsigned)((fileSize + blockSize - 1)  / blockSize);
  DedupTable *table;
  if (nBlocks < 100) {
    table = new DedupTable(blockSize, false, nBlocks/4);
  } else {
    table = new DedupTable(blockSize, true, 128);
  }
  if (!table) return NULL;

  size_t bufSize;
  int bufReps;
  if (blockSize > CREATE_BUF_SIZE) {
    bufSize = blockSize;
    bufReps = 1;
  } else {
    bufReps = CREATE_BUF_SIZE / blockSize;
    bufSize = blockSize * bufReps;
  }
  char *buf = (char*) malloc(bufSize);
  assert(buf);

  u64 nextStatusUpdate = CREATE_STATUS_INTERVAL;
  u64 totalBytesRead = 0;

  bool done = false;
  while (!done) {
    size_t bytesRead = fread(buf, 1, bufSize, inf);
    if (bytesRead == 0) break;

    totalBytesRead += bytesRead;
    if (totalBytesRead > nextStatusUpdate) {
      nextStatusUpdate = totalBytesRead + CREATE_STATUS_INTERVAL;
      char buf[27];
      printf("  %s bytes read...\r", commafy(buf, totalBytesRead));
      fflush(stdout);
    }

    // if the block wasn't fully read, fill the rest of the buffer with zeros
    if (bytesRead < bufSize) {
      memset(buf+bytesRead, 0, bufSize-bytesRead);
      done = true;
    }

    for (size_t offset = 0; offset < bytesRead; offset += blockSize) {
      table->addBlock(buf+offset);
    }
  }
  free(buf);

  return table;
}


DedupTable *DedupTable::readFromFile(const char *filename, bool verbose) {
  FILE *inf = fopen(filename, "rb");
  if (!inf) {
    fprintf(stderr, "Cannot read \"%s\"\n", filename);
    return NULL;
  }

  DedupTable *table = NULL;
  char header[HEADER_SIZE];
  size_t readLen;
  
  readLen = fread(header, 1, HEADER_SIZE, inf);
  if (readLen != HEADER_SIZE) goto fail;

  if (strncmp(header, "ddup", 4)) goto fail;

  unsigned versionNo, blockSize, allocatedSize, entryCount, 
    hashAlg1No, hashAlg2No, blockCount;
  
  versionNo =     *(unsigned*)(header+4);
  blockSize =     *(unsigned*)(header+8);
  allocatedSize = *(unsigned*)(header+12);
  entryCount =    *(unsigned*)(header+16);
  blockCount =    *(unsigned*)(header+20);
  hashAlg1No =    *(unsigned char*)(header+24);
  hashAlg2No =    *(unsigned char*)(header+25);

  if (hashAlg1No != HASH_ALGORITHM) {
    fprintf(stderr, "Error: \"%s\" built with %s algorithm, \n"
	    "  but this code was compiled with %s algorithm.\n", filename,
	    getHashFunctionName(hashAlg1No),
	    getHashFunctionName(getDefaultHashFunctionId()));
    return NULL;
  }

  if (verbose) {
    printf("Reading %u entries, %.2f %% filled\n",
	   allocatedSize, 100.0*entryCount / allocatedSize);
    fflush(stdout);
  }

  // sanity check
  if (entryCount > allocatedSize) goto fail;

  // multiple versions not supported yet
  if (versionNo != 0 || hashAlg2No != 0) goto fail;

  table = new DedupTable(blockSize, true, allocatedSize);
  table->entryCount = entryCount;
  readLen = fread(table->entries, sizeof(Entry), allocatedSize, inf);
  if (readLen != allocatedSize) goto fail;

  if (verbose) {
    printf("Reading %u hash values\n", blockCount);
    fflush(stdout);
  }
  if (table->blockHashes.readEntries(inf, blockCount) != blockCount)
    goto fail;

  // read the duplicateBlocks entries
  unsigned dupBlockCount;
  if (sizeof(unsigned) != fread(&dupBlockCount, 1, sizeof(unsigned), inf))
    goto fail;
  if (verbose) {
    printf("Reading %u duplicate block chains\n", dupBlockCount);
    fflush(stdout);
  }
  for (unsigned dupBlockNo=0; dupBlockNo < dupBlockCount; dupBlockNo++) {
    DuplicateBlocks *dup = new DuplicateBlocks;
    if (sizeof(hash_t) != fread(&dup->hashValue, 1, sizeof(hash_t), inf))
      goto fail;
    unsigned numBlocks;
    if (sizeof(unsigned) != fread(&numBlocks, 1, sizeof(unsigned), inf))
      goto fail;
    if (numBlocks != dup->blocks.readEntries(inf, numBlocks))
      goto fail;
    table->duplicateBlocks.push_back(dup);
  }

  fclose(inf);
  return table;

 fail:
  fclose(inf);
  if (table) delete table;
  fprintf(stderr, "Format error reading \"%s\".\n", filename);
  return NULL;
}


void DedupTable::printStats() {
  char buf1[27], buf2[27];
  printf("Table: %s %u-byte blocks\n", commafy(buf1, size()), blockSize);
  printf("  %s of %s entries filled (%.2f %%)\n",
	 commafy(buf2, entryCount), commafy(buf1, allocatedSize),
	 100.0f*entryCount/allocatedSize);
  printf("  %s duplicate blocks\n", commafy(buf1, (u64)duplicateBlocks.size()));

  hash_t mostCommonHashValue = 0;
  unsigned mostCommonCount = 0;
  unsigned commonBlockNo = 0;
  unsigned nDups = duplicateBlocks.size();
  for (unsigned i=0; i < nDups; i++) {
    DuplicateBlocks *d = duplicateBlocks[i];
    if (d->blocks.size() > mostCommonCount) {
      mostCommonCount = d->blocks.size();
      mostCommonHashValue = d->hashValue;
      commonBlockNo = d->blocks[0];
    }
  }
  printf("  Most common hash value: %llx (%u blocks, first at offset %llu)\n",
	 (u64)mostCommonHashValue, mostCommonCount, 
	 (u64)commonBlockNo * blockSize);
}
