#include "Encoder.hpp"
#include "APP0.hpp"
#include "Common.hpp"
#include "Image.hpp"
#include "MCU.hpp"
#include "Type.hpp"
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

  int MCUCount = (imgWidth * imgHeight) / (MCU_UNIT_SIZE);
  int index = 0;
  string scanData;

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
    if (quantizationTables.size() == 0 || quantizationTables[0].size() == 0 ||
        quantizationTables[1].size() == 0) {
      std::cerr << "\033[31mError -> quantizationTables.size\033[0m"
                << std::endl;
      return -1;
    }
    //_MCU.push_back(MCU(matrix, quantizationTables));
    MCU mcu(matrix, quantizationTables);
    RLE rle = mcu.getAllRLE();

    for (int imageComponent = 0; imageComponent < 3; imageComponent++) {
      if (rle[imageComponent].size() == 0) {
      } else {
        for (int i = 0; i <= rle[imageComponent].size() - 2; i += 2) {
          int zeroCount = rle[imageComponent][i];
          string zeros(zeroCount, '0');
          string coeffDC = VLIEncode(rle[imageComponent][i + 1]);
          scanData.append(zeros).append(coeffDC);
          /* TODO YangJing Huffman编码 <24-05-01 00:18:03> */
        }
      }
    }
    std::cout << "scanData:" << scanData << std::endl;
    exit(0);
    /* TODO YangJing  <24-05-01 00:18:07> */
  }

  uint8_t EOI[2] = {0xff, JFIF::EOI};
  outputFile.write((const char *)EOI, sizeof(EOI));

  SAFE_DELETE_ARRAY(buffer);
  bufferSize = 0;
  outputFile.close();
  return 0;
}

/* VLI编码：
 * 1. 计算绝对值的二进制表示。
 * 2. 绝对值二进制表示中的位数决定编码的长度字段。
 * 3. 对于正数，直接使用其绝对值的二进制表示作为编码的剩余部分。
 * 4. 对于负数，采用其绝对值二进制表示的反码（所有1变为0，所有0变为1）。
 * */
string Encoder::VLIEncode(int value) {
  if (value == 0)
    return "-1";
  /* 这里有种特殊情况，如果数值为0则直接返回-1,因为-1解码会得到0（如果按照正常编码0编码后还是0）*/

  /* 确定这个二进制表示占用的位数,即编码后的总长度（使用绝对值） */
  string binaryValue = bitset<16>(abs(value)).to_string();
  binaryValue = eraseHeightOfZero(binaryValue);
  int binaryValueLen = binaryValue.size();

  /* 判断原数据的正负性，如果是负数需要采用补码形式 */
  if (value < 0)
    for (int i = 0; i < (int)binaryValue.size(); i++)
      /* 二进制补码形式：按位取反 */
      binaryValue[i] = binaryValue[i] == '0' ? '1' : '0';

  binaryValue = eraseHeightOfZero(binaryValue);

  for (int i = (int)binaryValue.size(); i < binaryValueLen; i++)
    binaryValue.insert(0, "0");

  return binaryValue;
}
