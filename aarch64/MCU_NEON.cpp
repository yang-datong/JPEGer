#include "MCU.hpp"

#if defined(__aarch64__) || defined(__arm__)
void MCU::startDCT_neon() {
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
          float32x4_t cos_u_vec = vdupq_n_f32(cos_values_u[u][i]);

          for (int j = 0; j < 8; j += 4) {
            float32x4_t cos_v_vec = vld1q_f32(&cos_values_v[v][j]);
            float32x4_t coeff_vec =
                vld1q_f32(&_dctCoeffs[imageComponent][i][j]);

            float32x4_t temp = vmulq_f32(cos_u_vec, cos_v_vec);
            temp = vmulq_f32(temp, coeff_vec);

            float terms[4];
            vst1q_f32(terms, temp);
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

extern "C" __attribute__((noinline)) void startIDCT_asm(MCU *mcu_instance);

void MCU::startIDCT_neon() {
#ifdef ASM_IDCT
  startIDCT_asm(this);
  return;
#endif
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent) {
    const float sqrt2_inv = 1.0f / sqrtf(2.0f);
    float cos_values_u[8][8], cos_values_v[8][8];

    for (int u = 0; u < 8; ++u)
      for (int i = 0; i < 8; ++i)
        cos_values_u[u][i] = cosf((2 * i + 1) * u * M_PI / 16.0f);

    for (int v = 0; v < 8; ++v)
      for (int j = 0; j < 8; ++j)
        cos_values_v[v][j] = cosf((2 * j + 1) * v * M_PI / 16.0f);

    for (int j = 0; j < 8; ++j) {
      for (int i = 0; i < 8; ++i) {
        float sum = 0.0f;

        for (int u = 0; u < 8; ++u) {
          float Cu = (u == 0) ? sqrt2_inv : 1.0f;
          float32x4_t Cu_cos_u_vec = vdupq_n_f32(Cu * cos_values_u[u][i]);

          for (int v = 0; v < 8; v += 4) {
            const float cv_vals[] = {(v + 0 == 0) ? sqrt2_inv : 1.0f,
                                     (v + 1 == 0) ? sqrt2_inv : 1.0f,
                                     (v + 2 == 0) ? sqrt2_inv : 1.0f,
                                     (v + 3 == 0) ? sqrt2_inv : 1.0f};
            float32x4_t Cv_vec = vld1q_f32(cv_vals);

            const float cos_v_vals[] = {
                cos_values_v[v + 0][j], cos_values_v[v + 1][j],
                cos_values_v[v + 2][j], cos_values_v[v + 3][j]};
            float32x4_t cos_v_vec = vld1q_f32(cos_v_vals);

            int16x4_t coeff_s16_vec = vld1_s16(&_matrix[imageComponent][u][v]);
            int32x4_t coeff_s32_vec = vmovl_s16(coeff_s16_vec);
            float32x4_t coeff_f32_vec = vcvtq_f32_s32(coeff_s32_vec);

            float32x4_t temp = vmulq_f32(Cu_cos_u_vec, Cv_vec);
            temp = vmulq_f32(temp, cos_v_vec);
            temp = vmulq_f32(temp, coeff_f32_vec);

            float terms[4];
            vst1q_f32(terms, temp);
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
