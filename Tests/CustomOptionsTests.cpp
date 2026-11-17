#include <Miro/Miro.h>

#include <NanoTest/NanoTest.h>

#include <string>
#include <type_traits>
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

// --- Shared, not copied ---

// Records which CustomOptions object each level of the walk sees.
struct OptionsAddressSpy
{
    void reflect(Miro::Reflector& ref)
    {
        seen = &ref.customOptions();
        ref["x"](x);
    }

    int x = 0;
    const CustomOptions* seen = nullptr;
};

struct OptionsAddressOuter
{
    void reflect(Miro::Reflector& ref)
    {
        seen = &ref.customOptions();
        ref["inner"](inner);
        ref["items"](items);
    }

    OptionsAddressSpy inner;
    std::vector<OptionsAddressSpy> items;
    const CustomOptions* seen = nullptr;
};

static_assert(std::is_trivially_copyable_v<Options>,
              "spawning a child copies Options; it must never allocate");

auto customSharedByEveryChild =
    test("Every child shares the root's CustomOptions object") = []
{
    auto outer = OptionsAddressOuter {};
    fromJSONString(outer,
                   R"({"inner":{"x":1},"items":[{"x":2}]})",
                   CustomOptions {.tag = "shared"});

    check(outer.seen != nullptr);
    check(outer.inner.seen == outer.seen);
    check(outer.items.size() == 1);
    check(outer.items[0].seen == outer.seen);
};

auto customDefaultsToNone = test("A walk without CustomOptions sees none()") = []
{
    auto outer = OptionsAddressOuter {};
    fromJSONString(outer, R"({"inner":{"x":1}})");

    check(outer.seen != nullptr);
    check(outer.seen->tag.empty());

    auto json = JSON {};
    auto root = JsonReflector {json, Options {.shape = Shape::Object}};
    check(&root.customOptions() == &CustomOptions::none());
};

auto childOptionsInheritCustom =
    test("childOptionsFor keeps mode, schema and custom") = []
{
    auto custom = CustomOptions {.tag = "t"};
    auto parent =
        Detail::topLevelOptions<OptionsAddressOuter>(Mode::Load, true, &custom);
    auto child = Detail::childOptionsFor<std::vector<int>>(parent);

    check(child.mode == Mode::Load);
    check(child.schema);
    check(child.custom == &custom);
    check(child.shape == Shape::Array);
};
