#include "QuantizationTable.hpp"

/* 这个函数不参与实际的解码(Option) */
inline void QuantizationTable::printMatrix(int arr[8][8]) {
  int rows = 8, cols = 8;
  for (int i = 0; i < rows; ++i) {
    std::cout << "|";
    for (int j = 0; j < cols; ++j)
      std::cout << std::setw(3) << arr[i][j];
    std::cout << "|" << std::endl;
  }
}

/* 这个函数不参与实际的解码(Option) */
inline void QuantizationTable::encodeZigZag(int a[64], int matrix[8][8]) {
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
