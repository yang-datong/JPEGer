#ifndef DHT_HPP_GENU2LNE
#define DHT_HPP_GENU2LNE

#include "HuffmanTree.hpp"
#include "Marker.hpp"

namespace mark {
typedef array<array<HuffmanTree, 2>, 2> HuffmanTrees;
class DHT : public Marker {
 private:
  HuffmanTable _huffmanTable[2][2];
  /* 对应四张表的HuffmanTree */
  HuffmanTrees _huffmanTree;

 public:
  /* ISO/IEC 10918-1 : 1993(E) : page 40  */
  int parse(int index, uint8_t *buf, int bufSize) override;
  int package(uint8_t *&buf, int &bufSize) override;

  const HuffmanTrees &getHuffmanTree() { return _huffmanTree; }

 private:
  void printHuffmanTable(const HuffmanTable &hf);
};
} // namespace mark
#endif /* end of include guard: DHT_HPP_GENU2LNE */
