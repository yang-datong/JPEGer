#ifndef SOS_HPP_WCIRKBS1
#define SOS_HPP_WCIRKBS1

#include "Marker.hpp"
#include <cstdint>

namespace mark {
class SOS : public Marker {
 private:
  /* 帧中图像分量的数量 */
  string _scanData;
  uint8_t _imageComponentCount = 0;

 public:
  string getScanData() { return _scanData; }
  uint8_t getImageComponentCount() { return _imageComponentCount; }
  /* ISO/IEC 10918-1 : 1993(E) : page 37 */
  int parse(int index, uint8_t *buf, int bufSize) override;

 private:
  /* 读取编码图像数据 */
  int scanEntropyCodingImageData(ByteStream &bs);
};
} // namespace mark
#endif /* end of include guard: SOS_HPP_WCIRKBS1 */
