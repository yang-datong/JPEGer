#include "SOF0.hpp"

int mark::SOF0::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  const int begIndex = index;
  uint16_t len = bs.readBytes<uint16_t>(2);
  const int endIndex = len + begIndex;
  std::cout << "SOF0[" << begIndex << "~" << endIndex << "] --> {" << std::endl;
  uint8_t precision = bs.readByte();
  std::cout << "\tType:Baseline DCT" << std::endl;
  std::cout << "\tprecision:" << (int)precision << " bit" << std::endl;

  _imgHeight = bs.readBytes<uint16_t>(2);
  _imgWidth = bs.readBytes<uint16_t>(2);
  std::cout << "\timgWidth:" << (int)_imgWidth
            << ",imgHeight:" << (int)_imgHeight << std::endl;

  /* 帧中图像分量的数量 */
  uint8_t imageComponentCount = bs.readByte();
  std::cout << "\timageComponentCount:" << (int)imageComponentCount
            << std::endl;

  bool isNonSampled = true;
  std::cout << "\timageComponent{" << std::endl;
  for (int i = 0; i < imageComponentCount; i++) {
    uint8_t componentIdentifier = bs.readByte();
    uint8_t sampFactor = bs.readByte();
    uint8_t sampFactorH = sampFactor >> 4;
    uint8_t sampFactorV = sampFactor & 0b1111;
    uint8_t destinationSelector = bs.readByte();
    std::cout << "\t\tcomponentIdentifier:" << (int)componentIdentifier;
    std::cout << ",Horizontal sampFactor:" << (int)sampFactorH
              << ",Vertical sampFactor:" << (int)sampFactorV;
    std::cout << ",destinationSelector:" << (int)destinationSelector
              << std::endl;
    if ((sampFactor >> 4) != 1 || (sampFactor & 0xf) != 1)
      isNonSampled = false;
  }
  std::cout << "\t}" << std::endl;
  if (isNonSampled == false) {
    cout << "Chroma subsampling not yet supported!" << std::endl;
    cout << "Chroma subsampling is not 4:4:4, terminating..." << std::endl;
    return -1;
  }

  std::cout << "}" << std::endl;
  return 0;
}
