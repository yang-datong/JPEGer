#include "Image.hpp"
#include "Type.hpp"
#include <cstdint>
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
       << _imgWidth << "x" << _imgHeight << " -pixel_format yuv444p "
       << outputFileName + "`" << std::endl;
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

/* U开头表示matrix内的元素均为无符号整数或uint8_t,uint16_t,uint32_t */
int Image::YUVToRGB(UCompMatrices &Umatrix) {
  /* YUV444 packed 表示为三个字节一个像素，而矩阵是8x8个像素，故有8x8x3个字节*/
  for (int y = 0; y < COMPONENT_SIZE; ++y) {
    for (int x = 0; x < COMPONENT_SIZE; ++x) {
      float Y = Umatrix[0][y][x];
      float Cb = Umatrix[1][y][x];
      float Cr = Umatrix[2][y][x];

      int R = (int)floor(Y + 1.402 * (1.0 * Cr - 128));
      int G = (int)floor(Y - 0.34414 * (1.0 * Cb - 128) -
                         0.71414 * (1.0 * Cr - 128));
      int B = (int)floor(Y + 1.772 * (1.0 * Cb - 128));

      R = max(0, min(R, 255));
      G = max(0, min(G, 255));
      B = max(0, min(B, 255));

      Umatrix[0][y][x] = R;
      Umatrix[1][y][x] = G;
      Umatrix[2][y][x] = B;
    }
  }
  return 0;
}

int Image::RGBToYUV(uint8_t *rgbBuffer, int width, int height, int &YUVSize,
                    uint8_t *&yuv444PlanarBuffer,
                    uint8_t *&yuv444PackedBuffer) {
  YUVSize = height * width * 3;
  int size = height * width;
  uint8_t *_Y = nullptr, *_U = nullptr, *_V = nullptr;
  _Y = new uint8_t[size];
  _U = new uint8_t[size];
  _V = new uint8_t[size];
  int offset = -1;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      offset = i * height + j;
      uint8_t r = rgbBuffer[offset * 3];
      uint8_t g = rgbBuffer[offset * 3 + 1];
      uint8_t b = rgbBuffer[offset * 3 + 2];

      _Y[offset] = round(0.299f * r + 0.587f * g + 0.114f * b);
      _U[offset] = round(-0.1687f * r - 0.3313f * g + 0.5f * b + 128);
      _V[offset] = round(0.5f * r - 0.4187f * g - 0.0813f * b + 128);
    }
  }
  yuv444PlanarBuffer = new uint8_t[YUVSize];
  memcpy(yuv444PlanarBuffer, _Y, size);
  memcpy(yuv444PlanarBuffer + size, _U, size);
  memcpy(yuv444PlanarBuffer + size * 2, _V, size);

  yuv444PackedBuffer = new uint8_t[YUVSize];
  Image::YUV444PlanarToPacked(yuv444PlanarBuffer, yuv444PackedBuffer, width,
                              height);
  delete[] _Y;
  _Y = nullptr;
  delete[] _U;
  _U = nullptr;
  delete[] _V;
  _V = nullptr;
  return 0;
}
