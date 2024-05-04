#include "Encoder.hpp"
#include "Common.hpp"
#include <bitset>
#include <cstdint>
#include <fstream>

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

  encodeScanData(outputFile);

  uint8_t EOI[2] = {0xff, JFIF::EOI};
  outputFile.write((const char *)EOI, sizeof(EOI));

  outputFile.close();
  return 0;
}

void Encoder::writeBitStream(const string &scanData, ofstream &outputFile) {
  uint8_t byteBuffer = 0; // 用于暂存构建中的字节
  int bitCount = 0;       // 累计构建字节中的位数

  for (char bit : scanData) {
    byteBuffer = (byteBuffer << 1) | (bit - '0'); // 将下一个位加入到字节缓冲区
    bitCount++;                                   // 增加位的计数

    if (bitCount == 8) {                             // 如果字节已经填满
      outputFile.put(static_cast<char>(byteBuffer)); // 将字节写入文件
      if ((uint8_t)byteBuffer == 0xff) {
        char zero = 0x00;
        outputFile.write(&zero, 1);
      }
      bitCount = 0;   // 重置计数器
      byteBuffer = 0; // 重置字节缓冲区
    }
  }

  // 处理最后一个不完整的字节（如果有）
  if (bitCount > 0) {
    byteBuffer <<= (8 - bitCount); // 左移剩余的位，确保它们在字节的高位
    outputFile.put(static_cast<char>(byteBuffer)); // 将最后的字节写入文件
    if ((uint8_t)byteBuffer == 0xff) {
      char zero = 0x00;
      outputFile.write(&zero, 1);
    }
  }
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

int Encoder::encodeScanData(ofstream &outputFile) {
  /* TODO YangJing 暂时写死宽高 <24-04-29 22:11:07> */
  const int imgWidth = 512, imgHeight = 512;
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

  // int MCUCount = (imgWidth * imgHeight) / (MCU_UNIT_SIZE);
  // int index = 0;
  string scanData;

  mark::HuffmanTrees huffmanTree =
      static_cast<mark::DHT *>(_dht)->getHuffmanTree();

  vector<QuantizationTable> quantizationTables =
      static_cast<mark::DQT *>(_dqt)->getQuantizationTables();
  if (quantizationTables.size() == 0 || quantizationTables[0].size() == 0 ||
      quantizationTables[1].size() == 0) {
    std::cerr << "\033[31mError -> quantizationTables.size\033[0m" << std::endl;
    return -1;
  }

  for (int y = 0; y < imgHeight; y += 8) {
    for (int x = 0; x < imgWidth; x += 8) {
      UCompMatrices matrix;
      for (int dy = 0; dy < 8; ++dy) {
        for (int dx = 0; dx < 8; ++dx) {
          matrix[0][dy][dx] = bufferY[(y + dy) * imgHeight + (x + dx)];
          matrix[1][dy][dx] = bufferU[(y + dy) * imgHeight + (x + dx)];
          matrix[2][dy][dx] = bufferV[(y + dy) * imgHeight + (x + dx)];
        }
      }
      _MCU.push_back(MCU(matrix, quantizationTables));
    }
  }

  for (int i = 0; i < (int)_MCU.size(); i++) {
    auto mcu = _MCU[i];
    mcu.startEncode();
    for (int imageComponent = 0; imageComponent < 3; imageComponent++) {
      RLE rle = mcu.getRLE();
      bool HuffTableID = imageComponent == 0 ? HT_Y : HT_CbCr;
      if (rle[imageComponent].size() != 0) {
        // if (imageComponent == 0) {
        //   std::cout << "DC:" << rle[imageComponent][0] << std::endl;
        // }
        // std::cout << "pop DC:" << rle[imageComponent][0] << std::endl;
        string coeffDC = VLIEncode(rle[imageComponent][0]);
        uint8_t coeffDCLen = coeffDC.length();
        string value = huffmanTree[HT_DC][HuffTableID].encode(coeffDCLen);
        value += coeffDC;
        /* Huffman编码后不会存在0xff的字节 */
        scanData.append(value);
        /* DC的系数长度不应该大于12 */
        if (coeffDCLen > 12) {
          std::cerr << "\033[31mcoeffDCLen:" << (int)coeffDCLen << "\033[0m"
                    << std::endl;
          return -1;
        }
        for (int i = 1; i <= (int)rle[imageComponent].size() - 2; i += 2) {
          uint8_t zeroCount = rle[imageComponent][i];

          uint8_t symbol;
          if (zeroCount == 0 && rle[imageComponent][i + 1] == 0)
            symbol = 0x00;
          else if (zeroCount == 0xf && rle[imageComponent][i + 1] == 0)
            symbol = 0xf0;
          else {
            string coeffAC = VLIEncode(rle[imageComponent][i + 1]);
            uint8_t coeffACLen = coeffAC.length();
            symbol = combineOneByte(zeroCount, coeffACLen);
          }
          string value = huffmanTree[HT_AC][HuffTableID].encode(symbol);
          if (symbol == 0xf0) {
            // printZRLInfo(HuffTableID, value, symbol);
          } else if (symbol == 0x00) {
            /* 这里应该是EOB（有的图片编码，不一定会进这里） */
            // printEOBInfo(HuffTableID, value, symbol);
          } else {
            // printCommonInfo(HuffTableID, value, symbol, zeroCount,
            // coeffDCLen);
            scanData.append(value);
          }
        }
      }
    }
  }
  outputFile.write((const char *)scanData.c_str(), scanData.length());
  writeBitStream(scanData, outputFile);
  SAFE_DELETE_ARRAY(buffer);
  bufferSize = 0;
  return 0;
}

void Encoder::printZRLInfo(int HuffTableID, string &value, int symbol) {
  std::cout << "AC -> {" << std::endl;
  std::cout << "\tCommom:" << (HuffTableID == 0 ? "Luma" : "Chroma")
            << ", Run/Size:F/0(ZRL)"
            << ", Code Length:" << value.length() << ", Code word:" << value
            << ", " << (int)symbol << " -encode-> " << value << std::endl;
  std::cout << "}" << std::endl;
}
void Encoder::printEOBInfo(int HuffTableID, string &value, int symbol) {
  std::cout << "AC -> {" << std::endl;
  std::cout << "\tCommom:" << (HuffTableID == 0 ? "Luma" : "Chroma")
            << ", Run/Size:0/0(EOB)"
            << ", Code Length:" << value.length() << ", Code word:" << value
            << ", " << (int)symbol << " -encode-> " << value << std::endl;
  std::cout << "}" << std::endl;
}

void Encoder::printCommonInfo(int HuffTableID, string &value, int symbol,
                              int zeroCount, int coeffACLen) {
  ///* T.81 page 153 */
  std::cout << "AC -> {" << std::endl;
  std::cout << "\tCommon:" << (HuffTableID == 0 ? "Luma" : "Chroma")
            << ", Run/Size:" << (int)zeroCount << "/" << (int)coeffACLen
            << ", Code Length:" << value.length() << ", Code word:" << value
            << ", " << (int)symbol << " -encode-> " << value << std::endl;
  std::cout << "}" << std::endl;
}
