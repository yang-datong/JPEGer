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

Encoder::Encoder(const string &inputFilePath, const string &outputFilePath,
                 const int inputTypeFile, const int imgWidth,
                 const int imgHeight)
    : Encoder(inputFilePath, outputFilePath) {
  Image::sInputFileType = inputTypeFile;
  if (imgWidth > 0 && imgHeight > 0) {
    this->_imgWidth = imgWidth;
    this->_imgHeight = imgHeight;
  }
}

Encoder::~Encoder() {
  _bufSize = 0;
  SAFE_DELETE_ARRAY(_packedBuffer);
  SAFE_DELETE(_app0);
  SAFE_DELETE(_com);
  SAFE_DELETE(_dqt);
  SAFE_DELETE(_sof0);
  SAFE_DELETE(_dht);
  SAFE_DELETE(_sos);
}

int Encoder::readFileBuferToYUV444Packed() {
  if (Image::sInputFileType == FileFormat::YUV) {
    uint8_t *buffer = nullptr;
    if (Image::readYUVFile(_inputFilePath, buffer, _bufSize)) {
      std::cerr << "\033[31mreadYUVFile failed " + _inputFilePath + " \033[0m"
                << std::endl;
      return -1;
    }

    /* YUV444p */
    if (_bufSize != (_imgWidth * _imgHeight) * 3) {
      std::cout << "Only support YUV444p" << std::endl;
      return -1;
    }

    _packedBuffer = new uint8_t[_imgWidth * _imgHeight * 3];
    Image::YUV444PlanarToPacked(buffer, _packedBuffer, _imgWidth, _imgHeight);
    SAFE_DELETE_ARRAY(buffer);

  } else if (Image::sInputFileType == FileFormat::BMP) {
    File::BMP bmp;
    bmp.BMPToYUV(_inputFilePath);
    _imgWidth = bmp.getWidth();
    _imgHeight = bmp.getHeight();
    _packedBuffer = new uint8_t[_imgWidth * _imgHeight * 3];
    /* 走完这里的话，BMP对象会销毁，则会自动调用close函数，所以这里必须使用copy*/
    memcpy(_packedBuffer, bmp.getYUV444PackedBuffer(), bmp.getYUVSize());
  }

  if (_packedBuffer == nullptr) {
    std::cerr << "\033[31mFail to get packedBuffer\033[0m" << std::endl;
    return -1;
  }
  return 0;
}

int Encoder::startMakeMarker() {
  ofstream outputFile(_outputFilePath, ios_base::binary);

  mark::SOF0 *sof0 = static_cast<mark::SOF0 *>(_sof0);
  sof0->setImageWidth(_imgWidth);
  sof0->setImageHeight(_imgHeight);

  std::cout << "-------------------------------------------- startMakeMarker "
               "--------------------------------------------"
            << std::endl;
  std::cout << "Start OF Image" << std::endl;
  uint8_t SOI[2] = {0xff, JFIF::SOI};
  outputFile.write((const char *)SOI, sizeof(SOI));

  std::cout << "Decode Application-specific" << std::endl;
  _app0->package(outputFile);
  std::cout << "Comment" << std::endl;
  _com->package(outputFile);
  std::cout << "Define Quantization Table(s)" << std::endl;
  _dqt->package(outputFile);
  std::cout << "Start Of Frame 0" << std::endl;
  _sof0->package(outputFile);
  std::cout << "Define Huffman Table(s)" << std::endl;
  _dht->package(outputFile);
  std::cout << "Start of Scan" << std::endl;
  _sos->package(outputFile);

  std::cout << "-------------------------------------------- encodeScanData "
               "--------------------------------------------"
            << std::endl;
  encodeScanData(outputFile);

  std::cout << "End OF Image" << std::endl;
  uint8_t EOI[2] = {0xff, JFIF::EOI};
  outputFile.write((const char *)EOI, sizeof(EOI));

  cout << "JPEG image data write to file: " + _outputFilePath << std::endl;
  outputFile.close();
  return 0;
}

void Encoder::writeBitStream(const uint32_t data, uint32_t len,
                             ofstream &outputFile, bool is_flush = false) {
  static uint32_t bit_ptr = 0;
  static uint32_t bitbuf = 0x00000000;
  uint8_t w = 0x00;

  bitbuf |= data << (32 - bit_ptr - len);
  bit_ptr += len;

  while (bit_ptr >= 8) {
    w = (uint8_t)((bitbuf & 0xFF000000) >> 24);
    outputFile.write((char *)&w, 1);
    if (w == 0xFF) {
      char zero = 0x00;
      outputFile.write((char *)&zero, 1);
    }
    bitbuf <<= 8;
    bit_ptr -= 8;
  }

  if (is_flush) {
    w = (uint8_t)((bitbuf & 0xFF000000) >> 24);
    outputFile.write((char *)&w, 1);
  }
}

/* VLI编码：
 * 1. 计算绝对值的二进制表示。
 * 2. 绝对值二进制表示中的位数决定编码的长度字段。
 * 3. 对于正数，直接使用其绝对值的二进制表示作为编码的剩余部分。
 * 4. 对于负数，采用其绝对值二进制表示的反码（所有1变为0，所有0变为1）。
 * */
string Encoder::VLIEncode(int value) {
  if (value == 0) return "-1";
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
  mark::HuffmanTrees huffmanTree =
      static_cast<mark::DHT *>(_dht)->getHuffmanTree();

  vector<QuantizationTable> quantizationTables =
      static_cast<mark::DQT *>(_dqt)->getQuantizationTables();
  if (quantizationTables.size() == 0 || quantizationTables[0].size() == 0 ||
      quantizationTables[1].size() == 0) {
    std::cerr << "\033[31mError -> quantizationTables.size\033[0m" << std::endl;
    return -1;
  }

  for (int y = 0; y < _imgHeight; y += 8) {
    for (int x = 0; x < _imgWidth; x += 8) {
      UCompMatrices matrix;
      for (int dy = 0; dy < 8; ++dy) {
        for (int dx = 0; dx < 8; ++dx) {
          int offset = ((y + dy) * _imgHeight + (x + dx)) * 3;
          matrix[0][dy][dx] = _packedBuffer[offset];
          matrix[1][dy][dx] = _packedBuffer[offset + 1];
          matrix[2][dy][dx] = _packedBuffer[offset + 2];
        }
      }
      _MCU.push_back(MCU(matrix, quantizationTables));
    }
  }

  _encodeScanData(huffmanTree, outputFile);

  SAFE_DELETE_ARRAY(_packedBuffer);
  return 0;
}

int Encoder::_encodeScanData(mark::HuffmanTrees huffmanTree,
                             ofstream &outputFile) {
  for (int i = 0; i < (int)_MCU.size(); i++) {
    auto mcu = _MCU[i];
    mcu.startEncode();
    RLE rle = mcu.getRLE();
    /*NOTE: 每个分量编码并写入文件的操作是分开进行的，而不是等到所有分量都编码完成后再合并写入。因为JPEG文件是按照一定的结构组织的，其中每个分量的数据在文件中通常有明确的开始和结束标示，所以它们是分别处理的*/
    for (int imageComponent = 0; imageComponent < 3; imageComponent++) {
      bool HuffTableID = imageComponent == 0 ? HT_Y : HT_CbCr;
      string coeffDC = VLIEncode(rle[imageComponent][1]);
      uint8_t coeffDCLen = coeffDC == "-1" ? 0 : coeffDC.length();
      uint8_t &category = coeffDCLen;
      string value = huffmanTree[HT_DC][HuffTableID].encode(category);
      // printDCInfo(HuffTableID, category, value.length(), value);

      writeBitStream(bitset<16>(value).to_ulong(), value.length(), outputFile);
      if (rle[imageComponent][1] != 0)
        writeBitStream(bitset<16>(coeffDC).to_ulong(), coeffDC.length(),
                       outputFile);

      if (category > 11)
        std::cerr << "\033[31mDC Category:" << (int)category << "\033[0m"
                  << std::endl;

      for (int i = 2; i <= (int)rle[imageComponent].size() - 2; i += 2) {
        uint8_t zeroCount = rle[imageComponent][i];

        uint8_t symbol;
        string coeffAC = "";
        uint8_t coeffACLen;

        if (zeroCount == 0 && rle[imageComponent][i + 1] == 0)
          symbol = 0x00;
        else if (zeroCount == 0xf && rle[imageComponent][i + 1] == 0)
          symbol = 0xf0;
        else {
          coeffAC = VLIEncode(rle[imageComponent][i + 1]);
          coeffACLen = coeffAC.length();
          symbol = combineOneByte(zeroCount, coeffACLen);
        }
        string value = huffmanTree[HT_AC][HuffTableID].encode(symbol);
        writeBitStream(bitset<16>(value).to_ulong(), value.length(),
                       outputFile);
        writeBitStream(bitset<16>(coeffAC).to_ulong(), coeffAC.length(),
                       outputFile);

        // if (symbol == 0xf0)
        //   printZRLInfo(HuffTableID, value, symbol);
        // else if (symbol == 0x00)
        //   printEOBInfo(HuffTableID, value, symbol);
        // else
        //   printCommonInfo(HuffTableID, value, symbol, zeroCount, coeffDCLen);

        //在Huffman编码后的数据中，不会预设一个编码是0xFF，因为Huffman编码是根据数据的频率动态生成的，它不会固定地将某个值编码为0xFF。出现0xFF字节的情况是在你将Huffman编码后的比特流合并并以每8位分割成字节后可能发生的。比如ZRL和EOI也是这样，每个huffman编码按顺序读取都是唯一的。
      }
    }
  }
  writeBitStream(0, 0, outputFile, true);
  return 0;
}

void Encoder::printDCInfo(int HuffTableID, int category, int codeLen,
                          string &value) {
  std::cout << "DC -> {" << std::endl;
  std::cout << "\tCommom:" << (HuffTableID == 0 ? "Luma" : "Chroma")
            << ", Category: " << category << ", Code Length:" << codeLen
            << ", Code word:" << value << ", " << category << " -encode-> "
            << value << std::endl;
  std::cout << "}" << std::endl;
}

void Encoder::printZRLInfo(int HuffTableID, string &value, int symbol) {
  std::cout << "AC -> {" << std::endl;
  std::cout << "\tCommom:" << (HuffTableID == 0 ? "Luma" : "Chroma")
            << ", Run/Size:F/0(ZRL)" << ", Code Length:" << value.length()
            << ", Code word:" << value << ", " << (int)symbol << " -encode-> "
            << value << std::endl;
  std::cout << "}" << std::endl;
}

void Encoder::printEOBInfo(int HuffTableID, string &value, int symbol) {
  std::cout << "AC -> {" << std::endl;
  std::cout << "\tCommom:" << (HuffTableID == 0 ? "Luma" : "Chroma")
            << ", Run/Size:0/0(EOB)" << ", Code Length:" << value.length()
            << ", Code word:" << value << ", " << (int)symbol << " -encode-> "
            << value << std::endl;
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
