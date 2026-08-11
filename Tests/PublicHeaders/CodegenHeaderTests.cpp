// The real assertion is that this TU compiles with one Miro include.

#include <Miro/Codegen.h>

#include <NanoTest/NanoTest.h>

using namespace nano;

namespace
{
struct Point
{
    void reflect(Miro::Reflector& r)
    {
        r["x"](x);
        r["y"](y);
    }

    int x = 0;
    int y = 0;
};
} // namespace

auto codegenEntryHeader = test("Entry header: Miro/Codegen.h is self-contained") = []
{
    auto tree = Miro::TypeTree::buildTree<Point>();
    check(tree.shape == Miro::TypeTree::TypeNode::Shape::Object);
    check(tree.fields.size() == 2);
};
