#ifndef APP0_HPP_FTJOCA2E
#define APP0_HPP_FTJOCA2E

#include "Marker.hpp"
#include <cstdint>

namespace mark {
typedef struct __attribute__((packed)) _APP0 {
  uint8_t APP0[2] = {0xff, JFIF::APP0};
  uint16_t len = 0;
  uint8_t identifier[5] = {0};
  uint8_t jfifVersion[2] = {0};
  uint8_t unit = 0;
  uint16_t Xdensity = 0;
  uint16_t Ydensity = 0;
  uint8_t Xthumbnail = 0;
  uint8_t Ythumbnail = 0;
} APP0Header;

class APP0 : public Marker {
 public:
  APP0Header header;
  /* https://www.w3.org/Graphics/JPEG/jfif3.pdf page6 */
  int parse(int index, uint8_t *buf, int bufSize) override;
  int package(ofstream &outputFile) override;
};
} // namespace mark
#endif /* end of include guard: APP0_HPP_FTJOCA2E */
