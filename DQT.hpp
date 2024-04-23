#ifndef DQT_HPP_2CMNIVI4
#define DQT_HPP_2CMNIVI4

#include "Marker.hpp"

namespace mark {
class DQT : public Marker {
 private:
  /* 应对多个量化表的情况 */
  vector<vector<uint16_t>> _quantizationTables;
  /* 这个函数不参与实际的解码(Option) */
  void printQuantizationTable(vector<uint16_t> quantizationTable);
  /* 这个函数不参与实际的解码(Option) */
  inline void encodeZigZag(int a[64], int matrix[8][8]);
  /* 这个函数不参与实际的解码(Option) */
  inline void printMatrix(int arr[8][8]);

 public:
  vector<vector<uint16_t>> getQuantizationTables() {
    return _quantizationTables;
  }

  /* ISO/IEC 10918-1 : 1993(E) : page 39 */
  int parse(int index, uint8_t *buf, int bufSize) override;
};
} // namespace mark
#endif /* end of include guard: DQT_HPP_2CMNIVI4 */
