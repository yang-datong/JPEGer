#include "DHT.hpp"
#include "Type.hpp"
#include <cstdint>

int mark::DHT::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  header.len = bs.readBytes<uint16_t>(2);
  std::cout << "DHT[" << index << "~" << index + header.len << "] --> {"
            << std::endl;
  while (bs.getActualReadSize() + index < index + header.len) {
    /* 1. 首字节的高4位表示Huffman编码表的类型，而低4位表示霍夫曼表的标识符*/
    header.TcTh = bs.readByte();
    /* - Huffman编码表的类型: 0表示DC，1表示AC */
    uint8_t huffmanTableClass = header.TcTh >> 4;
    if (huffmanTableClass == 0)
      std::cout << "\tHuffman table class:DC" << std::endl;
    else if (huffmanTableClass == 1)
      std::cout << "\tHuffman table class:AC" << std::endl;
    /* - 标识符: 编号范围是0-3*/
    uint8_t huffmanTableIdentifier = header.TcTh & 0b1111;
    std::cout << "\tHuffman table destination identifier:"
              << (int)huffmanTableIdentifier << std::endl;

    /* 读取和记录符号计数 */
    int totalHuffmanCodeLen = 0;
    HuffmanTable hfTable =
        _huffmanTable[huffmanTableClass][huffmanTableIdentifier];

    // Huffman-code的个数固定为16个字节，比如：
    // huffmanCodeLen:[0 2 1 3 3 2 4 3 5 5 4 4 0 0 1 125 ]
    // 0:表示第一个索引位置的下对应的容器不能存储编码字符，2：表示这个索引位置下要存放2个字符
    for (int i = 0; i < HUFFMAN_CODE_LENGTH_POSSIBLE; i++) {
      header.huffmanCodeLens[i] = bs.readByte();
      hfTable[i].first = header.huffmanCodeLens[i];
      totalHuffmanCodeLen += header.huffmanCodeLens[i];
    }
    //  std::cout << "\ttotalSymbolCount:" << totalHuffmanCodeLen <<
    //  std::endl;

    /* 按字符长度与对应字符，一一对应存储到本地容器（这并不是Huffman的解码规则，就是为了方便写代码）*/
    int readedSymbols = 0;
    header.huffmanCodes = new uint8_t[totalHuffmanCodeLen];
    /* 根据总长度来读取所有Huffman-codes*/
    for (int i = 0; i < totalHuffmanCodeLen; i++) {
      /* 如果遇到huffmanCodeLen=0，则说明该索引的对应codes容器不要存放Huffman-codes*/
      while (hfTable[readedSymbols].first == 0)
        readedSymbols++;

      /* 直到读取的huffmanCodeLen=0,则表示该索引处可以存放Huffman-codes*/
      header.huffmanCodes[i] = bs.readByte();

      /* 保存读取到的Huffman-code，存储到对应Huffman-code-length的位置，长度与符号一定要对应*/
      hfTable[readedSymbols].second.push_back(header.huffmanCodes[i]);

      /* 读满Huffman-code-length指定长度的Huffman-codes后，开始读取下一个Huffman-code-length*/
      if (hfTable[readedSymbols].first ==
          (int)hfTable[readedSymbols].second.size())
        readedSymbols++;
    }
    /* TODO-YangJing感觉这里直接用hfTable[readedSymbols].second放到header.huffmanCodes也可以<24-04-28-12:48:00>*/
    printHuffmanTable(hfTable);
    /* TODO YangJing HuffmanTree 还是有点没搞懂，需要重新写<24-04-15-17:54:18>*/
    _huffmanTree[huffmanTableClass][huffmanTableIdentifier].buildHuffmanTree(
        hfTable);
  }
  std::cout << "}" << std::endl;
  return 0;
}

void mark::DHT::printHuffmanTable(const HuffmanTable &hf) {
  for (int i = 0; i < HUFFMAN_CODE_LENGTH_POSSIBLE; i++) {
    std::cout << "\thuffmanCodeLen:[";
    std::cout << hf[i].first;
    std::cout << "]";
    std::cout << ",huffmanCodes:{";
    for (const auto &it : hf[i].second) {
      std::cout << (int)it << " ";
    }
    std::cout << "}" << std::endl;
  }
}

int mark::DHT::package(ofstream &outputFile) {

  buildLumaTable(0, 0, outputFile); // Y,DC
  buildLumaTable(1, 0, outputFile); // Y,DC
  buildLumaTable(0, 1, outputFile); // Y,DC
  buildLumaTable(1, 1, outputFile); // Y,DC

  // buildLumaTable(0, 0, outputFile);   // Y,DC
  // buildChromaTable(1, 0, outputFile); // Y,AC
  // buildLumaTable(0, 1, outputFile);   // UV,DC
  // buildChromaTable(1, 1, outputFile); // UV,AC
  /* TODO YangJing  <24-04-28 21:56:26> */
  //_huffmanTree[0].buildHuffmanTree();
  return 0;
};

int mark::DHT::buildLumaTable(uint8_t coefficientType, uint8_t id,
                              ofstream &outputFile) {
  uint8_t TcTh = coefficientType; /* DC or AC */
  TcTh <<= 4;
  TcTh |= id;
  header.TcTh = TcTh;

  int totalSymbolCount = 0;
  for (int i = 0; i < HUFFMAN_CODE_LENGTH_POSSIBLE; i++) {
    header.huffmanCodeLens[i] = i;
    totalSymbolCount += i;
  }

  uint8_t *huffmanCodes = new uint8_t[totalSymbolCount];
  for (int i = 0; i < totalSymbolCount; i++) {
    huffmanCodes[i] = i;
  }
  header.huffmanCodes = huffmanCodes;

  /* 减去一个指针大小，加上后面的HuffmanCodes */
  header.len = htons(sizeof(DHTHeader) - 2 - sizeof(void *) + totalSymbolCount);

  uint8_t tmp[sizeof(header)] = {0};
  memcpy(tmp, &header, sizeof(header));

  // 减去最后的一个指针大小
  outputFile.write((const char *)tmp, sizeof(header) - sizeof(void *));

  outputFile.write((const char *)huffmanCodes, totalSymbolCount);
  return 0;
}

// int mark::DHT::buildChromaTable(uint8_t coefficientType, uint8_t id,
//                                 ofstream &outputFile) {
//   return 0;
// }
