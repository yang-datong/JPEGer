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
  uint8_t element[64] = {0};
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

  const uint8_t _lumaTable[QUANTIZATION_TAB_SIZE] = {
      16, 11, 10, 16, 24,  40,  51,  61,  12, 12, 14, 19, 26,  58,  60,  55,
      14, 13, 16, 24, 40,  57,  69,  56,  14, 17, 22, 29, 51,  87,  80,  62,
      18, 22, 37, 56, 68,  109, 103, 77,  24, 35, 55, 64, 81,  104, 113, 92,
      49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99,
  };

  const uint8_t _chromaTable[QUANTIZATION_TAB_SIZE] = {
      17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99,
      24, 26, 56, 99, 99, 99, 99, 99, 47, 66, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
  };

  int buildLumaDQTable(ofstream &outputFile);
  int buildChromaDQTable(ofstream &outputFile);
};
} // namespace mark
#endif /* end of include guard: DQT_HPP_2CMNIVI4 */
