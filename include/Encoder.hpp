#ifndef ENCODER_HPP_UW8N0IPM
#define ENCODER_HPP_UW8N0IPM

#include "APP0.hpp"
#include "COM.hpp"
#include "DHT.hpp"
#include "DQT.hpp"
#include "Image.hpp"
#include "SOF0.hpp"
#include "SOS.hpp"

#include "ByteStream.hpp"
#include "HuffmanTree.hpp"
#include "MCU.hpp"

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

  const int HT_DC = 0;
  const int HT_AC = 1;
  const int HT_Y = 0;
  const int HT_CbCr = 1;
  int readFile();
  inline int16_t encodeVLI(const string &value);
  inline int fillPaddingBytes(string &scanData);
};

#endif /* end of include guard: ENCODER_HPP_UW8N0IPM */
