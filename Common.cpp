#include "Common.hpp"

void arrayToMatrixUseZigZag(
    const array<int, MCU_UNIT_SIZE> a,
    array<array<int, COMPONENT_SIZE>, COMPONENT_SIZE> &matrix) {
  int row = 0, col = 0;
  for (int i = 0; i < MCU_UNIT_SIZE; ++i) {
    matrix[row][col] = a[i];
    // 奇数斜线往上
    if ((row + col) % 2 == 0) {
      if (col == COMPONENT_SIZE - 1)
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
      if (row == COMPONENT_SIZE - 1)
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
