#pragma once

#include "Miro.h"

#include <memory>

namespace Miro::Json
{

struct Loader : Reflector
{
    Loader(const Value& value)
        : data(&value.asObject())
    {
    }

    Loader(const Object* obj)
        : data(obj)
    {
    }

    bool isSaving() const override { return false; }

    void reflect(std::string_view key, Value& value) override
    {
        if (auto found = find(*data, key))
            value = *found;
    }

    Child addChild(std::string_view key) override
    {
        auto it = find(*data, key);
        return {std::make_unique<Loader>(&it->asObject())};
    }

    const Object* data;
};

} // namespace Miro::Json
