#include "SOS.hpp"

int mark::SOS::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  uint16_t len = bs.readBytes<uint16_t>(2);
  std::cout << "SOS[" << index << "~" << index + len << "] --> {" << std::endl;

  _imageComponentCount = bs.readByte();
  std::cout << "\timageComponentCount:" << (int)_imageComponentCount
            << std::endl;
  std::cout << "\timageComponent{" << std::endl;
  for (int i = 0; i < _imageComponentCount; i++) {
    /* 扫描分量选择器 */
    uint8_t scanComponentSelector = bs.readByte();
    std::cout << "\t\tscanComponentSelector:" << (int)scanComponentSelector;
    uint8_t TdTa = bs.readByte();
    /* DC,AC熵编码表目的地选择器 */
    uint8_t entropyCodingTableDestinationSelectorDC = TdTa >> 4;
    std::cout << ",entropyCodingTableDestinationSelectorDC:"
              << (int)entropyCodingTableDestinationSelectorDC;
    uint8_t entropyCodingTableDestinationSelectorAC = TdTa & 0b1111;
    std::cout << ",entropyCodingTableDestinationSelectorAC:"
              << (int)entropyCodingTableDestinationSelectorAC << std::endl;
  }
  std::cout << "\t}" << std::endl;

  uint8_t startOfSpectral = bs.readByte();
  std::cout << "\tstartOfSpectral:" << (int)startOfSpectral << std::endl;
  uint8_t endOfSpectral = bs.readByte();
  std::cout << "\tendOfSpectral:" << (int)endOfSpectral << std::endl;
  uint8_t AhAl = bs.readByte();
  uint8_t successiveApproximationBitH = AhAl >> 4;
  std::cout << "\tsuccessiveApproximationBitH:"
            << (int)successiveApproximationBitH << std::endl;
  uint8_t successiveApproximationBitL = AhAl & 0b1111;
  std::cout << "\tsuccessiveApproximationBitL:"
            << (int)successiveApproximationBitL << std::endl;

  std::cout << "}" << std::endl;
  scanEntropyCodingImageData(bs);
  return 0;
}

int mark::SOS::package(uint8_t *&buf, int &bufSize) { return 0; };

int mark::SOS::scanEntropyCodingImageData(ByteStream &bs) {
  uint16_t a = 1;
  /* 读完剩余Buffer*/
  while (bs.getBufSize()) {
    a = bs.readBytes<uint16_t>(2);
    if (a >> 8 == 0xff && (a & 0xff) == EOI)
      return 0;
    bitset<16> bits(a);
    _scanData.append(bits.to_string());
  }
  return 0;
}
