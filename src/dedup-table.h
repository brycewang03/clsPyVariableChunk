#ifndef __DEDUP_TABLE__
#define __DEDUP_TABLE__

#include <vector>
#include <climits>
#include "u64.h"
#include "HashAlgs.h"
#include "serializable_vector.h"


// must be a power of 2
#define DEDUP_TABLE_INITIAL_SIZE 4
#define DEDUP_TABLE_MAX_LOAD_FACTOR .5f

// #define PROFILE_PROBING


/*
  This provides a foward and reverse index between the hash value of a block
  of data and the index of the block.

  In other words, one the indexes are built for a file, there are two forms
  of lookups:
    1) Return the hash value for a given block index.
    2) Return the indices of the blocks with a given hash value.

  Capacity requirements:
    This should be able to handle a file of at least a terbyte.
      File offsets: 64 bits
      block ids: can be 32 bits as long as the fileSize/blockSize < 4G
        so for a terabyte file, the blocks must be at least 256 bytes
*/
class DedupTable {

 public:
  struct Entry {
    // cannot be zero.  If the hash algorithm returns 0, use 0xFFFFFFFF instead.
    hash_t hashValue;

    // number of blocks with this hash value
    unsigned count;

    // If count == 1, then this is the index of the block with this hash value
    // If count > 1, then this is the index into repeatedBlocks of the
    //   array of indices of blocks with this hash value
    unsigned blockNo;
  };

 private:
  struct DuplicateBlocks {
    hash_t hashValue;
    serializable_vector<unsigned> blocks;
  };
  

  // Closed-addressing table of Entry structures.  Any hash value will
  // appear only once.
  Entry *entries;

  // blockHashes[blockNo] is the hash of block <blockNo>, or bytes
  //   [blockNo*blockSize .. (blockNo+1)*blockSize)
  serializable_vector<hash_t> blockHashes;

  std::vector<DuplicateBlocks*> duplicateBlocks;

  // number of bytes per block
  unsigned blockSize;

  // size of 'entries' array
  unsigned allocatedSize;

  // number of filled entries in 'entries'
  unsigned entryCount;

  // Maximum load factor, usually .5, must be <= 1
  float maxLoadFactor;

  // maximum number of entries that can be in the table given the current
  // allocatedSize.  This is just a cache of (int)(maxLoadFactor * allocatedSize).
  unsigned maxCount;

  // allocate memory for 'size' entries, complaining if the malloc fails
  void allocateEntries(int size);

  // Increase the capacity and move all the existing entries to the new table.
  void grow(unsigned minimumNewSize);

  // Add an entry that has already been hashed.
  // This method doesn't check if the table has exceeded maxLoadFactor.
  void addHashedEntry(hash_t hashValue, unsigned blockNo);

  // move an entry over during a rehash
  void moveEntry(Entry *entry);


  // round up a value for allocatedSize to the next higher power of 2
  static unsigned roundUpSize(unsigned n);

  DedupTable(unsigned blockSize,
	     bool capacityIsAllocationSize,
	     unsigned initialCapacity,
	     float maxLoadFactor_ = DEDUP_TABLE_MAX_LOAD_FACTOR);

  // grow to fit an index for all the blocks in the given file
  void insureCapacityForFile(FILE *f);

  u64 probeCalls;
  u64 probeIters;

  // find the entry in the table that contains the given hash
  // value, or NULL if not found.
  Entry *probeTable(hash_t hashValue) const {
#ifdef PROFILE_PROBING
    probeCalls++;
#endif
    int sizeMask = allocatedSize-1;
    int entryNo = hashValue & sizeMask;

    // linear probing
    while (entries[entryNo].hashValue != 0) {
#ifdef PROFILE_PROBING
      probeIters++;
#endif
      if (entries[entryNo].hashValue == hashValue) return &entries[entryNo];
      entryNo = (entryNo+1) & sizeMask;
    }

    return NULL;
  }


public:
  static DedupTable *createEmpty(unsigned blockSize);

  // read a regular file and create a DedupTable index of it
  static DedupTable *createFromFile(const char *filename, unsigned blockSize);
  static DedupTable *createFromFile(FILE *inf, unsigned blockSize);

  // read a serialized DedupTable from a file
  static DedupTable *readFromFile(const char *filename, bool verbose=false);

  void addBlock(const char *data);

  // serialize a DedupTable to a file, return the number of bytes written
  u64 writeToFile(const char *filename, bool verbose=false);

  ~DedupTable();

  void printStats();

  // Returns the hashed value for a given block.
  hash_t getBlockHash(unsigned blockNo) {
    return blockHashes[blockNo];
  }

  // map hash value of 0 to ffff...
  HD static hash_t filterHash(hash_t h) {
    return h == 0 ? ~h : h;
  }

  // primary hash algorithm
  static hash_t hash(const void *data, unsigned len) {
    hash_t hashValue = DEFAULT_HASH_FN(data, len);

    // to save space when implementing closed-addressing hash table; hash
    // value must never be 0.  That value is reserved to denote an empty entry.
    if (hashValue == 0) hashValue = ~hashValue;
    return hashValue;
  }

  // secondary hash algorithm
  // static unsigned hash2(const void *data, unsigned len);

  bool hasMatch(hash_t hashValue) const {
    return probeTable(hashValue) != NULL;
  }

  // If a block with a matching hash value was found, set *blockNo to
  // the block index and return true.  Otherwise return false.
  bool findFirstMatch(const char *data, unsigned *blockNo);
  bool findHashedFirstMatch(hash_t hashValue, unsigned *blockNo);

  // If any matching blocks are found, store them in blockNos and return
  // true.  Otherwise return false.
  bool findAllMatches(const char *data,
		      std::vector<unsigned> &blockNos,
		      unsigned maxMatchCount = INT_MAX);
  bool findHashedAllMatches(hash_t hashValue,
			    std::vector<unsigned> &blockNos,
			    unsigned maxMatchCount = INT_MAX);

  // Returns true iff the given block number if found somewhere
  // in the list of blocks with this hash value.
  bool findHashedMatch(hash_t hashValue, unsigned blockNo);

  u64 outputFileSize() {
    return 64 + (u64)allocatedSize * sizeof(Entry)
      + (u64)entryCount * sizeof(unsigned);
  }

  unsigned size() {return blockHashes.size();}
  unsigned getBlockSize() {return blockSize;}
  unsigned getUniqueCount() {return entryCount;}

  // Caution: these two methods break encapsulation and are only made 
  // available for optimization.  Their values should be considered 
  // invalidated the next time an entry is added to the table.
  unsigned getAllocatedSize() const {return allocatedSize;}
  const Entry *getEntryArray() const {return entries;}
};


#endif // __DEDUP_TABLE__
