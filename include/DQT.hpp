#ifndef DQT_HPP_2CMNIVI4
#define DQT_HPP_2CMNIVI4

#include "Common.hpp"
#include "Marker.hpp"
#include "Type.hpp"

namespace mark {
class DQT : public Marker {
 private:
  vector<QuantizationTable> _quantizationTables;

 public:
  const vector<QuantizationTable> &getQuantizationTables() {
    return _quantizationTables;
  }

  /* ISO/IEC 10918-1 : 1993(E) : page 39 */
  int parse(int index, uint8_t *buf, int bufSize) override;

 private:
  /* 这个函数不参与实际的解码(Option) */
  void printQuantizationTable(QuantizationTable quantizationTable);
};
} // namespace mark
#endif /* end of include guard: DQT_HPP_2CMNIVI4 */
