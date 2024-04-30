#ifndef COMMON_HPP_S7ZCNIWC
#define COMMON_HPP_S7ZCNIWC

#include "Type.hpp"
#include <cstdint>

#define SAFE_DELETE(p)                                                         \
  do {                                                                         \
    if (p) {                                                                   \
      delete p;                                                                \
      p = nullptr;                                                             \
    }                                                                          \
  } while (0)

#define SAFE_DELETE_ARRAY(p)                                                   \
  do {                                                                         \
    if (p) {                                                                   \
      delete[] p;                                                              \
      p = nullptr;                                                             \
    }                                                                          \
  } while (0)

#define RET(ret, str)                                                          \
  if (ret) {                                                                   \
    std::cerr << "\033[31m" << str << "\033[0m" << std::endl;                  \
    return -1;                                                                 \
  }

inline const bool checkSpace(const string &value) {
  if (value.empty())
    return false;
  for (const auto &it : value) {
    if (iscntrl(it) || isblank(it) || isspace(it))
      return false;
  }
  return true;
}

inline void
printMatrix(const array<array<int, COMPONENT_SIZE>, COMPONENT_SIZE> matrix) {
  int rows = COMPONENT_SIZE, cols = COMPONENT_SIZE;
  for (int i = 0; i < rows; ++i) {
    std::cout << "|";
    for (int j = 0; j < cols; ++j)
      std::cout << setw(4) << matrix[i][j];
    std::cout << "|" << std::endl;
  }
}

/* TODO YangJing 这里有点混乱，我认为应该都使用uint8_t <24-04-30 16:07:41> */
int arrayToMatrixUseZigZag(
    const array<int, MCU_UNIT_SIZE> arr,
    array<array<int, COMPONENT_SIZE>, COMPONENT_SIZE> &matrix);

int matrixToArrayUseZigZag(const uint8_t matrix[COMPONENT_SIZE][COMPONENT_SIZE],
                           uint8_t arr[MCU_UNIT_SIZE]);

int matrixToArrayUseZigZag(
    const array<array<int, COMPONENT_SIZE>, COMPONENT_SIZE> &matrix,
    array<int, MCU_UNIT_SIZE> &arr);
/* TODO YangJing 这里有点混乱，我认为应该都使用uint8_t <24-04-30 16:07:41> */

inline string getFileType(string &fileName) {
  return fileName.substr(fileName.find_last_of('.') + 1);
}
#endif /* end of include guard: COMMON_HPP_S7ZCNIWC */
