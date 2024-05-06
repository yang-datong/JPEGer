#include "Decoder.hpp"
#include "Encoder.hpp"
#include "Type.hpp"
#include <getopt.h>
#include <iostream>

#define VERSION 1.0

static string inputFileName;
static string outputFileName;
static int imgWidth = 0, imgHeight = 0;

inline void use(char *me);
int parseImageSize(string resolution);
void checkArgument(int argc, char *argv[]);

int encode(int inpuType) {
  int ret = 0;
  if (inpuType == FileFormat::YUV) {
    if (imgWidth <= 0 || imgHeight <= 0) {
      std::cerr << "\033[31mNeed imgWidth and imgHeight.\033[0m" << std::endl;
      return -1;
    }
  }
  Encoder encoder(inputFileName, outputFileName, inpuType, imgWidth, imgHeight);
  ret = encoder.startMakeMarker();
  RET(ret, "Failed for startMakeMarker()")
  return ret;
}

int decode(int outType) {
  int ret = 0;
  Decoder decoder(inputFileName, outType);
  std::cout << "-------------------------------------------- startFindMarker "
               "--------------------------------------------"
            << std::endl;
  ret = decoder.startFindMarker();
  RET(ret, "Failed for startFindMarker()")

  std::cout << "-------------------------------------------- decodeScanData "
               "--------------------------------------------"
            << std::endl;
  ret = decoder.decodeScanData();
  RET(ret, "Failed for decodeScanData()")

  std::cout << "---------------------------------------------- createImage "
               "----------------------------------------------"
            << std::endl;
  ret = decoder.createImage(outputFileName);
  RET(ret, "Failed for createImage()")
  return ret;
}

int main(int argc, char *argv[]) {
  checkArgument(argc, argv);
  if (inputFileName.empty() || outputFileName.empty()) {
    use(argv[0]);
    return -1;
  }
  string inSuffixStr = getFileType(inputFileName);
  string outSuffixStr = getFileType(outputFileName);
  if (inSuffixStr == "jpg" || inSuffixStr == "jpeg") {
    /* Decode */
    if (outSuffixStr == "yuv")
      decode(FileFormat::YUV);
    else if (outSuffixStr == "ppm")
      decode(FileFormat::PPM);
    else
      use(argv[0]);
  } else {
    /* Encode */
    if (inSuffixStr == "yuv")
      encode(FileFormat::YUV);
    else if (inSuffixStr == "bmp")
      encode(FileFormat::BMP);
    else
      use(argv[0]);
  }
  return 0;
}

inline int parseImageSize(string resolution) {
  istringstream iss(resolution);
  string width_str, height_str;
  getline(iss, width_str, 'x');
  getline(iss, height_str);
  if (!width_str.empty() && !height_str.empty()) {
    imgWidth = stoi(width_str);
    imgHeight = stoi(height_str);
  }
  return 0;
}

inline void use(char *me) {
  cout << "Usage: " << me << " [options] [--] -i inputFile -o outputFile \
                    \n\n  Options:\
                      \n  -h, --help           Display this message\
                      \n  -v, --version        Display version\
                      \n  -i, --input          Encode/Decode input file\
                      \n  -o, --output         Encode/Decode output file\
                      \n  -s, --video_size     Encode input file size as: [-s 1920x1080]\
                      \n";
}

void checkArgument(int argc, char *argv[]) {
  const char *short_opts = "hvi:o:s:";
  const option long_opts[] = {{"help", no_argument, nullptr, 'h'},
                              {"version", no_argument, nullptr, 'v'},
                              {"input", required_argument, nullptr, 'i'},
                              {"output", required_argument, nullptr, 'o'},
                              {"video_size", required_argument, nullptr, 's'},
                              {nullptr, no_argument, nullptr, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) !=
         -1) {
    switch (opt) {
    case 'h':
      use(argv[0]);
      exit(0);
    case 'v':
      cout << "Version: " << VERSION << endl;
      exit(0);
    case 'i':
      inputFileName = optarg;
      break;
    case 'o':
      outputFileName = optarg;
      break;
    case 's':
      parseImageSize(optarg);
      break;
    default:
      cout << "Unknown option: " << opt << endl;
      exit(1);
    }
  }
}
