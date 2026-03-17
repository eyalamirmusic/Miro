#pragma once

#include "Miro.h"

#include <memory>

namespace Miro::Json
{

struct Saver : Reflector
{
    bool isSaving() const override { return true; }

    void reflect(std::string_view key, Value& value) override
    {
        auto& obj = std::get<Object>(root.data);
        obj[std::string(key)] = value;
    }

    Child addChild(std::string_view key) override
    {
        auto child = std::make_unique<Saver>();
        child->parent = this;
        child->parentKey = std::string(key);
        return {std::move(child)};
    }

    ~Saver() override
    {
        if (parent)
        {
            auto& obj = std::get<Object>(parent->root.data);
            obj[parentKey] = std::move(root);
        }
    }

    Value root {Object {}};
    Saver* parent = nullptr;
    std::string parentKey;

};

} // namespace Miro::Json
