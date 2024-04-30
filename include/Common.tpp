template <typename T>
int arrayToMatrixUseZigZag(
    const array<T, MCU_UNIT_SIZE> a,
    array<array<T, COMPONENT_SIZE>, COMPONENT_SIZE> &matrix) {
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

template <typename T>
int matrixToArrayUseZigZag(const T matrix[COMPONENT_SIZE][COMPONENT_SIZE],
                           T arr[MCU_UNIT_SIZE]) {
  // Zig-Zag解码
  int index = 0; // 一维数组的索引
  int row = 0, col = 0;
  for (int i = 0; i < MCU_UNIT_SIZE; ++i) {
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

template <typename T>
int matrixToArrayUseZigZag(
    const array<array<T, COMPONENT_SIZE>, COMPONENT_SIZE> &matrix,
    array<T, MCU_UNIT_SIZE> &arr) {
  int ret = -1;

  // 创建一个临时的原始数组用于调用主函数
  T tempMatrix[COMPONENT_SIZE][COMPONENT_SIZE];
  for (int i = 0; i < COMPONENT_SIZE; ++i) {
    for (int j = 0; j < COMPONENT_SIZE; ++j) {
      tempMatrix[i][j] = static_cast<T>(matrix[i][j]);
    }
  }
  T tempArr[MCU_UNIT_SIZE];
  // 调用使用原始数组的函数版本
  ret = matrixToArrayUseZigZag(tempMatrix, tempArr);

  for (int i = 0; i < MCU_UNIT_SIZE; ++i)
    arr[i] = tempArr[i];

  return ret;
}
