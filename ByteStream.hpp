#ifndef BYTESTREAM_HPP_IBRZ3JY0
#define BYTESTREAM_HPP_IBRZ3JY0

#include <cstdint>
#include <iostream>
#include <math.h>
#include <string.h>

class ByteStream {
 public:
  ByteStream(uint8_t *buf, int bufSize) : _buf(buf), _bufSize(bufSize) {
    _actualReadSize = 0;
  };
  ~ByteStream(){};

  const uint8_t readByte();
  template <typename T> const T readBytes(int num);

  int getBufSize() { return _bufSize; }
  int getActualReadSize() { return _actualReadSize; }

 private:
  uint8_t *_buf = nullptr;
  int _bufSize = 0;
  int _actualReadSize = 0;
};
#include "ByteStream.tpp"

#endif /* end of include guard: BYTESTREAM_HPP_IBRZ3JY0 */
