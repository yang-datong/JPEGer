#ifndef DQT_HPP_2CMNIVI4
#define DQT_HPP_2CMNIVI4

#include "Common.hpp"
#include "Marker.hpp"

namespace mark {
class DQT : public Marker {
 private:
  /* 应对多个量化表的情况 */
  vector<vector<uint16_t>> _quantizationTables;

 public:
  vector<vector<uint16_t>> getQuantizationTables() {
    return _quantizationTables;
  }

  /* ISO/IEC 10918-1 : 1993(E) : page 39 */
  int parse(int index, uint8_t *buf, int bufSize) override;

 private:
  /* 这个函数不参与实际的解码(Option) */
  void printQuantizationTable(vector<uint16_t> quantizationTable);
};
} // namespace mark
#endif /* end of include guard: DQT_HPP_2CMNIVI4 */
