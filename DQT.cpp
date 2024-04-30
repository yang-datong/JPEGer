#include "DQT.hpp"
#include "Common.hpp"
#include "Type.hpp"
#include <cstdint>

int mark::DQT::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  header.len = bs.readBytes<uint16_t>(2);
  std::cout << "DQT[" << index << "~" << index + header.len << "] --> {"
            << std::endl;

  /* 1. 应对多个量化表的情况(总长度需要+ 1个字节的PqTq） */
  for (int i = 0; i < header.len / (QUANTIZATION_TAB_SIZE + 1); i++) {
    /* 2. 首字节的高4位表示精度，而低4位表示量化表的编号*/
    header.PqTq = bs.readByte();
    /* - 量化表元素精度：值0表示8位Qk值；值1表示16位Qk值*/
    uint8_t precision = header.PqTq >> 4;
    if (precision == 0)
      std::cout << "\tprecision: 8 bit" << std::endl;
    else if (precision == 1)
      std::cout << "\tprecision: 16 bit" << std::endl;

    /* - 量化表目标标识符：指定解码器上应安装量化表的四个可能目标之一*/
    uint8_t identifier = header.PqTq & 0b1111;
    std::cout << "\tidentifier:" << (int)identifier << std::endl;
    /* 这里的QTtableNumber是按照实际的量化表个数来的，比如：
     * - 一个JPEG文件中有2个量化表那么编号会依次为0,1
     * - 一个JPEG文件中有3个量化表那么编号会依次为0,1,2
     * - 最多为四个量化表
     * */

    _quantizationTables.push_back({}); /* 先申请空间 */
    /* 指定64个元素中的第k个元素，其中k是DCT系数的锯齿形排序中的索引。量化元素应以之字形扫描顺序指定*/
    for (int k = 0; k < QUANTIZATION_TAB_SIZE; k++) {
      /* element的大小可能是8、16*/
      if (precision == 0) {
        header.element8[k] = bs.readByte();
        _quantizationTables[identifier].push_back(header.element8[k]);
      } else if (precision == 1) {
        header.element16[k] = bs.readBytes<uint16_t>(2);
        _quantizationTables[identifier].push_back(header.element16[k]);
      }
    }
    printQuantizationTable(_quantizationTables[identifier]);
  }
  /* 基于 8 位 DCT 的过程不得使用 16 位精度量化表。 */
  std::cout << "}" << std::endl;
  return 0;
}

/* 这个函数不参与实际的解码(Option) */
void mark::DQT::printQuantizationTable(QuantizationTable quantizationTable) {
  int tmp[QUANTIZATION_TAB_SIZE] = {0};
  for (int i = 0; i < QUANTIZATION_TAB_SIZE; i++)
    tmp[i] = quantizationTable[i];

  array<uint8_t, MCU_UNIT_SIZE> arr;
  copy(tmp, tmp + QUANTIZATION_TAB_SIZE, arr.begin());
  array<array<uint8_t, COMPONENT_SIZE>, COMPONENT_SIZE> matrix;
  arrayToMatrixUseZigZag(arr, matrix);
  printMatrix(matrix);
}

int mark::DQT::package(ofstream &outputFile) {
  int ret = -1;
  ret |= buildDQTable(0, LumaTable, outputFile);
  ret |= buildDQTable(1, ChromaTable, outputFile);
  /* YangJing 这里不能乱序，必须是0,然后是1 <24-04-30 17:00:23> */
  if (ret)
    return -1;
  return 0;
};

int mark::DQT::buildDQTable(
    const uint8_t id,
    const uint8_t (&componentTab)[COMPONENT_SIZE][COMPONENT_SIZE],
    ofstream &outputFile) {

  uint8_t precision = 0; // 8 bit
  // uint8_t precision = 1; //16 bit
  uint8_t identifier = id;

  header.PqTq = precision;
  header.PqTq <<= 4;
  header.PqTq |= identifier;

  uint8_t matrix[COMPONENT_SIZE][COMPONENT_SIZE];
  float alpha = 2.0f - QUANTIZATION_FLOAT / 50.0f;
  for (int i = 0; i < COMPONENT_SIZE; i++) {
    for (int j = 0; j < COMPONENT_SIZE; j++) {
      float tmp = componentTab[i][j] * alpha;
      tmp = tmp < 1 ? 1 : tmp > 255 ? 255 : tmp;
      matrix[i][j] = tmp;
    }
  }
  if (precision == 0) {
    matrixToArrayUseZigZag(matrix, header.element8);
    vector<uint16_t> vec(begin(header.element8), end(header.element8));
    _quantizationTables.push_back({}); /* 先申请空间 */
    _quantizationTables[id] = vec;
    int writeSize = (int)(sizeof(header) - sizeof(uint16_t) * MCU_UNIT_SIZE);
    header.len = htons(writeSize - 2);
    uint8_t *tmp = new uint8_t[writeSize];
    memcpy(tmp, &header, writeSize);
    outputFile.write((const char *)tmp, writeSize);
    SAFE_DELETE_ARRAY(tmp);
  } else if (precision == 1) {
    std::cerr << "\033[31mNo support precision -> 1\033[0m" << std::endl;
    return -1;
  }
  return 0;
}
