#include <Miro/Miro.h>

#include <NanoTest/NanoTest.h>

#include <string>
#include <vector>

using namespace nano;
using namespace Miro;

// A leaf type that, on save, emits whatever custom tag drilled down to
// it into a "tag" field — so a test can read it back out of the JSON.
struct SaveTagEmitter
{
    void reflect(Miro::Reflector& ref)
    {
        auto tag = ref.customOptions().tag;
        ref["tag"](tag);
    }
};

struct SaveTagOuter
{
    SaveTagEmitter inner;

    void reflect(Miro::Reflector& ref) { ref["inner"](inner); }
};

// A leaf type that records the drilled-down tag into a member that is
// NOT part of reflection, so the load path can't overwrite it — the
// test inspects it directly after fromJSON.
struct LoadTagSpy
{
    int x = 0;
    std::string seenTag;

    void reflect(Miro::Reflector& ref)
    {
        seenTag = ref.customOptions().tag;
        ref["x"](x);
    }
};

struct LoadTagOuter
{
    LoadTagSpy inner;

    void reflect(Miro::Reflector& ref) { ref["inner"](inner); }
};

struct LoadTagVec
{
    std::vector<LoadTagSpy> items;

    void reflect(Miro::Reflector& ref) { ref["items"](items); }
};

// --- Defaults ---

auto customDefaultsEmptyOnSave = test("Custom tag defaults to empty (save)") = []
{
    auto json = toJSON(SaveTagOuter {});

    check(json["inner"]["tag"].asString() == "");
};

auto customDefaultsEmptyOnLoad = test("Custom tag defaults to empty (load)") = []
{
    auto outer = createFromJSONString<LoadTagOuter>(R"({"inner":{"x":5}})");

    check(outer.inner.x == 5);
    check(outer.inner.seenTag == "");
};

// --- Drill-down on save ---

auto customDrillsDownOnSave =
    test("Custom tag drills down to nested type (save)") = []
{
    auto json = toJSON(SaveTagOuter {}, CustomOptions {.tag = "session:abc"});

    check(json["inner"]["tag"].asString() == "session:abc");
};

// --- Drill-down on load ---

auto customDrillsDownOnLoad =
    test("Custom tag drills down to nested type (load)") = []
{
    auto outer = LoadTagOuter {};
    fromJSONString(outer, R"({"inner":{"x":9}})", CustomOptions {.tag = "load-tag"});

    check(outer.inner.x == 9);
    check(outer.inner.seenTag == "load-tag");
};

auto customFactoryOverloadCarriesTag =
    test("createFromJSONString carries the custom tag") = []
{
    auto outer = createFromJSONString<LoadTagOuter>(
        R"({"inner":{"x":1}})", CustomOptions {.tag = "factory-tag"});

    check(outer.inner.seenTag == "factory-tag");
};

// --- Drill-down through a container ---

auto customDrillsThroughContainer =
    test("Custom tag drills down through vector elements") = []
{
    auto vec = LoadTagVec {};
    fromJSONString(
        vec, R"({"items":[{"x":1},{"x":2}]})", CustomOptions {.tag = "vec-tag"});

    check(vec.items.size() == 2);
    check(vec.items[0].seenTag == "vec-tag");
    check(vec.items[1].seenTag == "vec-tag");
};
