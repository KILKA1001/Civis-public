#include "civ5/ui/religion_belief_preview.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
}

int main() {
    try {
        using namespace civ5::ui;
        constexpr auto source =
            "Assets/DLC/Expansion2/UI/InGame/Popups/ChooseReligionPopup.xml";
        PopupEvent event;
        event.context_id = "ChooseReligion";
        event.event_id = 81;
        event.choices = {{3, "Buddhism", {}, true,
                          "RELIGION_ATLAS_WHITE", 2}};
        event.choose_religion = ChooseReligionOwnerEvidence{
            0, 1, 91, 81, 4, 5, ChooseReligionMode::founding, source};
        ReligionSelectionView selected;
        selected.player_id = 0;
        selected.turn = 1;
        selected.owner_sequence = 91;
        selected.event_id = 81;
        selected.religion_id = 3;
        selected.selected_source_path = source;
        selected.viewport_width = 1280;
        selected.viewport_height = 960;
        selected.default_name = selected.name = selected.draft = "Buddhism";
        ReligionBeliefOwnerEvidence rows;
        rows.player_id = 0;
        rows.turn = 1;
        rows.owner_sequence = 91;
        rows.activation_sequence = 5;
        rows.event_id = 81;
        rows.religion_id = 3;
        rows.selected_source_path = source;
        rows.viewport_width = 1280;
        rows.viewport_height = 960;
        rows.category = ReligionBeliefCategory::founder;
        rows.panel_bounds = {647, 358, 450, 412};
        rows.visible_rows = {{11, "Founder A", "Effect A", {647, 358, 450, 72}},
                             {12, "Founder B", "Longer effect B",
                              {647, 432, 450, 95}}};
        const auto frame = UiFrameSnapshot(50, 0, 1)
            .with<PopupFrame>(std::make_shared<const PopupFrame>(
                PopupFrame{0, {event}, 1, 91}))
            .with<ReligionSelectionView>(
                std::make_shared<const ReligionSelectionView>(selected))
            .with<ReligionBeliefOwnerEvidence>(
                std::make_shared<const ReligionBeliefOwnerEvidence>(rows));
        const auto valid = capture_religion_belief_preview(frame, 1280, 960);
        require(valid.resolved && valid.visible_rows.size() == 2 &&
                valid.category == ReligionBeliefCategory::founder &&
                valid.visible_rows[1].clipped_bounds.height == 95,
                "current owner publishes read-only rows at measured variable heights");
        require(!capture_religion_belief_preview(
                    frame.with<ReligionBeliefOwnerEvidence>(
                        std::make_shared<const ReligionBeliefOwnerEvidence>(
                            ReligionBeliefOwnerEvidence{})),
                    1280, 960).resolved,
                "missing owner token cannot publish beliefs");
        auto stale = rows;
        stale.owner_sequence = 92;
        require(!capture_religion_belief_preview(
                    frame.with<ReligionBeliefOwnerEvidence>(
                        std::make_shared<const ReligionBeliefOwnerEvidence>(stale)),
                    1280, 960).resolved,
                "another popup owner cannot reuse belief rows");
        require(!capture_religion_belief_preview(frame, 1920, 1080).resolved,
                "viewport change invalidates measured belief clips");
        selected.religion_id = 4;
        require(!capture_religion_belief_preview(
                    frame.with<ReligionSelectionView>(
                        std::make_shared<const ReligionSelectionView>(selected)),
                    1280, 960).resolved,
                "belief rows cannot attach to an unselected religion symbol");
        selected.religion_id = 3;
        stale = rows;
        stale.category = ReligionBeliefCategory::enhancer;
        require(!capture_religion_belief_preview(
                    frame.with<ReligionBeliefOwnerEvidence>(
                        std::make_shared<const ReligionBeliefOwnerEvidence>(stale)),
                    1280, 960).resolved,
                "founding cannot show enhancement-only belief category");
        stale = rows;
        stale.visible_rows[1].belief_id = 11;
        require(!capture_religion_belief_preview(
                    frame.with<ReligionBeliefOwnerEvidence>(
                        std::make_shared<const ReligionBeliefOwnerEvidence>(stale)),
                    1280, 960).resolved,
                "duplicate live belief IDs are rejected");
        stale = rows;
        stale.visible_rows[1].clipped_bounds.y = 400;
        require(!capture_religion_belief_preview(
                    frame.with<ReligionBeliefOwnerEvidence>(
                        std::make_shared<const ReligionBeliefOwnerEvidence>(stale)),
                    1280, 960).resolved,
                "overlapping dynamic rows are rejected");
        selected.editing_name = true;
        require(!capture_religion_belief_preview(
                    frame.with<ReligionSelectionView>(
                        std::make_shared<const ReligionSelectionView>(selected)),
                    1280, 960).resolved,
                "name dialog takes focus over belief list");
        std::cout << "ui religion belief preview tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
