#ifndef SOF0_HPP_WEPMVZS4
#define SOF0_HPP_WEPMVZS4

#include "Marker.hpp"
#include <cstdint>

namespace mark {
typedef struct __attribute__((packed)) _ImageComponent {
  uint8_t componentIdentifier;
  uint8_t sampFactor;
  uint8_t destinationSelector;
} ImageComponent;

typedef struct __attribute__((packed)) _SOF0 {
  uint8_t SOF0[2] = {0xff, JFIF::SOF0};
  uint16_t len = 0;
  uint8_t precision = 0;
  uint16_t imgWidth = 0;
  uint16_t imgHeight = 0;
  uint8_t imageComponentCount = 0;
} SOF0Header;

class SOF0 : public Marker {
 private:
  int _imgWidth = 0;
  int _imgHeight = 0;

 public:
  SOF0Header header;
  ImageComponent *imageComponent = nullptr;
  uint16_t getimgWidth() { return header.imgWidth; }
  uint16_t getimgHeight() { return header.imgHeight; }
  void setImageWidth(int imgWidth) { this->_imgWidth = imgWidth; }
  void setImageHeight(int imgHeight) { this->_imgHeight = imgHeight; }
  int parse(int index, uint8_t *buf, int bufSize) override;
  int package(ofstream &outputFile) override;
};
} // namespace mark

#endif /* end of include guard: SOF0_HPP_WEPMVZS4 */
