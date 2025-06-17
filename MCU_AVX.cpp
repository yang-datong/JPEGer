#include "MCU.hpp"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)
void MCU::dctComponent_avx(int imageComponent) {
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

void MCU::idctComponent_avx(int imageComponent) {
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

void MCU::startDCT_avx2() {
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

void MCU::startIDCT_avx2() {
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
#endif
