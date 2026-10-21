#include "civ5/runtime/bnw_fresh_slot_info.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
std::optional<std::int32_t> native_id(
    std::span<const content::GameplayDefine> values, std::string_view key) {
    std::optional<std::int32_t> found;
    for (const auto &value : values) {
        if (value.name != key) continue;
        if (found || value.value < 0 ||
            value.value > std::numeric_limits<std::int32_t>::max())
            return std::nullopt;
        found = static_cast<std::int32_t>(value.value);
    }
    return found;
}
bool has_id(const content::RulesDatabase &rules, const char *table,
            std::int32_t id) {
    if (id < 0) return false;
    const auto report = rules.query_rows(
        {std::string("SELECT ID FROM ") + table + " WHERE ID = ?",
         {static_cast<std::int64_t>(id)}},
        [](const content::RulesRowView &) {}, 2);
    return report.all_succeeded() && report.rows == 1;
}
std::optional<std::int32_t> minor_color(const content::RulesDatabase &rules,
                                        std::int32_t minor_id) {
    if (minor_id < 0) return std::nullopt;
    std::optional<std::string> type;
    const auto info = rules.query_rows(
        {"SELECT DefaultPlayerColor FROM MinorCivilizations WHERE ID = ?",
         {static_cast<std::int64_t>(minor_id)}},
        [&](const content::RulesRowView &row) {
            type = row.text("DefaultPlayerColor");
        }, 2);
    if (!info.all_succeeded() || info.rows != 1 || !type || type->empty())
        return std::nullopt;
    std::optional<std::int32_t> color;
    const auto match = rules.query_rows(
        {"SELECT ID FROM PlayerColors WHERE Type = ?", {*type}},
        [&](const content::RulesRowView &row) {
            const auto id = row.integer("ID");
            if (id < 0 || id > std::numeric_limits<std::int32_t>::max())
                throw std::invalid_argument("Minor player color ID is invalid");
            color = static_cast<std::int32_t>(id);
        }, 2);
    if (!match.all_succeeded() || match.rows != 1) return std::nullopt;
    return color;
}
void write(BnwFreshSlotInfoDraft &draft, std::int32_t player,
           BnwFreshSlotWriteField field, std::int32_t value) {
    auto &slot = draft.slots[static_cast<std::size_t>(player)];
    switch (field) {
    case BnwFreshSlotWriteField::team: slot.slot.team_id = value; break;
    case BnwFreshSlotWriteField::status:
        slot.slot.status = static_cast<BnwFreshSlotStatus>(value); break;
    case BnwFreshSlotWriteField::net_id: slot.net_id = value; break;
    case BnwFreshSlotWriteField::handicap: slot.handicap = value; break;
    case BnwFreshSlotWriteField::civilization: slot.civilization = value; break;
    case BnwFreshSlotWriteField::leader: slot.leader = value; break;
    case BnwFreshSlotWriteField::player_color: slot.player_color = value; break;
    case BnwFreshSlotWriteField::minor_civ: slot.minor_civ = value != 0; break;
    }
    draft.source_order.push_back({player,field,value});
}
void block(BnwFreshSlotInfoDraft &draft, BnwFreshSlotInfoBlockerCode code,
           std::string message) {
    draft.rollback();
    draft.blockers.push_back({code,std::move(message)});
}
} // namespace

bool BnwFreshSlotInfoDraft::has_blocker(BnwFreshSlotInfoBlockerCode code) const {
    return std::any_of(blockers.begin(),blockers.end(),
                       [code](const auto &value) { return value.code == code; });
}
void BnwFreshSlotInfoDraft::rollback() noexcept {
    slots.clear();
    source_order.clear();
    selected_rules_sha256.clear();
    blockers.clear();
    pregame_writes_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshSlotInfoDraft draft_bnw_fresh_slot_info(
    const BnwFreshGameContentResult &selected,
    const BnwFreshPregameRosterDraft &roster,
    const BnwFreshPlayerColorsDraft &colors,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants,
    std::span<const std::int32_t> active_minor_civ_ids) {
    BnwFreshSlotInfoDraft draft;
    if (!selected.content_ready() || !selected.rules || !selected.activation ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        !roster.slot_prefix_complete || !colors.major_colors_complete ||
        roster.initialized_by_gamecore || roster.slots.empty() ||
        selected_rules_sha256.empty() ||
        roster.selected_rules_sha256 != selected_rules_sha256 ||
        colors.selected_rules_sha256 != selected_rules_sha256 ||
        identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256 !=
            selected_rules_sha256) {
        block(draft,BnwFreshSlotInfoBlockerCode::roster_colors_or_rules_unverified,
              "Selected roster, player colors and rules must share identity");
        return draft;
    }
    const auto max_major = native_id(native_constants,"MAX_MAJOR_CIVS");
    const auto max_civ = native_id(native_constants,"MAX_CIV_PLAYERS");
    const auto barbarian_player = native_id(native_constants,"BARBARIAN_PLAYER");
    const auto barbarian_team = native_id(native_constants,"BARBARIAN_TEAM");
    const auto barbarian_civ = native_id(native_constants,"BARBARIAN_CIVILIZATION");
    const auto barbarian_handicap = native_id(native_constants,"BARBARIAN_HANDICAP");
    const auto barbarian_leader = native_id(native_constants,"BARBARIAN_LEADER");
    const auto minor_civ = native_id(native_constants,"MINOR_CIVILIZATION");
    const auto minor_handicap = native_id(native_constants,"MINOR_CIV_HANDICAP");
    const auto ai_handicap = native_id(native_constants,"AI_HANDICAP");
    if (!max_major || !max_civ || !barbarian_player || !barbarian_team ||
        !barbarian_civ || !barbarian_handicap || !barbarian_leader ||
        !minor_civ || !minor_handicap || !ai_handicap ||
        *max_major <= 0 || *max_major > *max_civ ||
        *max_civ != *barbarian_player ||
        roster.slots.size() != static_cast<std::size_t>(*max_civ + 1) ||
        colors.major_colors.size() != static_cast<std::size_t>(*max_major) ||
        *barbarian_team >= static_cast<std::int32_t>(roster.teams_reset.size())) {
        block(draft,BnwFreshSlotInfoBlockerCode::selected_gamecore_constants_unverified,
              "Selected Expansion2 cached slot constants are incomplete");
        return draft;
    }
    std::vector<std::int32_t> active_minor_players;
    for (const auto &event : roster.source_order)
        if (event.kind == BnwFreshRosterEventKind::minor_computer_slot)
            active_minor_players.push_back(event.index);
    if (active_minor_players.size() != active_minor_civ_ids.size() ||
        !std::is_sorted(active_minor_players.begin(),active_minor_players.end()) ||
        std::adjacent_find(active_minor_players.begin(),active_minor_players.end()) !=
            active_minor_players.end() ||
        std::any_of(active_minor_players.begin(),active_minor_players.end(),
                    [&](auto player) {
                        return player < *max_major || player >= *max_civ ||
                            roster.slots[static_cast<std::size_t>(player)].status !=
                                BnwFreshSlotStatus::computer;
                    })) {
        block(draft,BnwFreshSlotInfoBlockerCode::minor_pregame_types_missing,
              "Every active minor requires its selected CvPreGame minor-civ ID");
        return draft;
    }
    try {
        if (!has_id(*selected.rules,"Civilizations",*barbarian_civ) ||
            !has_id(*selected.rules,"Leaders",*barbarian_leader) ||
            !has_id(*selected.rules,"HandicapInfos",*barbarian_handicap) ||
            !has_id(*selected.rules,"HandicapInfos",*ai_handicap) ||
            (!active_minor_players.empty() &&
             (!has_id(*selected.rules,"Civilizations",*minor_civ) ||
              !has_id(*selected.rules,"HandicapInfos",*minor_handicap))))
            throw std::invalid_argument("Selected slot Info is incomplete");
        std::vector<std::int32_t> minor_colors;
        minor_colors.reserve(active_minor_civ_ids.size());
        for (auto id : active_minor_civ_ids) {
            const auto color = minor_color(*selected.rules,id);
            if (!color) throw std::invalid_argument("Selected minor color is unresolved");
            minor_colors.push_back(*color);
        }
        draft.slots.reserve(roster.slots.size());
        for (const auto &slot : roster.slots) {
            BnwFreshSlotInfoState state;
            state.slot = slot;
            draft.slots.push_back(std::move(state));
        }
        std::size_t minor_index = 0;
        for (std::int32_t player=0; player<static_cast<std::int32_t>(draft.slots.size()); ++player) {
            if (player == *barbarian_player) {
                write(draft,player,BnwFreshSlotWriteField::team,*barbarian_team);
                write(draft,player,BnwFreshSlotWriteField::status,
                      static_cast<std::int32_t>(BnwFreshSlotStatus::computer));
                write(draft,player,BnwFreshSlotWriteField::net_id,-1);
                write(draft,player,BnwFreshSlotWriteField::handicap,*barbarian_handicap);
                write(draft,player,BnwFreshSlotWriteField::civilization,*barbarian_civ);
                write(draft,player,BnwFreshSlotWriteField::leader,*barbarian_leader);
                write(draft,player,BnwFreshSlotWriteField::player_color,colors.barbarian_color);
                write(draft,player,BnwFreshSlotWriteField::minor_civ,0);
            } else if (player < *max_major) {
                write(draft,player,BnwFreshSlotWriteField::player_color,
                      colors.major_colors[static_cast<std::size_t>(player)]);
                const auto status = roster.slots[static_cast<std::size_t>(player)].status;
                if (status == BnwFreshSlotStatus::computer)
                    write(draft,player,BnwFreshSlotWriteField::handicap,*ai_handicap);
                else if (status == BnwFreshSlotStatus::observer)
                    write(draft,player,BnwFreshSlotWriteField::team,
                          roster.slots[static_cast<std::size_t>(player)].team_id);
            } else if (player < *max_civ &&
                       minor_index < active_minor_players.size() &&
                       active_minor_players[minor_index] == player) {
                write(draft,player,BnwFreshSlotWriteField::status,
                      static_cast<std::int32_t>(BnwFreshSlotStatus::computer));
                write(draft,player,BnwFreshSlotWriteField::net_id,-1);
                write(draft,player,BnwFreshSlotWriteField::handicap,*minor_handicap);
                write(draft,player,BnwFreshSlotWriteField::civilization,*minor_civ);
                write(draft,player,BnwFreshSlotWriteField::leader,*barbarian_leader);
                write(draft,player,BnwFreshSlotWriteField::player_color,
                      minor_colors[minor_index++]);
                write(draft,player,BnwFreshSlotWriteField::minor_civ,1);
            }
        }
        if (minor_index != active_minor_civ_ids.size())
            throw std::invalid_argument("Active minor slot order is inconsistent");
        draft.selected_rules_sha256 = std::string(selected_rules_sha256);
        draft.pregame_writes_complete = true;
        draft.blockers.push_back({BnwFreshSlotInfoBlockerCode::player_init_pending,
                                  "CvPlayerAI::init follows the completed CvPreGame writes"});
    } catch (const std::exception &) {
        block(draft,BnwFreshSlotInfoBlockerCode::selected_info_unverified,
              "Selected CvPreGame cached Info is missing or ambiguous");
    }
    return draft;
}
} // namespace civ5::runtime
