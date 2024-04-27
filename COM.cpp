#include "COM.hpp"
#include <cstdint>
#include <cstring>
#include <netinet/in.h>

int mark::COM::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  header.len = bs.readBytes<uint16_t>(2);
  std::cout << "COM[" << index << "~" << index + len << "] --> {" << std::endl;
  std::cout << "\tcomment:";
  for (int i = 0; i < header.len - 2; i++)
    std::cout << (char)bs.readByte();
  std::cout << std::endl;
  std::cout << "}" << std::endl;
  return 0;
}

int mark::COM::package(ofstream &outputFile) {
  const char *comment = "From the JEPGer encode - Yangdatong";
  const uint16_t commentLen = strlen(comment);
  header.len = htons(commentLen - 2);
  uint8_t *tmp = new uint8_t[sizeof(header)];
  memcpy(tmp, &header, sizeof(header));
  outputFile.write((const char *)tmp, sizeof(header));
  delete[] tmp;
  tmp = nullptr;
  outputFile.write(comment, commentLen);
  return 0;
};
