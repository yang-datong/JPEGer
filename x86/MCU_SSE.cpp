#include "MCU.hpp"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)

TARGET_SSE41
void MCU::startDCT_sse41() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    const float sqrt2_inv = 1.0f / sqrtf(2.0f);
    float cos_values_u[8][8], cos_values_v[8][8];

    for (int u = 0; u < 8; ++u)
      for (int i = 0; i < 8; ++i)
        cos_values_u[u][i] = cosf((2 * i + 1) * u * M_PI / 16.0f);

    for (int v = 0; v < 8; ++v)
      for (int j = 0; j < 8; ++j)
        cos_values_v[v][j] = cosf((2 * j + 1) * v * M_PI / 16.0f);

    for (int v = 0; v < 8; ++v) {
      for (int u = 0; u < 8; ++u) {
        float sum = 0.0f;

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

            alignas(16) float terms[4];
            _mm_storeu_ps(terms, temp);
            sum += terms[0];
            sum += terms[1];
            sum += terms[2];
            sum += terms[3];
          }
        }

        float Cu = (u == 0) ? sqrt2_inv : 1.0f;
        float Cv = (v == 0) ? sqrt2_inv : 1.0f;
        _matrix[imageComponent][u][v] = roundf((Cu * Cv) / 4.0f * sum);
      }
    }
  }
}

void MCU::startIDCT_sse41() {
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    const float sqrt2_inv = 1.0f / sqrtf(2.0f);
    float cos_values_u[8][8], cos_values_v[8][8];

    /* 为了减少重复计算，预先计算cos值并存储在cos_values数组中 */
    for (int u = 0; u < 8; ++u)
      for (int i = 0; i < 8; ++i)
        cos_values_u[u][i] = cosf((2 * i + 1) * u * M_PI / 16.0f);

    for (int v = 0; v < 8; ++v)
      for (int j = 0; j < 8; ++j)
        cos_values_v[v][j] = cosf((2 * j + 1) * v * M_PI / 16.0f);

    /* 由于累加性质，所以这里只对内层循环实现SIMD重写，对于外层不改动，才可以确保累加操作的正确性  */
    for (int j = 0; j < 8; ++j) {
      for (int i = 0; i < 8; ++i) {
        float sum = 0.0f;

        /* 内层循环SIMD化：在u和v的内层循环中使用SIMD指令来并行处理4个v值的计算。*/
        for (int u = 0; u < 8; ++u) {
          float Cu = (u == 0) ? sqrt2_inv : 1.0f;
          __m128 Cu_vec = _mm_set1_ps(Cu * cos_values_u[u][i]);

          for (int v = 0; v < 8; v += 4) {
            __m128 Cv_vec = _mm_set_ps(
                (v + 3 == 0) ? sqrt2_inv : 1.0f,
                (v + 2 == 0) ? sqrt2_inv : 1.0f,
                (v + 1 == 0) ? sqrt2_inv : 1.0f,
                (v == 0) ? sqrt2_inv : 1.0f);

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

            alignas(16) float terms[4];
            _mm_storeu_ps(terms, temp);
            sum += terms[0];
            sum += terms[1];
            sum += terms[2];
            sum += terms[3];
          }
        }

        _idctCoeffs[imageComponent][i][j] = roundf(1.0f / 4.0f * sum);
      }
    }
  }
}
#endif
