#ifndef COM_HPP_L6D0NFR5
#define COM_HPP_L6D0NFR5

#include "Marker.hpp"

namespace mark {

typedef struct __attribute__((packed)) _COM {
  uint8_t COM[2] = {0xff, JFIF::COM};
  uint16_t len = 0;
} COMHeader;

class COM : public Marker {
 public:
  COMHeader header;
  int parse(int index, uint8_t *buf, int bufSize) override;
  int package(ofstream &outputFile) override;
};
} // namespace mark

#endif /* end of include guard: COM_HPP_L6D0NFR5 */
