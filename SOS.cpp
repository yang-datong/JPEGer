#include "SOS.hpp"
#include <cstdint>

int mark::SOS::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  header.len = bs.readBytes<uint16_t>(2);
  std::cout << "SOS[" << index << "~" << index + header.len << "] --> {"
            << std::endl;

  header.imageComponentCount = bs.readByte();
  std::cout << "\timageComponentCount:" << (int)header.imageComponentCount
            << std::endl;
  std::cout << "\timageComponent{" << std::endl;
  for (int i = 0; i < header.imageComponentCount; i++) {
    /* 扫描分量选择器 */
    /* TODO YangJing
     * 这里严重有问题：如果header.imageComponentCount不为3则很危险，不过正常情况下都是3，先这样吧
     * <24-04-28 19:41:07> */
    header.scanComponent[i].scanComponentSelector = bs.readByte();
    std::cout << "\t\tscanComponentSelector:"
              << (int)header.scanComponent[i].scanComponentSelector;
    header.scanComponent[i].TdTa = bs.readByte();
    /* DC,AC熵编码表目的地选择器 */
    uint8_t entropyCodingTableDestinationSelectorDC =
        header.scanComponent[i].TdTa >> 4;
    std::cout << ",entropyCodingTableDestinationSelectorDC:"
              << (int)entropyCodingTableDestinationSelectorDC;
    uint8_t entropyCodingTableDestinationSelectorAC =
        header.scanComponent[i].TdTa & 0b1111;
    std::cout << ",entropyCodingTableDestinationSelectorAC:"
              << (int)entropyCodingTableDestinationSelectorAC << std::endl;
  }
  std::cout << "\t}" << std::endl;

  header.startOfSpectral = bs.readByte();
  std::cout << "\tstartOfSpectral:" << (int)header.startOfSpectral << std::endl;
  header.endOfSpectral = bs.readByte();
  std::cout << "\tendOfSpectral:" << (int)header.endOfSpectral << std::endl;
  header.AhAl = bs.readByte();
  uint8_t successiveApproximationBitH = header.AhAl >> 4;
  std::cout << "\tsuccessiveApproximationBitH:"
            << (int)successiveApproximationBitH << std::endl;
  uint8_t successiveApproximationBitL = header.AhAl & 0b1111;
  std::cout << "\tsuccessiveApproximationBitL:"
            << (int)successiveApproximationBitL << std::endl;

  std::cout << "}" << std::endl;
  scanEntropyCodingImageData(bs);
  return 0;
}

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

int mark::SOS::package(ofstream &outputFile) {
  header.len = htons(sizeof(header) - 2);

  header.imageComponentCount = 3;

  uint8_t entropyCodingTableDestinationSelectorDC[SCAN_COMPONENT] = {0, 1, 1};
  uint8_t entropyCodingTableDestinationSelectorAC[SCAN_COMPONENT] = {0, 1, 1};

  for (int i = 0; i < SCAN_COMPONENT; i++) {
    header.scanComponent[i].scanComponentSelector = i + 1;
    header.scanComponent[i].TdTa = entropyCodingTableDestinationSelectorDC[i];
    header.scanComponent[i].TdTa <<= 4;
    header.scanComponent[i].TdTa |= entropyCodingTableDestinationSelectorAC[i];
  }

  header.startOfSpectral = 0;
  header.endOfSpectral = 63;
  header.AhAl = 0;

  entropyCodingImageData();

  uint8_t tmp[sizeof(header)] = {0};
  memcpy(tmp, &header, sizeof(header));
  outputFile.write((const char *)tmp, sizeof(header));
  return 0;
};

int mark::SOS::entropyCodingImageData() { return 0; }
