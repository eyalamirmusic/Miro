#pragma once

#include "../Containers.h"

#include <string>
#include <string_view>

namespace Miro::Detail
{

bool isAsciiIdentStart(char c);
bool isAsciiIdentPart(char c);
bool isAsciiIdentifier(std::string_view name);

bool isJsIdentStart(char c);
bool isJsIdentPart(char c);
bool isJsIdentifier(std::string_view name);

Vector<std::string> splitOn(std::string_view input, std::string_view delimiter);

std::string
    replaceAll(std::string_view input, std::string_view from, std::string_view to);

std::string trimAsciiWhitespace(std::string_view input);

std::string makeIndent(int width, int depth);

} // namespace Miro::Detail
