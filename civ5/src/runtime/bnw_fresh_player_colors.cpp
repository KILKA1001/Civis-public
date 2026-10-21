#include "civ5/runtime/bnw_fresh_player_colors.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
void block(BnwFreshPlayerColorsDraft &result,
           BnwFreshColorsBlockerCode code, std::string message) {
    result.blockers.push_back({code, std::move(message)});
}

std::optional<std::int32_t> selected_default_color(
    const content::RulesDatabase &rules, std::int32_t civilization_id) {
    if (civilization_id < 0) return std::nullopt;
    std::optional<std::string> type;
    const auto civilization = rules.query_rows(
        {"SELECT DefaultPlayerColor FROM Civilizations WHERE ID = ?", {
             static_cast<std::int64_t>(civilization_id)}},
        [&](const content::RulesRowView &row) {
            if (type) throw std::invalid_argument("Ambiguous civilization color");
            type = row.text("DefaultPlayerColor");
        }, 2);
    if (!civilization.all_succeeded() || civilization.rows != 1 ||
        !type || type->empty()) return std::nullopt;
    std::optional<std::int32_t> color;
    const auto selected = rules.query_rows(
        {"SELECT ID FROM PlayerColors WHERE Type = ?", {*type}},
        [&](const content::RulesRowView &row) {
            const auto id = row.integer("ID");
            if (color || id < 0 || id > std::numeric_limits<std::int32_t>::max())
                throw std::invalid_argument("Ambiguous selected player color");
            color = static_cast<std::int32_t>(id);
        }, 2);
    if (!selected.all_succeeded() || selected.rows != 1) return std::nullopt;
    return color;
}
} // namespace

bool BnwFreshPlayerColorsDraft::has_blocker(BnwFreshColorsBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshPlayerColorsDraft::rollback() noexcept {
    major_colors.clear();
    barbarian_color = -1;
    major_colors_complete = false;
}

BnwFreshPlayerColorsDraft draft_bnw_fresh_player_colors(
    const BnwFreshGameContentResult &selected,
    std::string_view selected_rules_sha256,
    std::span<const BnwFreshMajorColorSlot> major_slots,
    std::int32_t barbarian_civilization_id,
    std::uint64_t maximum_player_colors) {
    BnwFreshPlayerColorsDraft result;
    if (!selected.content_ready() || !selected.activation ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        selected_rules_sha256.empty()) {
        block(result, BnwFreshColorsBlockerCode::selected_rules_unverified,
              "Selected BNW GameCore rules are required for initial colors");
        return result;
    }
    result.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    if (result.selected_rules_sha256 != selected_rules_sha256) {
        block(result, BnwFreshColorsBlockerCode::selected_rules_unverified,
              "Player colors and map must use the same selected rules");
        return result;
    }
    if (major_slots.empty() || major_slots.size() > 4096 ||
        barbarian_civilization_id < 0) {
        block(result, BnwFreshColorsBlockerCode::initial_colors_incomplete,
              "Complete major CvPreGame colors and barbarian civilization are required");
        return result;
    }
    try {
        std::int64_t color_count = -1;
        const auto count = selected.rules->query_rows(
            {"SELECT count(*) AS Count FROM PlayerColors", {}},
            [&](const content::RulesRowView &row) {
                color_count = row.integer("Count");
            }, 2);
        if (!count.all_succeeded() || count.rows != 1 || color_count <= 0 ||
            static_cast<std::uint64_t>(color_count) > maximum_player_colors ||
            color_count > std::numeric_limits<std::int32_t>::max())
            throw std::invalid_argument("Selected PlayerColors count is incomplete");
        std::int64_t next_id = 0;
        const auto ids = selected.rules->query_rows(
            {"SELECT ID FROM PlayerColors ORDER BY ID", {}},
            [&](const content::RulesRowView &row) {
                if (row.integer("ID") != next_id++)
                    throw std::invalid_argument("PlayerColors Info IDs have holes");
            }, maximum_player_colors + 1);
        if (!ids.all_succeeded() || ids.rows !=
                static_cast<std::uint64_t>(color_count))
            throw std::invalid_argument("Selected PlayerColors Info array is incomplete");
        const auto barbarian = selected_default_color(
            *selected.rules, barbarian_civilization_id);
        if (!barbarian || *barbarian >= color_count)
            throw std::invalid_argument("Selected barbarian default color is unresolved");
        result.barbarian_color = *barbarian;
        for (const auto &slot : major_slots) {
            if (slot.chosen_color_id < -1 || slot.chosen_color_id >= color_count ||
                slot.status > BnwFreshSlotStatus::observer)
                throw std::invalid_argument("Pregame major color or status is invalid");
            std::int32_t color = slot.chosen_color_id;
            if (color == -1 && (slot.status == BnwFreshSlotStatus::taken ||
                                slot.status == BnwFreshSlotStatus::computer ||
                                slot.status == BnwFreshSlotStatus::observer)) {
                const auto found = selected_default_color(
                    *selected.rules, slot.civilization_id);
                if (!found || *found >= color_count)
                    throw std::invalid_argument("Selected major civilization default color is unresolved");
                color = *found;
            }
            result.major_colors.push_back(color);
        }
        // SDK scans all major slots for each duplicate. It does not break
        // after finding a free replacement: the final available ID wins.
        for (std::size_t i = 0; i < result.major_colors.size(); ++i) {
            if (result.major_colors[i] == -1) continue;
            for (std::size_t j = 0; j < i; ++j) {
                if (result.major_colors[i] != result.major_colors[j]) continue;
                for (std::int32_t candidate = 0; candidate < color_count;
                     ++candidate) {
                    if (candidate == result.barbarian_color) continue;
                    const bool used = std::find(result.major_colors.begin(),
                                                result.major_colors.end(),
                                                candidate) != result.major_colors.end();
                    if (!used) result.major_colors[i] = candidate;
                }
                break;
            }
        }
    } catch (const std::exception &) {
        result.rollback();
        block(result, BnwFreshColorsBlockerCode::selected_color_info_missing,
              "Selected Civilization/PlayerColors Info or initial color is unresolved");
        return result;
    }
    result.major_colors_complete = true;
    block(result, BnwFreshColorsBlockerCode::remaining_init_players_pending,
          "Barbarian/minor Info writes and CvPlayerAI::init remain pending");
    return result;
}
} // namespace civ5::runtime
