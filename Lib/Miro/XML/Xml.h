#pragma once

#include "../Containers.h"

#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Miro::Xml
{

// Mixed content is not representable: `text` is meaningful for leaf elements
// only. Out of scope for both parser and printer: CDATA, comments, namespaces,
// DOCTYPE, processing instructions and validation.
struct Node
{
    std::string name;
    std::map<std::string, std::string> attributes {};
    Vector<Node> children {};
    std::string text {};

    bool operator==(const Node& otherToUse) const = default;
};

class ParseError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

const std::string* findAttribute(const Node& node, std::string_view key);
Node* findChild(Node& node, std::string_view name);
const Node* findChild(const Node& node, std::string_view name);

Node parse(std::string_view inputToUse);
std::string print(const Node& nodeToUse, int indentToUse = 0);
void log(const Node& nodeToUse, int indentToUse = 0);

} // namespace Miro::Xml

namespace Miro
{
using XML = Xml::Node;
} // namespace Miro
