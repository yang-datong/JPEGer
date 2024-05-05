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
  static int readFile(const string &filePath, uint8_t *&buf, int &bufSize);

 public:
  static int sOutputFileType;
  static int sInputFileType;
  static int readJPEGFile(const string &filePath, uint8_t *&buf, int &bufSize);
  static int readYUVFile(const string &filePath, uint8_t *&buf, int &bufSize);

  int createImageFromMCUs(const vector<MCU> &MCUs, uint16_t imgWidth,
                          uint16_t imgHeight);
  int outputToYUVFile(const string &outputFileName);
  int outputToPPMFile(const string &outputFileName);

  static int YUV444PlanarToPacked(uint8_t *planarBuffer, uint8_t *packedBuffer,
                                  int width, int height);
};
#endif /* end of include guard: IMAGE_HPP_J7ITHE8O */
