#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/selected_game_option_provider.hpp"
#include "civ5/saves/pregame.hpp"
#include "civ5/ui/civis_civilopedia_category_callbacks.hpp"
#include <array>

namespace civ5::ui {
enum class CivisGameOptionResolutionOrigin { saved_prefix, selected_default, missing_default_zero };
struct CivisSelectedGameOptionResolution {
    std::string key="GAMEOPTION_NO_RELIGION",rules_sha256;
    std::int64_t token=21,value=0;
    CivisGameOptionResolutionOrigin origin=CivisGameOptionResolutionOrigin::missing_default_zero;
    std::optional<std::size_t> saved_ordinal;
};
struct CivisGameOptionSourcePin {
    std::string_view sdk_relative_path,sha256;
};
// Reconstructs only the selected SDK's NO_RELIGION enum-cache read for an
// explicitly diagnostic owner. It does not claim the native Pregame read hook
// ran, publish a gameplay epoch, or implement general Game.IsOption overloads.
// Raw options and resolution are copied at open. No borrowed Pregame/DB
// reference survives; later source mutations do not refresh this cache.
class CivisSelectedGameOptionOwner:public std::enable_shared_from_this<CivisSelectedGameOptionOwner> {
  public:
    static std::shared_ptr<CivisSelectedGameOptionOwner> open(
        const saves::PregameObservation &raw,const content::RulesDatabase &selected_rules,
        CivilopediaLayoutOwnerKey identity,const std::shared_ptr<CivisCivilopediaCallbackOwner> &current_owner);
    ~CivisSelectedGameOptionOwner();
    runtime::ScriptSelectedGameOptionProvider provider();
    CivisSelectedGameOptionResolution resolution()const;
    void close();
    static const std::array<CivisGameOptionSourcePin,5> &source_pins();
  private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit CivisSelectedGameOptionOwner(std::unique_ptr<Impl>);
};
}
