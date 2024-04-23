#include "ByteStream.hpp"

ByteStream::ByteStream(uint8_t *buf, int bufSize)
    : _buf(buf), _bufSize(bufSize) {
  _actualReadSize = 0;
};

const uint8_t ByteStream::readByte() {
  if (_bufSize < 1)
    return -1;

  uint8_t p = *_buf;
  _buf++;
  _bufSize--;
  _actualReadSize++;
  return p;
}
