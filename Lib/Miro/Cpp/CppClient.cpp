#include "CppClient.h"

#include "../CommandExport/ResolvedTypes.h"
#include "../Detail/StringUtilities.h"

#include <sstream>
#include <string>
#include <string_view>

namespace Miro::Cpp
{

namespace
{

std::string flattenMethodName(std::string_view command)
{
    return Detail::replaceAll(command, "::", "_");
}

void emitMethod(std::ostringstream& out,
                const CommandExport::CommandEntry& cmd,
                const CommandExport::ResolvedTypes& resolved)
{
    auto resType = std::string {"void"};

    if (cmd.hasResponse)
        resType =
            "::" + resolved.nameFor(cmd.responseQualifiedName, cmd.responseTypeName);

    auto reqEmpty = resolved.isRequestEmpty(cmd);
    auto methodName = flattenMethodName(cmd.name);

    out << "    " << resType << " " << methodName << "(";

    if (cmd.hasRequest && !reqEmpty)
    {
        auto reqType =
            "::" + resolved.nameFor(cmd.requestQualifiedName, cmd.requestTypeName);
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
    auto resolved = CommandExport::resolveTypes(typeRoots);

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
