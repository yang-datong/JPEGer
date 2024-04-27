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
  uint16_t element[64] = {0};
  /* TODO YangJing 这里不太确定是uint8_t,uint16_t后续再看 <24-04-27 16:37:57> */
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
};
} // namespace mark
#endif /* end of include guard: DQT_HPP_2CMNIVI4 */
