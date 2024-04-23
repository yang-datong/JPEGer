#include "Decoder.hpp"

int main() {
  string inputFileName = "./lenna.jpg";
  string ouputFileName = "./demo";

  Decoder decoder(inputFileName);
  int ret = -1;
  std::cout << "-------------------------------------------- startFindMarker "
               "--------------------------------------------"
            << std::endl;
  ret = decoder.startFindMarker();
  if (ret) {
    std::cerr << "\033[31mFailed for startFindMarker()\033[0m" << std::endl;
    return -1;
  }

  std::cout << "-------------------------------------------- decodeScanData "
               "--------------------------------------------"
            << std::endl;
  ret = decoder.decodeScanData();
  if (ret) {
    std::cerr << "\033[31mFailed for decodeScanData()\033[0m" << std::endl;
    return -1;
  }

  std::cout << "---------------------------------------------- createImage "
               "----------------------------------------------"
            << std::endl;
  ret = decoder.createImage(ouputFileName);
  if (ret) {
    std::cerr << "\033[31mFailed for createImage()\033[0m" << std::endl;
    return -1;
  }
  return 0;
}
