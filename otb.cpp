#include "otb.h"

#include <stack>
#include <iostream>
#include <string>

#include "file.h"
#include "util.h"

namespace OTB
{

  using NodeStack = std::stack<Node *, std::vector<Node *>>;

  constexpr Identifier wildcard = {{'\0', '\0', '\0', '\0'}};

  Loader::Loader(const std::string &fileName, const Identifier &acceptedIdentifier)
  {
    fileBuffer = File::read(fileName);

    constexpr auto minimalSize = sizeof(Identifier) + sizeof(Node::START) + sizeof(Node::type) + sizeof(Node::END);
    if (fileBuffer.size() <= minimalSize)
    {
      throw InvalidOTBFormat{};
    }

    Identifier fileIdentifier;
    std::copy(fileBuffer.begin(), fileBuffer.begin() + fileIdentifier.size(), fileIdentifier.begin());
    if (fileIdentifier != acceptedIdentifier && fileIdentifier != wildcard)
    {
      throw InvalidOTBFormat{};
    }
  }

  static Node &getCurrentNode(const NodeStack &nodeStack)
  {
    if (nodeStack.empty())
    {
      throw InvalidOTBFormat{};
    }

    return *nodeStack.top();
  }

  const Node &Loader::parseTree()
  {

    auto cursor = fileBuffer.begin() + sizeof(Identifier);

    if (static_cast<uint8_t>(*cursor) != Node::START)
    {
      throw InvalidOTBFormat{};
    }

    root.type = *(++cursor);
    root.propsBegin = ++cursor;
    NodeStack parseStack;
    parseStack.push(&root);

    for (; cursor != fileBuffer.end(); ++cursor)
    {
      switch (static_cast<uint8_t>(*cursor))
      {
      case Node::START:
      {
        auto &currentNode = getCurrentNode(parseStack);
        if (currentNode.children.empty())
        {
          currentNode.propsEnd = cursor;
        }
        currentNode.children.emplace_back();
        auto &child = currentNode.children.back();
        if (++cursor == fileBuffer.end())
        {
          throw InvalidOTBFormat{};
        }
        child.type = *cursor;
        child.propsBegin = cursor + sizeof(Node::type);
        parseStack.push(&child);
        break;
      }

      case Node::END:
      {
        auto &currentNode = getCurrentNode(parseStack);
        if (currentNode.children.empty())
        {
          currentNode.propsEnd = cursor;
        }
        parseStack.pop();
        break;
      }

      case Node::ESCAPE:
      {
        if (++cursor == fileBuffer.end())
        {
          throw InvalidOTBFormat{};
        }
        break;
      }

      default:
        break;
      }
    }
    if (!parseStack.empty())
    {
      throw InvalidOTBFormat{};
    }

    return root;
  }

  bool Loader::getProps(const Node &node, PropStream &props)
  {
    auto size = std::distance(node.propsBegin, node.propsEnd);
    if (size == 0)
    {
      return false;
    }
    propBuffer.resize(size);
    bool lastEscaped = false;

    auto escapedPropEnd = std::copy_if(node.propsBegin, node.propsEnd, propBuffer.begin(), [&lastEscaped](const char &byte) {
      lastEscaped = byte == static_cast<char>(Node::ESCAPE) && !lastEscaped;
      return !lastEscaped;
    });
    props.init(&propBuffer[0], std::distance(propBuffer.begin(), escapedPropEnd));
    return true;
  }

} //namespace OTB