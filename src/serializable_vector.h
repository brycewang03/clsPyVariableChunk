#ifndef __SERIALIZABLE_VECTOR_H__
#define __SERIALIZABLE_VECTOR_H__

#include <cassert>


template<class T>
class serializable_vector {
  T *data;
  size_t capacity; // allocated size of data[]
  size_t count;    // # of entries used

  void grow(size_t newCapacity=0) {
    if (newCapacity == 0) {
      assert(capacity < 0x80000000);
      newCapacity = capacity*2;
    }
    T *newData = new T[newCapacity];
    assert(newData);

    memcpy(newData, data, sizeof(T) * capacity);
    delete[] data;
    data = newData;
    capacity = newCapacity;
  }

 //serializable_vector(const serializable_vector &that) {}

 public:
  serializable_vector(size_t initialCapacity = 10) {
    capacity = initialCapacity;
    data = new T[capacity];
    count = 0;
  }

  /*
  serializable_vector(const serializable_vector &that) {
    capacity = that.capacity;
    data = new T[capacity];
    count = that.count;
    memcpy(data, that.data, sizeof(T) * count);
  }
  */
    

  ~serializable_vector() {
    delete[] data;
  }

  size_t size() {return count;}

  T& operator[](unsigned i) {
    return data[i];
  }

  const T& operator[](unsigned i) const {
    return data[i];
  }

  void push_back(T x) {
    if (capacity == count) grow();
    data[count++] = x;
  }

  void reserve(size_t minimumCapacity) {
    if (minimumCapacity > capacity) grow(minimumCapacity);
  }
  
  // returns the number of bytes written
  off_t writeEntries(FILE *outf) {
    off_t countWritten = fwrite(data, sizeof(T), count, outf);
    return countWritten * sizeof(T);
  }
  
  // returns the number of entries read
  unsigned readEntries(FILE *inf, unsigned readCount) {
    reserve(readCount);
    count = readCount;
    unsigned countRead = fread(data, sizeof(T), count, inf);
    return countRead;
  }
};


#endif // __SERIALIZABLE_VECTOR_H__
