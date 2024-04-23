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

  template <typename T> const T readBytes(int num) {
    if (num > (int)sizeof(T))
      throw std::runtime_error("Read bytes a smaller type.");
    T value = readByte();
    for (int i = 0; i < num - 1; i++)
      value = (value << 8) | readByte();
    return value;
  }

  int getBufSize() { return _bufSize; }
  int getActualReadSize() { return _actualReadSize; }

 private:
  uint8_t *_buf = nullptr;
  int _bufSize = 0;
  int _actualReadSize = 0;
};

#endif /* end of include guard: BYTESTREAM_HPP_IBRZ3JY0 */
