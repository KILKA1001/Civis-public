#pragma once
#include "civ5/runtime/bnw_end_turn_request.hpp"
#include <limits>
namespace civ5::runtime {
using BnwUnitOrderKey=std::pair<std::int64_t,std::int64_t>;
struct BnwUnitOrderSourceFile {std::string_view path,sha256;};
inline constexpr std::array<BnwUnitOrderSourceFile,2> kBnwUnitOrderOwnerSources{{
    {"CvGameCoreDLL_Expansion2/CvUnit.cpp","646e6c26ffb5ca20b77320d1bb7d78af440072eec210a760c2c76275f5e80b82"},
    {"CvGameCoreDLL_Expansion2/Lua/CvLuaUnit.cpp","1d9e89106343b9fd4edfbc2a2efe89c7519f7fbcc06f4c8380da38f37d7bf09f"}
}};
struct BnwUnitRetainedPathNode {std::int32_t x=0,y=0,data1=0,data2=0;std::uint32_t flags=0;bool operator==(const BnwUnitRetainedPathNode&)const=default;};
struct BnwUnitRetainedPath {
    std::vector<BnwUnitRetainedPathNode> nodes;
    std::uint32_t destination=std::numeric_limits<std::uint32_t>::max();
    std::uint64_t clear_operations=0,generation_operations=0,path_to_reject_clear_operations=0;
    bool operator==(const BnwUnitRetainedPath&)const=default;
};
// Original BNW Unit::Read -> reset clears both fields; BNW does not serialize
// the path. This owner survives commands in SAME Session common storage.
// Only genuine Core search can publish it; empty import grants no path.
class BnwNativeUnitAllocatorCore;class LiveSession;
class BnwUnitPathOwner {
  public:
    const auto &paths() const noexcept {return paths_;}
    const auto &identity() const noexcept {return identity_;}
    const auto &map_guid() const noexcept {return map_guid_;}
    void clear(BnwUnitOrderKey);
  private:
    friend class BnwNativeUnitAllocatorCore;friend class LiveSession;
    friend BnwUnitPathOwner bind_bnw_unit_paths_original_import(const saves::SavedSession &,const SessionCheckpointIdentity &);
    BnwUnitPathOwner(SessionCheckpointIdentity identity,std::array<std::uint8_t,16> guid)
      :identity_(std::move(identity)),map_guid_(guid) {}
    // Pure normalized projection; only actual Core continuation preflight
    // uses it. It never mutates the field or grants a store/search.
    static std::string project_unit_flags_revision(const LiveSessionState&,
        const SessionCheckpointIdentity&,const content::RulesDatabase&,BnwUnitOrderKey,std::int32_t);
    SessionCheckpointIdentity identity_;
    std::array<std::uint8_t,16> map_guid_{};
    std::map<BnwUnitOrderKey,BnwUnitRetainedPath> paths_;
};
BnwUnitPathOwner bind_bnw_unit_paths_original_import(const saves::SavedSession &,const SessionCheckpointIdentity &);
// Complete retained unit-command inputs: clock, active/slot roster, every
// decoded unit core/tail and map-unit links. Not a whole-game persistence hash.
std::string bnw_unit_order_world_revision(const LiveSessionState &,const SessionCheckpointIdentity &,const content::RulesDatabase &);
std::uint64_t bnw_unit_order_plot(const LiveSessionState &,BnwUnitOrderKey);

enum class BnwUnitPresentationEffectKind : std::uint8_t {
    // This channel carries source setMoves/load dim values. Other authored
    // forced-highlight notifications need a distinct presentation event.
    should_dim_flag,dirty_unit_info,dirty_plot_list_buttons,dirty_selection_buttons,
    dirty_waypoints,cycle_selection_delta,has_moved_unit
};
struct BnwUnitPresentationEffect {
    BnwUnitPresentationEffectKind kind;
    BnwUnitOrderKey unit;
    std::int64_t value=1;
};
struct BnwUnitPresentationBatch {
    SessionCheckpointIdentity identity;
    std::array<std::uint8_t,16> map_guid{};
    std::string before_world_revision,after_world_revision;
    std::uint64_t expected_client_revision=0;
    std::vector<BnwUnitPresentationEffect> effects;
};
// New Civis client startup owns actual selection and invalidation state. No
// serialized original UI selection/cycle/dirty state is inferred. Load's dim
// feed is applied from actual unit moves, as CvUnit::Read at line1017.
class BnwUnitPresentationOwner {
  public:
    const auto &identity() const noexcept {return identity_;}
    const auto &map_guid() const noexcept {return map_guid_;}
    const auto &world_revision() const noexcept {return world_revision_;}
    std::uint64_t client_revision() const noexcept {return client_revision_;}
    const auto &selected_unit() const noexcept {return selected_unit_;}
    const auto &selection_plot() const noexcept {return selection_plot_;}
    const auto &dim_flags() const noexcept {return dim_flags_;}
    std::int64_t cycle_selection_counter() const noexcept {return cycle_selection_counter_;}
    bool unit_info_dirty() const noexcept {return unit_info_dirty_;}
    bool plot_list_dirty() const noexcept {return plot_list_dirty_;}
    bool selection_buttons_dirty() const noexcept {return selection_buttons_dirty_;}
    bool waypoints_dirty() const noexcept {return waypoints_dirty_;}
    bool has_moved_unit() const noexcept {return has_moved_unit_;}
    const auto &received_effects() const noexcept {return received_effects_;}
  private:
    friend BnwUnitPresentationOwner start_bnw_unit_presentation_client(const LiveSessionState &,const SessionCheckpointIdentity &,const content::RulesDatabase &);
    friend void select_bnw_unit_presentation(const LiveSessionState &,BnwUnitPresentationOwner &,const content::RulesDatabase &,BnwUnitOrderKey,std::uint64_t);
    friend void apply_bnw_unit_presentation_batch(const LiveSessionState &,BnwUnitPresentationOwner &,const content::RulesDatabase &,const BnwUnitPresentationBatch &);
    BnwUnitPresentationOwner(SessionCheckpointIdentity identity,std::array<std::uint8_t,16> guid,std::string revision)
      :identity_(std::move(identity)),map_guid_(guid),world_revision_(std::move(revision)) {}
    SessionCheckpointIdentity identity_;
    std::array<std::uint8_t,16> map_guid_{};
    std::string world_revision_;
    std::uint64_t client_revision_=0;
    std::optional<BnwUnitOrderKey> selected_unit_;
    std::optional<std::uint64_t> selection_plot_;
    std::map<BnwUnitOrderKey,bool> dim_flags_;
    std::int64_t cycle_selection_counter_=0;
    bool unit_info_dirty_=false,plot_list_dirty_=false,selection_buttons_dirty_=false,waypoints_dirty_=false,has_moved_unit_=false;
    std::vector<BnwUnitPresentationEffect> received_effects_;
};
BnwUnitPresentationOwner start_bnw_unit_presentation_client(const LiveSessionState &,const SessionCheckpointIdentity &,const content::RulesDatabase &);
// Explicit standalone client input. Does not execute a gameplay order.
void select_bnw_unit_presentation(const LiveSessionState &,BnwUnitPresentationOwner &,const content::RulesDatabase &,BnwUnitOrderKey,std::uint64_t expected_client_revision);
// Actual retained consumer, not a committed callback. Installs flags/counter/
// invalidation effects atomically on a private owner, with stale feed rejection.
void apply_bnw_unit_presentation_batch(const LiveSessionState &after_world,BnwUnitPresentationOwner &,const content::RulesDatabase &,const BnwUnitPresentationBatch &);
} // namespace civ5::runtime
