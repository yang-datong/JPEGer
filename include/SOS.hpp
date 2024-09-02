#ifndef SOS_HPP_WCIRKBS1
#define SOS_HPP_WCIRKBS1

#include "Marker.hpp"
#include <cstdint>

namespace mark {
/* 保证结构体是POD类型，确保在使用时明确初始化所有成员*/
typedef struct _ScanComponent {
  uint8_t scanComponentSelector;
  uint8_t TdTa;
} ScanComponent;

typedef struct __attribute__((packed)) _SOS {
  uint8_t SOS[2];
  uint16_t len;
  uint8_t imageComponentCount;
  ScanComponent scanComponent[SCAN_COMPONENT];
  uint8_t startOfSpectral;
  uint8_t endOfSpectral;
  uint8_t AhAl;

  _SOS()
      : SOS{0xff, JFIF::SOS}, len(0), imageComponentCount(0),
        startOfSpectral(0), endOfSpectral(0), AhAl(0) {}
} SOSHeader;

class SOS : public Marker {
 private:
  /* 帧中图像分量的数量 */
  string _scanData;
  SOSHeader header;

 public:
  string getScanData() { return _scanData; }
  uint8_t getImageComponentCount() { return header.imageComponentCount; }
  /* ISO/IEC 10918-1 : 1993(E) : page 37 */
  int parse(int index, uint8_t *buf, int bufSize) override;
  int package(ofstream &outputFile) override;

 private:
  /* 读取编码图像数据 */
  int scanEntropyCodingImageData(ByteStream &bs);
  int entropyCodingImageData();
};
} // namespace mark
#endif /* end of include guard: SOS_HPP_WCIRKBS1 */
