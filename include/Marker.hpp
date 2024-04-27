#ifndef MARKER_HPP_WXZAZHWM
#define MARKER_HPP_WXZAZHWM

#include "ByteStream.hpp"
#include "MCU.hpp"
#include <cstdint>

class Marker {
 public:
  virtual ~Marker() {}
  virtual int parse(int index, uint8_t *buf, int bufSize) = 0;

  /* 对于二个字节以上的，需要转换大小端 */
  virtual int package(ofstream &outputFile) = 0;
};

#endif /* end of include guard: MARKER_HPP_WXZAZHWM */
