#include "MCU.hpp"
#include "Common.hpp"
#include "Image.hpp"
#include "Type.hpp"
#include <cstdint>

int16_t MCU::_DCDiff[3] = {0, 0, 0};
int MCU::_MCUCount = 0;

MCU::MCU(RLE rle, const vector<QuantizationTable> &qTables)
    : _rle(rle), _qtTables(qTables) {
  _MCUCount++;
  _order = _MCUCount;

  decodeACandDC();
  startIDCT();
  performLevelShift();
  if (Image::sOutputFileType != FileFormat::YUV)
    YUVToRGB();
}

/* 提供进来编码的宏块应该是原始状态的，不应该包含负数像素值 */
MCU::MCU(UCompMatrices &matrix, const vector<QuantizationTable> &qTables)
    : _Umatrix(matrix), _qtTables(qTables) {
  _MCUCount++;
  _order = _MCUCount;

  levelShift();
  startDCT();
  encodeACandDC();
}

void MCU::encodeACandDC() {
  for (int imageComponent = 0; imageComponent < 3; imageComponent++) {
    _zzOrder = {0};
    matrixToArrayUseZigZag(_matrix[imageComponent], _zzOrder);
    int qtIndex = imageComponent == 0 ? HT_Y : HT_CbCr;
    for (int i = 0; i < MCU_UNIT_SIZE; i++) {
      if (_qtTables[qtIndex][i] == 0) {
        std::cerr << "\033[31mFail _qtTables[qtIndex][i] == 0 \033[0m"
                  << std::endl;
        return;
      }
      _zzOrder[i] /= _qtTables[qtIndex][i];
    }
    // DC
    int16_t &DC = _zzOrder[0];
    DC -= _DCDiff[imageComponent];
    _DCDiff[imageComponent] = DC;
    _rle[imageComponent].push_back(DC);
    // AC
    int zeroCount = 0;
    for (int indexAC = 1; indexAC < MCU_UNIT_SIZE; indexAC++) {
      int16_t AC = _zzOrder[indexAC];
      /* 如果最后剩了5个0,则表示为(4,0) */
      if (AC == 0 && indexAC != MCU_UNIT_SIZE)
        zeroCount++;
      else if (AC == 0 && indexAC == MCU_UNIT_SIZE) {
        _rle[imageComponent].push_back(0);
        _rle[imageComponent].push_back(0);
      } else {
        _rle[imageComponent].push_back(zeroCount);
        _rle[imageComponent].push_back(AC);
        zeroCount = 0;
      }
    }
  }
}

void MCU::decodeACandDC() {
  for (int imageComponent = 0; imageComponent < 3; imageComponent++) {
    _zzOrder = {0};
    /* 经过了还未解码的宏块应该是包含负数的，所以应该使用int8_t，而不是uint8_t*/

    /*1. 对DC,AC系数进行RLC解码 */
    int index = 0;
    // 注意index的取值范围是0-64，也就是说DC,AC系数（非第一个系数）都在这里解码得到。
    for (int i = 0; i <= (int)_rle[imageComponent].size() - 2; i += 2) {
      /* RLE编码是成对的：第一个值表示0的数量，第二个值是非零值。如果一对值是（0,0），表示这一行程长度编码结束*/
      if (_rle[imageComponent][i] == 0 && _rle[imageComponent][i + 1] == 0)
        break;
      index += _rle[imageComponent][i];
      //这里是零的个数，也就意味着其值也为0，由于_zzOrder这里初始化为0,故可以直接跳过赋值0的操作
      _zzOrder[index] = _rle[imageComponent][i + 1];
      index++;
    }

    /*2. 对DC系数进行差分解码：除了Y,U,V的第一个是正常值外，后面的值都是差数值*/
    int16_t &DC = _zzOrder[0]; // DC系数（第一个系数，代表块的平均值）
    _DCDiff[imageComponent] += DC;
    DC = _DCDiff[imageComponent];
    //第一次将zzOrder0的值与前一个DC系数的差分值相加*/

    /*反量化：根据Y分量，Cb,Cr分量使用不同的量化表*/
    int qtIndex = imageComponent == 0 ? HT_Y : HT_CbCr;
    for (int i = 0; i < MCU_UNIT_SIZE; i++)
      _zzOrder[i] *= _qtTables[qtIndex][i];

    /* 按Zig-Zag顺序转换回8x8的矩阵 */
    arrayToMatrixUseZigZag(_zzOrder, _matrix[imageComponent]);
    // printMatrix(_matrix[imageComponent]);
  }
}

inline void MCU::startDCT() {
  float sum = 0.0, Cu = 0.0, Cv = 0.0;
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    for (int v = 0; v < COMPONENT_SIZE; ++v) {
      for (int u = 0; u < COMPONENT_SIZE; ++u) {
        sum = 0;
        for (int i = 0; i < COMPONENT_SIZE; ++i) {
          for (int j = 0; j < COMPONENT_SIZE; ++j) {
            sum += _dctCoeffs[imageComponent][i][j] *
                   cos((2 * i + 1) * u * M_PI / 16) *
                   cos((2 * j + 1) * v * M_PI / 16);
          }
        }

        Cu = u == 0 ? 1.0 / sqrt(2.0) : 1.0;
        Cv = v == 0 ? 1.0 / sqrt(2.0) : 1.0;
        _matrix[imageComponent][u][v] = (Cu * Cv) / 4.0 * sum;
      }
    }
  }
}

inline void MCU::startIDCT() {
  float sum = 0.0, Cu = 0.0, Cv = 0.0;
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    for (int j = 0; j < COMPONENT_SIZE; ++j) {
      for (int i = 0; i < COMPONENT_SIZE; ++i) {
        sum = 0.0;
        for (int u = 0; u < COMPONENT_SIZE; ++u) {
          for (int v = 0; v < COMPONENT_SIZE; ++v) {
            Cu = u == 0 ? 1.0 / sqrt(2.0) : 1.0;
            Cv = v == 0 ? 1.0 / sqrt(2.0) : 1.0;

            sum += Cu * Cv * _matrix[imageComponent][u][v] *
                   cos((2 * i + 1) * u * M_PI / 16.0) *
                   cos((2 * j + 1) * v * M_PI / 16.0);
          }
        }

        _idctCoeffs[imageComponent][i][j] = 1.0 / 4.0 * sum;
      }
    }
  }
}

/* 中心化*/
inline void MCU::levelShift() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    for (int y = 0; y < COMPONENT_SIZE; ++y)
      for (int x = 0; x < COMPONENT_SIZE; ++x)
        _dctCoeffs[imageComponent][y][x] = _Umatrix[imageComponent][y][x] - 128;
}

/* 反中心化（反级别移位）*/
inline void MCU::performLevelShift() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    for (int y = 0; y < COMPONENT_SIZE; ++y)
      for (int x = 0; x < COMPONENT_SIZE; ++x)
        _Umatrix[imageComponent][y][x] =
            roundl(_idctCoeffs[imageComponent][y][x]) + 128;
  // 1. 使用 roundl 函数对IDCT变换后的系数进行四舍五入到最近的整数。
  // 2. 向每个四舍五入后的系数加上128，以执行反中心化处理。
}

inline void MCU::YUVToRGB() {
  /* YUV444 packed 表示为三个字节一个像素，而矩阵是8x8个像素，故有8x8x3个字节*/
  for (int y = 0; y < COMPONENT_SIZE; ++y) {
    for (int x = 0; x < COMPONENT_SIZE; ++x) {
      float Y = _Umatrix[0][y][x];
      float Cb = _Umatrix[1][y][x];
      float Cr = _Umatrix[2][y][x];

      int R = (int)floor(Y + 1.402 * (1.0 * Cr - 128));
      int G = (int)floor(Y - 0.34414 * (1.0 * Cb - 128) -
                         0.71414 * (1.0 * Cr - 128));
      int B = (int)floor(Y + 1.772 * (1.0 * Cb - 128));

      R = max(0, min(R, 255));
      G = max(0, min(G, 255));
      B = max(0, min(B, 255));

      _Umatrix[0][y][x] = R;
      _Umatrix[1][y][x] = G;
      _Umatrix[2][y][x] = B;
    }
  }
}
