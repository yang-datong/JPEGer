#include "MCU.hpp"
#include "Common.hpp"
#include "Image.hpp"
#include "Type.hpp"

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
  if (Image::sOutputFileType != FileFormat::YUV)
    Image::YUVToRGB(_Umatrix);
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
    if (t.joinable())
      t.join();
}

#if defined(SSE)
void MCU::startIDCT() {
  __m128 sum_sse = _mm_setzero_ps(); // 使用SSE指令集置零
  float sum_array[4];

  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    for (int j = 0; j < COMPONENT_SIZE; ++j) {
      for (int i = 0; i < COMPONENT_SIZE; ++i) {
        sum_sse = _mm_setzero_ps();

        for (int u = 0; u < COMPONENT_SIZE; u += 4) { //
          以4个元素为一个处理单位
          for (int v = 0; v < COMPONENT_SIZE; ++v) {
            // 从_matrix加载数据
            __m128i data = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(
                &_matrix[imageComponent][u][v]));
            data = _mm_unpacklo_epi8(data, _mm_setzero_si128()); //转换成16位

            // 计算Cu, Cv和余弦值，需要将这些值转换为__m128类型以便使用
            __m128 Cu_sse = _mm_set_ps1(u == 0 ? 1.0 / std::sqrt(2.0) : 1.0);
            __m128 Cv_sse = _mm_set_ps1(v == 0 ? 1.0 / std::sqrt(2.0) : 1.0);
            __m128 cos1 = _mm_set_ps1(std::cos((2 * i + 1) * u * M_PI / 16.0));
            __m128 cos2 = _mm_set_ps1(std::cos((2 * j + 1) * v * M_PI / 16.0));

            // 转换data为float
            __m128 data_ps =
                _mm_cvtepi32_ps(_mm_unpacklo_epi16(data, _mm_setzero_si128()));

            // 执行计算
            __m128 partial_sum = _mm_mul_ps(data_ps, Cu_sse);
            partial_sum = _mm_mul_ps(partial_sum, Cv_sse);
            partial_sum = _mm_mul_ps(partial_sum, cos1);
            partial_sum = _mm_mul_ps(partial_sum, cos2);
            sum_sse = _mm_add_ps(sum_sse, partial_sum);
          }
        }

        // 将累加的结果从SSE寄存器sum_sse转移到常规浮点数组sum_array中
        _mm_storeu_ps(sum_array, sum_sse);

        // 对sum_array中的结果进行后处理
        float sum = 0.0;
        for (int k = 0; k < 4; ++k) {
          sum += sum_array[k];
        }

        _idctCoeffs[imageComponent][i][j] = round(1.0 / 4.0 * sum);
      }
    }
  }
}
#else
void MCU::idctComponent(int imageComponent) {
  float sum = 0.0, Cu = 0.0, Cv = 0.0;

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
    if (t.joinable())
      t.join();
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
