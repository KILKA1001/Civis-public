#pragma once
#include "civ5/assets/asset_source.hpp"
#include "coast_curves.hpp"

namespace civ5::terrain {
struct CoastStyleDocument {
    CoastProfiles profiles;
    std::vector<std::string> character_files;
};
// Bounded authored XML subset: UTF-8, attributes, comments and element children.
// Required fields are exact/case-sensitive; malformed input, DTDs and external
// entities are rejected. This is not an emulation of malformed native XML.
CoastStyleDocument parse_coast_styles(std::string_view xml);
// The native loader consumes the first 98 lines only. Tail records are ignored.
std::vector<CoastCharacterSourcePoint> parse_coast_character(std::string_view text);

class CoastLibrary {
  public:
    // The caller selects the asset source and precise style key. No implicit
    // archive precedence, basename search, loading-thread state or default FP.
    static CoastLibrary load(const assets::AssetSource &source, const assets::AssetKey &style_key,
                             X87Precision character_precision);
    const CoastProfiles &profiles() const { return document_.profiles; }
    const CoastCharacterTables &characters() const { return characters_; }
    std::span<const std::string> character_files() const { return document_.character_files; }
    const assets::AssetHandle &style_asset() const { return style_asset_; }
    std::span<const assets::AssetHandle> character_assets() const { return character_assets_; }
    X87Precision character_precision() const { return precision_; }

  private:
    CoastLibrary(CoastStyleDocument document, assets::AssetHandle style, X87Precision precision);
    CoastStyleDocument document_;
    assets::AssetHandle style_asset_;
    std::vector<assets::AssetHandle> character_assets_;
    CoastCharacterTables characters_;
    X87Precision precision_;
};
} // namespace civ5::terrain
