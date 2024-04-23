#ifndef COM_HPP_L6D0NFR5
#define COM_HPP_L6D0NFR5

#include "Marker.hpp"

namespace mark {
class COM : public Marker {
 public:
  int parse(int index, uint8_t *buf, int bufSize) override;
};
} // namespace mark

#endif /* end of include guard: COM_HPP_L6D0NFR5 */
