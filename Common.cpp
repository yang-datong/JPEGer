#include "Common.hpp"
#include "Type.hpp"
#include <cstdint>

int arrayToMatrixUseZigZag(
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
  return 0;
}

int matrixToArrayUseZigZag(const uint8_t matrix[COMPONENT_SIZE][COMPONENT_SIZE],
                           uint8_t arr[MCU_UNIT_SIZE]) {
  // Zig-Zag解码
  int index = 0; // 一维数组的索引
  int row = 0, col = 0;
  for (int i = 0; i < 64; ++i) {
    arr[index++] = matrix[row][col];
    // 向上行走
    if ((row + col) % 2 == 0) {
      if (col == 7) { // 到最右边了，只能向下移动
        row++;
      } else if (row == 0) { // 到最顶了，只能向右移动
        col++;
      } else { // 没到边界，继续向右上方移动
        row--;
        col++;
      }
    }
    // 向下行走
    else {
      if (row == 7) { // 到最底部了，只能向右移动
        col++;
      } else if (col == 0) { // 到最左边了，只能向下移动
        row++;
      } else { // 没到边界，继续向左下方移动
        col--;
        row++;
      }
    }
  }
  return 0;
}

/* TODO YangJing 和上面的函数一模一样，需要再思考优化 <24-04-30 16:09:04> */
int matrixToArrayUseZigZag(
    const array<array<int, COMPONENT_SIZE>, COMPONENT_SIZE> &matrix,
    array<int, MCU_UNIT_SIZE> &arr) {
  // Zig-Zag解码
  int index = 0; // 一维数组的索引
  int row = 0, col = 0;
  for (int i = 0; i < 64; ++i) {
    arr[index++] = matrix[row][col];
    // 向上行走
    if ((row + col) % 2 == 0) {
      if (col == 7) { // 到最右边了，只能向下移动
        row++;
      } else if (row == 0) { // 到最顶了，只能向右移动
        col++;
      } else { // 没到边界，继续向右上方移动
        row--;
        col++;
      }
    }
    // 向下行走
    else {
      if (row == 7) { // 到最底部了，只能向右移动
        col++;
      } else if (col == 0) { // 到最左边了，只能向下移动
        row++;
      } else { // 没到边界，继续向左下方移动
        col--;
        row++;
      }
    }
  }
  return 0;
}
