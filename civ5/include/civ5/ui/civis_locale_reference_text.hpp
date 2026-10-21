#pragma once
#include <functional>
#include <string>
#include <string_view>

namespace civ5::ui {
// Bounded Civis semantics for the installed plain {TXT_KEY_NAME} references.
// The resolver must return a selected, verified source payload or throw.
// Numeric placeholders remain for the existing argument formatter. Native
// grammar/modifier parity and arbitrary localization expressions are not claimed.
std::string expand_civis_locale_references(std::string_view key,
    const std::function<std::string(std::string_view)> &selected_source);
} // namespace civ5::ui
