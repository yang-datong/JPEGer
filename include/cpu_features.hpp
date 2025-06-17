#ifndef CPU_FEATURES_H
#define CPU_FEATURES_H

#include <stdbool.h>

// 定义一个结构体来保存检测到的CPU特性
typedef struct {
  bool sse;
  bool sse2;
  bool sse4_1;
  bool avx;
  bool avx2;
  bool neon;
} CpuFeatures;

// 声明一个函数来获取CPU特性
// 这个函数会在运行时检测并返回一个填充好的CpuFeatures结构体
CpuFeatures detect_cpu_features(void);

#endif // CPU_FEATURES_H
