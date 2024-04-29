#ifndef SOS_HPP_WCIRKBS1
#define SOS_HPP_WCIRKBS1

#include "Marker.hpp"
#include <cstdint>

/* TODO YangJing 需要整理这里 <24-04-28 19:37:55> */
#define SCAN_COMPONENT 3

namespace mark {
typedef struct _ScanComponent {
  uint8_t scanComponentSelector = 0;
  uint8_t TdTa = 0;
} ScanComponent;

typedef struct __attribute__((packed)) _SOS {
  uint8_t SOS[2] = {0xff, JFIF::SOS};
  uint16_t len = 0;
  uint8_t imageComponentCount = 0;
  /* TODO YangJing 这里写死为固定3个分量，问题也不大 <24-04-28 19:34:22> */
  ScanComponent scanComponent[SCAN_COMPONENT];
  uint8_t startOfSpectral = 0;
  uint8_t endOfSpectral = 0;
  uint8_t AhAl = 0;
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
