#include "civ5/ui/economic_overview_close_service.hpp"

namespace civ5::ui {
namespace {

bool same_owner(const EconomicTabOwner &a, const EconomicTabOwner &b) {
    return a.player_id == b.player_id && a.event_id == b.event_id &&
           a.turn == b.turn && a.frame == b.frame &&
           a.owner_sequence == b.owner_sequence &&
           a.lua_sha256 == b.lua_sha256 && a.xml_path == b.xml_path &&
           a.viewport_width == b.viewport_width &&
           a.viewport_height == b.viewport_height;
}

} // namespace

EconomicCloseService make_economic_close_service(
    UiScreenController &controller, UiFrameSnapshot bound_frame,
    EconomicCloseOwnerServices services) {
    if (!services.current_owner || !services.decrement_turn_timer ||
        !services.publish_processed) return {};
    auto *screens = &controller;
    return [screens, frame = std::move(bound_frame),
            services = std::move(services)](const EconomicTabOwner &owner) {
        EconomicCloseReceipt receipt;
        if (!frame.active_player_id() || !frame.turn() ||
            frame.frame() != owner.frame ||
            *frame.active_player_id() != owner.player_id ||
            *frame.turn() != owner.turn ||
            screens->focused_screen() !=
                std::optional<std::string>{"EconomicOverview"}) return receipt;
        const auto current = services.current_owner();
        if (!current || !same_owner(*current, owner)) return receipt;
        const auto closed = screens->dispatch(UiCloseScreen{"EconomicOverview"}, frame);
        if (closed.status != UiCommandStatus::navigation) return receipt;
        receipt.dequeued = true;
        receipt.context_hidden = !screens->visible("EconomicOverview");
        if (!receipt.context_hidden) return receipt;
        receipt.turn_timer_decremented = services.decrement_turn_timer(owner);
        if (!receipt.turn_timer_decremented) return receipt;
        receipt.popup_processed = services.publish_processed(owner);
        return receipt;
    };
}

} // namespace civ5::ui
