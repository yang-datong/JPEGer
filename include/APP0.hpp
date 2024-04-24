#ifndef APP0_HPP_FTJOCA2E
#define APP0_HPP_FTJOCA2E

#include "Marker.hpp"

namespace mark {
class APP0 : public Marker {
 public:
  /* https://www.w3.org/Graphics/JPEG/jfif3.pdf page6 */
  int parse(int index, uint8_t *buf, int bufSize) override;
  int package(uint8_t *&buf, int &bufSize) override;
  uint8_t identifier[5] = {0};
  uint8_t jfifVersion[2] = {0};
  uint8_t unit = 0;
  uint16_t Xdensity = 0;
  uint16_t Ydensity = 0;
  uint8_t Xthumbnail = 0;
  uint8_t Ythumbnail = 0;
};
} // namespace mark
#endif /* end of include guard: APP0_HPP_FTJOCA2E */
