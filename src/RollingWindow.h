#include "u64.h"
class RollingWindow
{
public:
	unsigned getChunkLength(const unsigned char *chunkStart, u64 bytesRemaining);
	u64 chunkSize, minChunkSize, maxChunkSize, modBase, modValue, slidingWindowSize, modSize;
};
