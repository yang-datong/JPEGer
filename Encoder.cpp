#include "Encoder.hpp"
#include "APP0.hpp"
#include "Common.hpp"
#include "Image.hpp"
#include "MCU.hpp"
#include <cstdint>
#include <fstream>
#include <ios>

Encoder::Encoder(const string &inputFilePath, const string &outputFilePath)
    : _inputFilePath(inputFilePath), _outputFilePath(outputFilePath) {
  _app0 = new mark::APP0();
  _com = new mark::COM();
  _dqt = new mark::DQT();
  _sof0 = new mark::SOF0();
  _dht = new mark::DHT();
  _sos = new mark::SOS();
};

Encoder::Encoder(const string &inputFilePath, const string &outputFilePath,
                 const int inputTypeFile)
    : Encoder(inputFilePath, outputFilePath) {
  Image::sInputFileType = inputTypeFile;
}

Encoder::~Encoder() {
  _bufSize = 0;
  SAFE_DELETE_ARRAY(_buf);
  SAFE_DELETE(_app0);
  SAFE_DELETE(_com);
  SAFE_DELETE(_dqt);
  SAFE_DELETE(_sof0);
  SAFE_DELETE(_dht);
  SAFE_DELETE(_sos);
}

int Encoder::startMakeMarker() {
  /* TODO YangJing 暂时写死宽高 <24-04-29 22:11:07> */
  const int imgWidth = 512, imgHeight = 512;

  ofstream outputFile(_outputFilePath, ios_base::binary);
  std::cout << "_inputFilePath:" << _inputFilePath << std::endl;
  std::cout << "_outputFilePath:" << _outputFilePath << std::endl;

  uint8_t SOI[2] = {0xff, JFIF::SOI};
  outputFile.write((const char *)SOI, sizeof(SOI));

  _app0->package(outputFile);
  _com->package(outputFile);
  _dqt->package(outputFile);
  _sof0->package(outputFile);
  _dht->package(outputFile);
  _sos->package(outputFile);

  /* TODO YangJing 这里要放在另一个地方，或者说截取下面的一部分放在MCU.cpp中
   * <24-04-29 22:24:29> */
  uint8_t *buffer = nullptr;
  int bufferSize = 0;
  if (Image::readYUVFile(_inputFilePath, buffer, bufferSize))
    return -1;

  /* YUV444p */
  if (bufferSize != (imgWidth * imgHeight) * 3) {
    std::cout << "Only support YUV444p" << std::endl;
    return -1;
  }

  int WH = imgWidth * imgHeight;
  // int bufferYSzie = WH, bufferUSize = WH, bufferVSize = WH;
  uint8_t *bufferY = buffer;
  uint8_t *bufferU = buffer + WH;
  uint8_t *bufferV = buffer + WH * 2;

  /* TODO YangJing 明天做 <24-04-29 22:26:30> */
  int MCUCount = (imgWidth * imgHeight) / (MCU_UNIT_SIZE);
  int index = 0;
  for (int i = 0; i < MCUCount; i++) {
    UCompMatrices matrix;
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        matrix[0][y][x] = bufferY[index];
        matrix[1][y][x] = bufferU[index];
        matrix[2][y][x] = bufferV[index];
        index++;
      }
    }

    vector<QuantizationTable> quantizationTables =
        static_cast<mark::DQT *>(_dqt)->getQuantizationTables();
    /* TODO YangJing 这里还需要再进一步改进 <24-04-30 17:01:57> */
    if (quantizationTables.size() == 0 || quantizationTables[0].size() == 0 ||
        quantizationTables[1].size() == 0) {
      std::cerr << "\033[31mError -> quantizationTables.size\033[0m"
                << std::endl;
      return -1;
    }
    _MCU.push_back(MCU(matrix, quantizationTables));
  }
  /* TODO YangJing  <24-04-29 22:24:05> */

  uint8_t EOI[2] = {0xff, JFIF::EOI};
  outputFile.write((const char *)EOI, sizeof(EOI));

  SAFE_DELETE_ARRAY(buffer);
  bufferSize = 0;
  outputFile.close();
  return 0;
}
