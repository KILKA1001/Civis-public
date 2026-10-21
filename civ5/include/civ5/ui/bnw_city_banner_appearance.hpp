#pragma once
#include "civ5/ui/bnw_city_banner_appearance_types.hpp"
#include <memory>
#include <optional>
#include <string>

namespace civ5::runtime {class LiveSession;struct PregameRoster;}
namespace civ5::content {class RulesDatabase;}
namespace civ5::assets {class ResourceMountTable;}
namespace civ5::ui {
// Borrows the actual current owners. A copied/restored roster per query cannot
// establish current state. Root shares this same roster with Lua PreGame APIs.
// Binds the LiveSession state generation at construction. Construct a new owner
// after successful state replacement; all borrowed objects must outlive it.
class BnwCityBannerAppearanceOwner {
    struct Impl;
    std::unique_ptr<Impl> impl_;
  public:
    BnwCityBannerAppearanceOwner(const runtime::LiveSession&,const runtime::PregameRoster&,
        const content::RulesDatabase&,const assets::ResourceMountTable&);
    ~BnwCityBannerAppearanceOwner();
    BnwCityBannerAppearanceOwner(const BnwCityBannerAppearanceOwner&)=delete;
    BnwCityBannerAppearanceOwner& operator=(const BnwCityBannerAppearanceOwner&)=delete;
    std::optional<BnwCityBannerAppearance> capture(std::int64_t active_player,
        std::int64_t city_player,std::int64_t city_id,std::uint64_t frame_number);
    bool matches(const BnwCityBannerAppearance&) const;
    const std::string &blocker() const noexcept;
};
} // namespace civ5::ui
