#include "HuffmanTree.hpp"
#include "MCU.hpp"

inline NodePtr HuffmanTree::createRootNode(const uint16_t value) {
  NodePtr root = std::make_shared<Node>("", value);
  root->root = true;
  return root;
}

void HuffmanTree::insertLeft(NodePtr node, const uint16_t value) {
  if (node == nullptr)
    return;

  if (node->lChild != nullptr)
    return;

  NodePtr lNode = createNode();
  lNode->parent = node;
  node->lChild = lNode;

  lNode->code = node->code + "0";
  lNode->value = value;
}

void HuffmanTree::insertRight(NodePtr node, const uint16_t value) {
  if (node == nullptr)
    return;

  if (node->rChild != nullptr)
    return;

  NodePtr rNode = createNode();
  rNode->parent = node;
  node->rChild = rNode;

  rNode->code = node->code + "1";
  rNode->value = value;
}

NodePtr HuffmanTree::getRightLevelNode(NodePtr node) {
  if (node == nullptr)
    return nullptr;

  if (node->parent != nullptr && node->parent->lChild == node)
    return node->parent->rChild;

  int count = 0;
  NodePtr nptr = node;
  while (nptr->parent != nullptr && nptr->parent->rChild == nptr) {
    nptr = nptr->parent;
    count++;
  }

  if (nptr->parent == nullptr)
    return nullptr;

  nptr = nptr->parent->rChild;

  while (count > 0) {
    nptr = nptr->lChild;
    count--;
  }

  return nptr;
}

void HuffmanTree::printHuufmanTree(NodePtr node, string str) {
  if (node == nullptr)
    return;

  if (node->value != 0)
    std::cout << node->value << ":" << str;

  printHuufmanTree(node->lChild, str + "0");
  printHuufmanTree(node->rChild, str + "1");
  if (node->value != 0)
    std::cout << ", ";
}

int HuffmanTree::buildHuffmanTree(HuffmanTable &htable) {
  _root = createRootNode(0);
  insertLeft(_root, 0);
  insertRight(_root, 0);
  NodePtr leftMost = _root->lChild;

  for (int i = 0; i < HUFFMAN_CODE_LENGTH_POSSIBLE; ++i) {
    // 如果计数为零，则为所有未分配的叶节点添加左和右子节点。
    if (htable[i].first == 0) {
      for (NodePtr nptr = leftMost; nptr; nptr = getRightLevelNode(nptr)) {
        insertLeft(nptr, 0);
        insertRight(nptr, 0);
      }
      leftMost = leftMost->lChild;
    } else {
      for (auto &&huffVal : htable[i].second) {
        leftMost->value = huffVal;
        leftMost->leaf = true;
        leftMost = getRightLevelNode(leftMost);
      }
      insertLeft(leftMost, 0);
      insertRight(leftMost, 0);
      NodePtr nptr = getRightLevelNode(leftMost);
      leftMost = leftMost->lChild;
      while (nptr != nullptr) {
        insertLeft(nptr, 0);
        insertRight(nptr, 0);
        nptr = getRightLevelNode(nptr);
      }
    }
  }
  //  std::cout << "\t{";
  //  printHuufmanTree(m_root, "");
  //  std::cout << "}" << std::endl;
  return 0;
}

const string HuffmanTree::decode(const string &huffCode) {
  if (!checkSpace(huffCode))
    return "";

  int i = 0;
  NodePtr nptr = _root;
  do {
    if (huffCode[i] == '0')
      nptr = nptr->lChild;
    else
      nptr = nptr->rChild;
    if (nptr != nullptr && nptr->leaf && nptr->code == huffCode) {
      if (nptr->value == 0x0000)
        return "EOB";
      return std::to_string(nptr->value);
    }
    i++;
  } while (nptr != nullptr && i < (int)huffCode.size());

  return "";
}
