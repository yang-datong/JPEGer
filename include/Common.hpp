#ifndef COMMON_HPP_S7ZCNIWC
#define COMMON_HPP_S7ZCNIWC

#include "Type.hpp"

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

void arrayToMatrixUseZigZag(
    const array<int, MCU_UNIT_SIZE> a,
    array<array<int, COMPONENT_SIZE>, COMPONENT_SIZE> &matrix);

#endif /* end of include guard: COMMON_HPP_S7ZCNIWC */
