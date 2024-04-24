#include "APP0.hpp"
#include <cstdint>
#include <netinet/in.h>
#include <vector>

int mark::APP0::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  len = bs.readBytes<uint16_t>(2);
  std::cout << "APP0[" << index << "~" << len + index << "] --> {" << std::endl;
  for (int i = 0; i < (int)sizeof(identifier); i++)
    identifier[i] = bs.readByte();

  std::cout << "\tidentifier{" << identifier[0] << "," << identifier[1] << ","
            << identifier[2] << "," << identifier[3] << "," << identifier[4]
            << "}" << std::endl;

  for (int i = 0; i < (int)sizeof(jfifVersion); i++)
    jfifVersion[i] = bs.readByte();

  std::cout << "\tJFIFVersion:" << (int)jfifVersion[0] << "."
            << (int)jfifVersion[1] << endl;

  unit = bs.readByte();
  if (unit == 0)
    std::cout << "\tdensity unit: 无单位" << std::endl;
  else if (unit == 1)
    std::cout << "\tdensity unit: 每英寸像素" << std::endl;
  else if (unit == 2)
    std::cout << "\tdensity unit: 每厘米像素" << std::endl;

  Xdensity = bs.readBytes<uint16_t>(2);
  std::cout << "\tXdensity:" << Xdensity << std::endl;
  Ydensity = bs.readBytes<uint16_t>(2);
  std::cout << "\tYdensity:" << Ydensity << std::endl;

  Xthumbnail = bs.readByte();
  std::cout << "\tXthumbnail:" << (int)Xthumbnail << std::endl;
  Ythumbnail = bs.readByte();
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

int mark::APP0::package(uint8_t *&buf, int &bufSize) {
  /* TODO YangJing 思考用vector会不会更加方便 <24-04-24 23:54:44> */
  //  vector<uint8_t> pack;
  uint8_t APP0[2] = {0xff, JFIF::APP0};
  identifier[0] = 0x4a;
  identifier[1] = 0x46;
  identifier[2] = 0x49;
  identifier[3] = 0x46;
  identifier[4] = 0;

  jfifVersion[0] = 1;
  jfifVersion[1] = 1;
  unit = 1;

  /* 对于二个字节以上的，需要转换大小端 */
  Xdensity = htons(72);
  Ydensity = htons(72);
  Xthumbnail = 0;
  Ythumbnail = 0;

  len = sizeof(APP0) + sizeof(identifier) + sizeof(jfifVersion) + sizeof(unit) +
        sizeof(Xdensity) + sizeof(Ydensity) + sizeof(Xthumbnail) +
        sizeof(Ythumbnail);

  bufSize = len + sizeof(len);
  // 创建足够大的缓冲区
  uint8_t *buffer = new uint8_t[bufSize];

  // 指向缓冲区当前写入位置的指针
  uint8_t *p = buffer;

  // 将数据按顺序复制到缓冲区
  memcpy(p, APP0, sizeof(APP0));
  p += sizeof(APP0);
  len = htons(len);
  memcpy(p, &len, sizeof(len));
  p += sizeof(len);
  memcpy(p, identifier, sizeof(identifier));
  p += sizeof(identifier);
  memcpy(p, jfifVersion, sizeof(jfifVersion));
  p += sizeof(jfifVersion);
  memcpy(p, &unit, sizeof(unit));
  p += sizeof(unit);
  memcpy(p, &Xdensity, sizeof(Xdensity));
  p += sizeof(Xdensity);
  memcpy(p, &Ydensity, sizeof(Ydensity));
  p += sizeof(Ydensity);
  memcpy(p, &Xthumbnail, sizeof(Xthumbnail));
  p += sizeof(Xthumbnail);
  memcpy(p, &Ythumbnail, sizeof(Ythumbnail));
  p += sizeof(Ythumbnail);

  buf = buffer;
  p = nullptr;
  return 0;
}
