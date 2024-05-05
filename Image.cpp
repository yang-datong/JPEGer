#include "Image.hpp"
#include "Type.hpp"
#include <fstream>

int Image::sOutputFileType = FileFormat::PPM;
int Image::sInputFileType = FileFormat::YUV;

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
      auto pixelBlock = MCUs[mcuNum].getMatrices();

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
  ofstream outputFile(outputFileName, std::ios::out);
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
              " `ffplay -video_size "
       << _imgWidth << "x" << _imgHeight << " -pixel_format yuv444p  "
       << outputFileName << std::endl;
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
  std::ofstream outputFile(outputFileName, std::ios::out);

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

  cout << "Raw image data dumped to file: " + outputFileName << std::endl;
  outputFile.close();
  return 0;
}

int Image::readJPEGFile(const string &filePath, uint8_t *&buf, int &bufSize) {
  return readFile(filePath, buf, bufSize);
}

int Image::readYUVFile(const string &filePath, uint8_t *&buf, int &bufSize) {
  return readFile(filePath, buf, bufSize);
}

int Image::readFile(const string &filePath, uint8_t *&buf, int &bufSize) {
  ifstream file(filePath, std::ios::binary | std::ios::in);
  if (!file.is_open())
    return -1;

  /* 适当调整，读取一个17M的文件要花近1分钟 */
  const int readBufSize = 1024;
  /* 适当调整，读取一个17M的文件要花近1秒 */
  // const int readBufSize = 1024 * 1024;
  bool isRead = false;

  uint8_t *fileBuffer = new uint8_t[readBufSize];
  while (!isRead) {
    file.read(reinterpret_cast<char *>(fileBuffer), readBufSize);
    if (file.gcount() != 0) {
      uint8_t *const tmp = new uint8_t[bufSize + file.gcount()];
      memcpy(tmp, buf, bufSize);
      memcpy(tmp + bufSize, fileBuffer, file.gcount());
      if (buf) {
        delete[] buf;
        buf = nullptr;
      }
      buf = tmp;
      bufSize += file.gcount();
    } else
      isRead = true;
  }
  delete[] fileBuffer;
  fileBuffer = nullptr;
  file.close();
  if (buf == nullptr || bufSize <= 0)
    return -1;

  if (!file.eof() && file.fail())
    return -2;
  return 0;
}

/* YUV444的planner格式转为packed模式 */
int Image::YUV444PlanarToPacked(uint8_t *planarBuffer, uint8_t *packedBuffer,
                                int width, int height) {
  if (planarBuffer == nullptr || width <= 0 || height <= 0)
    return -1;

  int frameSize = width * height;

  uint8_t *bufferY = planarBuffer;
  uint8_t *bufferU = planarBuffer + frameSize;
  uint8_t *bufferV = planarBuffer + frameSize * 2;

  for (int i = 0; i < frameSize; i++) {
    *packedBuffer++ = bufferY[i]; // Y
    *packedBuffer++ = bufferU[i]; // U
    *packedBuffer++ = bufferV[i]; // V
  }
  return 0;
}
