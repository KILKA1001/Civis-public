#pragma once
#include "civ5/content/manifest.hpp"
#include <cstdint>
namespace civ5::graphics {
enum class BnwUnitFxFileClass {texture,granny,other};
struct BnwUnitFxFileDeclaration {
    std::string node_name,file;
    std::vector<std::int32_t> event_codes;
    bool has_event_attribute=false;
    BnwUnitFxFileClass extension_class=BnwUnitFxFileClass::other;
};
// Pure source declaration projection. No native resource/caller authority.
std::vector<BnwUnitFxFileDeclaration> read_bnw_unit_fx_declarations(const content::XmlNode&);
std::string bnw_unit_fx_native_extension(std::string_view);
} // namespace civ5::graphics
