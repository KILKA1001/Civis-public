#pragma once

#include "civ5/runtime/bnw_fresh_game_content.hpp"
#include <cstdint>
#include <array>
#include <memory>
#include <optional>
#include <stdexcept>
#include <span>
#include <string>
#include <vector>
#include <map>

namespace civ5::content { class RulesRowView; }
namespace civ5::runtime {

enum class BnwFreshSlotStatus : std::uint8_t {
    open, computer, closed, taken, observer,
};
struct BnwFreshPregameSlot {
    BnwFreshSlotStatus status = BnwFreshSlotStatus::closed;
    std::int32_t team_id = -1;
    bool operator==(const BnwFreshPregameSlot &) const = default;
};
enum class BnwFreshRosterEventKind : std::uint8_t {
    team_reset, major_color_pass_pending, barbarian_team,
    barbarian_computer_slot, observer_team, minor_computer_slot,
};
struct BnwFreshRosterEvent {
    BnwFreshRosterEventKind kind;
    std::int32_t index;
    bool operator==(const BnwFreshRosterEvent &) const = default;
};
enum class BnwFreshRosterBlockerCode {
    selected_content_unverified,
    native_constants_unverified,
    pregame_slots_incomplete,
    major_colors_pending,
    selected_barbarian_minor_info_pending,
    player_ai_init_pending,
};
struct BnwFreshRosterBlocker {
    BnwFreshRosterBlockerCode code;
    std::string message;
};
struct BnwFreshPregameRosterDraft {
    std::vector<BnwFreshPregameSlot> slots;
    std::vector<bool> teams_reset;
    std::vector<BnwFreshRosterEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshRosterBlocker> blockers;
    bool slot_prefix_complete = false;
    bool initialized_by_gamecore = false; // remains false until CvPlayerAI::init
    bool has_blocker(BnwFreshRosterBlockerCode code) const;
    void rollback() noexcept;
};

// Detached CvGame::InitPlayers prefix: CvTeam::init for every native team,
// then the source-order observer/barbarian/minor slot writes. Colors and the
// remaining Info writes are explicitly pending before CvPlayerAI::init.
BnwFreshPregameRosterDraft draft_bnw_fresh_pregame_roster(
    const BnwFreshGameContentResult &selected,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants,
    std::span<const BnwFreshPregameSlot> pregame_slots,
    std::int32_t selected_minor_count,
    std::int32_t observer_team = -1);


// Native namespace / ResetSlots storage. This does not consume the detached
// InitPlayers Draft above and has no public issuer or mutation endpoint.
class BnwCreationConfig;
class BnwFreshPregameSpeed;
enum class BnwFreshPregameRosterBoundary {
    namespace_constructor, standard_handicap_required, failed_prefix,
    constructor_single_player_slots_reset, selected_world_single_player_slots_reset
};
enum class BnwFreshPregameRosterError {
    parent_missing, wrong_thread, foreign_pregame, wrong_phase,
    native_source_unavailable
};
class BnwFreshPregameRosterRefusal : public std::runtime_error {
 public:
    BnwFreshPregameRosterRefusal(BnwFreshPregameRosterError, const std::string&);
    BnwFreshPregameRosterError code() const noexcept { return code_; }
 private:
    BnwFreshPregameRosterError code_;
};
// Value-initialized native vectors use 0, including enum fields. These copied
// observations grant no ResetSlots/InitPlayers or startup authority.
struct BnwFreshNativePregameSlot {
    std::int32_t civilization, leader, minor_civ, team, handicap,
        last_human_handicap, player_color, art_style, status, claim, net_id;
    std::string leader_name, civilization_description,
        civilization_short_description, civilization_adjective,
        civilization_password, email, civilization_key, leader_key,
        nickname;
    // Native s_displayNicknames map has NO entry on namespace construction.
    std::optional<std::string> display_nickname;
    std::array<std::uint8_t,16> civilization_package, leader_package;
    bool white_flag, civilization_available, civilization_playable,
        leader_available, playable, minor, ready;
    std::vector<std::array<std::uint8_t,16>> available_dlc;
    bool operator==(const BnwFreshNativePregameSlot&) const = default;
};
enum class BnwFreshPregameRosterWrite {
    leader_name, civilization_description, civilization_short_description,
    civilization_adjective, civilization_password, email, white_flag,
    civilization, civilization_key, civilization_package,
    civilization_available, civilization_playable, leader, leader_key,
    leader_package, leader_available, minor_civ, team, handicap,
    last_human_handicap, player_color, art_style, status, claim,
    playable, minor, net_id, ready, display_nickname, nickname, available_dlc
};
struct BnwFreshPregameRosterObservation {
    BnwFreshPregameRosterBoundary boundary;
    bool reset_attempted;
    std::int32_t world_size, world_info_id, world_default_players;
    std::array<BnwFreshNativePregameSlot,64> slots;
    std::vector<BnwFreshPregameRosterWrite> writes;
    // Complete owned native WorldInfo cache, with ordered reached stores.
    // These copied observations never issue a world setter or slot reset.
    std::map<std::string,std::int32_t,std::less<>> world_numbers;
    std::map<std::string,std::string,std::less<>> world_text;
    std::vector<std::string> world_order;
    std::uint64_t selected_world_reset_calls;
};
namespace detail {
// Detached source diagnostics only. SAME internal source leaf is used by the
// privately issued owner. Neither result can construct or reset that owner.
std::array<BnwFreshNativePregameSlot,64> bnw_pregame_namespace_slots();
struct BnwFreshPregamePlayerPrefixQuote {
    BnwFreshNativePregameSlot slot;
    std::vector<BnwFreshPregameRosterWrite> writes;
};
BnwFreshPregamePlayerPrefixQuote quote_bnw_pregame_reset_player_to_handicap(
    BnwFreshNativePregameSlot, std::int32_t player);
// Detached setter diagnostics use the SAME leaf as the genuine source owner.
// A supplied handicap here never authorizes that owner's getter or stores.
BnwFreshPregamePlayerPrefixQuote quote_bnw_pregame_reset_player_after_handicap(
    BnwFreshNativePregameSlot, std::int32_t selected_cached_handicap);
} // namespace detail
class BnwFreshPregameRoster {
 public:
    BnwFreshPregameRoster(const BnwFreshPregameRoster&) = delete;
    BnwFreshPregameRoster& operator=(const BnwFreshPregameRoster&) = delete;
    void require_current() const;
    BnwFreshPregameRosterObservation observe() const;
 private:
    friend class BnwFreshPregameSpeed;
    explicit BnwFreshPregameRoster(std::shared_ptr<const BnwCreationConfig>,
                                  const BnwFreshPregameSpeed&);
    void reset_constructor_slots_from_pregame(const BnwFreshPregameSpeed&);
    void cache_world_from_selected_row(const BnwFreshPregameSpeed&,
                                      const content::RulesRowView&);
    void publish_world_size_from_cache(const BnwFreshPregameSpeed&);
    void reset_selected_world_slots_from_pregame(const BnwFreshPregameSpeed&);
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

} // namespace civ5::runtime
