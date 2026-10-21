#include "civ5/runtime/bnw_fresh_tactical_map.hpp"
#include <limits>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

namespace civ5::runtime {
namespace {
[[noreturn]] void refuse(BnwFreshTacticalMapError code,const char* text) {
    throw BnwFreshTacticalMapRefusal(code,text);
}
// Cell constructor initializers, then Clear(), whose first call clears flags.
// No units/plots/zone objects are supplied by a caller or synthesized as live.
struct Cell {
    std::array<const void*,6> units{nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
    std::int32_t defense=0,deployment=0,target=0,zone=-1;
    std::uint32_t flags=0;
    BnwTacticalCellSnapshot snapshot() const {
        BnwTacticalCellSnapshot out;
        out.flags=flags;
        for(std::size_t i=0;i<units.size();++i) out.unit_pointer_null[i]=units[i]==nullptr;
        out.defense_modifier=defense;out.deployment_score=deployment;
        out.target_type=target;out.dominance_zone=zone;return out;
    }
};
struct TemporaryZone {
    BnwTacticalTemporaryZoneSnapshot fields;
};
std::int32_t reached_dominance_define(const content::RulesDatabase& db) {
    const auto v=db.global_define("AI_TACTICAL_MAP_DOMINANCE_PERCENTAGE",content::GlobalDefineKind::integer);
    if(!v.error.empty() || !std::holds_alternative<std::int64_t>(v.value))
        refuse(BnwFreshTacticalMapError::native_define_unavailable,"Tactical dominance cached Define unavailable");
    const auto n=std::get<std::int64_t>(v.value);
    if(n<std::numeric_limits<std::int32_t>::min() || n>std::numeric_limits<std::int32_t>::max())
        refuse(BnwFreshTacticalMapError::native_define_unavailable,"Tactical dominance exceeds native int32");
    return static_cast<std::int32_t>(n);
}
} // namespace
BnwFreshTacticalMapRefusal::BnwFreshTacticalMapRefusal(BnwFreshTacticalMapError c,const std::string& s)
    :std::runtime_error(s),code_(c) {}

struct detail::BnwTacticalMapStorage::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    // All these scalars are WRITTEN by the native ctor. No omitted native
    // member is made readable via zero initialization of a generic snapshot.
    std::unique_ptr<Cell[]> plots;
    std::int32_t dominance=25,unit_strength=5,range=8,best_range=0,num_plots=0;
    const void* player=nullptr;
    std::int32_t turn=-1;
    bool ignore_los=false,at_war=false,built=false,failed=false;
    TemporaryZone temp;
    std::vector<TemporaryZone> zones;
    std::vector<const void*> enemies;
    Impl() {
        // FStaticVector's constructor allocates capacity but constructs no
        // element. Its native maximum initial capacities are 5*22 and250.
        zones.reserve(110);enemies.reserve(250);
        zones.clear(); // CvTacticalAnalysisMap ctor body, after built/turn/atWar.
    }
    void current() const {
        if(thread!=std::this_thread::get_id())
            refuse(BnwFreshTacticalMapError::wrong_thread,"Tactical allocation belongs to another thread");
    }
};
detail::BnwTacticalMapStorage::BnwTacticalMapStorage():impl_(std::make_unique<Impl>()) {}
detail::BnwTacticalMapStorage::~BnwTacticalMapStorage()=default;
BnwTacticalMapSnapshot detail::BnwTacticalMapStorage::observe() const {
    impl_->current();BnwTacticalMapSnapshot out;
    out.plots_allocated=impl_->plots!=nullptr;out.player_pointer_null=impl_->player==nullptr;
    out.dominance_percentage=impl_->dominance;out.unit_strength_multiplier=impl_->unit_strength;
    out.tactical_range=impl_->range;out.best_friendly_range=impl_->best_range;
    out.num_plots=impl_->num_plots;out.turn_built=impl_->turn;
    out.ignore_los=impl_->ignore_los;out.at_war=impl_->at_war;out.is_built=impl_->built;
    out.dominance_zones=static_cast<std::uint32_t>(impl_->zones.size());
    out.enemy_units=static_cast<std::uint32_t>(impl_->enemies.size());
    out.temporary_zone=impl_->temp.fields;return out;
}
BnwTacticalCellSnapshot detail::BnwTacticalMapStorage::cell(std::int32_t i) const {
    impl_->current();
    if(!impl_->plots || i<0 || i>=impl_->num_plots)
        refuse(BnwFreshTacticalMapError::cell_unavailable,"Tactical cell has no actual allocated native index");
    return impl_->plots[static_cast<std::size_t>(i)].snapshot();
}
void detail::BnwTacticalMapStorage::initialize_detached(std::int32_t count,const content::RulesDatabase& db) {
    impl_->current();
    if(impl_->failed) refuse(BnwFreshTacticalMapError::poisoned,"Partial Tactical Init cannot replay");
    // Host budget/negative native-array length are a refusal, not a smaller map.
    if(count<0 || count>1048576)
        refuse(BnwFreshTacticalMapError::native_count_unavailable,"Tactical Init native count/budget unsupported");
    if(!db.immutable_preview_fingerprints_current())
        refuse(BnwFreshTacticalMapError::parent_retired,"Tactical Init requires actual immutable rules");
    const auto sha=db.gameplay_fingerprint_sha256();
    impl_->failed=true; // Host retained partial-effects protection, not an SDK store.
    // Exact SDK sequence: delete old array, construct ALL new cells, store
    // count, THEN read/store cached dominance. Later failure retains prefix.
    impl_->plots.reset();
    impl_->plots=std::make_unique<Cell[]>(static_cast<std::size_t>(count));
    impl_->num_plots=count;
    const auto dominance=reached_dominance_define(db);
    if(!db.immutable_preview_fingerprints_current() || db.gameplay_fingerprint_sha256()!=sha)
        refuse(BnwFreshTacticalMapError::parent_retired,"Tactical rules changed during Init");
    impl_->dominance=dominance;
    impl_->failed=false;
    // SDK Init does not write player/turn/built/range/strength or zones/enemies.
}

struct BnwFreshTacticalMap::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::shared_ptr<const BnwCreationConfig> config;
    const content::RulesDatabase* db=nullptr;
    detail::BnwTacticalMapStorage storage;
    void current() const {
        if(thread!=std::this_thread::get_id())
            refuse(BnwFreshTacticalMapError::wrong_thread,"Game Tactical owner belongs to another thread");
        if(!config || !db) refuse(BnwFreshTacticalMapError::parent_unavailable,"Game Tactical owner lost actual rules parent");
        // Actual facade retirement/profile/source checks before raw DB borrow.
        config->require_current();
        if(!db->immutable_preview_fingerprints_current() || db->gameplay_fingerprint_sha256()!=config->metadata().rules_sha256)
            refuse(BnwFreshTacticalMapError::parent_retired,"Game Tactical immutable rules changed");
    }
};
BnwFreshTacticalMap::BnwFreshTacticalMap(std::shared_ptr<const BnwCreationConfig> config,const content::RulesDatabase& db) {
    if(!config) refuse(BnwFreshTacticalMapError::parent_unavailable,"Tactical owner requires actual Game config");
    config->require_current();
    impl_=std::make_unique<Impl>();impl_->config=std::move(config);impl_->db=&db;impl_->current();
}
BnwFreshTacticalMap::~BnwFreshTacticalMap()=default;
void BnwFreshTacticalMap::require_current() const {impl_->current();}
void BnwFreshTacticalMap::initialize_from_game_plot_source(std::int32_t count) {
    require_current();impl_->storage.initialize_detached(count,*impl_->db);require_current();
}
BnwTacticalMapSnapshot BnwFreshTacticalMap::observe() const {
    require_current();return impl_->storage.observe();
}
BnwTacticalCellSnapshot BnwFreshTacticalMap::cell(std::int32_t index) const {
    require_current();return impl_->storage.cell(index);
}
} // namespace civ5::runtime
