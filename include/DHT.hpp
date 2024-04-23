#ifndef DHT_HPP_GENU2LNE
#define DHT_HPP_GENU2LNE

#include "HuffmanTree.hpp"
#include "Marker.hpp"

namespace mark {
class DHT : public Marker {
 private:
  HuffmanTable _huffmanTable[2][2];
  /* 对应四张表的HuffmanTree */
  HuffmanTree _huffmanTree[2][2];

 public:
  /* ISO/IEC 10918-1 : 1993(E) : page 40  */
  int parse(int index, uint8_t *buf, int bufSize) override;

  /* TODO YangJing 很新奇写法 <24-04-23 11:24:47> */
  const HuffmanTree (*getHuffmanTree() const)[2] { return _huffmanTree; }

 private:
  void printHuffmanTable(const HuffmanTable &hf);
};
} // namespace mark
#endif /* end of include guard: DHT_HPP_GENU2LNE */
