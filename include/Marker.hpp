#ifndef MARKER_HPP_WXZAZHWM
#define MARKER_HPP_WXZAZHWM

#include "ByteStream.hpp"
#include "MCU.hpp"
#include <cstdint>

class Marker {
 public:
  uint16_t len = 0;
  virtual ~Marker() {}
  virtual int parse(int index, uint8_t *buf, int bufSize) = 0;
  virtual int package(uint8_t *&buf, int &bufSize) = 0;
};

#endif /* end of include guard: MARKER_HPP_WXZAZHWM */
