#ifndef MCU_HPP_MJKLNXG9
#define MCU_HPP_MJKLNXG9

#include "Common.hpp"
#include "Type.hpp"

class MCU {
 public:
  MCU(array<vector<int>, 3> RLE, vector<QuantizationTable> qTables);

 private:
  static int _MCUCount;
  static int _DCDiff[3];

  int _order = 0;
  array<vector<int>, 3> _RLE;
  vector<QuantizationTable> _qtTables;

  CompMatrices _matrix;
  IDCTCoeffs _idctCoeffs;

  void decodeACandDC();
  inline void startIDCT();
  inline void performLevelShift();
  inline void YUVToRGB();

 public:
  const CompMatrices &getAllMatrices() const { return _matrix; }
};
#endif /* end of include guard: MCU_HPP_MJKLNXG9 */
