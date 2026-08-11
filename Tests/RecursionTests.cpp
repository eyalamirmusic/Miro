// Own executable (MiroRecursionTests): a runaway exporter recursion here
// crashes the process, so it must not take the main MiroTests binary down.

#include "TestHelpers.h"

#include <Miro/Miro.h>
#include <NanoTest/NanoTest.h>

#include <vector>

using namespace nano;
using namespace Miro;

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
    auto out = TypeScript::toZod<Node>();

    check(!out.empty());
    check(contains(out, "export const Node = z.object({"));
    check(contains(out, "value: z.number().int()"));
    check(contains(out, "children: z.array(Node)"));
};

auto recursiveSchema =
    test("Recursion: Schema reflector handles self-referencing types") = []
{
    auto schema = schemaOf<Node>();

    check(schema["$ref"].asString() == "#/$defs/Node");

    auto& body = schema["$defs"]["Node"];
    check(body["type"].asString() == "object");
    check(body["properties"]["value"]["type"].asString() == "integer");
    check(body["properties"]["children"]["type"].asString() == "array");

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

    auto restored = createFromJSONString<Node>(toJSONString(root));

    check(restored.value == 1);
    check(restored.children.size() == 2);
    check(restored.children[0].value == 2);
    check(restored.children[0].children.empty());
    check(restored.children[1].value == 3);
    check(restored.children[1].children.size() == 1);
    check(restored.children[1].children[0].value == 4);
};
