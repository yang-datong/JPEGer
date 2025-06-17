#include "cpu_features.hpp"
#include <string.h>
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)
#if defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#elif defined(_MSC_VER)
#include <intrin.h>
#endif
#endif

// 只在 x86 架构下才需要定义 cpuid 辅助函数
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)
static inline void cpuid(int info[4], int info_type) {
#if defined(__GNUC__) || defined(__clang__)
  __cpuid_count(info_type, 0, info[0], info[1], info[2], info[3]);
#elif defined(_MSC_VER)
  __cpuidex(info, info_type, 0);
#else
  // 对于其他不支持的编译器，定义一个空的cpuid函数。
  // 这保证了代码总能编译通过，只是不会报告任何SIMD特性。
  memset(info, 0, sizeof(int) * 4);
#endif
}
#endif

CpuFeatures detect_cpu_features(void) {
  CpuFeatures features;
  memset(&features, 0, sizeof(features));

  // 对于x86架构
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)

  int info[4];

  // EAX=1: 获取基本特性
  cpuid(info, 1);

  // SSE
  features.sse = (info[3] & (1 << 25)) != 0;
  // SSE2
  features.sse2 = (info[3] & (1 << 26)) != 0;
  // SSE4
  features.sse4_1 = (info[2] & (1 << 19)) != 0;

  // AVX：仅当OSXSAVE位（CPUID.1.ECX[27]）被设置时，AVX才可用 (Intel OSX?)
  bool os_supports_avx = (info[2] & (1 << 27)) != 0;
  // AVX2
  if (os_supports_avx) {
    // EAX=7, ECX=0: 获取扩展特性
    cpuid(info, 7);
    features.avx = (info[1] & (1 << 5)) != 0;
    features.avx2 = (info[1] & (1 << 8)) != 0;
  }

  // 对于Arm架构
#elif defined(__aarch64__) || defined(__arm__)

// 检查NEON支持预定义宏检测即可
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  features.neon = true;
#else
  features.neon = false;
#endif

#endif

  return features;
}
