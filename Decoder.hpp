#ifndef DECODER_HPP_4KCLDUFQ
#define DECODER_HPP_4KCLDUFQ

#include "ByteStream.hpp"
#include "HuffmanTree.hpp"
#include "MCU.hpp"

class Decoder {
 public:
  Decoder(string filePath);
  ~Decoder();
  int startDecode();
  int startFindMarker();
  int decodeScanData();
  int createImage(const string ouputFileName);

 private:
  string _filePath;
  ifstream _file;
  uint8_t *_buf = nullptr;
  int _bufSize = 0;
  uint16_t _imgHeight = 0;
  uint16_t _imgWidth = 0;

  int readFile();

  int parseAPP0(int index);
  int parseComment(int index);
  int parseDQT(int index);
  /* 应对多个量化表的情况 */
  vector<vector<uint16_t>> _quantizationTables;
  /* 这个函数不参与实际的解码(Option) */
  void printQuantizationTable(vector<uint16_t> quantizationTable);
  /* 这个函数不参与实际的解码(Option) */
  inline void encodeZigZag(int a[64], int matrix[8][8]);
  /* 这个函数不参与实际的解码(Option) */
  inline void printMatrix(int arr[8][8]);

  int parseSOF0(int index);
  int parseDHT(int index);

  /* 对应四张表的HuffmanTree */
  HuffmanTable huffmanTable[2][2];
  HuffmanTree huffmanTree[2][2];
  void printHuffmanTable(const HuffmanTable &hf);

  int parseSOS(int index);
  /* 帧中图像分量的数量 */
  uint8_t _imageComponentCount = 0;

  int scanEntropyCodingImageData(ByteStream &bs);
  string _scanData;

  const int HT_DC = 0;
  const int HT_AC = 1;
  const int HT_Y = 0;
  const int HT_CbCr = 1;
  vector<MCU> _MCU;
  inline bool checkSpace(const string &value);
  inline int16_t decodeVLI(const string &value);
  inline void decodeACNumber();
  inline int erasePaddingBytes();

  int createImageFromMCUs(const vector<MCU> &MCUs);
  typedef shared_ptr<vector<vector<Pixel>>> PixelPtr;
  // typedef Pixel **PixelPtr;
  PixelPtr _pixelPtr = nullptr;

  int outputToPPMFile(const string &outputFileName);
  int outputToYUVFile(const string &outputFileName);
};

#endif /* end of include guard: DECODER_HPP_4KCLDUFQ */
