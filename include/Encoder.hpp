#ifndef ENCODER_HPP_UW8N0IPM
#define ENCODER_HPP_UW8N0IPM

#include "APP0.hpp" // IWYU pragma: export
#include "BMP.hpp"  // IWYU pragma: export
#include "COM.hpp"  // IWYU pragma: export
#include "DQT.hpp"  // IWYU pragma: export
#include "SOF0.hpp" // IWYU pragma: export
#include "SOS.hpp"  // IWYU pragma: export

#include "DHT.hpp"
#include "Image.hpp"
#include "MCU.hpp"

class Encoder {
 public:
  Encoder(const string &inputFilePath, const string &outputFilePath);
  Encoder(const string &inputFilePath, const string &outputFilePath,
          const int inputTypeFile);
  Encoder(const string &inputFilePath, const string &outputFilePath,
          const int inputTypeFile, const int imgWidth, const int imgHeight);
  ~Encoder();
  int startMakeMarker();
  int startEncode();
  int createImage(const string ouputFileName);
  /* 读取用于编码的数据，如果不是YUV文件则转码为YUV，然后再读取YUV数据，同时初始化宽、高*/
  int readFileBuferToYUV444Packed();

 private:
  ifstream _file;
  string _inputFilePath;
  string _outputFilePath;
  int _imgWidth = 0;
  int _imgHeight = 0;

  uint8_t *_packedBuffer = nullptr;
  int _bufSize = 0;
  vector<MCU> _MCU;

  Marker *_app0;
  Marker *_com;
  Marker *_dqt;
  Marker *_sof0;
  Marker *_dht;
  Marker *_sos;
  Image _image;

  int encodeScanData(ofstream &outputFile);
  string VLIEncode(int value);
  void writeBitStream(const uint32_t data, uint32_t len, ofstream &outputFile,
                      bool is_flush);

  int _encodeScanData(mark::HuffmanTrees huffmanTree, ofstream &outputFile);

  void printDCInfo(int HuffTableID, int category, int codeLen, string &value);
  void printZRLInfo(int HuffTableID, string &value, int symbol);
  void printEOBInfo(int HuffTableID, string &value, int symbol);
  void printCommonInfo(int HuffTableID, string &value, int symbol,
                       int zeroCount, int coeffACLen);
};

#endif /* end of include guard: ENCODER_HPP_UW8N0IPM */
