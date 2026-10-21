#pragma once
#include "civ5/runtime/live_session.hpp"
#include <array>
#include <optional>
#include <string_view>

namespace civ5::runtime {
struct BnwPathfinderCacheSourceFile {std::string_view name,sha256;};
// Exact Expansion2 SDK bytes used for this source port. Tests may verify the
// installed SDK against these pins; production does not require SDK files.
inline constexpr std::array<BnwPathfinderCacheSourceFile,4> kBnwPathfinderCacheSourceFiles{{
    {"CvAStar.h","701f13907c21aceac2cda8b135b7d3083df13298fd9bb331e27055cd8a882043"},
    {"CvAStar.cpp","a4b74ded578b8eab4c381ccdec05e0fe1ed46584e63b5fda5edf067f32e9a908"},
    {"CvMap.cpp","303efe905e35612c518227c4df905587924e70e31c2fa3a9f431f06e1a9fa898"},
    {"CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"}
}};
class BnwTwoLayerUnitPathFinder;
enum class BnwActivationFinder : std::uint8_t {
    common,ignore_units,tactical,influence,route,water_route
};
// The actual CvAStar control plane, retained alongside the owning world.
// These source members are independent of the still-missing search callbacks,
// node trees and unit/route algorithms. No path result is produced here.
class BnwAStarCacheControl {
  public:
    bool mp_cache_safe() const noexcept {return mp_cache_safe_;}
    bool force_reset() const noexcept {return force_reset_;}
    bool data_change_invalidates() const noexcept {return data_invalidates_;}
    std::optional<std::uint64_t> data_identity() const noexcept {return data_;}
    bool set_mp_cache_safe(bool state) noexcept;
    void force_reset_cache() noexcept {force_reset_=true;}
    void set_data(std::optional<std::uint64_t> stable_identity) noexcept;
    // Source CvAStar::GeneratePath reuse gate only, not path readiness.
    bool permit_requested_reuse(bool requested,bool network_multiplayer,
        std::int64_t start_x,std::int64_t start_y,std::int64_t info) const noexcept;
  private:
    friend struct BnwPathfinderCacheControlOwner;
    friend class BnwTwoLayerUnitPathFinder;
    bool mp_cache_safe_=false,force_reset_=false,data_invalidates_=false;
    std::optional<std::uint64_t> data_;
    // CvMap::setup Initialize reset, not a fabricated successful search.
    std::int64_t start_x_=-1,start_y_=-1,info_=0;
};
struct BnwPathfinderCacheScope {
    std::uint64_t id=0;
    std::int64_t player=-1,turn=-1;
    std::array<bool,6> previous{};
};
struct BnwPathfinderCacheControlOwner {
    SessionCheckpointIdentity identity;
    std::array<std::uint8_t,16> map_guid{};
    saves::MapGrid grid;
    bool wrap_x=false,wrap_y=false;
    // New controls perform ctor + Initialize + setup invalidation policy;
    // an unknown imported runtime control owner must never be inferred false.
    bool initialized=false;
    std::array<BnwAStarCacheControl,6> controls;
    // Retained proof of the six covered calls' actual CvMap::setup order.
    std::vector<BnwActivationFinder> initialization_order;
    std::vector<BnwPathfinderCacheScope> scopes;
    std::uint64_t next_scope=1;
    BnwAStarCacheControl &at(BnwActivationFinder finder) {return controls.at(static_cast<std::size_t>(finder));}
    const BnwAStarCacheControl &at(BnwActivationFinder finder) const {return controls.at(static_cast<std::size_t>(finder));}
    // CvAStar.cpp:139-181 + CvMap.cpp:482-497. Control initialization
    // performs the source six-finder order, including policy setters.
    void initialize_source_controls();
};
// Creates a NEW six-finder control plane at this standalone original import.
// It does not restore EXE path trees or authorize any GeneratePath callback.
BnwPathfinderCacheControlOwner make_bnw_pathfinder_cache_control_import_owner(
    const saves::SavedSession &source,const SessionCheckpointIdentity &identity);
// Source order: common, ignore-units, tactical, influence, route, water-route.
// Nested native AI scopes use a stack and preserve the exact previous flags.
// Atomic on failure. Restore uses SetMPCacheSafe(previous); ForceReset persists.
void enter_bnw_pathfinder_cache_scope(const LiveSessionState &state,
    BnwPathfinderCacheControlOwner &owner,std::int64_t player);
void restore_bnw_pathfinder_cache_scope(const LiveSessionState &state,
    BnwPathfinderCacheControlOwner &owner,std::int64_t player);
// CvGame::updateMoves resets the common finder before querying ready units.
// This changes the SAME retained source control plane, never another cache.
void force_reset_bnw_update_moves_common_cache(const LiveSessionState &,
    BnwPathfinderCacheControlOwner &,std::int64_t player);
} // namespace civ5::runtime
