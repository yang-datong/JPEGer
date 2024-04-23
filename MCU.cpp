#include "MCU.hpp"
#include <cstdint>

int MCU::_DCDiff[3] = {0, 0, 0};
int MCU::_MCUCount = 0;

MCU::MCU(array<vector<int>, 3> RLE, vector<vector<uint16_t>> qTables)
    : _RLE(RLE), _QTables(qTables) {
  _MCUCount++;
  _order = _MCUCount;

  arrayToMatrixUseZigZag();
  startIDCT();
  if (gOutputFileType != OutputFileType::YUV)
    YUVToRGB();
}

void MCU::arrayToMatrixUseZigZag() {
  // std::cout << "Performing Array to Matrix: " << _order << "..." <<
  // std::endl;
  for (int imageComponent = 0; imageComponent < 3; imageComponent++) {
    array<int, MCU_UNIT_SIZE> zzOrder = {0};

    /*1. 对AC系数进行RLC解码 */
    int indexAC = -1; // AC系数（非第一个系数）
    for (int i = 0; i <= (int)_RLE[imageComponent].size() - 2; i += 2) {
      /* RLE编码是成对的：第一个值表示0的数量，第二个值是非零值。如果一对值是（0,0），表示这一行程长度编码结束*/
      if (_RLE[imageComponent][i] == 0 && _RLE[imageComponent][i + 1] == 0)
        break;
      indexAC += _RLE[imageComponent][i] + 1;
      zzOrder[indexAC] = _RLE[imageComponent][i + 1];
    }

    /*2. 对DC系数进行差分解码*/
    int &DC = zzOrder[0]; // DC系数（第一个系数，代表块的平均值）
    _DCDiff[imageComponent] += DC;
    DC = _DCDiff[imageComponent];
    //第一次将zzOrder0的值与前一个DC系数的差分值相加*/

    /*反量化：根据Y分量，Cb,Cr分量使用不同的量化表*/
    int QIndex = imageComponent == 0 ? 0 : 1;
    for (auto i = 0; i < MCU_UNIT_SIZE; ++i)
      zzOrder[i] *= _QTables[QIndex][i];

    /* 按Zig-Zag顺序转换回8x8的矩阵 */
    arrayToMatrixUseZigZag(zzOrder, _matrix[imageComponent]);
  }
}

void MCU::arrayToMatrixUseZigZag(
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
  // printMatrix(matrix);
}

inline void MCU::printMatrix(
    const array<array<int, COMPONENT_SIZE>, COMPONENT_SIZE> matrix) {
  int rows = COMPONENT_SIZE, cols = COMPONENT_SIZE;
  for (int i = 0; i < rows; ++i) {
    std::cout << "|";
    for (int j = 0; j < cols; ++j)
      std::cout << setw(4) << matrix[i][j];
    std::cout << "|" << std::endl;
  }
}

inline void MCU::startIDCT() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    for (int y = 0; y < COMPONENT_SIZE; ++y) {
      for (int x = 0; x < COMPONENT_SIZE; ++x) {
        float sum = 0.0;

        for (int u = 0; u < COMPONENT_SIZE; ++u) {
          for (int v = 0; v < COMPONENT_SIZE; ++v) {
            float Cu = u == 0 ? 1.0 / sqrt(2.0) : 1.0;
            float Cv = v == 0 ? 1.0 / sqrt(2.0) : 1.0;

            sum += Cu * Cv * _matrix[imageComponent][u][v] *
                   cos((2 * x + 1) * u * M_PI / 16.0) *
                   cos((2 * y + 1) * v * M_PI / 16.0);
          }
        }

        _idctCoeffs[imageComponent][x][y] = 1.0 / 4.0 * sum;
      }
    }
  }
  /* 还原数值范围[0,255] -> [-128,127] */
  performLevelShift();
}

/* 反中心化（反级别移位）*/
inline void MCU::performLevelShift() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    for (int y = 0; y < COMPONENT_SIZE; ++y)
      for (int x = 0; x < COMPONENT_SIZE; ++x)
        _matrix[imageComponent][y][x] =
            roundl(_idctCoeffs[imageComponent][y][x]) + 128;
  // 1. 使用 roundl 函数对IDCT变换后的系数进行四舍五入到最近的整数。
  // 2. 向每个四舍五入后的系数加上128，以执行反中心化处理。
}

inline void MCU::YUVToRGB() {
  /* YUV444 packed 表示为三个字节一个像素，而矩阵是8x8个像素，故有8x8x3个字节*/
  for (int y = 0; y < COMPONENT_SIZE; ++y) {
    for (int x = 0; x < COMPONENT_SIZE; ++x) {
      float Y = _matrix[0][y][x];
      float Cb = _matrix[1][y][x];
      float Cr = _matrix[2][y][x];

      int R = (int)floor(Y + 1.402 * (1.0 * Cr - 128));
      int G = (int)floor(Y - 0.34414 * (1.0 * Cb - 128) -
                         0.71414 * (1.0 * Cr - 128));
      int B = (int)floor(Y + 1.772 * (1.0 * Cb - 128));

      R = max(0, min(R, 255));
      G = max(0, min(G, 255));
      B = max(0, min(B, 255));

      _matrix[0][y][x] = R;
      _matrix[1][y][x] = G;
      _matrix[2][y][x] = B;
    }
  }
}
