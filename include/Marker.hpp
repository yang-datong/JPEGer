#ifndef MARKER_HPP_WXZAZHWM
#define MARKER_HPP_WXZAZHWM

#include "ByteStream.hpp"
#include "MCU.hpp"

class Marker {
 public:
  virtual int parse(int index, uint8_t *buf, int bufSize) = 0;
};

#endif /* end of include guard: MARKER_HPP_WXZAZHWM */
