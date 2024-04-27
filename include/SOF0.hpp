#ifndef SOF0_HPP_WEPMVZS4
#define SOF0_HPP_WEPMVZS4

#include "Marker.hpp"
#include <cstdint>

namespace mark {
class SOF0 : public Marker {
 private:
  uint16_t _imgHeight = 0;
  uint16_t _imgWidth = 0;

 public:
  uint16_t getimgWidth() { return _imgWidth; }
  uint16_t getimgHeight() { return _imgHeight; }
  int parse(int index, uint8_t *buf, int bufSize) override;
  int package(ofstream &outputFile) override;
};
} // namespace mark
#endif /* end of include guard: SOF0_HPP_WEPMVZS4 */
