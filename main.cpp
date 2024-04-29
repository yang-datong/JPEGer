#include "Common.hpp"
#include "Decoder.hpp"
#include "Encoder.hpp"
#include "Type.hpp"

int main(int argc, char *argv[]) {
  /* 不指定参数则执行编码操作 */
  string inputFileName = "./lenna.yuv";
  string outputFileName = "./demo.jpg";
  if (argc > 2) {
    inputFileName = argv[1];
    outputFileName = argv[2];
  }

  int ret = -1;
  string inSuffixStr = getFileType(inputFileName);
  string outSuffixStr = getFileType(outputFileName);
  int inpuType = FileFormat::YUV, outType = FileFormat::YUV;

  if (inSuffixStr == "jpg" || inSuffixStr == "jpeg") {
    if (outSuffixStr == "yuv")
      outType = FileFormat::YUV;
    else if (outSuffixStr == "ppm")
      outType = FileFormat::PPM;
    Decoder decoder(inputFileName, outType);
    std::cout << "-------------------------------------------- startFindMarker "
                 "--------------------------------------------"
              << std::endl;
    ret = decoder.startFindMarker();
    RET(ret, "Failed for startFindMarker()")

    std::cout << "-------------------------------------------- decodeScanData "
                 "--------------------------------------------"
              << std::endl;
    ret = decoder.decodeScanData();
    RET(ret, "Failed for decodeScanData()")

    std::cout << "---------------------------------------------- createImage "
                 "----------------------------------------------"
              << std::endl;
    ret = decoder.createImage(outputFileName);
    RET(ret, "Failed for createImage()")
  } else {
    Encoder encoder(inputFileName, outputFileName, inpuType);
    std::cout << "---------------------------------------------- createImage "
                 "----------------------------------------------"
              << std::endl;
    ret = encoder.startMakeMarker();
    RET(ret, "Failed for startMakeMarker()")
  }
  return 0;
}
