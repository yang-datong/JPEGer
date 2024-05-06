#include "DHT.hpp"
#include "Type.hpp"
#include <cstdint>
#include <vector>

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
  buildHuffmanTableAndTree(HT_DC, HT_Y, HuffmanLumaDCLenTable,
                           HuffmanLumaDCValueTable,
                           outputFile); // DC,Y
  buildHuffmanTableAndTree(HT_AC, HT_Y, HuffmanLumaACLenTable,
                           HuffmanLumaACValueTable,
                           outputFile); // AC,Y

  buildHuffmanTableAndTree(HT_DC, HT_CbCr, HuffmanChromaDCLenTable,
                           HuffmanChromaDCValueTable,
                           outputFile); // DC,UV
  buildHuffmanTableAndTree(HT_AC, HT_CbCr, HuffmanChromaACLenTable,
                           HuffmanChromaACValueTable,
                           outputFile); // AC,UV
  return 0;
};

int mark::DHT::buildHuffmanTableAndTree(uint8_t coefficientType, uint8_t id,
                                        const uint8_t *buildHuffmanCodeLens,
                                        const uint8_t *buildHuffmanCodes,
                                        ofstream &outputFile) {
  uint8_t TcTh = coefficientType; /* DC or AC */
  TcTh <<= 4;
  TcTh |= id;
  header.TcTh = TcTh;
  int totalSymbolCount = 0;
  int index = 0;
  for (int i = 0; i < HUFFMAN_CODE_LENGTH_POSSIBLE; i++) {
    int len = buildHuffmanCodeLens[i];
    /* 一份将要写入到JPEG文件中 */
    header.huffmanCodeLens[i] = len;
    /* 一份保存到类中，用于构建Huffmen Tree */
    _huffmanTable[coefficientType][id][i].first = len;
    for (int j = 0; j < buildHuffmanCodeLens[i]; j++)
      _huffmanTable[coefficientType][id][i].second.push_back(
          buildHuffmanCodes[index++]);

    /* 一份用于统计 */
    totalSymbolCount += len;
  }

  uint8_t *huffmanCodes = new uint8_t[totalSymbolCount];
  for (int i = 0; i < totalSymbolCount; i++)
    huffmanCodes[i] = buildHuffmanCodes[i];
  header.huffmanCodes = huffmanCodes;

  /* 减去一个指针大小，加上后面的HuffmanCodes */
  header.len = htons(sizeof(DHTHeader) - 2 - sizeof(void *) + totalSymbolCount);

  uint8_t tmp[sizeof(header)] = {0};
  memcpy(tmp, &header, sizeof(header));

  // 减去最后的一个指针大小
  outputFile.write((const char *)tmp, sizeof(header) - sizeof(void *));
  outputFile.write((const char *)huffmanCodes, totalSymbolCount);

  /* 构建Huffman Tree用于编码数据 */
  _huffmanTree[coefficientType][id].buildHuffmanTree(
      _huffmanTable[coefficientType][id]);
  return 0;
}
