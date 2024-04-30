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

inline void printMatrix(
    const array<array<uint8_t, COMPONENT_SIZE>, COMPONENT_SIZE> matrix) {
  int rows = COMPONENT_SIZE, cols = COMPONENT_SIZE;
  for (int i = 0; i < rows; ++i) {
    std::cout << "|";
    for (int j = 0; j < cols; ++j)
      std::cout << setw(4) << (int)matrix[i][j];
    std::cout << "|" << std::endl;
  }
}

inline string eraseHeightOfZero(string &binaryString) {
  auto loc = binaryString.find('1');
  if (loc != string::npos)
    return binaryString.substr(loc);
  return "";
}

template <typename T>
int arrayToMatrixUseZigZag(
    const array<T, MCU_UNIT_SIZE> a,
    array<array<T, COMPONENT_SIZE>, COMPONENT_SIZE> &matrix);

template <typename T>
int matrixToArrayUseZigZag(const T matrix[COMPONENT_SIZE][COMPONENT_SIZE],
                           T arr[MCU_UNIT_SIZE]);
template <typename T>
int matrixToArrayUseZigZag(
    const array<array<T, COMPONENT_SIZE>, COMPONENT_SIZE> &matrix,
    array<T, MCU_UNIT_SIZE> &arr);

inline string getFileType(string &fileName) {
  return fileName.substr(fileName.find_last_of('.') + 1);
}

#include "Common.tpp"

#endif /* end of include guard: COMMON_HPP_S7ZCNIWC */
