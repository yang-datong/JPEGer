#ifndef MCU_HPP_MJKLNXG9
#define MCU_HPP_MJKLNXG9

#include "Type.hpp"
#include <mutex>
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)
#include <immintrin.h>
#elif defined(__aarch64__) || defined(__arm__)
#include <arm_neon.h>
#endif

#if (defined(__GNUC__) || defined(__clang__)) &&                               \
    (defined(__x86_64__) || defined(__i386__))
#define TARGET_AVX2 __attribute__((target("avx2")))
#define TARGET_SSE41 __attribute__((target("sse4.1")))
#else
#define TARGET_AVX2
#define TARGET_SSE41
#endif

class MCU {
 private:
  static int _MCUCount;
  static int16_t _DCDiff[3];

  CompMatrices _matrix;
  UCompMatrices _Umatrix;
  IDCTCoeffs _idctCoeffs;
  DCTCoeffs _dctCoeffs;
  RLE _rle;

  vector<QuantizationTable> _qtTables;
  /* 编码时：经过了中心化操作后的宏块应该是包含负数的，所以应该使用int16_t，而不是uint16_t*/
  array<int16_t, MCU_UNIT_SIZE> _zzOrder = {0};
  /* 解码时：经过了还未解码的宏块应该是包含负数的，所以应该使用int16_t，而不是uint16_t*/

 public:
  MCU(UCompMatrices &matrix, const vector<QuantizationTable> &qTables);
  MCU(RLE rle, const vector<QuantizationTable> &qTables);

  void startEncode();
  void startDecode();

 private:
  void encodeACandDC();
  void decodeACandDC();

 private:
  //接口（函数指针）
  void startDCT();
  void startIDCT();

  //默认c实现(低效率)
  void startDCT_c();
  void startIDCT_c();

  //Threads
  void startIDCT_threads();
  void startDCT_threads();
  void dctComponent(int imageComponent);
  void idctComponent(int imageComponent);

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)
  //SSE
  void startDCT_sse41();
  void startIDCT_sse41();

  //AVX
  void startDCT_avx2();
  void startIDCT_avx2();
  void startDCT_threads_avx();
  void startIDCT_threads_avx();
  void dctComponent_avx(int imageComponent);
  void idctComponent_avx(int imageComponent);
#endif

#if defined(__aarch64__) || defined(__arm__)
  void startDCT_neon();
  void startIDCT_neon();
#endif

 private:
  using start_dct_ptr_t = void (MCU::*)();
  using start_idct_ptr_t = void (MCU::*)();

  struct SimdDispatchTable {
    start_dct_ptr_t start_dct;
    start_idct_ptr_t start_idct;
  };

  static SimdDispatchTable s_dispatch_table;
  // 初始化标志 TODO 初始化的方式不好，可能需要重新调整架构 <25-06-17 13:54:10, YangJing>
  static std::once_flag s_init_flag;

  static void initialize_dispatcher();

 private:
  void levelShift();
  void performLevelShift();

  void fillACRLE(int imageComponent);
  void fillACRLE2(int imageComponent);

  void printZZOrder();
  void printMatrix();
  void printUmatrix();
  void printDCTCoeffs();
  void printIDCTCoeffs();
  void printRLE();

 public:
  /* 将反中心化的解码矩阵提供给外界创建图片等操作 */
  const UCompMatrices &getMatrices() const { return _Umatrix; }
  const RLE &getRLE() const { return _rle; }
};
#endif /* end of include guard: MCU_HPP_MJKLNXG9 */
