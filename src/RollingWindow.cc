#include "HashAlgs.h"
#include "RollingWindow.h"

unsigned RollingWindow::getChunkLength(const unsigned char *chunkStart, u64 bytesRemaining) {

  SlidingWindowHash hasher;

  // if the data remaining is smaller than the minimum chunk size,
  // just use it
  if (bytesRemaining <= minChunkSize)
    return (unsigned)bytesRemaining;

  // start with the minimum chunk size
  const unsigned char *chunkEnd = chunkStart + minChunkSize;
  const unsigned char *chunkRemove = chunkEnd - slidingWindowSize;

  // hash the last slidingWindowSize bytes
  hasher.addChars(chunkRemove, slidingWindowSize);

  unsigned maxLen = (bytesRemaining > maxChunkSize)
    ? maxChunkSize
    : (unsigned)bytesRemaining;
  const unsigned char *maxChunkEnd = chunkStart + maxLen;

  while (true) {
    // These two cases could be in the 'while' condition, but this
    // way, with the 'break' statements on their own lines, it's easier
    // to set useful debugger breakpoints on each of the cases.

    // end the chunk if:

    // A) the chunk hashes to the right modulo value
    if ((hasher.getHash() % modBase) == modValue) {
      break;
    }

    // B) the chunk has reached the maximum chunk size
    if (chunkEnd >= maxChunkEnd) {
      break;
    }

    // move start byte and add a new bytes to the chunk
    hasher.moveChar(*chunkEnd++, *chunkRemove++);
  }

  return (unsigned)(chunkEnd - chunkStart);
}
