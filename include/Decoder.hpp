#ifndef DECODER_HPP_4KCLDUFQ
#define DECODER_HPP_4KCLDUFQ

#include "APP0.hpp" // IWYU pragma: export
#include "COM.hpp"  // IWYU pragma: export
#include "DHT.hpp"  // IWYU pragma: export
#include "DQT.hpp"  // IWYU pragma: export
#include "SOF0.hpp" // IWYU pragma: export
#include "SOS.hpp"  // IWYU pragma: export

#include "Image.hpp"
#include "MCU.hpp"

class Decoder {
 public:
  Decoder(const string &filePath);
  Decoder(const string &filePath, const int outputTypeFile);
  ~Decoder();
  int startDecode();
  int startFindMarker();
  int decodeScanData();
  int createImage(const string ouputFileName);

 private:
  string _filePath;
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

  int16_t decodeVLI(const string &value);
  inline int erasePaddingBytes(string &scanData);

  inline void printDCCoefficient(int HuffTableID, string value,
                                 string &bitsScanned) {
    /* T.81 page 149 */
    std::cout << "DC -> {" << std::endl;
    std::cout << "\tCommom:" << (HuffTableID == 0 ? "Luma" : "Chroma")
              << ", Category:" << value
              << ", Code Length:" << bitsScanned.length()
              << ", Code word:" << bitsScanned << std::endl;
    std::cout << "}" << std::endl;
  }

  inline void printACCoefficient(int HuffTableID, string value,
                                 string &bitsScanned, int zeroCount,
                                 int lengthVLI) {
    /* T.81 page 153 */
    std::cout << "AC -> {" << std::endl;
    std::cout << "\tCommom:" << (HuffTableID == 0 ? "Luma" : "Chroma")
              << ", Category:" << value << ", Run/Size:" << zeroCount << "/"
              << lengthVLI << ", Code Length:" << bitsScanned.length()
              << ", Code word:" << bitsScanned << std::endl;
    std::cout << "}" << std::endl;
  }
};

#endif /* end of include guard: DECODER_HPP_4KCLDUFQ */
