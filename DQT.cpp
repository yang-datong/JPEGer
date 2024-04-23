#include "DQT.hpp"

int mark::DQT::parse(int index, uint8_t *buf, int bufSize) {
  ByteStream bs(buf + index, bufSize - index);
  uint16_t len = bs.readBytes<uint16_t>(2);
  std::cout << "DQT[" << index << "~" << index + len << "] --> {" << std::endl;

  /* 1. 应对多个量化表的情况(总长度需要+ 1个字节的PqTq） */
  for (int i = 0; i < len / (QUANTIZATION_TAB_SIZE + 1); i++) {
    /* 2. 首字节的高4位表示精度，而低4位表示量化表的编号*/
    uint8_t PqTq = bs.readByte();
    /* - 量化表元素精度：值0表示8位Qk值；值1表示16位Qk值*/
    uint8_t precision = PqTq >> 4;
    if (precision == 0)
      std::cout << "\tprecision: 8 bit" << std::endl;
    else if (precision == 1)
      std::cout << "\tprecision: 16 bit" << std::endl;

    /* - 量化表目标标识符：指定解码器上应安装量化表的四个可能目标之一*/
    uint8_t identifier = PqTq & 0b1111;
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
        uint8_t element = bs.readByte();
        _quantizationTables[identifier].push_back(element);
      } else if (precision == 1) {
        uint16_t element = (uint16_t)bs.readByte();
        _quantizationTables[identifier].push_back(element);
      }
    }
    printQuantizationTable(_quantizationTables[identifier]);
  }
  /* 基于 8 位 DCT 的过程不得使用 16 位精度量化表。 */
  std::cout << "}" << std::endl;
  return 0;
}

/* 这个函数不参与实际的解码(Option) */
inline void mark::DQT::printMatrix(int arr[8][8]) {
  int rows = 8, cols = 8;
  for (int i = 0; i < rows; ++i) {
    std::cout << "\t|";
    for (int j = 0; j < cols; ++j)
      std::cout << std::setw(3) << arr[i][j];
    std::cout << "|" << std::endl;
  }
}

/* 这个函数不参与实际的解码(Option) */
inline void mark::DQT::encodeZigZag(int a[64], int matrix[8][8]) {
  int row = 0, col = 0;
  for (int i = 0; i < 64; ++i) {
    matrix[row][col] = a[i];
    // 奇数斜线往上
    if ((row + col) % 2 == 0) {
      if (col == 7)
        row++;
      else if (row == 0)
        col++;
      else {
        row--;
        col++;
      }
    }
    // 偶数斜线往下
    else {
      if (row == 7)
        col++;
      else if (col == 0)
        row++;
      else {
        col--;
        row++;
      }
    }
  }
}

/* 这个函数不参与实际的解码(Option) */
void mark::DQT::printQuantizationTable(vector<uint16_t> quantizationTable) {
  int arr[QUANTIZATION_TAB_SIZE] = {0};
  for (int i = 0; i < QUANTIZATION_TAB_SIZE; i++)
    arr[i] = quantizationTable[i];

  int matrix[8][8];
  encodeZigZag(arr, matrix);
  printMatrix(matrix);
}
