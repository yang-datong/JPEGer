#include "SOF0.hpp"
#include <cstdint>

int mark::SOF0::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  header.len = bs.readBytes<uint16_t>(2);
  std::cout << "SOF0[" << index << "~" << header.len + index << "] --> {"
            << std::endl;
  header.precision = bs.readByte();
  std::cout << "\tType:Baseline DCT" << std::endl;
  std::cout << "\tprecision:" << (int)header.precision << " bit" << std::endl;

  header.imgHeight = bs.readBytes<uint16_t>(2);
  header.imgWidth = bs.readBytes<uint16_t>(2);
  std::cout << "\timgWidth:" << (int)header.imgWidth
            << ",imgHeight:" << (int)header.imgHeight << std::endl;

  /* 帧中图像分量的数量 */
  header.imageComponentCount = bs.readByte();
  std::cout << "\timageComponentCount:" << (int)header.imageComponentCount
            << std::endl;

  bool isNonSampled = true;
  std::cout << "\timageComponent{" << std::endl;

  ImageComponent *imageComponent =
      new ImageComponent[header.imageComponentCount];
  for (int i = 0; i < header.imageComponentCount; i++) {
    imageComponent[i].componentIdentifier = bs.readByte();
    imageComponent[i].sampFactor = bs.readByte();
    uint8_t sampFactorH = imageComponent[i].sampFactor >> 4;
    uint8_t sampFactorV = imageComponent[i].sampFactor & 0b1111;
    imageComponent[i].destinationSelector = bs.readByte();
    std::cout << "\t\tcomponentIdentifier:"
              << (int)imageComponent[i].componentIdentifier;
    std::cout << ",Horizontal sampFactor:" << (int)sampFactorH
              << ",Vertical sampFactor:" << (int)sampFactorV;
    std::cout << ",destinationSelector:"
              << (int)imageComponent[i].destinationSelector << std::endl;
    if ((imageComponent[i].sampFactor >> 4) != 1 ||
        (imageComponent[i].sampFactor & 0xf) != 1)
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

int mark::SOF0::package(ofstream &outputFile) {
  const uint8_t comCount = 3;

  header.len = sizeof(header) - 2;
  header.precision = 8;
  header.imgWidth = htons(512);
  header.imgHeight = htons(512);
  header.imageComponentCount = comCount;
  if (comCount > 0)
    header.len = htons(header.len + comCount * sizeof(ImageComponent));

  uint8_t tmp[sizeof(header)] = {0};
  memcpy(tmp, &header, sizeof(header));
  outputFile.write((const char *)tmp, sizeof(header));

  ImageComponent imageComponent[comCount];

  for (int i = 0; i < comCount; i++) {
    imageComponent[i].componentIdentifier = i + 1;
    imageComponent[i].sampFactor = 0b0001'0001;
    imageComponent[i].destinationSelector = 1;
  }

  uint8_t tmp2[comCount * sizeof(ImageComponent)] = {0};
  memcpy(tmp2, imageComponent, comCount * sizeof(ImageComponent));
  outputFile.write((const char *)tmp2, comCount * sizeof(ImageComponent));

  return 0;
};
