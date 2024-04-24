#include "DHT.hpp"

int mark::DHT::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  uint16_t len = bs.readBytes<uint16_t>(2);
  std::cout << "DHT[" << index << "~" << index + len << "] --> {" << std::endl;
  while (bs.getActualReadSize() + index < index + len) {
    /* 1. 首字节的高4位表示Huffman编码表的类型，而低4位表示霍夫曼表的标识符*/
    uint8_t TcTh = bs.readByte();
    /* - Huffman编码表的类型: 0表示DC，1表示AC */
    uint8_t huffmanTableClass = TcTh >> 4;
    if (huffmanTableClass == 0)
      std::cout << "\tHuffman table class:DC" << std::endl;
    else if (huffmanTableClass == 1)
      std::cout << "\tHuffman table class:AC" << std::endl;
    /* - 标识符: 编号范围是0-3*/
    uint8_t huffmanTableIdentifier = TcTh & 0b1111;
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
      uint8_t huffmanCodeLen = bs.readByte();
      hfTable[i].first = huffmanCodeLen;
      totalHuffmanCodeLen += huffmanCodeLen;
    }
    //  std::cout << "\ttotalSymbolCount:" << totalHuffmanCodeLen <<
    //  std::endl;

    /* 按字符长度与对应字符，一一对应存储到本地容器（这并不是Huffman的解码规则，就是为了方便写代码）*/
    int readedSymbols = 0;
    /* 根据总长度来读取所有Huffman-codes*/
    for (int i = 0; i < totalHuffmanCodeLen; i++) {
      /* 如果遇到huffmanCodeLen=0，则说明该索引的对应codes容器不要存放Huffman-codes*/
      while (hfTable[readedSymbols].first == 0)
        readedSymbols++;

      /* 直到读取的huffmanCodeLen=0,则表示该索引处可以存放Huffman-codes*/
      uint8_t huffmanCode = bs.readByte();

      /* 保存读取到的Huffman-code，存储到对应Huffman-code-length的位置，长度与符号一定要对应*/
      hfTable[readedSymbols].second.push_back(huffmanCode);

      /* 读满Huffman-code-length指定长度的Huffman-codes后，开始读取下一个Huffman-code-length*/
      if (hfTable[readedSymbols].first ==
          (int)hfTable[readedSymbols].second.size())
        readedSymbols++;
    }
    printHuffmanTable(hfTable);
    /* TODO YangJing HuffmanTree 还是有点没搞懂，需要重新写<24-04-15-17:54:18>*/
    _huffmanTree[huffmanTableClass][huffmanTableIdentifier].builedHuffmanTree(
        hfTable);
  }
  std::cout << "}" << std::endl;
  return 0;
}

int mark::DHT::package(uint8_t *&buf, int &bufSize) { return 0; };

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
