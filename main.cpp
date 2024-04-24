#include "Common.hpp"
#include "Decoder.hpp"
#include "Type.hpp"

int main() {
  string inputFileName = "./lenna.jpg";
  string ouputFileName = "./demo";

  Decoder decoder(inputFileName, OutputFileType::YUV);
  int ret = -1;
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
  ret = decoder.createImage(ouputFileName);
  RET(ret, "Failed for createImage()")
  return 0;
}
