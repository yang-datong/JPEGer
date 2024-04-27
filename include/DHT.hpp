#ifndef DHT_HPP_GENU2LNE
#define DHT_HPP_GENU2LNE

#include "HuffmanTree.hpp"
#include "Marker.hpp"

namespace mark {
/* 这里源文件设计的东西比较多，需要严谨的处理下，我眼睛好困啊，洗澡睡觉去了 TODO
 * <24-04-28 00:04:16, YangJing>  */
// typedef struct __attribute__((packed)) _DHT {
//   uint8_t DHT[2] = {0xff, JFIF::DHT};
//   uint16_t len = 0;
//   uint8_t TcTh = 0;
//   uint8_t identifier[16] = {0};
//   uint8_t jfifVersion[16] = {0};
// } DHTHeader;

typedef array<array<HuffmanTree, 2>, 2> HuffmanTrees;
class DHT : public Marker {
 private:
  HuffmanTable _huffmanTable[2][2];
  /* 对应四张表的HuffmanTree */
  HuffmanTrees _huffmanTree;
  // DHTHeader header;

 public:
  /* ISO/IEC 10918-1 : 1993(E) : page 40  */
  int parse(int index, uint8_t *buf, int bufSize) override;
  int package(ofstream &outputFile) override;

  const HuffmanTrees &getHuffmanTree() { return _huffmanTree; }

 private:
  void printHuffmanTable(const HuffmanTable &hf);
};
} // namespace mark
#endif /* end of include guard: DHT_HPP_GENU2LNE */
