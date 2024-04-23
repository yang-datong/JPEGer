#include "Decoder.hpp"
#include "HuffmanTree.hpp"
#include <cstdint>

Decoder::Decoder(string filePath) : _filePath(filePath) {
  _app0 = new mark::APP0();
  _com = new mark::COM();
  _dqt = new mark::DQT();
  _sof0 = new mark::SOF0();
  _dht = new mark::DHT();
  _sos = new mark::SOS();
};

Decoder::~Decoder() {
  if (_buf) {
    delete[] _buf;
    _buf = nullptr;
  }
  _bufSize = 0;
}

int Decoder::readFile() {
  _file.open(_filePath, std::ios::binary | std::ios::in);
  if (!_file.is_open())
    return -1;

  /* 适当调整，读取一个17M的文件要花近1分钟 */
  const int readBufSize = 1024;
  /* 适当调整，读取一个17M的文件要花近1秒 */
  // const int readBufSize = 1024 * 1024;
  bool isRead = false;

  uint8_t *fileBuffer = new uint8_t[readBufSize];
  while (!isRead) {
    _file.read(reinterpret_cast<char *>(fileBuffer), readBufSize);
    if (_file.gcount() != 0) {
      uint8_t *const tmp = new uint8_t[_bufSize + _file.gcount()];
      memcpy(tmp, _buf, _bufSize);
      memcpy(tmp + _bufSize, fileBuffer, _file.gcount());
      if (_buf) {
        delete[] _buf;
        _buf = nullptr;
      }
      _buf = tmp;
      _bufSize += _file.gcount();
    } else
      isRead = true;
  }
  delete[] fileBuffer;
  fileBuffer = nullptr;
  _file.close();
  /* ifstream对象被销毁时，它的析构函数会自动关闭文件，如果需要及时释放可以写 */

  if (!_file.eof() && _file.fail())
    return -2;
  return 0;
}

int Decoder::startFindMarker() {
  if (readFile())
    return -1;

  /* 不读取最后一个字节 */
  for (int i = 0; i < _bufSize - 1; i++) {
    if (_buf[i] == 0xff) {
      switch (_buf[i + 1]) {
      case SOI:
        std::cout << "Start OF Image" << std::endl;
        break;
      case APP0:
        std::cout << "Decode Application-specific" << std::endl;
        _app0->parse(i + 2, _buf, _bufSize);
        break;
      case COM:
        std::cout << "Comment" << std::endl;
        _com->parse(i + 2, _buf, _bufSize);
        break;
      case DQT:
        std::cout << "Define Quantization Table(s)" << std::endl;
        _dqt->parse(i + 2, _buf, _bufSize);
        break;
      case SOF0:
        std::cout << "Start Of Frame 0" << std::endl;
        _sof0->parse(i + 2, _buf, _bufSize);
        break;
      case DHT:
        std::cout << "Define Huffman Table(s)" << std::endl;
        _dht->parse(i + 2, _buf, _bufSize);
        break;
      case SOS:
        std::cout << "Start of Scan" << std::endl;
        _sos->parse(i + 2, _buf, _bufSize);
        break;
      case EOI:
        std::cout << "End OF Image" << std::endl;
        if (i + 2 != _bufSize)
          return -1;
        break;
      }
    }
  }
  return 0;
}

int Decoder::decodeScanData() {
  uint16_t imgWidth = static_cast<mark::SOF0 *>(_sof0)->getimgWidth();
  uint16_t imgHeight = static_cast<mark::SOF0 *>(_sof0)->getimgHeight();
  string scanData = static_cast<mark::SOS *>(_sos)->getScanData();
  uint8_t imageComponentCount =
      static_cast<mark::SOS *>(_sos)->getImageComponentCount();

  /* TODO YangJing 这里写法感觉不太好 <24-04-23 11:30:26> */
  vector<vector<uint16_t>> quantizationTables =
      static_cast<mark::DQT *>(_dqt)->getQuantizationTables();

  /* TODO 很好玩的写法 <24-04-23 11:26:24, YangJing>  */
  const HuffmanTree(*readOnlyHuffmanTree)[2] =
      static_cast<mark::DHT *>(_dht)->getHuffmanTree();

  /* TODO YangJing 这里写法感觉不太好 <24-04-23 11:30:26> */
  HuffmanTree huffmanTree[2][2];
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      huffmanTree[i][j] = readOnlyHuffmanTree[i][j];
    }
  }

  /* 移除在JPEG图像的压缩码流中由于编码规则添加进去的填充字节 */
  erasePaddingBytes(scanData);
  /* 正式进入解码层 */

  if (_MCU.size() != 0)
    _MCU.clear();

  int MCUCount = (imgWidth * imgHeight) / (MCU_UNIT_SIZE);

  int index = 0;
  for (int i = 0; i < MCUCount; i++) {
    /* 1.设置游程编码数组（RLE）：每个MCU可能多个组件的数据（Y,U,V）*/
    array<vector<int>, 3> RLE;

    /*解码DC/AC系数：DCT变换后将矩阵的能量压缩到第一个元素中，左上角第一个元素被称为直流（DC）系数，其余的元素被称为交流（AC）系数*/
    for (int imageComponent = 0; imageComponent < imageComponentCount;
         imageComponent++) {
      string bitsScanned;
      /* Y单独Huffman解码表，UV共享一张Huffman解码表 */
      bool HuffTableID = imageComponent == 0 ? 0 : 1;

      /* 1.解码直流系数（DC）：对于每个颜色分量（Y,U,V），直流系数表示一个8x8宏块（或MCU）的平均值*/
      while (true) {
        bitsScanned += scanData[index];
        index++;
        /* 先解码最外层的Huffman编码（按照对应的Huffman类型解码） */
        string value = huffmanTree[HT_DC][HuffTableID].decode(bitsScanned);
        if (checkSpace(value)) {
          uint8_t zeroCount = 0;
          int16_t coeffDC = 0;
          if (value != "EOB") {
            /* 获取DC系数中 零的游程 */
            zeroCount = uint8_t(stoi(value)) >> 4;
            /* 得到VLI的长度 */
            uint8_t lengthVLI = stoi(value) & 0xf;
            /* 再解码VLI编码拿到DC系数 */
            coeffDC = decodeVLI(scanData.substr(index, lengthVLI));
            index += lengthVLI;
          }
          /* 零的游程 */
          RLE[imageComponent].push_back(zeroCount);
          /* DC系数 */
          RLE[imageComponent].push_back(coeffDC);
          bitsScanned.clear();
          break;
        }
      }
      bitsScanned.clear();

      /*2.解码交流系数（AC）：与直流系数类似，通过扫描位字符串定位相应的系数，并记录游程编码和相应的量化值*/
      int ACCodesCount = 1;
      while (ACCodesCount < MCU_UNIT_SIZE) {
        bitsScanned += scanData[index];
        index++;
        string value = huffmanTree[HT_AC][HuffTableID].decode(bitsScanned);
        if (checkSpace(value)) {
          uint8_t zeroCount = 0;
          int16_t coeffAC = 0;
          if (value != "EOB") {
            /* 获取AC系数中 零的游程 */
            zeroCount = uint8_t(stoi(value)) >> 4;
            uint8_t lengthVLI = uint8_t(stoi(value)) & 0xf;
            /* 再解码VLI编码拿到AC系数 */
            coeffAC = decodeVLI(scanData.substr(index, lengthVLI));
            index += lengthVLI;
            ACCodesCount += zeroCount + 1;
          }

          bitsScanned.clear();
          /* 零的游程 */
          RLE[imageComponent].push_back(zeroCount);
          /* DC系数 */
          RLE[imageComponent].push_back(coeffAC);

          if (value == "EOB")
            break;
        }
      }

      /*3.处理边界情况：解码后，如果一个宏块的系数列表只有两个零（表示该宏块完全被压缩为0），则移除，因为该宏块实际上不包含任何信息。*/
      if (RLE[imageComponent].size() == 2 && RLE[imageComponent][0] == 0 &&
          RLE[imageComponent][1] == 0) {
        RLE[imageComponent].pop_back();
        RLE[imageComponent].pop_back();
      }
    }

    /*6.构建MCU块并添加到数组：输入流的每个MCU部分都被解码成含有直流和交流系数的RLE数组。然后使用RLE数据和量化表（m_QTables），构建出每个MCU的8x8矩阵，并将这个MCU添加到m_MCU数组中以保存解码数据。*/
    _MCU.push_back(MCU(RLE, quantizationTables));
  }
  return 0;
}

inline int Decoder::erasePaddingBytes(string &scanData) {
  std::cout << "Encode data size(Befor erase):" << scanData.size() << std::endl;

  for (int i = 0; i < (int)scanData.size() - 8; i += 8) {
    string str8Len = scanData.substr(i, 8);
    if (str8Len == "11111111" && (i + 8) < ((int)scanData.size() - 8))
      if (scanData.substr(i + 8, 8) == "00000000")
        scanData.erase(i + 8, 8);
  }

  std::cout << "Encode data size(After erase):" << scanData.size() << std::endl;
  return 0;
}

int Decoder::createImage(const string ouputFileName) {
  int ret = -1;

  uint16_t imgWidth = static_cast<mark::SOF0 *>(_sof0)->getimgWidth();
  uint16_t imgHeight = static_cast<mark::SOF0 *>(_sof0)->getimgHeight();
  ret = _image.createImageFromMCUs(_MCU, imgWidth, imgHeight);

  /* TODO YangJing 这里感觉移动到Image中好些 <24-04-23 12:59:52> */
  switch (gOutputFileType) {
  case OutputFileType::YUV:
    ret = _image.outputToYUVFile(ouputFileName);
    break;
  case OutputFileType::PPM:
    ret = _image.outputToPPMFile(ouputFileName);
    break;
  default:
    ret = -1;
    break;
  }

  if (ret < 0)
    return -1;
  return 0;
}

/* VLI解码：
 * 1.如果首个元素是0,则表示源数据是负数
 * 2.如果首个元素非0,则该二进制字符串就是一个完整的二进制数（因为没有在前面补0的操作）
 * 3.如果是负数则去除前面的所有0字符，再按位取反，乘上-1得到原来的负数
 * */
inline int16_t Decoder::decodeVLI(const string &value) {
  if (value.empty())
    return 0;
  int sign = value[0] == '0' ? -1 : 1;
  int16_t result = 0;

  string tmp = value;

  if (sign == -1) {
    /* 按位取反 */
    for (int i = 0; i < (int)value.size(); i++)
      tmp[i] = value[i] == '0' ? '1' : '0';
  }

  /* 转为整型 */
  for (int i = 0; i < (int)tmp.size(); i++)
    if (tmp[i] == '1')
      result |= 1 << (tmp.size() - i - 1);

  return result * sign;
}

inline bool Decoder::checkSpace(const string &value) {
  if (value.empty())
    return false;
  for (const auto &it : value) {
    if (iscntrl(it) || isblank(it) || isspace(it))
      return false;
  }
  return true;
}
