#include "MCU.hpp"

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

void MCU::startDCT_threads() {
  vector<thread> threads;
  // 先为每个图像分量创建线程
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    threads.push_back(thread(&MCU::dctComponent, this, imageComponent));

  // 等待所有线程完成
  for (thread &t : threads)
    if (t.joinable()) t.join();
}

void MCU::startDCT_threads_avx() {
  vector<thread> threads;
  // 先为每个图像分量创建线程
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    threads.push_back(thread(&MCU::dctComponent_avx, this, imageComponent));

  // 等待所有线程完成
  for (thread &t : threads)
    if (t.joinable()) t.join();
}

void MCU::idctComponent(int imageComponent) {
  float sum = 0.0;
  for (int j = 0; j < COMPONENT_SIZE; ++j) {
    for (int i = 0; i < COMPONENT_SIZE; ++i) {
      sum = 0.0;
      for (int u = 0; u < COMPONENT_SIZE; ++u) {
        const float Cu = u == 0 ? C_1_div_sqrt2 : 1.0;
        sum += C_1_div_sqrt2 * Cu * _matrix[imageComponent][u][0] *
               cosTable[i][u] * cosTable[j][0];
        sum += Cu * _matrix[imageComponent][u][1] * cosTable[i][u] *
               cosTable[j][1];
        sum += Cu * _matrix[imageComponent][u][2] * cosTable[i][u] *
               cosTable[j][2];
        sum += Cu * _matrix[imageComponent][u][3] * cosTable[i][u] *
               cosTable[j][3];
        sum += Cu * _matrix[imageComponent][u][4] * cosTable[i][u] *
               cosTable[j][4];
        sum += Cu * _matrix[imageComponent][u][5] * cosTable[i][u] *
               cosTable[j][5];
        sum += Cu * _matrix[imageComponent][u][6] * cosTable[i][u] *
               cosTable[j][6];
        sum += Cu * _matrix[imageComponent][u][7] * cosTable[i][u] *
               cosTable[j][7];
      }
      _idctCoeffs[imageComponent][i][j] = round(0.25 * sum);
    }
  }
}

void MCU::startIDCT_threads() {
  vector<thread> threads;
  // 先为每个图像分量创建线程
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    threads.push_back(thread(&MCU::idctComponent, this, imageComponent));

  // 等待所有线程完成
  for (thread &t : threads)
    if (t.joinable()) t.join();
}

void MCU::startIDCT_threads_avx() {
  vector<thread> threads;
  // 先为每个图像分量创建线程
  for (int imageComponent = 0; imageComponent < 3; ++imageComponent)
    threads.push_back(thread(&MCU::idctComponent_avx, this, imageComponent));

  // 等待所有线程完成
  for (thread &t : threads)
    if (t.joinable()) t.join();
}
