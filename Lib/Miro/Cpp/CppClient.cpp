#include "CppClient.h"

#include "../CommandExport/CommandExport.h"

#include <map>
#include <sstream>
#include <string>
#include <string_view>

namespace Miro::Cpp
{

namespace
{

// Resolved per-type info derived from the supplied TypeNode roots:
// the final type name (post collision rewrite), and a flag for "object
// with zero fields" so empty-request handlers can elide the parameter.
// Keyed by raw qualified C++ name to match CommandEntry::*QualifiedName.
struct ResolvedTypes
{
    std::map<std::string, std::string> finalNameByQualified;
    std::map<std::string, bool> emptyByQualified;
};

ResolvedTypes resolveTypes(std::span<TypeTree::TypeNode> typeRoots)
{
    TypeTree::prepareRoots(typeRoots);

    auto resolved = ResolvedTypes {};
    for (auto& root: typeRoots)
    {
        if (root.qualifiedName.empty())
            continue;

        resolved.finalNameByQualified[root.qualifiedName] = root.typeName;
        resolved.emptyByQualified[root.qualifiedName] =
            root.shape == TypeTree::TypeNode::Shape::Object && root.fields.empty();
    }
    return resolved;
}

std::string flattenMethodName(std::string_view command)
{
    auto out = std::string {};
    out.reserve(command.size());

    auto i = std::size_t {0};
    while (i < command.size())
    {
        if (i + 1 < command.size() && command[i] == ':' && command[i + 1] == ':')
        {
            out.push_back('_');
            i += 2;
        }
        else
        {
            out.push_back(command[i]);
            ++i;
        }
    }
    return out;
}

std::string resolvedNameFor(const std::string& qualified,
                            const std::string& fallback,
                            const ResolvedTypes& resolved)
{
    auto it = resolved.finalNameByQualified.find(qualified);
    if (it != resolved.finalNameByQualified.end())
        return it->second;
    return fallback;
}

bool requestIsEmpty(const CommandExport::CommandEntry& cmd,
                    const ResolvedTypes& resolved)
{
    if (!cmd.hasRequest)
        return true;
    auto it = resolved.emptyByQualified.find(cmd.requestQualifiedName);
    return it != resolved.emptyByQualified.end() && it->second;
}

void emitMethod(std::ostringstream& out,
                const CommandExport::CommandEntry& cmd,
                const ResolvedTypes& resolved)
{
    auto resType = std::string {"void"};
    if (cmd.hasResponse)
        resType = "::"
                  + resolvedNameFor(
                      cmd.responseQualifiedName, cmd.responseTypeName, resolved);

    auto reqEmpty = requestIsEmpty(cmd, resolved);
    auto methodName = flattenMethodName(cmd.name);

    out << "    " << resType << " " << methodName << "(";

    if (cmd.hasRequest && !reqEmpty)
    {
        auto reqType = "::"
                       + resolvedNameFor(
                           cmd.requestQualifiedName, cmd.requestTypeName, resolved);
        out << "const " << reqType << "& req";
    }

    out << ")\n    {\n";

    if (cmd.hasRequest && !reqEmpty)
        out << "        auto payload = ::Miro::toJSON(req);\n";
    else
        out << "        auto payload =\n"
               "            ::Miro::JSON {::Miro::Json::Object {}};\n";

    if (cmd.hasResponse)
    {
        out << "        auto result = invoker(\"" << cmd.name << "\", payload);\n";
        out << "        auto out = " << resType << " {};\n";
        out << "        ::Miro::fromJSON(out, result);\n";
        out << "        return out;\n";
    }
    else
    {
        out << "        (void) invoker(\"" << cmd.name << "\", payload);\n";
    }

    out << "    }\n";
}

} // namespace

std::string formatClientHeader(std::span<TypeTree::TypeNode> typeRoots,
                               std::span<const CommandExport::CommandEntry> commands,
                               std::string_view typesHeader)
{
    auto resolved = resolveTypes(typeRoots);

    auto out = std::ostringstream {};
    out << "#pragma once\n\n";
    out << "#include \"" << typesHeader << ".miro.h\"\n\n";
    out << "#include <Miro/Miro.h>\n\n";
    out << "#include <functional>\n";
    out << "#include <string>\n";
    out << "#include <utility>\n\n";
    out << "namespace MiroClient\n{\n\n";
    out << "using Invoke = std::function<::Miro::JSON(const std::string& "
           "command,\n"
           "                                          const ::Miro::JSON& "
           "payload)>;\n\n";
    out << "class Client\n{\npublic:\n";
    out << "    explicit Client(Invoke invokerToUse)\n"
           "        : invoker(std::move(invokerToUse))\n"
           "    {}\n";

    for (auto& cmd: commands)
    {
        out << "\n";
        emitMethod(out, cmd, resolved);
    }

    out << "\nprivate:\n";
    out << "    Invoke invoker;\n";
    out << "};\n\n";
    out << "} // namespace MiroClient\n";

    return out.str();
}

} // namespace Miro::Cpp
