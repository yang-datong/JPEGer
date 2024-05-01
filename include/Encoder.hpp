#ifndef ENCODER_HPP_UW8N0IPM
#define ENCODER_HPP_UW8N0IPM

#include "APP0.hpp"
#include "ByteStream.hpp"
#include "COM.hpp"
#include "Common.hpp"
#include "DHT.hpp"
#include "DQT.hpp"
#include "HuffmanTree.hpp"
#include "Image.hpp"
#include "MCU.hpp"
#include "SOF0.hpp"
#include "SOS.hpp"
#include "Type.hpp"

class Encoder {
 public:
  Encoder(const string &inputFilePath, const string &outputFilePath);
  Encoder(const string &inputFilePath, const string &outputFilePath,
          const int inputTypeFile);
  ~Encoder();
  int startMakeMarker();
  int startEncode();
  int encodeScanData();
  int createImage(const string ouputFileName);

 private:
  ifstream _file;
  string _inputFilePath;
  string _outputFilePath;

  uint8_t *_buf = nullptr;
  int _bufSize = 0;
  vector<MCU> _MCU;

  Marker *_app0;
  Marker *_com;
  Marker *_dqt;
  Marker *_sof0;
  Marker *_dht;
  Marker *_sos;
  Image _image;

  int readFile();
  string VLIEncode(int value);
  inline int fillPaddingBytes(string &scanData);
};

#endif /* end of include guard: ENCODER_HPP_UW8N0IPM */
