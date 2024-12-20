#include "MCU.hpp"
#include <xmmintrin.h>

void MCU::startDCT_sse() {
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


void MCU::startIDCT_sse() {
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
