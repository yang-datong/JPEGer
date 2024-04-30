#ifndef DQT_HPP_2CMNIVI4
#define DQT_HPP_2CMNIVI4

#include "Common.hpp"
#include "Marker.hpp"
#include "Type.hpp"

namespace mark {

typedef struct __attribute__((packed)) _DQT {
  uint8_t DQT[2] = {0xff, JFIF::DQT};
  uint16_t len = 0;
  uint8_t PqTq = 0;
  uint8_t element8[MCU_UNIT_SIZE] = {0};
  uint16_t element16[MCU_UNIT_SIZE] = {0};
} DQTHeader;

class DQT : public Marker {
 private:
  vector<QuantizationTable> _quantizationTables;

 public:
  DQTHeader header;
  const vector<QuantizationTable> &getQuantizationTables() {
    return _quantizationTables;
  }

  /* ISO/IEC 10918-1 : 1993(E) : page 39 */
  int parse(int index, uint8_t *buf, int bufSize) override;
  int package(ofstream &outputFile) override;

 private:
  /* 这个函数不参与实际的解码(Option) */
  void printQuantizationTable(QuantizationTable quantizationTable);

  int buildDQTable(
      const uint8_t id,
      const uint8_t (&componentTab)[COMPONENT_SIZE][COMPONENT_SIZE],
      ofstream &outputFile);
};
} // namespace mark
#endif /* end of include guard: DQT_HPP_2CMNIVI4 */
