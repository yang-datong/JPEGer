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

  bool isNonSampled = false;
  std::cout << "\timageComponent{" << std::endl;

  ImageComponent *imageComponent =
      new ImageComponent[header.imageComponentCount];
  uint8_t sampFactorH[8] = {0}, sampFactorV[8] = {0};
  uint8_t sampFactorHMax = 1, sampFactorVMax = 1;
  for (int i = 0; i < header.imageComponentCount; i++) {
    imageComponent[i].componentIdentifier = bs.readByte();
    imageComponent[i].sampFactor = bs.readByte();
    sampFactorH[i] = imageComponent[i].sampFactor >> 4;
    sampFactorV[i] = imageComponent[i].sampFactor & 0b1111;
    if (sampFactorH[i] > sampFactorHMax) sampFactorHMax = sampFactorH[i];
    if (sampFactorV[i] > sampFactorVMax) sampFactorVMax = sampFactorV[i];
    imageComponent[i].destinationSelector = bs.readByte();
    std::cout << "\t\tcomponentIdentifier:"
              << (int)imageComponent[i].componentIdentifier;
    std::cout << ",Horizontal sampFactor:" << (int)sampFactorH[i]
              << ",Vertical sampFactor:" << (int)sampFactorV[i];
    std::cout << ",destinationSelector:"
              << (int)imageComponent[i].destinationSelector << std::endl;
    if ((imageComponent[i].sampFactor >> 4) != 1 ||
        (imageComponent[i].sampFactor & 0xf) != 1)
      isNonSampled = false;
  }
  // NOTE: From ffmpeg/libavcode/mjpegdec.c/500 line
  int pix_fmt_id = ((unsigned)sampFactorH[0] << 28) | (sampFactorV[0] << 24) |
                   (sampFactorH[1] << 20) | (sampFactorV[1] << 16) |
                   (sampFactorH[2] << 12) | (sampFactorV[2] << 8) |
                   (sampFactorH[3] << 4) | sampFactorV[3];
  if (!(pix_fmt_id & 0xD0D0D0D0)) pix_fmt_id -= (pix_fmt_id & 0xF0F0F0F0) >> 1;
  if (!(pix_fmt_id & 0x0D0D0D0D)) pix_fmt_id -= (pix_fmt_id & 0x0F0F0F0F) >> 1;
  if (pix_fmt_id == 0x11111100) {
    isNonSampled = true;
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
  header.imgWidth = htons(_imgWidth);
  header.imgHeight = htons(_imgHeight);
  header.imageComponentCount = comCount;
  if (comCount > 0)
    header.len = htons(header.len + comCount * sizeof(ImageComponent));

  uint8_t tmp[sizeof(header)] = {0};
  memcpy(tmp, &header, sizeof(header));
  outputFile.write((const char *)tmp, sizeof(header));

  ImageComponent imageComponent[comCount];

  // YUV 444：即sampFactor均为1
  /* Y分量 */
  imageComponent[0].componentIdentifier = 1;
  imageComponent[0].sampFactor = 0b0001'0001;
  imageComponent[0].destinationSelector = 0;
  /* U分量 */
  imageComponent[1].componentIdentifier = 2;
  imageComponent[1].sampFactor = 0b0001'0001;
  imageComponent[1].destinationSelector = 1;
  /* V分量 */
  imageComponent[2].componentIdentifier = 3;
  imageComponent[2].sampFactor = 0b0001'0001;
  imageComponent[2].destinationSelector = 1;

  uint8_t tmp2[comCount * sizeof(ImageComponent)] = {0};
  memcpy(tmp2, imageComponent, comCount * sizeof(ImageComponent));
  outputFile.write((const char *)tmp2, comCount * sizeof(ImageComponent));

  return 0;
};
