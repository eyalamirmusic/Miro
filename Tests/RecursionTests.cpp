// Tests around self-referencing types. JSON Save/Load already work —
// both branches walk the actual data (which is finite by construction),
// so recursion terminates. The Schema and TypeScript exporters do not:
// their schema-mode branch in ReflectContainers.h synthesizes an inner
// T{} regardless of the data, so a `struct Node { vector<Node> children; }`
// recurses forever during reflection and stack-overflows.
//
// These tests live in their own executable so the SIGSEGV from the
// crashing exporters is contained to its own CTest entries — the main
// MiroTests binary keeps running. Once the bug is fixed, the
// assertions below describe the minimum behavior the exporters should
// produce; they can be tightened as the fix lands.

#include "TestHelpers.h"

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <vector>

using namespace nano;

namespace
{

struct Node
{
    int value = 0;
    std::vector<Node> children;

    MIRO_REFLECT(value, children)
};

} // namespace

auto recursiveTypeScript =
    test("Recursion: TypeScript exporter handles self-referencing types") = []
{
    auto out = Miro::TypeScript::toZod<Node>();

    check(!out.empty());
    check(contains(out, "export const Node = z.object({"));
    check(contains(out, "value: z.number().int()"));
    check(contains(out, "children: z.array(Node)"));
};

auto recursiveSchema =
    test("Recursion: Schema reflector handles self-referencing types") = []
{
    auto schema = Miro::schemaOf<Node>();

    // Top-level $ref into the $defs entry for Node.
    check(schema["$ref"].asString() == "#/$defs/Node");

    auto& body = schema["$defs"]["Node"];
    check(body["type"].asString() == "object");
    check(body["properties"]["value"]["type"].asString() == "integer");
    check(body["properties"]["children"]["type"].asString() == "array");

    // The recursive back-edge — children's items should reference Node.
    check(body["properties"]["children"]["items"]["$ref"].asString()
          == "#/$defs/Node");
};

auto recursiveJsonRoundtrip =
    test("Recursion: JSON round-trip handles self-referencing types") = []
{
    auto root = Node {};
    root.value = 1;
    root.children.resize(2);
    root.children[0].value = 2;
    root.children[1].value = 3;
    root.children[1].children.resize(1);
    root.children[1].children[0].value = 4;

    auto restored = Miro::createFromJSONString<Node>(Miro::toJSONString(root));

    check(restored.value == 1);
    check(restored.children.size() == 2);
    check(restored.children[0].value == 2);
    check(restored.children[0].children.empty());
    check(restored.children[1].value == 3);
    check(restored.children[1].children.size() == 1);
    check(restored.children[1].children[0].value == 4);
};
