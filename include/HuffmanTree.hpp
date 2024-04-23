#ifndef HUFFMANTREE_HPP_KFQOQMNU
#define HUFFMANTREE_HPP_KFQOQMNU

#include "Common.hpp"
#include "MCU.hpp"

struct Node {
  bool root = false;
  bool leaf = false;
  std::string code;
  uint16_t value = 0;
  shared_ptr<Node> lChild = nullptr, rChild = nullptr;
  std::shared_ptr<Node> parent = nullptr;
  Node()
      : root{false}, leaf{false}, code{""}, value{0x00}, lChild{nullptr},
        rChild{nullptr}, parent{nullptr} {}

  Node(const std::string _code, const uint16_t _val)
      : root{false}, leaf{false}, code{_code}, value{_val}, lChild{nullptr},
        rChild{nullptr}, parent{nullptr} {}
};

typedef std::shared_ptr<Node> NodePtr;

class HuffmanTree {
 public:
  int builedHuffmanTree(HuffmanTable &htable);
  const string decode(const std::string &huffCode);

 private:
  NodePtr _root = nullptr;
  inline NodePtr createRootNode(const uint16_t value);
  inline NodePtr createNode() { return std::make_shared<Node>(); }
  void insertLeft(NodePtr node, const uint16_t value);
  void insertRight(NodePtr node, const uint16_t value);
  NodePtr getRightLevelNode(NodePtr node);
  void printHuufmanTree(NodePtr node, string str);
};

#endif /* end of include guard: HUFFMANTREE_HPP_KFQOQMNU */
