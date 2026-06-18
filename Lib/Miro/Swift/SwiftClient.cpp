#include "SwiftClient.h"

#include "../CommandExport/ResolvedTypes.h"
#include "../Detail/StringUtilities.h"
#include "SwiftNaming.h"

#include <sstream>
#include <string>
#include <string_view>

namespace Miro::Swift
{

namespace
{

// Dotted wire name -> Swift method name: "api.v2.echo" -> api_v2_echo,
// backtick-escaped if the result collides with a Swift keyword.
std::string methodName(std::string_view command)
{
    return Naming::swiftIdentifier(Detail::replaceAll(command, ".", "_"));
}

void emitMethod(std::ostringstream& out,
                const CommandExport::CommandEntry& cmd,
                const CommandExport::ResolvedTypes& resolved)
{
    auto hasParam = cmd.hasRequest && !resolved.isRequestEmpty(cmd);
    auto wire = Naming::escapeSwiftString(cmd.name);

    out << "    func " << methodName(cmd.name) << "(";

    if (hasParam)
    {
        auto reqType =
            resolved.nameFor(cmd.requestQualifiedName, cmd.requestTypeName);
        out << "_ req: " << reqType;
    }

    out << ") throws";

    if (cmd.hasResponse)
    {
        auto resType =
            resolved.nameFor(cmd.responseQualifiedName, cmd.responseTypeName);
        out << " -> " << resType;
    }

    out << " {\n";

    if (hasParam)
        out << "        let payload = try encoder.encode(req)\n";
    else
        out << "        let payload = Data(\"{}\".utf8)\n";

    if (cmd.hasResponse)
    {
        auto resType =
            resolved.nameFor(cmd.responseQualifiedName, cmd.responseTypeName);
        out << "        let result = try transport.invoke(\"" << wire
            << "\", payload)\n";
        out << "        return try decoder.decode(" << resType
            << ".self, from: result)\n";
    }
    else
    {
        out << "        _ = try transport.invoke(\"" << wire << "\", payload)\n";
    }

    out << "    }\n";
}

} // namespace

std::string formatClient(std::span<TypeTree::TypeNode> typeRoots,
                         std::span<const CommandExport::CommandEntry> commands)
{
    auto resolved = CommandExport::resolveTypes(typeRoots);

    auto out = std::ostringstream {};
    out << "// Typed client: one method per command over a MiroTransport.\n";
    out << "import Foundation\n\n";
    out << "final class Client {\n";
    out << "    private let transport: MiroTransport\n";
    out << "    private let encoder = JSONEncoder()\n";
    out << "    private let decoder = JSONDecoder()\n\n";
    out << "    init(_ transport: MiroTransport) {\n";
    out << "        self.transport = transport\n";
    out << "    }\n\n";
    out << "    // Convenience: wrap a plain closure as the transport.\n";
    out << "    convenience init(\n";
    out << "        invoke: @escaping (_ command: String, _ payload: Data) "
           "throws -> Data\n";
    out << "    ) {\n";
    out << "        self.init(MiroClosureTransport(invoke))\n";
    out << "    }\n";

    for (auto& cmd: commands)
    {
        out << "\n";
        emitMethod(out, cmd, resolved);
    }

    out << "}\n";

    return out.str();
}

} // namespace Miro::Swift
