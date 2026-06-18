#include "SwiftServer.h"

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

// Emits "(...) throws[ -> Res]" — the protocol method signature for one
// command. Empty-request types collapse to no-arg; void handlers drop the
// return.
void emitSignature(std::ostringstream& out,
                   const CommandExport::CommandEntry& cmd,
                   const CommandExport::ResolvedTypes& resolved)
{
    auto hasParam = cmd.hasRequest && !resolved.isRequestEmpty(cmd);

    out << "(";
    if (hasParam)
        out << "_ req: "
            << resolved.nameFor(cmd.requestQualifiedName, cmd.requestTypeName);
    out << ") throws";

    if (cmd.hasResponse)
        out << " -> "
            << resolved.nameFor(cmd.responseQualifiedName, cmd.responseTypeName);
}

void emitProtocol(std::ostringstream& out,
                  std::span<const CommandExport::CommandEntry> commands,
                  const CommandExport::ResolvedTypes& resolved)
{
    out << "protocol Handlers {\n";

    for (auto& cmd: commands)
    {
        out << "    func " << methodName(cmd.name);
        emitSignature(out, cmd, resolved);
        out << "\n";
    }

    out << "}\n";
}

void emitDispatch(std::ostringstream& out,
                  std::span<const CommandExport::CommandEntry> commands,
                  const CommandExport::ResolvedTypes& resolved)
{
    out << "func dispatch(_ handlers: any Handlers, _ command: String, "
           "_ payload: Data) throws -> Data {\n";
    out << "    switch command {\n";

    for (auto& cmd: commands)
    {
        auto hasParam = cmd.hasRequest && !resolved.isRequestEmpty(cmd);
        auto name = methodName(cmd.name);

        out << "    case \"" << Naming::escapeSwiftString(cmd.name) << "\":\n";

        auto call = std::string {"handlers."} + name + "(";
        if (hasParam)
        {
            auto reqType =
                resolved.nameFor(cmd.requestQualifiedName, cmd.requestTypeName);
            out << "        let req = try JSONDecoder().decode(" << reqType
                << ".self, from: payload)\n";
            call += "req";
        }
        call += ")";

        // One `try` covers every throwing call in the expression.
        if (cmd.hasResponse)
        {
            out << "        return try JSONEncoder().encode(" << call << ")\n";
        }
        else
        {
            out << "        try " << call << "\n";
            out << "        return Data(\"{}\".utf8)\n";
        }
    }

    out << "    default:\n";
    out << "        throw MiroDispatchError.unknownCommand(command)\n";
    out << "    }\n";
    out << "}\n";
}

// Static C-ABI adapter. References `dispatch` and `Handlers` from the same
// module. The boxed-context pattern lets a C caller hold a Handlers instance
// across calls without a Swift global.
const char* adapter()
{
    // Custom delimiter: the Swift body contains a `)"` sequence
    // (strdup("\(error)")) that would close a plain R"(...)".
    return R"SWIFT(
// --- C-ABI adapter: lets a C/C++ caller dispatch into these handlers. ---

final class MiroHandlerBox {
    let handlers: any Handlers
    init(_ handlers: any Handlers) { self.handlers = handlers }
}

// Boxes a Handlers instance and returns an opaque context pointer the C side
// passes back to miro_swift_dispatch. Release with miroReleaseHandlers.
func miroInstallHandlers(_ handlers: any Handlers) -> UnsafeMutableRawPointer {
    Unmanaged.passRetained(MiroHandlerBox(handlers)).toOpaque()
}

func miroReleaseHandlers(_ ctx: UnsafeMutableRawPointer) {
    Unmanaged<MiroHandlerBox>.fromOpaque(ctx).release()
}

// `public` so the symbol is exported from a Swift dynamic library (an
// internal @_cdecl func is emitted with local linkage and won't link).
@_cdecl("miro_swift_dispatch")
public func miroSwiftDispatch(
    _ ctx: UnsafeMutableRawPointer,
    _ command: UnsafePointer<CChar>,
    _ payload: UnsafePointer<CChar>,
    _ errorOut: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?
) -> UnsafeMutablePointer<CChar>? {
    let box = Unmanaged<MiroHandlerBox>.fromOpaque(ctx).takeUnretainedValue()
    let cmd = String(cString: command)
    let payloadData = Data(String(cString: payload).utf8)
    do {
        let result = try dispatch(box.handlers, cmd, payloadData)
        return strdup(String(decoding: result, as: UTF8.self))
    } catch {
        errorOut?.pointee = strdup("\(error)")
        return nil
    }
}

@_cdecl("miro_swift_string_free")
public func miroSwiftStringFree(_ ptr: UnsafeMutablePointer<CChar>?) {
    free(ptr)
}
)SWIFT";
}

} // namespace

std::string formatServer(std::span<TypeTree::TypeNode> typeRoots,
                         std::span<const CommandExport::CommandEntry> commands)
{
    auto resolved = CommandExport::resolveTypes(typeRoots);

    auto out = std::ostringstream {};
    out << "// Generated Swift server: Handlers + dispatch + C-ABI adapter.\n";
    out << "import Foundation\n\n";

    emitProtocol(out, commands, resolved);
    out << "\n";
    emitDispatch(out, commands, resolved);
    out << adapter();

    return out.str();
}

} // namespace Miro::Swift
