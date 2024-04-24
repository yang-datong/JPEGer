#ifndef IMAGE_HPP_J7ITHE8O
#define IMAGE_HPP_J7ITHE8O
#include "MCU.hpp"

class Image {
 private:
  uint16_t _imgHeight = 0;
  uint16_t _imgWidth = 0;

  typedef shared_ptr<vector<vector<Pixel>>> PixelPtr;
  // typedef Pixel **PixelPtr;
  PixelPtr _pixelPtr = nullptr;

 public:
  static int sOutputFileType;
  int createImageFromMCUs(const vector<MCU> &MCUs, uint16_t imgWidth,
                          uint16_t imgHeight);
  int outputToYUVFile(const string &outputFileName);
  int outputToPPMFile(const string &outputFileName);
};
#endif /* end of include guard: IMAGE_HPP_J7ITHE8O */
