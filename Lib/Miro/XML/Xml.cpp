#include "Xml.h"

namespace Miro::Xml
{

const std::string* findAttribute(const Node& node, std::string_view key)
{
    auto it = node.attributes.find(std::string(key));

    if (it != node.attributes.end())
        return &it->second;

    return nullptr;
}

Node* findChild(Node& node, std::string_view name)
{
    for (auto& child: node.children)
        if (child.name == name)
            return &child;

    return nullptr;
}

const Node* findChild(const Node& node, std::string_view name)
{
    for (const auto& child: node.children)
        if (child.name == name)
            return &child;

    return nullptr;
}

} // namespace Miro::Xml
