#ifndef DECODER_HPP_4KCLDUFQ
#define DECODER_HPP_4KCLDUFQ

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
  inline int16_t decodeVLI(const string &value);
  inline int erasePaddingBytes(string &scanData);
};

#endif /* end of include guard: DECODER_HPP_4KCLDUFQ */
