#ifndef APP0_HPP_FTJOCA2E
#define APP0_HPP_FTJOCA2E

#include "Marker.hpp"

namespace mark {
class APP0 : public Marker {
 public:
  /* https://www.w3.org/Graphics/JPEG/jfif3.pdf page6 */
  int parse(int index, uint8_t *buf, int bufSize) override;
};
} // namespace mark
#endif /* end of include guard: APP0_HPP_FTJOCA2E */
