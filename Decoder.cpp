#include "Decoder.hpp"
#include "HuffmanTree.hpp"
#include "MCU.hpp"
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

Decoder::Decoder(string filePath) : _filePath(filePath){};

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
        parseAPP0(i + 2); // Send to body data
        break;
      case COM:
        std::cout << "Comment" << std::endl;
        parseComment(i + 2);
        break;
      case DQT:
        std::cout << "Define Quantization Table(s)" << std::endl;
        parseDQT(i + 2);
        break;
      case SOF0:
        std::cout << "Start Of Frame 0" << std::endl;
        parseSOF0(i + 2);
        break;
      case DHT:
        std::cout << "Define Huffman Table(s)" << std::endl;
        parseDHT(i + 2);
        break;
      case SOS:
        std::cout << "Start of Scan" << std::endl;
        parseSOS(i + 2);
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

/* https://www.w3.org/Graphics/JPEG/jfif3.pdf page6 */
int Decoder::parseAPP0(int index) {
  ByteStream bs(_buf + index, _bufSize - index);
  uint16_t len = bs.readBytes<uint16_t>(2);
  std::cout << "APP0[" << index << "~" << len + index << "] --> {" << std::endl;
  uint8_t identifier[5] = {0};
  for (int i = 0; i < (int)sizeof(identifier); i++)
    identifier[i] = bs.readByte();

  std::cout << "\tidentifier{" << identifier[0] << "," << identifier[1] << ","
            << identifier[2] << "," << identifier[3] << "," << identifier[4]
            << "}" << std::endl;

  uint8_t jfifVersion[2] = {0};
  for (int i = 0; i < (int)sizeof(jfifVersion); i++)
    jfifVersion[i] = bs.readByte();

  std::cout << "\tJFIFVersion:" << (int)jfifVersion[0] << "."
            << (int)jfifVersion[1] << endl;

  uint8_t unit = bs.readByte();
  if (unit == 0)
    std::cout << "\tdensity unit: 无单位" << std::endl;
  else if (unit == 1)
    std::cout << "\tdensity unit: 每英寸像素" << std::endl;
  else if (unit == 2)
    std::cout << "\tdensity unit: 每厘米像素" << std::endl;

  uint16_t Xdensity = bs.readBytes<uint16_t>(2);
  std::cout << "\tXdensity:" << Xdensity << std::endl;

  uint16_t Ydensity = bs.readBytes<uint16_t>(2);
  std::cout << "\tYdensity:" << Ydensity << std::endl;

  uint8_t Xthumbnail = bs.readByte();
  std::cout << "\tXthumbnail:" << (int)Xthumbnail << std::endl;
  uint8_t Ythumbnail = bs.readByte();
  std::cout << "\tYthumbnail:" << (int)Ythumbnail << std::endl;

  int remain = len - bs.getActualReadSize();
  uint8_t thumbnailImage[remain];
  std::cout << "\tremain:" << remain << std::endl;
  if (remain > 0) {
    for (int i = 0; i < bs.getActualReadSize(); i++) {
      thumbnailImage[i] = bs.readByte();
      printf("%d,", thumbnailImage[i]);
    }
    printf("\n");
  }
  std::cout << "}" << std::endl;
  return 0;
}

int Decoder::parseComment(int index) {
  ByteStream bs(_buf + index, _bufSize - index);
  uint16_t len = bs.readBytes<uint16_t>(2);
  std::cout << "COM[" << index << "~" << index + len << "] --> {" << std::endl;
  std::cout << "\tcomment:";
  for (int i = 0; i < len - 2; i++)
    std::cout << (char)bs.readByte();
  std::cout << std::endl;
  std::cout << "}" << std::endl;
  return 0;
}

/* ISO/IEC 10918-1 : 1993(E) : page 39 */
int Decoder::parseDQT(int index) {
  ByteStream bs(_buf + index, _bufSize - index);
  uint16_t len = bs.readBytes<uint16_t>(2);
  std::cout << "DQT[" << index << "~" << index + len << "] --> {" << std::endl;

  /* 1. 应对多个量化表的情况(总长度需要+ 1个字节的PqTq） */
  for (int i = 0; i < len / (QUANTIZATION_TAB_SIZE + 1); i++) {
    /* 2. 首字节的高4位表示精度，而低4位表示量化表的编号*/
    uint8_t PqTq = bs.readByte();
    /* - 量化表元素精度：值0表示8位Qk值；值1表示16位Qk值*/
    uint8_t precision = PqTq >> 4;
    if (precision == 0)
      std::cout << "\tprecision: 8 bit" << std::endl;
    else if (precision == 1)
      std::cout << "\tprecision: 16 bit" << std::endl;

    /* - 量化表目标标识符：指定解码器上应安装量化表的四个可能目标之一*/
    uint8_t identifier = PqTq & 0b1111;
    std::cout << "\tidentifier:" << (int)identifier << std::endl;
    /* 这里的QTtableNumber是按照实际的量化表个数来的，比如：
     * - 一个JPEG文件中有2个量化表那么编号会依次为0,1
     * - 一个JPEG文件中有3个量化表那么编号会依次为0,1,2
     * - 最多为四个量化表
     * */

    _quantizationTables.push_back({}); /* 先申请空间 */
    /* 指定64个元素中的第k个元素，其中k是DCT系数的锯齿形排序中的索引。量化元素应以之字形扫描顺序指定*/
    for (int k = 0; k < QUANTIZATION_TAB_SIZE; k++) {
      /* element的大小可能是8、16*/
      if (precision == 0) {
        uint8_t element = bs.readByte();
        _quantizationTables[identifier].push_back(element);
      } else if (precision == 1) {
        uint16_t element = (uint16_t)bs.readByte();
        _quantizationTables[identifier].push_back(element);
      }
    }
    printQuantizationTable(_quantizationTables[identifier]);
  }
  /* 基于 8 位 DCT 的过程不得使用 16 位精度量化表。 */
  std::cout << "}" << std::endl;
  return 0;
}

/* 这个函数不参与实际的解码(Option) */
inline void Decoder::printMatrix(int arr[8][8]) {
  int rows = 8, cols = 8;
  for (int i = 0; i < rows; ++i) {
    std::cout << "\t|";
    for (int j = 0; j < cols; ++j)
      std::cout << std::setw(3) << arr[i][j];
    std::cout << "|" << std::endl;
  }
}

/* 这个函数不参与实际的解码(Option) */
inline void Decoder::encodeZigZag(int a[64], int matrix[8][8]) {
  int row = 0, col = 0;
  for (int i = 0; i < 64; ++i) {
    matrix[row][col] = a[i];
    // 奇数斜线往上
    if ((row + col) % 2 == 0) {
      if (col == 7)
        row++;
      else if (row == 0)
        col++;
      else {
        row--;
        col++;
      }
    }
    // 偶数斜线往下
    else {
      if (row == 7)
        col++;
      else if (col == 0)
        row++;
      else {
        col--;
        row++;
      }
    }
  }
}

/* 这个函数不参与实际的解码(Option) */
void Decoder::printQuantizationTable(vector<uint16_t> quantizationTable) {
  int arr[QUANTIZATION_TAB_SIZE] = {0};
  for (int i = 0; i < QUANTIZATION_TAB_SIZE; i++)
    arr[i] = quantizationTable[i];

  int matrix[8][8];
  encodeZigZag(arr, matrix);
  printMatrix(matrix);
}

int Decoder::parseSOF0(int index) {
  ByteStream bs(_buf + index, _bufSize - index);
  const int begIndex = index;
  uint16_t len = bs.readBytes<uint16_t>(2);
  const int endIndex = len + begIndex;
  std::cout << "SOF0[" << begIndex << "~" << endIndex << "] --> {" << std::endl;
  uint8_t precision = bs.readByte();
  std::cout << "\tType:Baseline DCT" << std::endl;
  std::cout << "\tprecision:" << (int)precision << " bit" << std::endl;

  _imgHeight = bs.readBytes<uint16_t>(2);
  _imgWidth = bs.readBytes<uint16_t>(2);
  std::cout << "\timgWidth:" << (int)_imgWidth
            << ",imgHeight:" << (int)_imgHeight << std::endl;

  /* 帧中图像分量的数量 */
  uint8_t imageComponentCount = bs.readByte();
  std::cout << "\timageComponentCount:" << (int)imageComponentCount
            << std::endl;

  bool isNonSampled = true;
  std::cout << "\timageComponent{" << std::endl;
  for (int i = 0; i < imageComponentCount; i++) {
    uint8_t componentIdentifier = bs.readByte();
    uint8_t sampFactor = bs.readByte();
    uint8_t sampFactorH = sampFactor >> 4;
    uint8_t sampFactorV = sampFactor & 0b1111;
    uint8_t destinationSelector = bs.readByte();
    std::cout << "\t\tcomponentIdentifier:" << (int)componentIdentifier;
    std::cout << ",Horizontal sampFactor:" << (int)sampFactorH
              << ",Vertical sampFactor:" << (int)sampFactorV;
    std::cout << ",destinationSelector:" << (int)destinationSelector
              << std::endl;
    if ((sampFactor >> 4) != 1 || (sampFactor & 0xf) != 1)
      isNonSampled = false;
  }
  std::cout << "\t}" << std::endl;
  if (isNonSampled == false) {
    cout << "Chroma subsampling not yet supported!" << std::endl;
    cout << "Chroma subsampling is not 4:4:4, terminating..." << std::endl;
    return -1;
  }

  std::cout << "}" << std::endl;
  return 0;
}

/* ISO/IEC 10918-1 : 1993(E) : page 40  */
int Decoder::parseDHT(int index) {
  ByteStream bs(_buf + index, _bufSize - index);
  uint16_t len = bs.readBytes<uint16_t>(2);
  std::cout << "DHT[" << index << "~" << index + len << "] --> {" << std::endl;
  while (bs.getActualReadSize() + index < index + len) {
    /* 1. 首字节的高4位表示Huffman编码表的类型，而低4位表示霍夫曼表的标识符*/
    uint8_t TcTh = bs.readByte();
    /* - Huffman编码表的类型: 0表示DC，1表示AC */
    uint8_t huffmanTableClass = TcTh >> 4;
    if (huffmanTableClass == 0)
      std::cout << "\tHuffman table class:DC" << std::endl;
    else if (huffmanTableClass == 1)
      std::cout << "\tHuffman table class:AC" << std::endl;
    /* - 标识符: 编号范围是0-3*/
    uint8_t huffmanTableIdentifier = TcTh & 0b1111;
    std::cout << "\tHuffman table destination identifier:"
              << (int)huffmanTableIdentifier << std::endl;

    /* 读取和记录符号计数 */
    int totalHuffmanCodeLen = 0;
    HuffmanTable hfTable =
        huffmanTable[huffmanTableClass][huffmanTableIdentifier];

    // Huffman-code的个数固定为16个字节，比如：
    // huffmanCodeLen:[0 2 1 3 3 2 4 3 5 5 4 4 0 0 1 125 ]
    // 0:表示第一个索引位置的下对应的容器不能存储编码字符，2：表示这个索引位置下要存放2个字符
    for (int i = 0; i < HUFFMAN_CODE_LENGTH_POSSIBLE; i++) {
      uint8_t huffmanCodeLen = bs.readByte();
      hfTable[i].first = huffmanCodeLen;
      totalHuffmanCodeLen += huffmanCodeLen;
    }
    //  std::cout << "\ttotalSymbolCount:" << totalHuffmanCodeLen << std::endl;

    /* 按字符长度与对应字符，一一对应存储到本地容器（这并不是Huffman的解码规则，就是为了方便写代码）*/
    int readedSymbols = 0;
    /* 根据总长度来读取所有Huffman-codes*/
    for (int i = 0; i < totalHuffmanCodeLen; i++) {
      /* 如果遇到huffmanCodeLen=0，则说明该索引的对应codes容器不要存放Huffman-codes*/
      while (hfTable[readedSymbols].first == 0)
        readedSymbols++;

      /* 直到读取的huffmanCodeLen=0,则表示该索引处可以存放Huffman-codes*/
      uint8_t huffmanCode = bs.readByte();

      /* 保存读取到的Huffman-code，存储到对应Huffman-code-length的位置，长度与符号一定要对应*/
      hfTable[readedSymbols].second.push_back(huffmanCode);

      /* 读满Huffman-code-length指定长度的Huffman-codes后，开始读取下一个Huffman-code-length*/
      if (hfTable[readedSymbols].first ==
          (int)hfTable[readedSymbols].second.size())
        readedSymbols++;
    }
    printHuffmanTable(hfTable);
    /* TODO YangJing HuffmanTree 还是有点没搞懂，需要重新写<24-04-15-17:54:18>*/
    huffmanTree[huffmanTableClass][huffmanTableIdentifier].builedHuffmanTree(
        hfTable);
  }
  std::cout << "}" << std::endl;
  return 0;
}

void Decoder::printHuffmanTable(const HuffmanTable &hf) {
  for (int i = 0; i < HUFFMAN_CODE_LENGTH_POSSIBLE; i++) {
    std::cout << "\thuffmanCodeLen:[";
    std::cout << hf[i].first;
    std::cout << "]";
    std::cout << ",huffmanCodes:{";
    for (const auto &it : hf[i].second) {
      std::cout << (int)it << " ";
    }
    std::cout << "}" << std::endl;
  }
}

/* ISO/IEC 10918-1 : 1993(E) : page 37 */
int Decoder::parseSOS(int index) {
  ByteStream bs(_buf + index, _bufSize - index);
  uint16_t len = bs.readBytes<uint16_t>(2);
  std::cout << "SOS[" << index << "~" << index + len << "] --> {" << std::endl;

  _imageComponentCount = bs.readByte();
  std::cout << "\timageComponentCount:" << (int)_imageComponentCount
            << std::endl;
  std::cout << "\timageComponent{" << std::endl;
  for (int i = 0; i < _imageComponentCount; i++) {
    /* 扫描分量选择器 */
    uint8_t scanComponentSelector = bs.readByte();
    std::cout << "\t\tscanComponentSelector:" << (int)scanComponentSelector;
    uint8_t TdTa = bs.readByte();
    /* DC,AC熵编码表目的地选择器 */
    uint8_t entropyCodingTableDestinationSelectorDC = TdTa >> 4;
    std::cout << ",entropyCodingTableDestinationSelectorDC:"
              << (int)entropyCodingTableDestinationSelectorDC;
    uint8_t entropyCodingTableDestinationSelectorAC = TdTa & 0b1111;
    std::cout << ",entropyCodingTableDestinationSelectorAC:"
              << (int)entropyCodingTableDestinationSelectorAC << std::endl;
  }
  std::cout << "\t}" << std::endl;

  uint8_t startOfSpectral = bs.readByte();
  std::cout << "\tstartOfSpectral:" << (int)startOfSpectral << std::endl;
  uint8_t endOfSpectral = bs.readByte();
  std::cout << "\tendOfSpectral:" << (int)endOfSpectral << std::endl;
  uint8_t AhAl = bs.readByte();
  uint8_t successiveApproximationBitH = AhAl >> 4;
  std::cout << "\tsuccessiveApproximationBitH:"
            << (int)successiveApproximationBitH << std::endl;
  uint8_t successiveApproximationBitL = AhAl & 0b1111;
  std::cout << "\tsuccessiveApproximationBitL:"
            << (int)successiveApproximationBitL << std::endl;

  std::cout << "}" << std::endl;
  scanEntropyCodingImageData(bs);
  return 0;
}

/* 读取编码图像数据 */
int Decoder::scanEntropyCodingImageData(ByteStream &bs) {
  uint16_t a = 1;
  /* 读完剩余Buffer*/
  while (bs.getBufSize()) {
    a = bs.readBytes<uint16_t>(2);
    if (a >> 8 == 0xff && (a & 0xff) == EOI)
      return 0;
    bitset<16> bits(a);
    _scanData.append(bits.to_string());
  }
  return 0;
}

int Decoder::createImage(const string ouputFileName) {
  int ret = -1;
  ret = createImageFromMCUs(_MCU);

  switch (gOutputFileType) {
  case OutputFileType::YUV:
    ret = outputToYUVFile(ouputFileName);
    break;
  case OutputFileType::PPM:
    ret = outputToPPMFile(ouputFileName);
    break;
  default:
    ret = -1;
    break;
  }

  if (ret < 0)
    return -1;
  return 0;
}

int Decoder::createImageFromMCUs(const vector<MCU> &MCUs) {
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

int Decoder::decodeScanData() {
  /* 移除在JPEG图像的压缩码流中由于编码规则添加进去的填充字节 */
  erasePaddingBytes();
  /* 正式进入解码层 */

  if (_MCU.size() != 0)
    _MCU.clear();

  int MCUCount = (_imgWidth * _imgHeight) / (MCU_UNIT_SIZE);

  int index = 0;
  for (int i = 0; i < MCUCount; i++) {
    /* 1.设置游程编码数组（RLE）：每个MCU可能多个组件的数据（Y,U,V）*/
    array<vector<int>, 3> RLE;

    /*解码DC/AC系数：DCT变换后将矩阵的能量压缩到第一个元素中，左上角第一个元素被称为直流（DC）系数，其余的元素被称为交流（AC）系数*/
    for (int imageComponent = 0; imageComponent < _imageComponentCount;
         imageComponent++) {
      string bitsScanned;
      /* Y单独Huffman解码表，UV共享一张Huffman解码表 */
      bool HuffTableID = imageComponent == 0 ? 0 : 1;

      /* 1.解码直流系数（DC）：对于每个颜色分量（Y,U,V），直流系数表示一个8x8宏块（或MCU）的平均值*/
      while (true) {
        bitsScanned += _scanData[index];
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
            coeffDC = decodeVLI(_scanData.substr(index, lengthVLI));
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
        bitsScanned += _scanData[index];
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
            coeffAC = decodeVLI(_scanData.substr(index, lengthVLI));
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
    _MCU.push_back(MCU(RLE, _quantizationTables));
  }
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

inline int Decoder::erasePaddingBytes() {
  std::cout << "Encode data size(Befor erase):" << _scanData.size()
            << std::endl;

  for (int i = 0; i < (int)_scanData.size() - 8; i += 8) {
    string str8Len = _scanData.substr(i, 8);
    if (str8Len == "11111111" && (i + 8) < ((int)_scanData.size() - 8))
      if (_scanData.substr(i + 8, 8) == "00000000")
        _scanData.erase(i + 8, 8);
  }

  std::cout << "Encode data size(After erase):" << _scanData.size()
            << std::endl;
  return 0;
}

int Decoder::outputToYUVFile(const string &outputFileName) {
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

int Decoder::outputToPPMFile(const string &outputFileName) {
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
