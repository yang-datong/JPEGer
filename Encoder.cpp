#include "Encoder.hpp"
#include "BMP.hpp"
#include "Common.hpp"
#include "Image.hpp"
#include "Type.hpp"
#include <array>
#include <bitset>
#include <cmath>
#include <cstddef>
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

Encoder::Encoder(const string &inputFilePath, const string &outputFilePath,
                 const int inputTypeFile, const int imgWidth,
                 const int imgHeight)
    : Encoder(inputFilePath, outputFilePath) {
  Image::sInputFileType = inputTypeFile;
  this->_imgWidth = imgWidth;
  this->_imgHeight = imgHeight;
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
  uint8_t *packedBuffer = nullptr;

  if (Image::sInputFileType == FileFormat::YUV) {
    uint8_t *buffer = nullptr;
    int bufferSize = 0;
    if (Image::readYUVFile(_inputFilePath, buffer, bufferSize)) {
      std::cerr << "\033[31mreadYUVFile failed" + _inputFilePath + " 033[0m"
                << std::endl;
      return -1;
    }

    /* YUV444p */
    if (bufferSize != (_imgWidth * _imgHeight) * 3) {
      std::cout << "Only support YUV444p" << std::endl;
      return -1;
    }

    packedBuffer = new uint8_t[_imgWidth * _imgHeight * 3];
    Image::YUV444PlanarToPacked(buffer, packedBuffer, _imgWidth, _imgHeight);
    SAFE_DELETE_ARRAY(buffer);
    bufferSize = 0;

  } else if (Image::sInputFileType == FileFormat::BMP) {
    File::BMP bmp;
    bmp.BMPToYUV(_inputFilePath);
    _imgWidth = bmp.getWidth();
    _imgHeight = bmp.getHeight();
    packedBuffer = new uint8_t[_imgWidth * _imgHeight * 3];
    /* 走完这里的话，BMP对象会销毁，则会自动调用close函数，所以这里必须使用copy*/
    memcpy(packedBuffer, bmp.getYUV444PackedBuffer(), bmp.getYUVSize());
  }

  if (packedBuffer == nullptr) {
    std::cerr << "\033[31mFail to get packedBuffer\033[0m" << std::endl;
    return -1;
  }

  mark::HuffmanTrees huffmanTree =
      static_cast<mark::DHT *>(_dht)->getHuffmanTree();

  vector<QuantizationTable> quantizationTables =
      static_cast<mark::DQT *>(_dqt)->getQuantizationTables();
  if (quantizationTables.size() == 0 || quantizationTables[0].size() == 0 ||
      quantizationTables[1].size() == 0) {
    std::cerr << "\033[31mError -> quantizationTables.size\033[0m" << std::endl;
    return -1;
  }

  // int8_t *Y = new int8_t[512 * 512];
  // int8_t *U = new int8_t[512 * 512];
  // int8_t *V = new int8_t[512 * 512];
  // int aaa = 0;

  //  for (int y = 0; y < _imgHeight; y += 8) {
  //    for (int x = 0; x < _imgWidth; x += 8) {
  //      UCompMatrices matrix;
  //      int block[3][64];
  //      for (int dy = 0; dy < 8; ++dy) {
  //        for (int dx = 0; dx < 8; ++dx) {
  //          int offset = ((y + dy) * _imgHeight + (x + dx)) * 3;
  //          int block_offset = (dy * 8 + dx) * 3;
  //
  //          block[0][block_offset] = round(packedBuffer[offset]);
  //          block[1][block_offset + 1] = round(packedBuffer[offset + 1]);
  //          block[2][block_offset + 2] = round(packedBuffer[offset + 2]);
  //
  //          // Y[aaa] = matrix[0][dy][dx] - 128;
  //          // U[aaa] = matrix[1][dy][dx] - 128;
  //          // V[aaa] = matrix[2][dy][dx] - 128;
  //          // aaa++;
  //        }
  //      }
  //      _MCU.push_back(MCU(matrix, quantizationTables));
  //    }
  //  }

  //  ofstream file("./demo.yuv");
  //  file.write((char *)Y, 512 * 512);
  //  file.write((char *)U, 512 * 512);
  //  file.write((char *)V, 512 * 512);

  // for (int y = 0; y < _imgHeight; y += 8) {
  //   for (int x = 0; x < _imgWidth; x += 8) {
  //     UCompMatrices matrix;
  //     array<array<uint16_t, 64>, 3> block;
  //     for (int dy = 0; dy < 8; ++dy) {
  //       for (int dx = 0; dx < 8; ++dx) {
  //         int offset = ((y + dy) * _imgHeight + (x + dx)) * 3;
  //         int block_offset = (dy * 8 + dx) * 3;
  //         block[0][block_offset] = packedBuffer[offset];
  //         block[1][block_offset] = packedBuffer[offset + 1];
  //         block[2][block_offset] = packedBuffer[offset + 2];
  //       }
  //     }
  //     arrayToMatrixUseZigZag(block[0], matrix[0]);
  //     arrayToMatrixUseZigZag(block[1], matrix[1]);
  //     arrayToMatrixUseZigZag(block[2], matrix[2]);
  //     _MCU.push_back(MCU(matrix, quantizationTables));
  //   }
  // }

  for (int y = 0; y < _imgHeight; y += 8) {
    for (int x = 0; x < _imgWidth; x += 8) {
      UCompMatrices matrix;
      for (int dy = 0; dy < 8; ++dy) {
        for (int dx = 0; dx < 8; ++dx) {
          int offset = ((y + dy) * _imgHeight + (x + dx)) * 3;
          matrix[0][dy][dx] = packedBuffer[offset];
          matrix[1][dy][dx] = packedBuffer[offset + 1];
          matrix[2][dy][dx] = packedBuffer[offset + 2];
        }
      }
      _MCU.push_back(MCU(matrix, quantizationTables));
    }
  }

  string scanData;
  for (int i = 0; i < (int)_MCU.size(); i++) {
    auto mcu = _MCU[i];
    mcu.startEncode();
    RLE rle = mcu.getRLE();
    for (int imageComponent = 0; imageComponent < 3; imageComponent++) {
      bool HuffTableID = imageComponent == 0 ? HT_Y : HT_CbCr;
      string coeffDC = VLIEncode(rle[imageComponent][1]);
      uint8_t coeffDCLen = coeffDC == "-1" ? 0 : coeffDC.length();
      uint8_t &category = coeffDCLen;
      string value = huffmanTree[HT_DC][HuffTableID].encode(category);
      // printDCInfo(HuffTableID, category, value.length(), value);

      value += coeffDC == "-1" ? "0" : coeffDC;
      if (rle[imageComponent][1] == 0) {
        scanData.append("0000000000000000");
        // jpeg_write_bits(0, 2, 0, outputFile);
      } else {
        scanData.append(value);
        // bitset<16> bit(value);
        // bitset<16> bit2(coeffDC);
        // jpeg_write_bits(bit.to_ulong(), value.length(), 0, outputFile);
        // jpeg_write_bits(bit2.to_ulong(), coeffDCLen, 0, outputFile);

        //        printf(
        //            "dc[0].code_word:%ld,dc[0].code_length:%ld,code:%ld,bit_num:%d\n",
        //            bit.to_ulong(), value.length(), bit2.to_ulong(),
        //            coeffDCLen);
        /* Huffman编码后最长长度为16 */
      }

      if (category > 11) {
        std::cerr << "\033[31mDC Category:" << (int)category << "\033[0m"
                  << std::endl;
        return -1;
      }
      for (int i = 2; i <= (int)rle[imageComponent].size() - 2; i += 2) {
        uint8_t zeroCount = rle[imageComponent][i];

        uint8_t symbol;
        string coeffAC;
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

        // if (symbol == 0xf0) {
        //   // printZRLInfo(HuffTableID, value, symbol);
        //   // bitset<16> bit(value);
        //   // jpeg_write_bits(bit.to_ulong(), value.length(), 0, outputFile);
        //   // printf("ac[0xf0].code_word:%ld,ac[0xf0].code_length:%ld\n",
        //   //        bit.to_ulong(), value.length());
        //   std::cout << "value:" << value << std::endl;
        //   scanData.append("0000000000000000");
        // } else if (symbol == 0x00) {
        //   // printEOBInfo(HuffTableID, value, symbol);
        //   scanData.append("1111111100000000");
        //   // bitset<16> bit(value);
        //   //  printf("ac[0xf0].code_word:%ld,ac[0xf0].code_length:%ld\n",
        //   //         bit.to_ulong(), value.length());
        //   // jpeg_write_bits(bit.to_ulong(), value.length(), 0, outputFile);
        // } else {
        //   // printCommonInfo(HuffTableID, value, symbol, zeroCount,
        //   // coeffDCLen);

        //  // bitset<16> bit(value);
        //  // bitset<16> bit2(coeffAC);
        //  //
        //  //
        //  printf("ac[run_size].code_word:%ld,ac[run_size].code_length:%ld,code:"
        //  //                   "%ld,bit_num:%d\n",
        //  //                   bit.to_ulong(), value.length(),
        //  // bit2.to_ulong(),
        //  //                   coeffACLen);

        //  // jpeg_write_bits(bit.to_ulong(), value.length(), 0, outputFile);
        //  // jpeg_write_bits(bit2.to_ulong(), coeffDCLen, 0, outputFile);
        //  scanData.append(value);
        //}
        //在Huffman编码后的数据中，你不会预设一个编码是0xFF，因为Huffman编码是根据数据的频率动态生成的，它不会固定地将某个值编码为0xFF。出现0xFF字节的情况是在你将Huffman编码后的比特流合并并以每8位分割成字节后可能发生的。比如ZRL和EOI也是这样，每个huffman编码按顺序读取都是唯一的。
        scanData.append(value);
      }

      /*每个分量编码并写入文件的操作是分开进行的，而不是等到所有分量都编码完成后再合并写入。这是因为JPEG文件是按照一定的结构组织的，其中每个分量的数据在文件中通常有明确的开始和结束标示，所以它们是分别处理的。*/
      writeBitStream(scanData, outputFile);
      scanData.clear();
    }
  }
  // 清除缓存
  // jpeg_write_bits(0, 0, 1, outputFile);
  SAFE_DELETE_ARRAY(packedBuffer);
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

int32_t Encoder::jpeg_write_bits(uint32_t data, int32_t len, int32_t flush,
                                 ofstream &outputFile) {
  static uint32_t bit_ptr = 0; // 与平时阅读习惯相反，最高位计为0，最低位计为31
  static uint32_t bitbuf = 0x00000000;
  uint8_t w = 0x00;

  bitbuf |= data << (32 - bit_ptr - len);
  bit_ptr += len;

  while (bit_ptr >= 8) {
    w = (uint8_t)((bitbuf & 0xFF000000) >> 24);
    jpeg_write_u8(w, outputFile);
    if (w == 0xFF) {
      jpeg_write_u8(0x00, outputFile);
    }
    bitbuf <<= 8;
    bit_ptr -= 8;
  }

  if (flush) {
    w = (uint8_t)((bitbuf & 0xFF000000) >> 24);
    jpeg_write_u8(w, outputFile);
  }
  return 0;
}

int32_t Encoder::jpeg_write_u8(uint8_t data, ofstream &outputFile) {
  uint8_t wd = data;
  outputFile.write((char *)&wd, 1);
  return 0;
}
