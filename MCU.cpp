#include "MCU.hpp"
#include "Common.hpp"
#include "Image.hpp"
#include "Type.hpp"

//#define SSE
//#define AVX
#define Threads
//#define Threads_AVX

int16_t MCU::_DCDiff[3] = {0, 0, 0};
int MCU::_MCUCount = 0;

MCU::MCU(RLE rle, const vector<QuantizationTable> &qTables)
    : _rle(rle), _qtTables(qTables) {
  _MCUCount++;
}

/* 提供进来编码的宏块应该是原始状态的，不应该包含负数像素值 */
MCU::MCU(UCompMatrices &matrix, const vector<QuantizationTable> &qTables)
    : _Umatrix(matrix), _qtTables(qTables) {
  _MCUCount++;
}

void MCU::startEncode() {
  // printUmatrix();
  levelShift();
  // printDCTCoeffs();
  startDCT();
  // printMatrix();
  encodeACandDC();
}

void MCU::startDecode() {
  decodeACandDC();
  // printMatrix();
  startIDCT();
  // printIDCTCoeffs();
  performLevelShift();
  // printUmatrix();
  if (Image::sOutputFileType != FileFormat::YUV) Image::YUVToRGB(_Umatrix);
}

void MCU::fillACRLE(int imageComponent) {
  //   AC ,RLE编码
  // std::cout << "AC:";
  int zeroCount = 0;
  for (int indexAC = 1; indexAC < MCU_UNIT_SIZE; indexAC++) {
    int16_t AC = _zzOrder[indexAC];
    //  std::cout << AC << ",";
    if (AC == 0) {
      zeroCount++;
      if (zeroCount == 16) { // 如果已经数了15个零，遇到第16个零
        _rle[imageComponent].push_back(0xf); // Major byte: 15 zeros
        _rle[imageComponent].push_back(0x0); // Minor byte: 0 value
        zeroCount = 0;
      }
    } else {
      _rle[imageComponent].push_back(zeroCount);
      _rle[imageComponent].push_back(AC);
      zeroCount = 0;
    }
    if (indexAC == MCU_UNIT_SIZE - 1 && zeroCount > 0) {
      // 如果是最后一个系数并且之前有零
      _rle[imageComponent].push_back(0x0); // Major byte: 0 zeros
      _rle[imageComponent].push_back(0x0); // Minor byte: EOB
      zeroCount = 0;                       // 重置0值的计数器
    }
  }
  // std::cout << std::endl;
}

/* 这个方式的输出jpeg会更加小 */
void MCU::fillACRLE2(int imageComponent) {
  int lastNotZeroCount = 0;
  for (int indexAC = MCU_UNIT_SIZE - 1; indexAC > 0; indexAC--) {
    if (_zzOrder[indexAC] != 0) {
      lastNotZeroCount = indexAC;
      break;
    }
  }

  int zeroCount = -1;
  for (int indexAC = 1; indexAC <= lastNotZeroCount; indexAC++) {
    zeroCount = 0;
    while (_zzOrder[indexAC] == 0) {
      zeroCount++;
      indexAC++;
      if (zeroCount == 16) {
        _rle[imageComponent].push_back(0xf); // Major byte: 15 zeros
        _rle[imageComponent].push_back(0x0); // Minor byte: 0 value
        zeroCount = 0;
      }
    }
    _rle[imageComponent].push_back(zeroCount);
    _rle[imageComponent].push_back(_zzOrder[indexAC]);
    zeroCount = 0;
  }
  if (lastNotZeroCount != MCU_UNIT_SIZE - 1) {
    _rle[imageComponent].push_back(0x0); // Major byte: 0 zeros
    _rle[imageComponent].push_back(0x0); // Minor byte: EOB
    zeroCount = 0;                       // 重置0值的计数器
  }
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
    // if (imageComponent == 0) {
    //   std::cout << "原来的 _zzOrder[0]:" << _zzOrder[0];
    //   DC ,差分编码
    int16_t DC = _zzOrder[0] - _DCDiff[imageComponent];
    _DCDiff[imageComponent] = _zzOrder[0];
    _zzOrder[0] = DC;
    _rle[imageComponent].push_back(0);
    _rle[imageComponent].push_back(DC);
    //    std::cout << "push DC:" << DC << std::endl;
    //   std::cout << ",差分编码后的 _zzOrder[0]:" << _zzOrder[0] << std::endl;
    // }

    // printZZOrder();
    // fillACRLE(imageComponent);
    fillACRLE2(imageComponent);
    //  printRLE();
  }
}

void MCU::decodeACandDC() {
  for (int imageComponent = 0; imageComponent < 3; imageComponent++) {
    // printRLE();
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

    // printZZOrder();

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
    // printMatrix();
  }
}

//===================================== SIMD SSE =============================================
#if defined(SSE)
#include <xmmintrin.h>
void MCU::startDCT() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    const float sqrt2_inv = 1.0 / sqrt(2.0);
    float cos_values_u[8][8], cos_values_v[8][8];

    for (int u = 0; u < 8; ++u)
      for (int i = 0; i < 8; ++i)
        cos_values_u[u][i] = cos((2 * i + 1) * u * M_PI / 16.0);

    for (int v = 0; v < 8; ++v)
      for (int j = 0; j < 8; ++j)
        cos_values_v[v][j] = cos((2 * j + 1) * v * M_PI / 16.0);

    for (int v = 0; v < 8; ++v) {
      for (int u = 0; u < 8; ++u) {
        __m128 sum = _mm_setzero_ps();

        for (int i = 0; i < 8; ++i) {
          __m128 cos_u_vec = _mm_set1_ps(cos_values_u[u][i]);

          for (int j = 0; j < 8; j += 4) {
            __m128 cos_v_vec =
                _mm_set_ps(cos_values_v[v][j + 3], cos_values_v[v][j + 2],
                           cos_values_v[v][j + 1], cos_values_v[v][j]);

            __m128 coeff = _mm_set_ps(_dctCoeffs[imageComponent][i][j + 3],
                                      _dctCoeffs[imageComponent][i][j + 2],
                                      _dctCoeffs[imageComponent][i][j + 1],
                                      _dctCoeffs[imageComponent][i][j]);

            __m128 temp = _mm_mul_ps(cos_u_vec, cos_v_vec);
            temp = _mm_mul_ps(temp, coeff);

            sum = _mm_add_ps(sum, temp);
          }
        }

        float result[4];
        _mm_storeu_ps(result, sum);
        float final_sum = result[0] + result[1] + result[2] + result[3];

        float Cu = (u == 0) ? sqrt2_inv : 1.0;
        float Cv = (v == 0) ? sqrt2_inv : 1.0;
        _matrix[imageComponent][u][v] = round((Cu * Cv) / 4.0 * final_sum);
      }
    }
  }
}
#elif defined(AVX)
#include <immintrin.h>
void MCU::startDCT() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    const float sqrt2_inv = 1.0 / sqrt(2.0);
    float cos_values_u[8][8], cos_values_v[8][8];

    for (int u = 0; u < 8; ++u)
      for (int i = 0; i < 8; ++i)
        cos_values_u[u][i] = cos((2 * i + 1) * u * M_PI / 16.0);

    for (int v = 0; v < 8; ++v)
      for (int j = 0; j < 8; ++j)
        cos_values_v[v][j] = cos((2 * j + 1) * v * M_PI / 16.0);

    for (int v = 0; v < 8; ++v) {
      for (int u = 0; u < 8; ++u) {
        __m256 sum = _mm256_setzero_ps();

        for (int i = 0; i < 8; ++i) {
          __m256 cos_u_vec = _mm256_set1_ps(cos_values_u[u][i]);

          for (int j = 0; j < 8; j += 8) {
            __m256 cos_v_vec =
                _mm256_set_ps(cos_values_v[v][j + 7], cos_values_v[v][j + 6],
                              cos_values_v[v][j + 5], cos_values_v[v][j + 4],
                              cos_values_v[v][j + 3], cos_values_v[v][j + 2],
                              cos_values_v[v][j + 1], cos_values_v[v][j]);

            __m256 coeff = _mm256_set_ps(_dctCoeffs[imageComponent][i][j + 7],
                                         _dctCoeffs[imageComponent][i][j + 6],
                                         _dctCoeffs[imageComponent][i][j + 5],
                                         _dctCoeffs[imageComponent][i][j + 4],
                                         _dctCoeffs[imageComponent][i][j + 3],
                                         _dctCoeffs[imageComponent][i][j + 2],
                                         _dctCoeffs[imageComponent][i][j + 1],
                                         _dctCoeffs[imageComponent][i][j]);

            __m256 temp = _mm256_mul_ps(cos_u_vec, cos_v_vec);
            temp = _mm256_mul_ps(temp, coeff);

            sum = _mm256_add_ps(sum, temp);
          }
        }

        float result[8];
        _mm256_storeu_ps(result, sum);
        float final_sum = result[0] + result[1] + result[2] + result[3] +
                          result[4] + result[5] + result[6] + result[7];

        float Cu = (u == 0) ? sqrt2_inv : 1.0;
        float Cv = (v == 0) ? sqrt2_inv : 1.0;
        _matrix[imageComponent][u][v] = round((Cu * Cv) / 4.0 * final_sum);
      }
    }
  }
}
#elif defined(Threads)
void MCU::dctComponent(int imageComponent) {
  float sum = 0.0, Cu = 0.0, Cv = 0.0;

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
      _matrix[imageComponent][u][v] = round((Cu * Cv) / 4.0 * sum);
    }
  }
}

void MCU::startDCT() {
  vector<thread> threads;
  // 先为每个图像分量创建线程
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    threads.push_back(thread(&MCU::dctComponent, this, imageComponent));

  // 等待所有线程完成
  for (thread &t : threads)
    if (t.joinable()) t.join();
}
#elif defined(Threads_AVX)
#include <immintrin.h>
void MCU::dctComponent(int imageComponent) {
  const float sqrt2_inv = 1.0 / sqrt(2.0);
  float cos_values_u[8][8], cos_values_v[8][8];

  for (int u = 0; u < 8; ++u)
    for (int i = 0; i < 8; ++i)
      cos_values_u[u][i] = cos((2 * i + 1) * u * M_PI / 16.0);

  for (int v = 0; v < 8; ++v)
    for (int j = 0; j < 8; ++j)
      cos_values_v[v][j] = cos((2 * j + 1) * v * M_PI / 16.0);

  for (int v = 0; v < 8; ++v) {
    for (int u = 0; u < 8; ++u) {
      __m256 sum = _mm256_setzero_ps();

      for (int i = 0; i < 8; ++i) {
        __m256 cos_u_vec = _mm256_set1_ps(cos_values_u[u][i]);

        for (int j = 0; j < 8; j += 8) {
          __m256 cos_v_vec =
              _mm256_set_ps(cos_values_v[v][j + 7], cos_values_v[v][j + 6],
                            cos_values_v[v][j + 5], cos_values_v[v][j + 4],
                            cos_values_v[v][j + 3], cos_values_v[v][j + 2],
                            cos_values_v[v][j + 1], cos_values_v[v][j]);

          __m256 coeff = _mm256_set_ps(_dctCoeffs[imageComponent][i][j + 7],
                                       _dctCoeffs[imageComponent][i][j + 6],
                                       _dctCoeffs[imageComponent][i][j + 5],
                                       _dctCoeffs[imageComponent][i][j + 4],
                                       _dctCoeffs[imageComponent][i][j + 3],
                                       _dctCoeffs[imageComponent][i][j + 2],
                                       _dctCoeffs[imageComponent][i][j + 1],
                                       _dctCoeffs[imageComponent][i][j]);

          __m256 temp = _mm256_mul_ps(cos_u_vec, cos_v_vec);
          temp = _mm256_mul_ps(temp, coeff);

          sum = _mm256_add_ps(sum, temp);
        }
      }

      float result[8];
      _mm256_storeu_ps(result, sum);
      float final_sum = result[0] + result[1] + result[2] + result[3] +
                        result[4] + result[5] + result[6] + result[7];

      float Cu = (u == 0) ? sqrt2_inv : 1.0;
      float Cv = (v == 0) ? sqrt2_inv : 1.0;
      _matrix[imageComponent][u][v] = round((Cu * Cv) / 4.0 * final_sum);
    }
  }
}

void MCU::startDCT() {
  vector<thread> threads;
  // 先为每个图像分量创建线程
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    threads.push_back(thread(&MCU::dctComponent, this, imageComponent));

  // 等待所有线程完成
  for (thread &t : threads)
    if (t.joinable()) t.join();
}
#else
void MCU::startDCT() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    const float sqrt2_inv = 1.0 / sqrt(2.0);
    float sum = 0.0, Cu = 0.0, Cv = 0.0;

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
        Cu = u == 0 ? sqrt2_inv : 1.0;
        Cv = v == 0 ? sqrt2_inv : 1.0;
        _matrix[imageComponent][u][v] = round((Cu * Cv) / 4.0 * sum);
      }
    }
  }
}
#endif

//===================================== SIMD SSE =============================================
#if defined(SSE)
#include <xmmintrin.h>
void MCU::startIDCT() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    const float sqrt2_inv = 1.0 / sqrt(2.0);
    float cos_values_u[8][8], cos_values_v[8][8];

    /* 为了减少重复计算，预先计算cos值并存储在cos_values数组中 */
    for (int u = 0; u < 8; ++u)
      for (int i = 0; i < 8; ++i)
        cos_values_u[u][i] = cos((2 * i + 1) * u * M_PI / 16.0);

    for (int v = 0; v < 8; ++v)
      for (int j = 0; j < 8; ++j)
        cos_values_v[v][j] = cos((2 * j + 1) * v * M_PI / 16.0);

    /* 由于累加性质，所以这里只对内层循环实现SIMD重写，对于外层不改动，才可以确保累加操作的正确性  */
    for (int j = 0; j < 8; ++j) {
      for (int i = 0; i < 8; ++i) {
        /* 初始化为0 */
        __m128 sum = _mm_setzero_ps();

        /* 内层循环SIMD化：在u和v的内层循环中使用SIMD指令来并行处理4个v值的计算。*/
        for (int u = 0; u < 8; ++u) {
          float Cu = (u == 0) ? sqrt2_inv : 1.0;
          __m128 Cu_vec = _mm_set1_ps(Cu * cos_values_u[u][i]);

          for (int v = 0; v < 8; v += 4) {
            __m128 Cv_vec = _mm_set_ps(
                (v + 3 == 0) ? sqrt2_inv : 1.0, (v + 2 == 0) ? sqrt2_inv : 1.0,
                (v + 1 == 0) ? sqrt2_inv : 1.0, (v == 0) ? sqrt2_inv : 1.0);

            __m128 cos_v_vec =
                _mm_set_ps(cos_values_v[v + 3][j], cos_values_v[v + 2][j],
                           cos_values_v[v + 1][j], cos_values_v[v][j]);

            __m128 coeff = _mm_set_ps(_matrix[imageComponent][u][v + 3],
                                      _matrix[imageComponent][u][v + 2],
                                      _matrix[imageComponent][u][v + 1],
                                      _matrix[imageComponent][u][v]);

            __m128 temp = _mm_mul_ps(Cu_vec, Cv_vec);
            temp = _mm_mul_ps(temp, cos_v_vec);
            temp = _mm_mul_ps(temp, coeff);

            sum = _mm_add_ps(sum, temp);
          }
        }

        //对__m128进行横向和，得到最终和
        float result[4];
        _mm_storeu_ps(result, sum);
        _idctCoeffs[imageComponent][i][j] =
            round(1.0 / 4.0 * (result[0] + result[1] + result[2] + result[3]));
      }
    }
  }
}

//===================================== SIMD AVX =============================================
#elif defined(AVX)
#include <immintrin.h> // AVX
void MCU::startIDCT() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    const float sqrt2_inv = 1.0 / sqrt(2.0);
    float cos_values_u[8][8], cos_values_v[8][8];

    for (int u = 0; u < 8; ++u)
      for (int i = 0; i < 8; ++i)
        cos_values_u[u][i] = cos((2 * i + 1) * u * M_PI / 16.0);

    for (int v = 0; v < 8; ++v)
      for (int j = 0; j < 8; ++j)
        cos_values_v[v][j] = cos((2 * j + 1) * v * M_PI / 16.0);

    for (int j = 0; j < 8; ++j) {
      for (int i = 0; i < 8; ++i) {
        __m256 sum = _mm256_setzero_ps(); // Initialize sum to 0

        for (int u = 0; u < 8; ++u) {
          float Cu = (u == 0) ? sqrt2_inv : 1.0;
          __m256 Cu_vec = _mm256_set1_ps(Cu * cos_values_u[u][i]);

          for (int v = 0; v < 8; v += 8) {
            __m256 Cv_vec = _mm256_set_ps(
                (v + 7 == 0) ? sqrt2_inv : 1.0, (v + 6 == 0) ? sqrt2_inv : 1.0,
                (v + 5 == 0) ? sqrt2_inv : 1.0, (v + 4 == 0) ? sqrt2_inv : 1.0,
                (v + 3 == 0) ? sqrt2_inv : 1.0, (v + 2 == 0) ? sqrt2_inv : 1.0,
                (v + 1 == 0) ? sqrt2_inv : 1.0, (v == 0) ? sqrt2_inv : 1.0);

            __m256 cos_v_vec =
                _mm256_set_ps(cos_values_v[v + 7][j], cos_values_v[v + 6][j],
                              cos_values_v[v + 5][j], cos_values_v[v + 4][j],
                              cos_values_v[v + 3][j], cos_values_v[v + 2][j],
                              cos_values_v[v + 1][j], cos_values_v[v][j]);

            __m256 coeff = _mm256_set_ps(_matrix[imageComponent][u][v + 7],
                                         _matrix[imageComponent][u][v + 6],
                                         _matrix[imageComponent][u][v + 5],
                                         _matrix[imageComponent][u][v + 4],
                                         _matrix[imageComponent][u][v + 3],
                                         _matrix[imageComponent][u][v + 2],
                                         _matrix[imageComponent][u][v + 1],
                                         _matrix[imageComponent][u][v]);

            __m256 temp = _mm256_mul_ps(Cu_vec, Cv_vec);
            temp = _mm256_mul_ps(temp, cos_v_vec);
            temp = _mm256_mul_ps(temp, coeff);

            sum = _mm256_add_ps(sum, temp);
          }
        }

        // Horizontal sum of the __m256 vector to get the final sum
        float result[8];
        _mm256_storeu_ps(result, sum);
        _idctCoeffs[imageComponent][i][j] =
            round(1.0 / 4.0 *
                  (result[0] + result[1] + result[2] + result[3] + result[4] +
                   result[5] + result[6] + result[7]));
      }
    }
  }
}

//===================================== Threads(3) =============================================
#elif defined(Threads)
void MCU::idctComponent(int imageComponent) {
  float sum = 0.0, Cu = 0.0, Cv = 0.0;
  const float sqrt2_inv = 1.0 / sqrt(2.0);

  for (int j = 0; j < COMPONENT_SIZE; ++j) {
    for (int i = 0; i < COMPONENT_SIZE; ++i) {
      sum = 0.0;
      for (int u = 0; u < COMPONENT_SIZE; ++u) {
        for (int v = 0; v < COMPONENT_SIZE; ++v) {
          Cu = u == 0 ? sqrt2_inv : 1.0;
          Cv = v == 0 ? sqrt2_inv : 1.0;

          sum += Cu * Cv * _matrix[imageComponent][u][v] *
                 cos((2 * i + 1) * u * M_PI / 16.0) *
                 cos((2 * j + 1) * v * M_PI / 16.0);
        }
      }

      _idctCoeffs[imageComponent][i][j] = round(1.0 / 4.0 * sum);
    }
  }
}

void MCU::startIDCT() {
  vector<thread> threads;
  // 先为每个图像分量创建线程
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    threads.push_back(thread(&MCU::idctComponent, this, imageComponent));

  // 等待所有线程完成
  for (thread &t : threads)
    if (t.joinable()) t.join();
}

//================================== Threads(3) And AVX =============================================
#elif defined(Threads_AVX)
#include <immintrin.h> // AVX
void MCU::idctComponent(int imageComponent) {
  const float sqrt2_inv = 1.0 / sqrt(2.0);
  float cos_values_u[8][8], cos_values_v[8][8];

  for (int u = 0; u < 8; ++u)
    for (int i = 0; i < 8; ++i)
      cos_values_u[u][i] = cos((2 * i + 1) * u * M_PI / 16.0);

  for (int v = 0; v < 8; ++v)
    for (int j = 0; j < 8; ++j)
      cos_values_v[v][j] = cos((2 * j + 1) * v * M_PI / 16.0);

  for (int j = 0; j < 8; ++j) {
    for (int i = 0; i < 8; ++i) {
      __m256 sum = _mm256_setzero_ps();

      for (int u = 0; u < 8; ++u) {
        float Cu = (u == 0) ? sqrt2_inv : 1.0;
        __m256 Cu_vec = _mm256_set1_ps(Cu * cos_values_u[u][i]);

        for (int v = 0; v < 8; v += 8) {
          __m256 Cv_vec = _mm256_set_ps(
              (v + 7 == 0) ? sqrt2_inv : 1.0, (v + 6 == 0) ? sqrt2_inv : 1.0,
              (v + 5 == 0) ? sqrt2_inv : 1.0, (v + 4 == 0) ? sqrt2_inv : 1.0,
              (v + 3 == 0) ? sqrt2_inv : 1.0, (v + 2 == 0) ? sqrt2_inv : 1.0,
              (v + 1 == 0) ? sqrt2_inv : 1.0, (v == 0) ? sqrt2_inv : 1.0);

          __m256 cos_v_vec =
              _mm256_set_ps(cos_values_v[v + 7][j], cos_values_v[v + 6][j],
                            cos_values_v[v + 5][j], cos_values_v[v + 4][j],
                            cos_values_v[v + 3][j], cos_values_v[v + 2][j],
                            cos_values_v[v + 1][j], cos_values_v[v][j]);

          __m256 coeff = _mm256_set_ps(_matrix[imageComponent][u][v + 7],
                                       _matrix[imageComponent][u][v + 6],
                                       _matrix[imageComponent][u][v + 5],
                                       _matrix[imageComponent][u][v + 4],
                                       _matrix[imageComponent][u][v + 3],
                                       _matrix[imageComponent][u][v + 2],
                                       _matrix[imageComponent][u][v + 1],
                                       _matrix[imageComponent][u][v]);

          __m256 temp = _mm256_mul_ps(Cu_vec, Cv_vec);
          temp = _mm256_mul_ps(temp, cos_v_vec);
          temp = _mm256_mul_ps(temp, coeff);

          sum = _mm256_add_ps(sum, temp);
        }
      }

      float result[8];
      _mm256_storeu_ps(result, sum);
      _idctCoeffs[imageComponent][i][j] =
          round(1.0 / 4.0 *
                (result[0] + result[1] + result[2] + result[3] + result[4] +
                 result[5] + result[6] + result[7]));
    }
  }
}

void MCU::startIDCT() {
  vector<thread> threads;
  // 先为每个图像分量创建线程
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    threads.push_back(thread(&MCU::idctComponent, this, imageComponent));

  // 等待所有线程完成
  for (thread &t : threads)
    if (t.joinable()) t.join();
}

//================================== Single Threads =============================================
#else
void MCU::startIDCT() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    float sum = 0.0, Cu = 0.0, Cv = 0.0;
    const float sqrt2_inv = 1.0 / sqrt(2.0);

    for (int j = 0; j < COMPONENT_SIZE; ++j) {
      for (int i = 0; i < COMPONENT_SIZE; ++i) {
        sum = 0.0;
        for (int u = 0; u < COMPONENT_SIZE; ++u) {
          for (int v = 0; v < COMPONENT_SIZE; ++v) {
            Cu = u == 0 ? sqrt2_inv : 1.0;
            Cv = v == 0 ? sqrt2_inv : 1.0;

            sum += Cu * Cv * _matrix[imageComponent][u][v] *
                   cos((2 * i + 1) * u * M_PI / 16.0) *
                   cos((2 * j + 1) * v * M_PI / 16.0);
          }
        }

        _idctCoeffs[imageComponent][i][j] = round(1.0 / 4.0 * sum);
      }
    }
  }
}
#endif

/* 中心化*/
void MCU::levelShift() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    for (int y = 0; y < COMPONENT_SIZE; ++y)
      for (int x = 0; x < COMPONENT_SIZE; ++x)
        _dctCoeffs[imageComponent][y][x] = _Umatrix[imageComponent][y][x] - 128;
}

/* 反中心化（反级别移位）*/
void MCU::performLevelShift() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    for (int y = 0; y < COMPONENT_SIZE; ++y)
      for (int x = 0; x < COMPONENT_SIZE; ++x)
        _Umatrix[imageComponent][y][x] =
            roundl(_idctCoeffs[imageComponent][y][x]) + 128;
  // 1. 使用 roundl 函数对IDCT变换后的系数进行四舍五入到最近的整数。
  // 2. 向每个四舍五入后的系数加上128，以执行反中心化处理。
}

void MCU::printUmatrix() {
  std::cout << "_Umatrix:";
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    for (int y = 0; y < COMPONENT_SIZE; ++y)
      for (int x = 0; x < COMPONENT_SIZE; ++x)
        cout << _Umatrix[imageComponent][y][x] << "["
             << (imageComponent == 0   ? "Y"
                 : imageComponent == 1 ? "U"
                                       : "V")
             << "],";
  std::cout << std::endl;
}

void MCU::printMatrix() {
  std::cout << "_matrix:";
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    for (int y = 0; y < COMPONENT_SIZE; ++y)
      for (int x = 0; x < COMPONENT_SIZE; ++x)
        cout << _matrix[imageComponent][y][x] << "["
             << (imageComponent == 0   ? "Y"
                 : imageComponent == 1 ? "U"
                                       : "V")
             << "],";
  std::cout << std::endl;
}

void MCU::printDCTCoeffs() {
  std::cout << "_dctCoeffs:";
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    for (int y = 0; y < COMPONENT_SIZE; ++y)
      for (int x = 0; x < COMPONENT_SIZE; ++x)
        cout << _dctCoeffs[imageComponent][y][x] << "["
             << (imageComponent == 0   ? "Y"
                 : imageComponent == 1 ? "U"
                                       : "V")
             << "],";
  std::cout << std::endl;
}

void MCU::printIDCTCoeffs() {
  std::cout << "_idctCoeffs:";
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    for (int y = 0; y < COMPONENT_SIZE; ++y)
      for (int x = 0; x < COMPONENT_SIZE; ++x)
        cout << _idctCoeffs[imageComponent][y][x] << "["
             << (imageComponent == 0   ? "Y"
                 : imageComponent == 1 ? "U"
                                       : "V")
             << "],";
  std::cout << std::endl;
}

void MCU::printZZOrder() {
  static int zzCount = 0;
  std::cout << "_zzOrder[" << zzCount << "]:";
  for (int i = 0; i < MCU_UNIT_SIZE; i++) {
    std::cout << _zzOrder[i] << ",";
  }
  std::cout << std::endl;
  zzCount++;
}

void MCU::printRLE() {
  static int rleCount = 0;
  std::cout << "_RLE[" << rleCount << "]:";
  for (int imageComponent = 0; imageComponent < 3; imageComponent++) {
    for (int i = 0; i < (int)_rle[imageComponent].size(); i++) {
      std::cout << _rle[imageComponent][i] << "["
                << (imageComponent == 0   ? "Y"
                    : imageComponent == 1 ? "U"
                                          : "V")
                << "],";
    }
  }
  rleCount++;
  std::cout << std::endl;
}
