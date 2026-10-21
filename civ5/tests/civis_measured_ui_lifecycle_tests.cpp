#include "civ5/ui/civis_measured_ui_lifecycle.hpp"
#include <iostream>
#include <stdexcept>

using namespace civ5::ui;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
CivilopediaLayoutOwnerKey owner() {
    return {31, "CivilopediaScreen:open-event-7", 0, 123,
        std::string(64, 'a'), std::string(64, 'b'), std::string(64, 'c'),
        std::string(64, 'd'), std::string(64, 'e')};
}
CivisMeasuredScrollSnapshot panel(const CivilopediaLayoutOwnerKey &key) {
    CivisMeasuredScrollSnapshot result;
    result.owner = key;
    result.panel_id = "ScrollPanel";
    result.geometry_revision = 1;
    result.viewport_width = 100; result.viewport_height = 80;
    result.participants_complete = true;
    result.children = {
        {"Article", true, CivisMeasuredRect{-10, -5, 130, 200}},
        {"Footer", true, CivisMeasuredRect{0, 230, 90, 20}},
        {"HiddenArticle", false, {}}
    };
    return result;
}
void layout_tests() {
    const auto key = owner();
    CivisMeasuredScrollLayout layout(key, CivisUiExecutionScope::diagnostic);
    check(!layout.calculate_internal_size(key, "ScrollPanel", 1).applied,
          "Absent measurements must fail without an empty-content substitute");
    auto source = panel(key);
    auto incomplete = source; incomplete.participants_complete = false;
    check(!layout.publish(key, incomplete).applied, "Partial renderer enumeration must fail");
    incomplete = source; incomplete.children[0].visible.reset();
    check(!layout.publish(key, incomplete).applied, "Unknown visibility must fail");
    incomplete = source; incomplete.children[0].resolved_rect.reset();
    check(!layout.publish(key, incomplete).applied, "Visible unresolved bounds must fail");
    incomplete = source; incomplete.scope = CivisUiExecutionScope::production;
    check(!layout.publish(key, incomplete).applied, "Diagnostic owner must reject production measurements");
    incomplete = source; incomplete.children.push_back(incomplete.children[0]);
    check(!layout.publish(key, incomplete).applied, "Duplicate child IDs must fail");
    check(layout.publish(key, source).applied, "Complete measured bounds must publish");
    check(!layout.state(key, "ScrollPanel").applied &&
          !layout.calculate_internal_size(key, "ScrollPanel", 2).applied,
          "Publishing must require calculation at the current measured revision");
    const auto result = layout.calculate_internal_size(key, "ScrollPanel", 1);
    check(result.applied && !result.native_parity_verified && result.state.min_x == -10 &&
          result.state.min_y == -5 && result.state.content_width == 130 &&
          result.state.content_height == 255 && result.state.max_scroll_x == 30 &&
          result.state.max_scroll_y == 175 && result.state.visible_children == 2,
          "Calculation must execute visible bounds union and scroll range");
    check(layout.set_scroll(key, "ScrollPanel", 99, 999).applied &&
          layout.state(key, "ScrollPanel").state.scroll_x == 30 &&
          layout.state(key, "ScrollPanel").state.scroll_y == 175,
          "Scroll request must clamp to actual calculated bounds");
    check(!layout.publish(key, source).applied, "Same geometry revision cannot overwrite live measurements");
    source.geometry_revision = 2; source.children.clear();
    check(layout.publish(key, source).applied && !layout.state(key, "ScrollPanel").applied,
          "New geometry invalidates old calculated state");
    const auto empty = layout.calculate_internal_size(key, "ScrollPanel", 2);
    check(empty.applied && empty.state.content_height == 0 && empty.state.scroll_y == 0,
          "Complete empty panel recalculation must shrink content and clamp retained scroll");
    source.geometry_revision = 3;
    source.children = {{"Overflow", true, CivisMeasuredRect{INT32_MAX, 0, 1, 1}}};
    check(layout.publish(key, source).applied &&
          !layout.calculate_internal_size(key, "ScrollPanel", 3).applied &&
          !layout.state(key, "ScrollPanel").applied,
          "Overflow cannot publish calculated state or issue success");
    auto stale = key; ++stale.frame_number;
    check(!layout.publish(stale, panel(key)).applied &&
          !layout.calculate_internal_size(stale, "ScrollPanel", 3).applied,
          "Stale frame must reject geometry access");
    layout.invalidate();
    check(!layout.publish(key, panel(key)).applied, "Invalidated owner must reject snapshots");
}
void lifecycle_tests() {
    const auto key = owner();
    CivisUiContextLifecycle life(key, CivisUiExecutionScope::diagnostic,
        "CivilopediaScreen", {"AfterModsDeactivate", "SearchForPediaEntry", "PopupProcessed"});
    check(!life.add(key, "MissingNativeApi", [](const auto &) {}).applied,
          "Missing event API must stay unavailable");
    std::vector<int> calls;
    std::uint64_t second = 0;
    bool changed = false;
    const auto first = life.add(key, "SearchForPediaEntry", [&](const auto &event) {
        calls.push_back(1);
        check(!life.dispatch(key, event).applied, "Reentrant same-event dispatch must be rejected");
        if (!changed) {
            changed = true;
            check(life.remove(key, second).applied, "Callback must unregister another live listener");
            check(life.add(key, "SearchForPediaEntry", [&](const auto &) { calls.push_back(3); }).applied,
                  "Callback may register next-dispatch listener");
        }
    });
    second = life.add(key, "SearchForPediaEntry", [&](const auto &) { calls.push_back(2); }).token;
    check(first.applied && second > first.token, "Listener tokens must preserve registration order");
    const auto emitted = life.dispatch(key, {"SearchForPediaEntry", 1});
    check(emitted.applied && emitted.invoked == 1 && calls == std::vector<int>{1},
          "Removed listener must be skipped and new listener deferred");
    check(life.dispatch(key, {"SearchForPediaEntry", 2}).applied &&
          calls == std::vector<int>({1, 1, 3}), "Second dispatch must preserve remaining source registration order");
    auto stale = key; stale.owner_token += ":reopened";
    check(!life.remove(stale, first.token).applied &&
          !life.dispatch(stale, {"SearchForPediaEntry", 3}).applied &&
          !life.process_once(stale, {"PopupProcessed", 1}, [](const auto &) {}).applied,
          "Stale context token must not remove listeners, dispatch or process events");
    int processed = 0;
    check(life.process_once(key, {"PopupProcessed", 9}, [&](const auto &event) {
        ++processed;
        check(!life.process_once(key, event, [](const auto &) {}).applied,
              "Processed identity must be reserved before entering callback");
    }).applied && processed == 1 &&
        life.processed_attempts().back().status == CivisUiProcessedStatus::completed,
        "Processed callback must execute once with completed journal");
    check(!life.process_once(key, {"PopupProcessed", 9}, [&](const auto &) { ++processed; }).applied &&
          processed == 1, "Duplicate processed event cannot execute callback again");
    check(!life.process_once(key, {"PopupProcessed", 10}, [&](const auto &) {
        ++processed; throw std::runtime_error("External callback failed after side effect");
    }).applied && life.processed_attempts().back().status == CivisUiProcessedStatus::uncertain,
          "Failed external callback must retain uncertain attempted identity");
    check(!life.process_once(key, {"PopupProcessed", 10}, [&](const auto &) { ++processed; }).applied &&
          processed == 2, "Failed attempt cannot be retried automatically");
    check(!life.process_once(key, {"PopupProcessed", 0}, [](const auto &) {}).applied &&
          life.processed_attempts().size() == 2, "Missing occurrence ID must not reserve a fake event");
    check(life.add(key, "AfterModsDeactivate", [](const auto &) { throw std::runtime_error("listener failed"); }).applied,
          "Real callback must register");
    const auto failed = life.dispatch(key, {"AfterModsDeactivate", 1});
    check(!failed.applied && failed.invoked == 1 && failed.errors.size() == 1,
          "Listener exception must be reported, not converted to success");
    check(life.close(key).applied && life.listener_count("SearchForPediaEntry") == 0 &&
          !life.dispatch(key, {"SearchForPediaEntry", 4}).applied &&
          !life.add(key, "SearchForPediaEntry", [](const auto &) {}).applied,
          "Closing context unregisters callbacks and prevents subsequent execution");
    check(life.scope() == CivisUiExecutionScope::diagnostic,
          "Diagnostic lifecycle remains explicitly diagnostic");
    CivisUiContextLifecycle last_listener(key, CivisUiExecutionScope::diagnostic,
        "CivilopediaScreen", {"AfterModsDeactivate"});
    check(last_listener.add(key, "AfterModsDeactivate", [&](const auto &) {
        check(last_listener.close(key).applied, "Last dispatched listener can close its context");
    }).applied && !last_listener.dispatch(key, {"AfterModsDeactivate", 1}).applied,
          "Last listener owner invalidation cannot publish dispatch success");
    CivisUiContextLifecycle closing(key, CivisUiExecutionScope::production,
        "CivilopediaScreen", {"PopupProcessed"});
    check(!closing.process_once(key, {"PopupProcessed", 9}, [&](const auto &) {
        check(closing.close(key).applied, "Callback can close its context");
    }).applied && closing.processed_attempts().back().status == CivisUiProcessedStatus::uncertain,
          "Owner invalidation during external callback prevents success publication");
}
}
int main() {
    layout_tests(); lifecycle_tests();
    std::cout << "Measured content bounds, scroll range, stale owners, source-order listeners, unregister and once-only processing passed; native parity unverified\n";
}
