#include "Image.hpp"

int Image::createImageFromMCUs(const vector<MCU> &MCUs, uint16_t imgWidth,
                               uint16_t imgHeight) {
  this->_imgWidth = imgWidth;
  this->_imgHeight = imgHeight;
  if (this->_imgWidth <= 0 || this->_imgHeight <= 0)
    return -1;
  int mcuNum = 0;
  std::cout << "jpegWidth:" << _imgWidth << ",jpegHeight:" << _imgHeight
            << " -> ";
  /* 补充宽高为8的倍数 */
  int outputWidth =
      _imgWidth % 8 == 0 ? _imgWidth : _imgWidth + 8 - (_imgWidth % 8);
  int outputHeight =
      _imgHeight % 8 == 0 ? _imgHeight : _imgHeight + 8 - (_imgHeight % 8);
  std::cout << "outputWidth:" << outputWidth << ",outputHeight:" << outputHeight
            << std::endl;
  // Create a pixel pointer of size (Image width) x (Image height)
  _pixelPtr = make_shared<vector<vector<Pixel>>>(
      outputHeight, vector<Pixel>(outputWidth, Pixel()));

  // Populate the pixel pointer based on data from the specified MCUs
  for (int y = 0; y < outputHeight / 8; y++) {
    for (int x = 0; x < outputWidth / 8; x++) {
      auto pixelBlock = MCUs[mcuNum].getAllMatrices();

      for (int v = 0; v < 8; ++v) {
        for (int u = 0; u < 8; ++u) {
          (*_pixelPtr)[y * 8 + v][x * 8 + u].comp[0] = pixelBlock[0][v][u]; // R
          (*_pixelPtr)[y * 8 + v][x * 8 + u].comp[1] = pixelBlock[1][v][u]; // G
          (*_pixelPtr)[y * 8 + v][x * 8 + u].comp[2] = pixelBlock[2][v][u]; // B
        }
      }
      mcuNum++;
    }
  }

  /*由于可能存在因最小编码单元MCU的8x8块处理而对图像尺寸的调整，该函数同时进行修剪以确保输出图像的尺寸与JPEG图像的原始尺寸匹配，去除多余的列和行*/
  if (_imgWidth != outputWidth) {
    for (auto &&row : *_pixelPtr)
      for (int c = 0; c < 8 - _imgWidth % 8; ++c)
        row.pop_back();
  }
  if (_imgHeight != outputHeight) {
    for (int c = 0; c < 8 - _imgHeight % 8; ++c)
      _pixelPtr->pop_back();
  }
  return 0;
}

int Image::outputToYUVFile(const string &outputFileName) {
  if (this->_imgWidth <= 0 || this->_imgHeight <= 0)
    return -1;
  std::ofstream outputFile(outputFileName + ".yuv", std::ios::out);
  if (!outputFile.is_open() || !outputFile.good()) {
    cout << "Unable to create dump file \'" + outputFileName + "\'."
         << std::endl;
    return -1;
  }

  vector<uint8_t> bufferY, bufferCb, bufferCr;
  const int fileSize = _imgWidth * _imgHeight;
  bufferY.reserve(fileSize);
  bufferCb.reserve(fileSize);
  bufferCr.reserve(fileSize);

  for (auto &&row : *_pixelPtr) {
    for (auto &&pixel : row) {
      bufferY.push_back((uint8_t)pixel.comp[YUVComponents::Y]);
      bufferCb.push_back((uint8_t)pixel.comp[YUVComponents::Cb]);
      bufferCr.push_back((uint8_t)pixel.comp[YUVComponents::Cr]);
    }
  }
  outputFile.write((const char *)bufferY.data(), bufferY.size());
  outputFile.write((const char *)bufferCb.data(), bufferCb.size());
  outputFile.write((const char *)bufferCr.data(), bufferCr.size());

  cout << "Raw image data dumped to file: " + outputFileName +
              "    `ffplay -video_size "
       << _imgWidth << "x" << _imgHeight << " -pixel_format yuv444p  "
       << outputFileName << ".yuv`" << std::endl;
  outputFile.close();
  return 0;
}

int Image::outputToPPMFile(const string &outputFileName) {
  if (this->_imgWidth <= 0 || this->_imgHeight <= 0)
    return -1;
  if (!_pixelPtr) {
    std::cerr << "\033[31mFail init _pixelPtr\033[0m" << std::endl;
    return -1;
  }
  std::ofstream outputFile(outputFileName + ".ppm", std::ios::out);

  if (!outputFile.is_open() || !outputFile.good()) {
    cout << "Unable to create dump file \'" + outputFileName + "\'."
         << std::endl;
    return -1;
  }

  outputFile << "P6" << std::endl;
  outputFile << "# PPM dump create using libKPEG: yangjing" << std::endl;
  outputFile << _imgWidth << " " << _imgHeight << std::endl;
  outputFile << 255 << std::endl;

  for (auto &&row : *_pixelPtr) {
    for (auto &&pixel : row)
      outputFile << (uint8_t)pixel.comp[RGBComponents::RED]
                 << (uint8_t)pixel.comp[RGBComponents::GREEN]
                 << (uint8_t)pixel.comp[RGBComponents::BLUE];
  }

  cout << "Raw image data dumped to file: " + outputFileName + ".ppm"
       << std::endl;
  outputFile.close();
  return 0;
}
