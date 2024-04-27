#include "DQT.hpp"
#include "Type.hpp"

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
        header.element[k] = bs.readByte();
        _quantizationTables[identifier].push_back(header.element[k]);
      } else if (precision == 1) {
        /* TODO*YangJing可能会有问题，这里的字节大小为uint16_t（需要分情况处理，暂时先不做吧，都按1字节处理，其实2字节的情况很少）<24-04-27-16:39:05>*/
        // header.element[k] = (uint16_t)bs.readByte();
        header.element[k] = bs.readByte();
        _quantizationTables[identifier].push_back(header.element[k]);
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
  int arr[QUANTIZATION_TAB_SIZE] = {0};
  for (int i = 0; i < QUANTIZATION_TAB_SIZE; i++)
    arr[i] = quantizationTable[i];

  array<int, MCU_UNIT_SIZE> a;
  copy(arr, arr + QUANTIZATION_TAB_SIZE, a.begin());
  array<array<int, COMPONENT_SIZE>, COMPONENT_SIZE> matrix;
  arrayToMatrixUseZigZag(a, matrix);
  printMatrix(matrix);
}

#define QUANTIZATION_FLOAT 100
int mark::DQT::package(ofstream &outputFile) {
  int ret = -1;
  ret |= buildLumaDQTable(outputFile);
  ret |= buildChromaDQTable(outputFile);
  if (ret)
    return -1;
  return 0;
};

int mark::DQT::buildLumaDQTable(ofstream &outputFile) {
  header.len = htons(sizeof(header) - 2);

  uint8_t precision = 0; // 8 bit
  // uint8_t precision = 1; //16 bit
  uint8_t identifier = 0;

  header.PqTq = precision;
  header.PqTq <<= 4;
  header.PqTq |= identifier;

  float alpha = 2.0f - QUANTIZATION_FLOAT / 50.0f;
  for (int i = 0; i < QUANTIZATION_TAB_SIZE; i++) {
    float tmp = _lumaTable[i] * alpha;
    if (tmp < 1)
      tmp = 1;
    else if (tmp > 255)
      tmp = 255;
    header.element[i] = tmp;
  }

  uint8_t tmp[sizeof(header)] = {0};
  memcpy(tmp, &header, sizeof(header));
  outputFile.write((const char *)tmp, sizeof(header));
  return 0;
}

int mark::DQT::buildChromaDQTable(ofstream &outputFile) {
  header.len = htons(sizeof(header) - 2);

  uint8_t precision = 0; // 8 bit
  // uint8_t precision = 1; //16 bit
  uint8_t identifier = 1;

  header.PqTq = precision;
  header.PqTq <<= 4;
  header.PqTq |= identifier;

  float alpha = 2.0f - QUANTIZATION_FLOAT / 50.0f;
  for (int i = 0; i < QUANTIZATION_TAB_SIZE; i++) {
    float tmp = _chromaTable[i] * alpha;
    if (tmp < 1)
      tmp = 1;
    else if (tmp > 255)
      tmp = 255;
    header.element[i] = tmp;
  }

  uint8_t tmp[sizeof(header)] = {0};
  memcpy(tmp, &header, sizeof(header));
  outputFile.write((const char *)tmp, sizeof(header));
  return 0;
}
