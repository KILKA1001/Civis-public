#pragma once
#include "civ5/ui/bnw_client_text_policy.hpp"
namespace civ5::ui::detail {
// Read-only values from the actual activated Engine localization database.
// This DTO supplies no Gui/cache/Label authority and is never caller input.
// The sealed cache issuer obtains it only on a genuine signed-height MISS.
struct BnwLabelSystemFontLanguageData final {
 BnwLabelLocaleSelection locale;
 std::string font_name;
 float width_scale,height_scale;
};
}
