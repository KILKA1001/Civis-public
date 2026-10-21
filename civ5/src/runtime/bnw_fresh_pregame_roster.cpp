#include "civ5/runtime/bnw_fresh_pregame_roster.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <thread>
#include <utility>
#include "civ5/runtime/bnw_creation_config.hpp"
#include "civ5/runtime/bnw_fresh_pregame_speed.hpp"

namespace civ5::runtime {
namespace {
std::optional<std::int32_t> selected_constant(
    std::span<const content::GameplayDefine> defines,
    std::string_view name) {
    std::optional<std::int32_t> result;
    for (const auto &define : defines) {
        if (define.name != name) continue;
        if (result || define.value < 0 ||
            define.value > std::numeric_limits<std::int32_t>::max())
            return std::nullopt;
        result = static_cast<std::int32_t>(define.value);
    }
    return result;
}
void block(BnwFreshPregameRosterDraft &result,
           BnwFreshRosterBlockerCode code, std::string message) {
    result.blockers.push_back({code, std::move(message)});
}
} // namespace

bool BnwFreshPregameRosterDraft::has_blocker(
    BnwFreshRosterBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshPregameRosterDraft::rollback() noexcept {
    slots.clear();
    teams_reset.clear();
    source_order.clear();
    slot_prefix_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshPregameRosterDraft draft_bnw_fresh_pregame_roster(
    const BnwFreshGameContentResult &selected,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants,
    std::span<const BnwFreshPregameSlot> pregame_slots,
    std::int32_t selected_minor_count,
    std::int32_t observer_team) {
    BnwFreshPregameRosterDraft result;
    if (!selected.content_ready() || !selected.activation ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        selected_rules_sha256.empty()) {
        block(result, BnwFreshRosterBlockerCode::selected_content_unverified,
              "CvGame::InitPlayers requires activated selected BNW rules");
        return result;
    }
    result.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    if (result.selected_rules_sha256 != selected_rules_sha256) {
        block(result, BnwFreshRosterBlockerCode::selected_content_unverified,
              "Pregame roster and map must use the same selected rules");
        return result;
    }
    const auto max_teams = selected_constant(native_constants, "MAX_TEAMS");
    const auto max_players = selected_constant(native_constants, "MAX_PLAYERS");
    const auto max_civ_players = selected_constant(native_constants, "MAX_CIV_PLAYERS");
    const auto max_major_civs = selected_constant(native_constants, "MAX_MAJOR_CIVS");
    const auto barbarian_player = selected_constant(native_constants, "BARBARIAN_PLAYER");
    const auto barbarian_team = selected_constant(native_constants, "BARBARIAN_TEAM");
    if (!max_teams || !max_players || !max_civ_players || !max_major_civs ||
        !barbarian_player || !barbarian_team || *max_teams <= 0 ||
        *max_players <= 0 || *max_players > 4096 ||
        *max_teams < *max_players || *max_teams > 4096 ||
        *max_major_civs <= 0 || *max_major_civs > *max_civ_players ||
        *max_civ_players + 1 != *max_players ||
        *barbarian_player != *max_civ_players ||
        *barbarian_team >= *max_teams ||
        (observer_team >= 0 && observer_team >= *max_teams)) {
        block(result, BnwFreshRosterBlockerCode::native_constants_unverified,
              "Selected Expansion2 MAX_* and barbarian native constants disagree");
        return result;
    }
    if (pregame_slots.size() != static_cast<std::size_t>(*max_players) ||
        selected_minor_count < 0 ||
        selected_minor_count > *max_civ_players - *max_major_civs) {
        block(result, BnwFreshRosterBlockerCode::pregame_slots_incomplete,
              "Complete CvPreGame slots and resolved minor count are required");
        return result;
    }
    for (std::size_t i = 0; i < pregame_slots.size(); ++i) {
        const auto &slot = pregame_slots[i];
        if (slot.status > BnwFreshSlotStatus::observer ||
            (static_cast<std::int32_t>(i) != *barbarian_player &&
             (slot.team_id < 0 || slot.team_id >= *max_teams)) ||
            (static_cast<std::int32_t>(i) < *max_major_civs &&
             slot.status == BnwFreshSlotStatus::observer && observer_team < 0)) {
            block(result, BnwFreshRosterBlockerCode::pregame_slots_incomplete,
                  "CvPreGame slot status, team or observer assignment is unresolved");
            return result;
        }
    }

    result.slots.assign(pregame_slots.begin(), pregame_slots.end());
    result.teams_reset.assign(*max_teams, false);
    // CvGame::InitPlayers initializes every team before the major color pass.
    for (std::int32_t team = 0; team < *max_teams; ++team)
        result.source_order.push_back({BnwFreshRosterEventKind::team_reset, team});
    result.source_order.push_back({BnwFreshRosterEventKind::major_color_pass_pending, -1});
    // This writes only the proven CvPreGame slot/team fields, after the
    // pending color pass. Remaining Info writes and CvPlayerAI::init are not
    // published from the detached draft.
    for (std::int32_t player = 0; player < *max_players; ++player) {
        auto &slot = result.slots[static_cast<std::size_t>(player)];
        if (player == *barbarian_player) {
            slot.team_id = *barbarian_team;
            result.source_order.push_back({BnwFreshRosterEventKind::barbarian_team,
                                           player});
            slot.status = BnwFreshSlotStatus::computer;
            result.source_order.push_back({BnwFreshRosterEventKind::barbarian_computer_slot,
                                           player});
        } else if (player < *max_major_civs) {
            if (slot.status == BnwFreshSlotStatus::observer) {
                slot.team_id = observer_team;
                result.source_order.push_back({BnwFreshRosterEventKind::observer_team,
                                               player});
            }
        } else if (player < *max_major_civs + selected_minor_count) {
            slot.status = BnwFreshSlotStatus::computer;
            result.source_order.push_back({BnwFreshRosterEventKind::minor_computer_slot,
                                           player});
        }
    }
    result.slot_prefix_complete = true;
    block(result, BnwFreshRosterBlockerCode::major_colors_pending,
          "Major player color selection precedes these slot writes in CvGame::InitPlayers");
    block(result, BnwFreshRosterBlockerCode::selected_barbarian_minor_info_pending,
          "Selected barbarian and minor civilization, leader, handicap and color writes remain");
    block(result, BnwFreshRosterBlockerCode::player_ai_init_pending,
          "CvPlayerAI::init/reset/setAlive and their league, minor-AI, turn and DLL effects remain");
    return result;
}

namespace {
using Write = BnwFreshPregameRosterWrite;
void set_civilization_random(BnwFreshNativePregameSlot& s,std::vector<Write>& writes) noexcept {
    s.civilization=-1; writes.push_back(Write::civilization);
    // bindCivilizationKeys(NO_CIVILIZATION) fails without SQL/Steam.
    s.civilization_key.clear(); writes.push_back(Write::civilization_key);
    s.civilization_package.fill(0); writes.push_back(Write::civilization_package);
    s.civilization_available=false; writes.push_back(Write::civilization_available);
    s.civilization_playable=false; writes.push_back(Write::civilization_playable);
    // setCivilization failure fallback.
    s.civilization=-1; writes.push_back(Write::civilization);
    s.civilization_key.clear(); writes.push_back(Write::civilization_key);
    s.civilization_package.fill(0); writes.push_back(Write::civilization_package);
    s.civilization_available=true; writes.push_back(Write::civilization_available);
    s.civilization_playable=true; writes.push_back(Write::civilization_playable);
}
void set_handicap(BnwFreshNativePregameSlot& s,std::int32_t value,std::vector<Write>& writes) noexcept {
    s.handicap=value;writes.push_back(Write::handicap);
    if(s.status==static_cast<std::int32_t>(BnwFreshSlotStatus::taken)) {
        s.last_human_handicap=value;writes.push_back(Write::last_human_handicap);
    }
}
// resetPlayer(0) before its FIRST cached Globals handicap getter. All string
// arguments in this admitted path are empty; GUID Clear is native memset0.
// Reserve journal capacity before effects; every source assignment is recorded,
// including duplicate sentinel/key/GUID writes inside setter fallback.
void reset_player_to_handicap(BnwFreshNativePregameSlot& s, std::int32_t player,
                              std::vector<Write>& writes) noexcept {
    s.leader_name.clear(); writes.push_back(Write::leader_name);
    s.civilization_description.clear(); writes.push_back(Write::civilization_description);
    s.civilization_short_description.clear(); writes.push_back(Write::civilization_short_description);
    s.civilization_adjective.clear(); writes.push_back(Write::civilization_adjective);
    s.civilization_password.clear(); writes.push_back(Write::civilization_password);
    s.email.clear(); writes.push_back(Write::email);
    s.white_flag=false; writes.push_back(Write::white_flag);
    set_civilization_random(s,writes);
    s.leader=-1; writes.push_back(Write::leader);
    // bindLeaderKeys(NO_LEADER), then its distinct setter fallback.
    s.leader_key.clear(); writes.push_back(Write::leader_key);
    s.leader_package.fill(0); writes.push_back(Write::leader_package);
    s.leader_available=false; writes.push_back(Write::leader_available);
    s.leader=-1; writes.push_back(Write::leader);
    s.leader_available=true; writes.push_back(Write::leader_available);
    s.minor_civ=-1; writes.push_back(Write::minor_civ);
    s.team=player; writes.push_back(Write::team);
}
void reset_player_after_handicap(BnwFreshNativePregameSlot& s,
                                 std::int32_t cached_handicap,
                                 std::vector<Write>& writes) {
    // Native setHandicap: this conditional precedes resetPlayer's explicit
    // setLastHumanHandicap(NO_HANDICAP), even when the later write overwrites it.
    set_handicap(s,cached_handicap,writes);
    s.last_human_handicap=-1;writes.push_back(Write::last_human_handicap);
    s.player_color=-1;writes.push_back(Write::player_color);
    s.art_style=-1;writes.push_back(Write::art_style);
    // Native setSlotStatus is a direct field store; no VerifyHandicap call.
    s.status=static_cast<std::int32_t>(BnwFreshSlotStatus::closed);writes.push_back(Write::status);
    s.claim=0;writes.push_back(Write::claim);
    s.playable=false;writes.push_back(Write::playable);
    s.minor=false;writes.push_back(Write::minor);
    s.net_id=-1;writes.push_back(Write::net_id);
    s.ready=false;writes.push_back(Write::ready);
    // setNickname(empty) inserts into the native display map BEFORE nickname.
    s.display_nickname.emplace();writes.push_back(Write::display_nickname);
    s.nickname.clear();writes.push_back(Write::nickname);
    s.available_dlc.clear();writes.push_back(Write::available_dlc);
}
[[noreturn]] void roster_refuse(BnwFreshPregameRosterError code,
                               const char* message) {
    throw BnwFreshPregameRosterRefusal(code,message);
}
} // namespace

BnwFreshPregameRosterRefusal::BnwFreshPregameRosterRefusal(
    BnwFreshPregameRosterError code, const std::string& message)
    : std::runtime_error(message), code_(code) {}

namespace detail {
std::array<BnwFreshNativePregameSlot,64> bnw_pregame_namespace_slots() {
    // CvDLLUtilDefines.h: MAX_CIV_PLAYERS63 +1, not REALLY_MAX_PLAYERS80.
    // CvPreGame.cpp vector(size) and GUID(size) value-initialize all elements.
    return {};
}
BnwFreshPregamePlayerPrefixQuote quote_bnw_pregame_reset_player_to_handicap(
    BnwFreshNativePregameSlot slot, std::int32_t player) {
    if(player<0 || player>=64)
        roster_refuse(BnwFreshPregameRosterError::native_source_unavailable,
                      "Detached Pregame player prefix index outside native64");
    BnwFreshPregamePlayerPrefixQuote q{std::move(slot),{}};
    q.writes.reserve(25);
    reset_player_to_handicap(q.slot,player,q.writes);
    return q;
}
BnwFreshPregamePlayerPrefixQuote quote_bnw_pregame_reset_player_after_handicap(
    BnwFreshNativePregameSlot slot, std::int32_t cached_handicap) {
    BnwFreshPregamePlayerPrefixQuote q{std::move(slot),{}};
    q.writes.reserve(14);
    reset_player_after_handicap(q.slot,cached_handicap,q.writes);
    return q;
}
} // namespace detail

struct BnwFreshPregameRoster::Impl {
    std::shared_ptr<const BnwCreationConfig> config;
    std::weak_ptr<BnwFreshPregameSpeed> pregame;
    const BnwFreshPregameSpeed* pregame_identity;
    std::thread::id thread;
    // Actual private namespace cache subset. No public roster World setter or
    // imported/ConfigWorld assignment; only the original PG issues row caches.
    BnwFreshPregameRosterObservation state;
};
BnwFreshPregameRoster::BnwFreshPregameRoster(
    std::shared_ptr<const BnwCreationConfig> config,
    const BnwFreshPregameSpeed& pg) {
    if(!config)
        roster_refuse(BnwFreshPregameRosterError::parent_missing,
                      "Pregame roster requires actual retained Config");
    pg.require_current();
    config->require_current();
    if(pg.actual_config()!=config.get())
        roster_refuse(BnwFreshPregameRosterError::foreign_pregame,
                      "Pregame roster requires SAME actual Config allocation");
    auto held=std::make_shared<Impl>();
    held->config=std::move(config);
    held->pregame=pg.current_parent_for_roster();
    if(held->pregame.lock().get()!=&pg)
        roster_refuse(BnwFreshPregameRosterError::foreign_pregame,
                      "Pregame roster requires the actual registered facade");
    held->pregame_identity=&pg;
    held->thread=std::this_thread::get_id();
    // CvWorldInfo ctor DefaultPlayers0 + CvBaseInfo ctor ID-1, with actual
    // CvPreGame namespace s_worldSize=NO_WORLDSIZE(-1). No DB query occurs.
    held->state={BnwFreshPregameRosterBoundary::namespace_constructor,
                 false,-1,-1,0,detail::bnw_pregame_namespace_slots(),{},{},{},{},0};
    held->state.world_numbers={{"ID",-1},{"DefaultPlayers",0},{"DefaultMinorCivs",0},
        {"FogTilesPerBarbarianCamp",0},{"NumNaturalWonders",0},{"UnitNameModifier",0},
        {"TargetNumCities",0},{"NumFreeBuildingResources",0},{"BuildingClassPrereqModifier",0},
        {"MaxConscriptModifier",0},{"GridWidth",0},{"GridHeight",0},{"MaxActiveReligions",0},
        {"TerrainGrainChange",0},{"FeatureGrainChange",0},{"ResearchPercent",0},
        {"AdvancedStartPointsMod",0},{"NumCitiesUnhappinessPercent",100},
        {"NumCitiesPolicyCostMod",10},{"NumCitiesTechCostMod",5},{"EstimatedNumCities",0}};
    held->state.world_text={{"Type",""},{"Text",""},{"Civilopedia",""},{"Strategy",""},
        {"Help",""},{"DisabledHelp",""},{"LocalizedText",""},{"DescriptionKey",""},{"Description",""}};
    impl_=std::move(held);
    // The caller registers its roster pointer only AFTER this constructor
    // returns. Do not require_same_roster here before that legitimate binding.
    pg.require_current();
    impl_->config->require_current();
}
void BnwFreshPregameRoster::require_current() const {
    if(!impl_ || !impl_->config)
        roster_refuse(BnwFreshPregameRosterError::parent_missing,
                      "Pregame roster lost actual Config");
    if(std::this_thread::get_id()!=impl_->thread)
        roster_refuse(BnwFreshPregameRosterError::wrong_thread,
                      "Pregame roster requires its owner thread");
    auto pg=impl_->pregame.lock();
    if(!pg || pg.get()!=impl_->pregame_identity)
        roster_refuse(BnwFreshPregameRosterError::parent_missing,
                      "Pregame roster lost the actual original facade");
    // Closed metadata-only reciprocal guard. It checks current Impl/registered
    // roster identity BEFORE Config queries and does not re-enter this method.
    pg->require_same_roster(*this);
    impl_->config->require_current();
}
BnwFreshPregameRosterObservation BnwFreshPregameRoster::observe() const {
    require_current();
    auto q=impl_->state;
    require_current();
    return q;
}
void BnwFreshPregameRoster::reset_constructor_slots_from_pregame(
    const BnwFreshPregameSpeed& pg) {
    require_current();
    if(&pg!=impl_->pregame_identity)
        roster_refuse(BnwFreshPregameRosterError::foreign_pregame,
                      "ResetSlots requires the issuing actual Pregame facade");
    pg.require_current();
    if(pg.actual_config()!=impl_->config.get())
        roster_refuse(BnwFreshPregameRosterError::foreign_pregame,
                      "ResetSlots lost SAME actual Config allocation");
    auto& s=impl_->state;
    if(s.reset_attempted || s.boundary!=BnwFreshPregameRosterBoundary::namespace_constructor)
        roster_refuse(BnwFreshPregameRosterError::wrong_phase,
                      "ResetSlots partial native effects cannot replay");
    // worldInfo getter checks the real privately held cache ID FIRST.
    if(s.world_size!=s.world_info_id)
        roster_refuse(BnwFreshPregameRosterError::native_source_unavailable,
                      "ResetSlots requires actual WorldInfo SelectAt/cache producer");
    const std::int32_t suggested_players=s.world_default_players;
    if(suggested_players!=0)
        roster_refuse(BnwFreshPregameRosterError::native_source_unavailable,
                      "ResetSlots requires reached admitted-slot/handicap owners");
    s.writes.reserve(63*39+22); // all native stores, including TAKEN setter side effect
    pg.require_current();require_current();
    s.reset_attempted=true;s.boundary=BnwFreshPregameRosterBoundary::failed_prefix;
    // MAX_CIV_PLAYERS63; original namespace barbarian63 is not reset here.
    for(std::int32_t player=0;player<63;++player) {
        // Each native loop iteration reaches the actual current mode getter.
        if(pg.current_game_type_for_roster()!=0)
            roster_refuse(BnwFreshPregameRosterError::native_source_unavailable,
                          "Constructor ResetSlots supports SAME single-player only");
        auto& slot=s.slots[static_cast<std::size_t>(player)];
        reset_player_to_handicap(slot,player,s.writes);
        // resetPlayer repeats its actual mode predicate AFTER its team store.
        if(pg.current_game_type_for_roster()!=0)
            roster_refuse(BnwFreshPregameRosterError::native_source_unavailable,
                          "ResetPlayer current network branch requires a separate issued producer");
        const auto handicap=pg.current_cached_standard_handicap_for_roster();
        reset_player_after_handicap(slot,handicap,s.writes);
        if(player>=suggested_players && player<22) {
            slot.status=static_cast<std::int32_t>(BnwFreshSlotStatus::observer);
            s.writes.push_back(Write::status);
        }
        require_current();
    }
    require_current();s.boundary=BnwFreshPregameRosterBoundary::constructor_single_player_slots_reset;
}

void BnwFreshPregameRoster::cache_world_from_selected_row(
    const BnwFreshPregameSpeed& pg,const content::RulesRowView& row) {
    require_current();pg.require_same_roster(*this);
    auto& s=impl_->state;
    s.world_order.reserve(s.world_order.size()+40);
    const auto number=[&](const char* key) {
        require_current();const auto value=row.integer(key);
        if(value<std::numeric_limits<std::int32_t>::min() || value>std::numeric_limits<std::int32_t>::max())
            roster_refuse(BnwFreshPregameRosterError::native_source_unavailable,"WorldInfo reached GetInt exceeds native i32");
        s.world_numbers.at(key)=static_cast<std::int32_t>(value);
        if(std::string_view(key)=="ID")s.world_info_id=static_cast<std::int32_t>(value);
        if(std::string_view(key)=="DefaultPlayers")s.world_default_players=static_cast<std::int32_t>(value);
        s.world_order.emplace_back(key);require_current();
    };
    const auto text=[&](const char* key) {
        require_current();s.world_text.at(key)=row.text(key).value_or("");
        s.world_order.emplace_back(key);require_current();
    };
    // CvBaseInfo::CacheResults BEFORE every CvWorldInfo scalar. NULL CvString
    // assignment clears; reached localized lookup can fail after these stores.
    number("ID");text("Type");text("Text");text("Civilopedia");
    text("Strategy");text("Help");text("DisabledHelp");
    if(!s.world_text.at("Text").empty()) {
        s.world_text.at("LocalizedText")=pg.current_world_localized_for_roster(s.world_text.at("Text"));
        s.world_order.emplace_back("LocalizedText");
    }
    require_current();const auto description=row.text("Description");
    if(description) {
        s.world_text.at("DescriptionKey")=*description;s.world_order.emplace_back("DescriptionKey");
        s.world_text.at("Description")=pg.current_world_localized_for_roster(*description);
        s.world_order.emplace_back("Description");
        if(s.world_text.at("Text").empty()) {
            s.world_text.at("Text")=*description;s.world_order.emplace_back("Text <- DescriptionKey");
            s.world_text.at("LocalizedText")=s.world_text.at("Description");
            s.world_order.emplace_back("LocalizedText <- Description");
        }
    }
    for(const char* key:{"DefaultPlayers","DefaultMinorCivs","FogTilesPerBarbarianCamp",
        "NumNaturalWonders","UnitNameModifier","TargetNumCities","NumFreeBuildingResources",
        "BuildingClassPrereqModifier","MaxConscriptModifier","GridWidth","GridHeight",
        "MaxActiveReligions","TerrainGrainChange","FeatureGrainChange","ResearchPercent",
        "AdvancedStartPointsMod","NumCitiesUnhappinessPercent","NumCitiesPolicyCostMod",
        "NumCitiesTechCostMod","EstimatedNumCities"})number(key);
    require_current();
}
void BnwFreshPregameRoster::publish_world_size_from_cache(const BnwFreshPregameSpeed& pg) {
    require_current();pg.require_same_roster(*this);
    impl_->state.world_size=impl_->state.world_info_id;
    impl_->state.world_order.emplace_back("s_worldSize <- cache.GetID");
    require_current();
}
void BnwFreshPregameRoster::reset_selected_world_slots_from_pregame(const BnwFreshPregameSpeed& pg) {
    require_current();pg.require_same_roster(*this);
    auto& s=impl_->state;const auto operation=pg.current_world_reset_operation_for_roster();
    if(operation!=s.selected_world_reset_calls+1 || s.world_size!=s.world_info_id)
        roster_refuse(BnwFreshPregameRosterError::wrong_phase,"World ResetSlots requires exact new actual cache operation");
    // The real getter's IDs already agree: no SelectAt can be inserted here.
    const auto suggested=s.world_default_players;std::int32_t assigned=0;
    s.writes.reserve(s.writes.size()+63*39+22);s.world_order.reserve(s.world_order.size()+65);
    s.selected_world_reset_calls=operation;s.boundary=BnwFreshPregameRosterBoundary::failed_prefix;
    s.world_order.emplace_back("ResetSlots getDefaultPlayers");
    for(std::int32_t player=0;player<63;++player) {
        if(pg.current_game_type_for_roster()!=0)
            roster_refuse(BnwFreshPregameRosterError::native_source_unavailable,"World ResetSlots reached network/hotseat source requires actual separate mode owner");
        auto& slot=s.slots[static_cast<std::size_t>(player)];
        s.world_order.emplace_back("ResetSlots player "+std::to_string(player));
        if(assigned<suggested) {
            const auto status=slot.status;
            if(status!=static_cast<std::int32_t>(BnwFreshSlotStatus::taken) &&
               status!=static_cast<std::int32_t>(BnwFreshSlotStatus::computer)) {
                slot.status=static_cast<std::int32_t>(BnwFreshSlotStatus::computer);s.writes.push_back(Write::status);
                slot.claim=2;s.writes.push_back(Write::claim); // SLOTCLAIM_ASSIGNED2 (RESERVED1)
                set_civilization_random(slot,s.writes);
            }
            if(slot.status==static_cast<std::int32_t>(BnwFreshSlotStatus::computer) && player!=0)
                set_handicap(slot,pg.current_cached_ai_handicap_for_roster(),s.writes);
            else if(pg.current_game_type_for_roster()==2)
                roster_refuse(BnwFreshPregameRosterError::native_source_unavailable,"World ResetSlots reached hotseat handicap owner unavailable");
            // Native SP branch intentionally leaves player0 handicap unchanged.
            ++assigned;
        } else {
            reset_player_to_handicap(slot,player,s.writes);
            if(pg.current_game_type_for_roster()!=0)
                roster_refuse(BnwFreshPregameRosterError::native_source_unavailable,"World resetPlayer reached network handicap unavailable");
            reset_player_after_handicap(slot,pg.current_cached_standard_handicap_for_roster(),s.writes);
            if(player>=suggested && player<22) {
                slot.status=static_cast<std::int32_t>(BnwFreshSlotStatus::observer);s.writes.push_back(Write::status);
            }
        }
        require_current();
    }
    require_current();s.boundary=BnwFreshPregameRosterBoundary::selected_world_single_player_slots_reset;
}
} // namespace civ5::runtime
