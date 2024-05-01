#include "Encoder.hpp"

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

  mark::HuffmanTrees huffmanTree =
      static_cast<mark::DHT *>(_dht)->getHuffmanTree();

  for (int i = 0; i < MCUCount; i++) {
    int matrixIndex = 0;
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
    MCU mcu(matrix, quantizationTables);
    RLE rle = mcu.getAllRLE();

    for (int imageComponent = 0; imageComponent < 3; imageComponent++) {
      bool HuffTableID = imageComponent == 0 ? HT_Y : HT_CbCr;
      if (rle[imageComponent].size() != 0) {
        if (matrixIndex == 0) {
          string coeffDC = VLIEncode(rle[imageComponent][0]);
          uint8_t coeffDCLen = coeffDC.length();
          string value = huffmanTree[HT_DC][HuffTableID].encode(coeffDCLen);
          value += coeffDC;
          scanData.append(value);
          /* TODO YangJing 这里的coeffDCLen不应该大于12,感觉错了 <24-05-02
           * 00:07:01> */
          //          std::cout << "coeffDCLen:" << (int)coeffDCLen <<
          //          std::endl;
        } else {
          for (int i = 1; i <= (int)rle[imageComponent].size() - 2; i += 2) {
            uint8_t zeroCount = rle[imageComponent][i];
            if (zeroCount == 0 && rle[imageComponent][i + 1] == 0)
              break;
            string coeffAC = VLIEncode(rle[imageComponent][i + 1]);
            uint8_t coeffACLen = coeffAC.length();
            uint8_t symbol = zeroCount;
            /* TODO YangJing 封装半个字节合并操作： <24-05-01 22:42:38> */
            /*
             * uint8_t combineRLEandCoeff(uint8_t runLength, uint8_t
             * coeffLength) { return (runLength << 4) | (coeffLength & 0x0F);
             * }
             * */
            symbol <<= 4;
            symbol |= coeffACLen;
            string value = huffmanTree[HT_AC][HuffTableID].encode(symbol);
            if (checkSpace(value)) {
              ///* T.81 page 153 */
              // std::cout << "AC -> {" << std::endl;
              // std::cout << "\tCommom:" << (HuffTableID == 0 ? "Luma" :
              // "Chroma")
              //           << ", Run/Size:" << (int)zeroCount << "/"
              //           << (int)coeffACLen << ", Code Length:" <<
              //           value.length()
              //           << ", Code word:" << value << ", " << (int)symbol
              //           << " -encode-> " << value << std::endl;
              // std::cout << "}" << std::endl;
              scanData.append(value);
            }
          }
        }
      }
      /* TODO YangJing 这个位置不对，不是放在这里 <24-05-02 00:07:58> */
      matrixIndex++;
    }
  }
  //  std::cout << "scanData:" << scanData << std::endl;
  //  TODO 有问题 <24-05-01 18:23:32, YangJing>
  outputFile.write((const char *)scanData.c_str(), scanData.length());

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
