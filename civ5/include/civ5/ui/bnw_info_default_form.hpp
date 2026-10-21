#pragma once
#include "civ5/ui/top_panel.hpp"
#include <array>

namespace civ5::ui {
struct BnwInfoDefaultFormSource { std::string_view path,sha256; };
inline constexpr std::array<BnwInfoDefaultFormSource,4> kBnwInfoDefaultFormSources{{
    {"CvLocalization/include/CvLocalization.h","504fa77703e7dbf70dc8bf20c4a4908c86077a896f6bfd67c137ffb44e54334a"},
    {"CvGameCoreDLL_Expansion2/CvInfos.cpp","b520062d7ee5bf67485b9bcc3b5940cab6616d3d56847999a6c11cde834c0e2c"},
    {"CvGameCoreDLL_Expansion2/Lua/CvLuaUnit.cpp","1d9e89106343b9fd4edfbc2a2efe89c7519f7fbcc06f4c8380da38f37d7bf09f"},
    {"CvGameCoreDLL_Expansion2/CvUnit.cpp","646e6c26ffb5ca20b77320d1bb7d78af440072eec210a760c2c76275f5e80b82"}
}};
// Literal, no-argument selected Info Description names only. Original
// GetName -> UnitInfo Description uses the default first localized form.
// This does not implement parameter, gender or plural composition grammar.
std::string bnw_info_default_form(const TopPanelLocalizationCatalog &,std::string_view key);
} // namespace civ5::ui
