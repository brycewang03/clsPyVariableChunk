// This is the main DLL file.
/*
Split a file (usually office or pdf) into variable-length chunks based on a
rolling hash scheme (as described in "A Low-bandwidth Network File System", Muthitacharoen et al.),
and compute a hash of each chunk.

Output a line of metadata for each file:
ID: file/chunk
Byte offset ( start )
Length in bytes
City Hash value for the full file if ID is "file"
City Hash value for the Chunks if ID is "chunk"

eg:
file	0	1656723	3cf7c2eb0afccb8b0b24bfecc05f6119
chunk	0	74047	dd79445d2db542a2c2e3e1e367a7e16d
chunk	74047	127207	5dfd3675aee381eb2a3f599788ac4937
chunk	201254	146064	0e0c02497b77c1f4a6f57e81e4cbb2a2
chunk	347318	72005	d78df0794062241daccd18d61b441df8
chunk	419323	184452	b473825db1cd39fff055e708d5528d69
chunk	603775	205962	20d39cebf675c96aac694c62f998f84f
chunk	809737	180829	5c7aaf0e178d0bfe4e0a8e983235b95a
chunk	990566	72609	a3020ad0b4e62711a7a3564e867d7fb4
chunk	1063175	143375	5b7297bb20ae5c5a319b0be1877a99df
chunk	1206550	80118	1466ef8bab5ac7189e2bfe6ed26be27a
chunk	1286668	87194	3a9add9548c5d45aa906485484a6f470
chunk	1373862	94041	1664a3a8815c1a4ea19a7674e661ddde
chunk	1467903	124202	2caf1bbc0e39aec704f420390bdb5066
chunk	1592105	64618	926ea86ab729bfe3473a529d958097ab
*/

#include <stdlib.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>
#include "dedup-util.h"
#include "HashAlgs.h"
#include "RollingWindow.h"

#include <array>
#include <sstream>
#include <math.h>
#include <cmath>
//#include "clsREST.h"

#include <fstream>
#include <vector>


#include "u64.h"
// This is the main DLL file.

//#include "clsNewVairableChunk.h"
//#include <jni.h>
#include <string>
#include <iostream>
#include <vector>

#include "md5.h"

using namespace std;

#define ARCHIVE_FILE_MAX_NAME_LEN 1023
#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)

/*According to Eshghi [5], the optimal minimum chunk size is 85% of the hash modulo,
and the optimal maximum chunk size is 200% of the hash modulo.
But add file round(size / 64) - DEFAULT_MOD_SIZE as modulo is a proper test eg: 16MB = 256K
Thus JBOX parameter profile: Min = 3565158 bytes(0.85M=3565158.4), Max = 8388608 bytes(8M), mod = 4194304(4M)*/

#define DEFAULT_CHUNK_SIZE 4194304
#define DEFAULT_MIN_CHUNK_SIZE 3565158
#define DEFAULT_MAX_CHUNK_SIZE 8388608
#define DEFAULT_MOD_BASE 4194304
#define DEFAULT_MOD_VALUE 23
#define DEFAULT_SLIDEWINDOW 48

//#define DEFAULT_MOD_SIZE 16
#define DEFAULT_MOD_SIZE 32
//#define DEFAULT_MOD_SIZE 64
//#define DEFAULT_MOD_SIZE 128

#define DEFAULT_ALG_FIB false
#define DEFAULT_VAR false
//#define DEFAULT ALG_HASH false  //false is verbose format, true is json format

/*According to LBFS
8KB Mod Base = 8388608 bytes
2KB Min = 2097152 bytes
64KB Max = 67108864 bytes
#define DEFAULT_CHUNK_SIZE 8192
#define DEFAULT_MIN_CHUNK_SIZE 2048
#define DEFAULT_MAX_CHUNK_SIZE 65536
#define DEFAULT_MOD_BASE 8192
#define DEFAULT_MOD_VALUE 23
#define DEFAULT_SLIDEWINDOW 48
*/

u64 chunkSize = DEFAULT_CHUNK_SIZE;
u64 minChunk = DEFAULT_MIN_CHUNK_SIZE;
u64 maxChunk = DEFAULT_MAX_CHUNK_SIZE;
u64 modBase = DEFAULT_MOD_BASE;
u64 modValue = DEFAULT_MOD_VALUE;
u64 slidingWindowSize = DEFAULT_SLIDEWINDOW;
u64 modSize = DEFAULT_MOD_SIZE;

bool bolFIB = DEFAULT_ALG_FIB;
bool bolVAR = DEFAULT_VAR;
//bool bolHASH = DEFAULT ALG_HASH;

//default minmum null length, logically small is better but took longer time and slower.
#define DEFAULT_MIN_NULL_LEN 64

// hash function for the whole file
typedef u128 file_hash_t;
#define FILE_HASH_FN cityHash128

// hash function for the chunks in each file
typedef u128 chunk_hash_t;
#define CHUNK_HASH_FN cityHash128

//void scanFile(MemoryMappedFile *file, const char *chrFilePath);
bool verbose = false;

struct OffsetLen {
	u64 offset=0;
	unsigned len=0;

	OffsetLen() {}
	OffsetLen(u64 o, unsigned l) : offset(o), len(l) {}
};

struct LengthAndCount {
	typedef map<u64, u64> Map;
	Map data;

	void addLength(u64 len) {
		Map::iterator it = data.find(len);
		if (it == data.end()) {
			data[len] = 1;
		}
		else {
			it->second++;
		}
	}
};

u64 getPower2_range(u64 n)
{
	//u64  x = n;
	u64  y = 2;
	u64  upper = pow(2, ceil(log(n) / log(y)));
	//u64  lower = pow(2, floor(log(n) / log(y)));
	//return std::make_pair(lower, upper);
	return upper;
}

const double pheta = 0.5*(std::sqrt(5) + 1);

long double fib(long double n)
{
	return (std::pow(pheta, n) - std::pow(1 - pheta, n)) / std::sqrt(5);
}

long double fibo_lowerbound(long double  N, long double  min, long double  max)
{
	u64  newpivot = (min + max) / 2;
	if (min == newpivot)
		return newpivot;
	if (fib(newpivot) <= N)
		return fibo_lowerbound(N, newpivot, max);
	else
		return fibo_lowerbound(N, min, newpivot);
}

u64 getFibo_range(u64 n)
{
	int ldbcounter = 0;
	long double tmpN = n;
	while (tmpN>1)
	{
		tmpN = tmpN / 10;
		ldbcounter++;
	}
	//ldbcounter++;
	long double max = pow(10, ldbcounter);
	long double lbound = fibo_lowerbound(n, 0, max);
	//long double tmpReturn = round(fib(lbound + 1));
	return round(fib(lbound + 1));
}

void GenSLOFiles(const char *ofpath, const char *ofname, const char *pos, unsigned len){
	ofstream of;
	string strpath = (string)ofpath + (string)ofname;
	of.open(strpath.c_str(), ofstream::binary);
	of.write(pos, len);
	of.close();
}

// For now this is a tree, but try it with a hash table too.
typedef map<chunk_hash_t, OffsetLen> chunkMapType;
chunkMapType chunkMap;
string returnBufferString, returnCityHash, returnGetString;

/*
* Class:     clsJavaVariableChunk
* Method:    getVariableChunkProfile
* Signature: (Ljava/lang/String;)Ljava/lang/String;
*/

//JNIEXPORT jstring JNICALL
//Java_clsCityHashCalc_clsJavaVariableChunk_getVariableChunkProfile(JNIEnv *env, jobject obj, jstring clsJavaVariableChunk, jint intMod)
//const char *clsVairableChunk::ProcessFileToVar(const char *FilePath, int intMod)
//
//string returnBufferString, returnCityHash, returnGetString;
extern "C" {
//const char *ProcessFileToVar(const char *FilePath)
//{
//Scan file via zero delimiter to find out Larger Chunk and split Larger Chunk into smaller Chunk via rolling hash
//__declspec(dllexport)

	const char * ProcessFileToVar(const char *chrFilePath, int intPower, int intMod, int intDivide, int intRefactor, bool boljson, bool bolhash, const char *ofpath, bool bolslo) {

		//intPower 0 is new # is anchor
		//intMod 0 is whole file, 1 is fix and 2 is var
		//intDivide , if 64 , then chunks # between 32 ~ 74, if 32 , then chunks # between 16 ~ 40 .. etc
		//intRefactor, 0 is no refactor, 3 is currect anchor - original anchor > 3, then we refactor using new anchor.

		//stringstream return buffer
		stringstream ssbuffer;

		string strFilename = (string)chrFilePath;
		//string strFilename = strFilePath;

		// declare mappedFile object
		MemoryMappedFile mappedFile;
		// map file into memory
		mappedFile.mapFile(chrFilePath);
		//mappedFile.mapFile(strFilePath.c_str());

		//declare data as the pointer for file content
		const char *data = (const char *)mappedFile.getAddress();

		//declare len as file size
		u64 len = mappedFile.getLength();

		//declare pos as pointer for file content "start", and dataEnd for file content end
		const char *pos = data;

		char buf[80];

		// hash the whole region
		file_hash_t fileHash = FILE_HASH_FN(pos, (unsigned int)len);

		MD5 FileHashMD5 = MD5((const byte*)pos, (unsigned int)len);
		//const char *chrFileHash = fileHash.toHex(buf);

		modSize = intDivide;
		//When MTU = 1500 Bytes, and if file size is less than 112942 bytes (111KB) = ROUND(1500 * (64) * (100/85), 0) + 1
		//doesn't required split into chunks, so set intPower=1
		u64 dblmin = round((1500 * modSize *100)/85)+1;
		if (len <= dblmin){
				intMod=0;
		}

		if (boljson){
			ssbuffer << "[";
		}

		if (intMod == 0){
			if (bolhash){
				//const char *chrFileIndex= strFileIndex.c_str();
				if (boljson){
					if (bolslo){
						ssbuffer << "{\"path\":\"/chunks/" << FileHashMD5.toStr()  << "\",\"size_bytes\":" << len << ",\"etag\":\"" << FileHashMD5.toStr() << "\"},";
						GenSLOFiles(ofpath, FileHashMD5.toStr().c_str(), pos, len);
					}else
					{
					    ssbuffer << "{\"type\":\"file\",\"start\":" << (u64)(pos - data) << ",\"len\":" << len << ",\"pow\":" << intPower << ",\"hash\":\"" << FileHashMD5.toStr() << "\"},";
					    ssbuffer << "{\"type\":\"chunk\",\"start\":" << (u64)(pos - data) << ", \"len\":" << len << ",\"pow\":" << 0 << ", \"hash\":\"" << FileHashMD5.toStr() << "\"},";
					}
				}else{
					ssbuffer << "file\t" << (u64)(pos - data) << "\t" << len << "\t" << intPower << "\t" << FileHashMD5.toStr() << "\n";
					ssbuffer << "chunk\t" << (u64)(pos - data) << "\t" << len << "\t" << 0 << "\t" << FileHashMD5.toStr() << "\n";
				}
			}else{
				//const char *chrFileIndex= strFileIndex.c_str();
				if (boljson){
					ssbuffer << "{\"type\":\"file\",\"start\":" << (u64)(pos - data) << ",\"len\":" << len << ",\"pow\":" << intPower << ",\"hash\":\"" << fileHash.toHex(buf) << "\"},";
					ssbuffer << "{\"type\":\"chunk\",\"start\":" << (u64)(pos - data) << ", \"len\":" << len << ",\"pow\":" << 0 << ", \"hash\":\"" << fileHash.toHex(buf) << "\"},";
				}else{
					ssbuffer << "file\t" << (u64)(pos - data) << "\t" << len << "\t" << intPower << "\t" << fileHash.toHex(buf) << "\n";
					ssbuffer << "chunk\t" << (u64)(pos - data) << "\t" << len << "\t" << 0 << "\t" << fileHash.toHex(buf) << "\n";
				}
			}
		}
		else //if file size is larger than 256K * 0.85, doesn't required split into chunks, PS: 256K/64 = 4K the min is 4K file
		{
			//declare rolling hash object and assign paramenters
			RollingWindow rollingWindow;

			/*check intPower = 0 or not, if 0 new one, find out , if not 0, then has anchor */
			if (intPower == 0){
				/*find out chunk size depends on file size*/
				if (bolFIB){
					//pair<u64, u64> range = getFibo_range(len / modSize);
					//rollingWindow.chunkSize = range.second; //get fib upper bound for chunk size
					rollingWindow.chunkSize = getFibo_range(round(len / modSize)); // len >> 6 shift 6 bytes
				}
				else{
					//pair<u64, u64> range = getPower2_range(len / modSize);
					//rollingWindow.chunkSize = range.second; //get power of 2 upper bound for chunk sizede
					rollingWindow.chunkSize = getPower2_range(round(len / modSize)); // len >> shift 6 bytes
				}
				//udpate intPower in metadata
				intPower=log(rollingWindow.chunkSize) / log(2);
			}
			else
			{
				//get the new anchor
				int intNewPower=log(getPower2_range(round(len / modSize))) / log(2);

				//check anchor = 0 , new or new anchor - orignal anchor < refactor #, then
				if (intRefactor ==0 || (intNewPower-intPower) < intRefactor){
					rollingWindow.chunkSize = pow(2,intPower);
				}else{
					intPower=intNewPower; //only when new anchor - original anchor >= refactor e.g. 2^13 - 2^10 --> 13 - 10 >=3
					rollingWindow.chunkSize = pow(2,intNewPower);
				}
			}

			// const char *chrFileIndex= strFileIndex.c_str();
	    // print out File level Info
			if (boljson){
				if (bolslo){
					//ssbuffer << "{\"path\":\"\/chunks\/" << FileHashMD5.toStr()  << "\",\"size_bytes\":" << len << ",\"etag\":\"" << FileHashMD5.toStr() << "\"},";
				}else
				{
					ssbuffer << "{\"type\":\"file\",\"start\":" << (u64)(pos - data) << ",\"len\":" << len << ",\"pow\":" << intPower << ",\"hash\":\"" << FileHashMD5.toStr() << "\"},";
				}
			}else{
				ssbuffer << "file\t" << (u64)(pos - data) << "\t" << len << "\t" << intPower << "\t" << FileHashMD5.toStr()<< "\n";
	    }

			if (intMod == 1){
				/*if anchor fix, the optimal minimum chunk size is 85% of the hash modulo,
				and the optimal maximum chunk size is 200% of the hash modulo.*/
				rollingWindow.minChunkSize = rollingWindow.chunkSize;
				rollingWindow.maxChunkSize = rollingWindow.chunkSize;
				rollingWindow.modBase = rollingWindow.chunkSize;
			}
			else
			{
				/*According to Eshghi [5], the optimal minimum chunk size is 85% of the hash modulo,
				and the optimal maximum chunk size is 200% of the hash modulo.*/
				rollingWindow.minChunkSize = ((rollingWindow.chunkSize) * 85) / 100;
				rollingWindow.maxChunkSize = (rollingWindow.chunkSize) * 2;
				rollingWindow.modBase = rollingWindow.chunkSize;
			}

			/*
			rollingWindow.chunkSize = chunkSize;
			rollingWindow.minChunkSize = minChunk;
			rollingWindow.maxChunkSize = maxChunk;
			rollingWindow.modBase = modBase;
			*/

			rollingWindow.modValue = modValue;
			rollingWindow.slidingWindowSize = slidingWindowSize;

			//rollingWindow.dataFile
			const char *startPos = pos;
			const char *endPos = pos + len;
			u64 fileOffset = pos - data;

			// process the first chunk to get chunk len
			unsigned chunkLen = rollingWindow.getChunkLength((const unsigned char*)pos, endPos - pos);

			//compute a hash of the whole chunk len
			chunk_hash_t leastHash = CHUNK_HASH_FN(pos, chunkLen);
			MD5 LeastChunkHashMD5 = MD5((const byte*)pos, chunkLen);

			// always process the 1st data chunk, check if an identical chunk has been seen already shows up in index
			chunkMapType::iterator existing = chunkMap.find(leastHash);
			if (existing == chunkMap.end()) {
				// if not, add this as a new chunk
				chunkMap[leastHash] = OffsetLen((u64)(pos - startPos) + fileOffset, chunkLen);
			}
			char hashBuf[80];
			//const char *chrFileIndex= strFileIndex.c_str();
			if (bolhash){
				if (boljson){
					if (bolslo){
						string strLeastChunkMD5=LeastChunkHashMD5.toStr();
						ssbuffer << "{\"path\":\"/chunks/" << strLeastChunkMD5  << "\",\"size_bytes\":" << chunkLen << ",\"etag\":\"" << strLeastChunkMD5 << "\"},";
						GenSLOFiles(ofpath, strLeastChunkMD5.c_str(), pos, chunkLen);
					}else{
						ssbuffer << "{\"type\":\"chunk\",\"start\":" << (u64)(pos - startPos) + fileOffset << ",\"len\":" << chunkLen << ",\"pow\":" << 0 << ",\"hash\":\"" << LeastChunkHashMD5.toStr() << "\"},";
					}
				}else{
					ssbuffer << "chunk\t" << (u64)(pos - startPos) + fileOffset << "\t" << chunkLen << "\t" <<  0 << "\t" << LeastChunkHashMD5.toStr() << "\n";
				}
			}
			else{
				if (boljson){
					ssbuffer << "{\"type\":\"chunk\",\"start\":" << (u64)(pos - startPos) + fileOffset << ",\"len\":" << chunkLen << ",\"pow\":" << 0 << ",\"hash\":\"" << leastHash.toHex(hashBuf) << "\"},";
				}else{
					ssbuffer << "chunk\t" << (u64)(pos - startPos) + fileOffset << "\t" << chunkLen << "\t" <<  0 << "\t" << leastHash.toHex(hashBuf) << "\n";
				}
				fileHash.toHex(buf);
			}

			pos += chunkLen;

			while (pos < endPos) {
				//get chunk length
				chunkLen = rollingWindow.getChunkLength((const unsigned char*)pos, endPos - pos);

				//compute a hash by chunk len
				chunk_hash_t hash = CHUNK_HASH_FN(pos, chunkLen);
				MD5 ChunkHashMD5 = MD5((const byte*)pos, chunkLen);

				// check if an identical chunk has been seen already shows up in index
				chunkMapType::iterator existing = chunkMap.find(hash);
				if (existing == chunkMap.end()) {
					chunkMap[hash] = OffsetLen((u64)(pos - startPos) + fileOffset, chunkLen);
				}
				char hashBuf[80];
				if (bolhash){
					if (boljson){
						if (bolslo){
							string strChunkMD5=ChunkHashMD5.toStr();
							ssbuffer << "{\"path\":\"/chunks/" << strChunkMD5  << "\",\"size_bytes\":" << chunkLen << ",\"etag\":\"" << strChunkMD5 << "\"},";
							GenSLOFiles(ofpath, strChunkMD5.c_str(), pos, chunkLen);
						}
						else{
							ssbuffer << "{\"type\":\"chunk\",\"start\":" << (u64)(pos - startPos) + fileOffset << ",\"len\":" << chunkLen << ",\"pow\":" << 0 << ",\"hash\":\"" << ChunkHashMD5.toStr() << "\"},";}
						}
					else{ssbuffer << "chunk\t" << (u64)(pos - startPos) + fileOffset << "\t" << chunkLen << "\t" << 0 << "\t" << ChunkHashMD5.toStr() << "\n";}

				}else{
					if (boljson){ssbuffer << "{\"type\":\"chunk\",\"start\":" << (u64)(pos - startPos) + fileOffset << ",\"len\":" << chunkLen << ",\"pow\":" << 0 << ",\"hash\":\"" << hash.toHex(hashBuf) << "\"},";}
					else{ssbuffer << "chunk\t" << (u64)(pos - startPos) + fileOffset << "\t" << chunkLen << "\t" << 0 << "\t" << hash.toHex(hashBuf) << "\n";}
					//GenSLOFiles(ofpath, hash.toHex(hashBuf), pos, chunkLen);
				}
				pos += chunkLen;
			}
		}

		// close mappedFile object
		mappedFile.close();

		returnBufferString = ssbuffer.str();

	  // clean out json closer
		if (boljson){
			returnBufferString = returnBufferString.substr(0, returnBufferString.size()-1);
			returnBufferString = returnBufferString + "]";
		}

		//return env->NewStringUTF(returnBufferString.c_str());
		return returnBufferString.c_str();
  }
}

