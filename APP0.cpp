#include "APP0.hpp"
int mark::APP0::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  uint16_t len = bs.readBytes<uint16_t>(2);
  std::cout << "APP0[" << index << "~" << len + index << "] --> {" << std::endl;
  uint8_t identifier[5] = {0};
  for (int i = 0; i < (int)sizeof(identifier); i++)
    identifier[i] = bs.readByte();

  std::cout << "\tidentifier{" << identifier[0] << "," << identifier[1] << ","
            << identifier[2] << "," << identifier[3] << "," << identifier[4]
            << "}" << std::endl;

  uint8_t jfifVersion[2] = {0};
  for (int i = 0; i < (int)sizeof(jfifVersion); i++)
    jfifVersion[i] = bs.readByte();

  std::cout << "\tJFIFVersion:" << (int)jfifVersion[0] << "."
            << (int)jfifVersion[1] << endl;

  uint8_t unit = bs.readByte();
  if (unit == 0)
    std::cout << "\tdensity unit: 无单位" << std::endl;
  else if (unit == 1)
    std::cout << "\tdensity unit: 每英寸像素" << std::endl;
  else if (unit == 2)
    std::cout << "\tdensity unit: 每厘米像素" << std::endl;

  uint16_t Xdensity = bs.readBytes<uint16_t>(2);
  std::cout << "\tXdensity:" << Xdensity << std::endl;

  uint16_t Ydensity = bs.readBytes<uint16_t>(2);
  std::cout << "\tYdensity:" << Ydensity << std::endl;

  uint8_t Xthumbnail = bs.readByte();
  std::cout << "\tXthumbnail:" << (int)Xthumbnail << std::endl;
  uint8_t Ythumbnail = bs.readByte();
  std::cout << "\tYthumbnail:" << (int)Ythumbnail << std::endl;

  int remain = len - bs.getActualReadSize();
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
