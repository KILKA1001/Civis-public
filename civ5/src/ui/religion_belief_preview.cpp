#include "civ5/ui/religion_belief_preview.hpp"

#include <algorithm>
#include <set>

namespace civ5::ui {
namespace {

bool inside(WorldViewRect inner, WorldViewRect outer) {
    return inner.width > 0 && inner.height > 0 &&
        inner.x >= outer.x && inner.y >= outer.y &&
        std::int64_t(inner.x) + inner.width <=
            std::int64_t(outer.x) + outer.width &&
        std::int64_t(inner.y) + inner.height <=
            std::int64_t(outer.y) + outer.height;
}

bool category_matches_mode(ReligionBeliefCategory category,
                           ChooseReligionMode mode) {
    if (mode == ChooseReligionMode::founding)
        return category == ReligionBeliefCategory::pantheon ||
            category == ReligionBeliefCategory::founder ||
            category == ReligionBeliefCategory::follower ||
            category == ReligionBeliefCategory::bonus;
    return category == ReligionBeliefCategory::follower2 ||
        category == ReligionBeliefCategory::enhancer;
}

ReligionBeliefReadOnlyPreview unavailable(std::string reason) {
    return {false, std::move(reason), std::nullopt, {}, {}};
}

} // namespace

ReligionBeliefReadOnlyPreview capture_religion_belief_preview(
    const UiFrameSnapshot &frame, std::int32_t viewport_width,
    std::int32_t viewport_height) {
    const auto *owner_rows = frame.find<ReligionBeliefOwnerEvidence>();
    const auto *queue = frame.find<PopupFrame>();
    const auto *selected = frame.find<ReligionSelectionView>();
    if (!owner_rows || !queue || !selected)
        return unavailable("Active religion belief owner rows are missing");
    const auto event = std::find_if(queue->events.begin(), queue->events.end(),
        [](const PopupEvent &entry) {
            return entry.context_id == "ChooseReligion";
        });
    if (event == queue->events.end() ||
        !valid_choose_religion_owner(frame, *event,
            owner_rows->selected_source_path) ||
        selected->editing_name ||
        selected->player_id != queue->player_id ||
        selected->turn != queue->turn ||
        selected->owner_sequence != queue->owner_sequence ||
        selected->event_id != event->event_id ||
        selected->selected_source_path != owner_rows->selected_source_path ||
        selected->viewport_width != viewport_width ||
        selected->viewport_height != viewport_height ||
        owner_rows->player_id != queue->player_id ||
        owner_rows->turn != queue->turn ||
        owner_rows->owner_sequence != queue->owner_sequence ||
        owner_rows->activation_sequence == 0 ||
        owner_rows->event_id != event->event_id ||
        owner_rows->religion_id != selected->religion_id ||
        owner_rows->viewport_width != viewport_width ||
        owner_rows->viewport_height != viewport_height ||
        !category_matches_mode(owner_rows->category,
            event->choose_religion->mode) ||
        std::none_of(event->choices.begin(), event->choices.end(),
            [&](const PopupChoice &choice) {
                return choice.id == selected->religion_id &&
                    choice.icon_atlas == "RELIGION_ATLAS_WHITE" &&
                    choice.portrait_index >= 0 && choice.portrait_index < 15;
            }))
        return unavailable("Religion belief rows do not match the active popup owner");
    const WorldViewRect viewport{0, 0, viewport_width, viewport_height};
    if (owner_rows->panel_bounds.width != 450 ||
        owner_rows->panel_bounds.height != 412 ||
        !inside(owner_rows->panel_bounds, viewport))
        return unavailable("BeliefPanel lacks measured 450x412 bounds");
    std::set<std::int64_t> ids;
    std::int64_t previous_bottom = owner_rows->panel_bounds.y;
    for (const auto &row : owner_rows->visible_rows) {
        if (row.belief_id < 0 || row.name.empty() ||
            row.description.empty() || !ids.insert(row.belief_id).second ||
            !inside(row.clipped_bounds, owner_rows->panel_bounds) ||
            row.clipped_bounds.y < previous_bottom)
            return unavailable("Belief rows lack unique localized IDs or measured clips");
        previous_bottom = std::int64_t(row.clipped_bounds.y) +
            row.clipped_bounds.height;
    }
    return {true, {}, owner_rows->category, owner_rows->panel_bounds,
            owner_rows->visible_rows};
}

} // namespace civ5::ui
