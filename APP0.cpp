#include "APP0.hpp"
#include <cstdint>
#include <cstring>

int mark::APP0::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  header.len = bs.readBytes<uint16_t>(2);
  std::cout << "APP0[" << index << "~" << header.len + index << "] --> {"
            << std::endl;
  for (int i = 0; i < (int)sizeof(header.identifier); i++)
    header.identifier[i] = bs.readByte();

  std::cout << "\theader.identifier{" << header.identifier[0] << ","
            << header.identifier[1] << "," << header.identifier[2] << ","
            << header.identifier[3] << "," << header.identifier[4] << "}"
            << std::endl;

  for (int i = 0; i < (int)sizeof(header.jfifVersion); i++)
    header.jfifVersion[i] = bs.readByte();

  std::cout << "\tJFIFVersion:" << (int)header.jfifVersion[0] << ".0"
            << (int)header.jfifVersion[1] << endl;

  header.unit = bs.readByte();
  if (header.unit == 0)
    std::cout << "\tdensity header.unit: 无单位" << std::endl;
  else if (header.unit == 1)
    std::cout << "\tdensity header.unit: 每英寸像素" << std::endl;
  else if (header.unit == 2)
    std::cout << "\tdensity header.unit: 每厘米像素" << std::endl;

  header.Xdensity = bs.readBytes<uint16_t>(2);
  std::cout << "\tXdensity:" << header.Xdensity << std::endl;
  header.Ydensity = bs.readBytes<uint16_t>(2);
  std::cout << "\tYdensity:" << header.Ydensity << std::endl;

  header.Xthumbnail = bs.readByte();
  std::cout << "\tXthumbnail:" << (int)header.Xthumbnail << std::endl;
  header.Ythumbnail = bs.readByte();
  std::cout << "\tYthumbnail:" << (int)header.Ythumbnail << std::endl;

  int remain = header.len - bs.getActualReadSize();
  uint8_t thumbnailImage[remain];
  std::cout << "\tremain:" << remain << std::endl;
  if (remain > 0) {
    for (int i = 0; i < bs.getActualReadSize(); i++) {
      thumbnailImage[i] = bs.readByte();
      printf("%d,", thumbnailImage[i]);
    }
    printf("\n");
  }
  std::cout << "}" << std::endl;
  return 0;
}

int mark::APP0::package(ofstream &outputFile) {
  header.len = htons(sizeof(header) - 2);

  header.identifier[0] = 0x4a;
  header.identifier[1] = 0x46;
  header.identifier[2] = 0x49;
  header.identifier[3] = 0x46;
  header.identifier[4] = 0;

  header.jfifVersion[0] = 1;
  header.jfifVersion[1] = 1;
  header.unit = 1;

  header.Xdensity = htons(72);
  header.Ydensity = htons(72);
  header.Xthumbnail = 0;
  header.Ythumbnail = 0;

  uint8_t tmp[sizeof(header)] = {0};
  memcpy(tmp, &header, sizeof(header));
  outputFile.write((const char *)tmp, sizeof(header));
  return 0;
}
