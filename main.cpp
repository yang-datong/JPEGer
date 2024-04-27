#include "Common.hpp"
#include "Decoder.hpp"
#include "Encoder.hpp"
#include "Type.hpp"

int main() {
  string inputFileName = "./lenna.yuv";
  string outputFileName = "./demo.jpg";

  inputFileName = "./lenna.jpg";
  outputFileName = "./demo";

  int ret = -1;
  string suffixStr = getFileType(inputFileName);
  if (suffixStr == "jpg" || suffixStr == "jpeg") {
    Decoder decoder(inputFileName, FileFormat::YUV);
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
    Encoder encoder(inputFileName, outputFileName, FileFormat::YUV);
    std::cout << "---------------------------------------------- createImage "
                 "----------------------------------------------"
              << std::endl;
    ret = encoder.startMakeMarker();
    RET(ret, "Failed for startMakeMarker()")
  }
  return 0;
}
