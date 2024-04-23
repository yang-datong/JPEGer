#include "COM.hpp"

int mark::COM::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  uint16_t len = bs.readBytes<uint16_t>(2);
  std::cout << "COM[" << index << "~" << index + len << "] --> {" << std::endl;
  std::cout << "\tcomment:";
  for (int i = 0; i < len - 2; i++)
    std::cout << (char)bs.readByte();
  std::cout << std::endl;
  std::cout << "}" << std::endl;
  return 0;
}
