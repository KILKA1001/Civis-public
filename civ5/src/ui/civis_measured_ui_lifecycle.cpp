#include "civ5/ui/civis_measured_ui_lifecycle.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::ui {
namespace {
bool valid_owner(const CivilopediaLayoutOwnerKey &owner) {
    return owner.frame_number != 0 && !owner.owner_token.empty() &&
        owner.player_id >= 0 && owner.game_turn >= 0 &&
        !owner.payload_sha256.empty() && !owner.rules_sha256.empty() &&
        !owner.styles_sha256.empty() && !owner.civilopedia_sha256.empty() &&
        !owner.xml_sha256.empty();
}
CivisMeasuredScrollResult blocked(std::string message) {
    CivisMeasuredScrollResult result;
    result.blocker = std::move(message);
    return result;
}
}
bool civis_same_ui_owner(const CivilopediaLayoutOwnerKey &a,
                         const CivilopediaLayoutOwnerKey &b) {
    return a.frame_number == b.frame_number && a.owner_token == b.owner_token &&
        a.player_id == b.player_id && a.game_turn == b.game_turn &&
        a.payload_sha256 == b.payload_sha256 && a.rules_sha256 == b.rules_sha256 &&
        a.styles_sha256 == b.styles_sha256 &&
        a.civilopedia_sha256 == b.civilopedia_sha256 && a.xml_sha256 == b.xml_sha256;
}
CivisMeasuredScrollLayout::CivisMeasuredScrollLayout(
    CivilopediaLayoutOwnerKey owner, CivisUiExecutionScope scope)
    : owner_(std::move(owner)), scope_(scope) {
    if (!valid_owner(owner_)) throw std::invalid_argument("Incomplete UI geometry owner");
}
CivisUiStep CivisMeasuredScrollLayout::publish(
    const CivilopediaLayoutOwnerKey &current, CivisMeasuredScrollSnapshot snapshot) {
    if (!valid_ || !civis_same_ui_owner(owner_, current) ||
        !civis_same_ui_owner(owner_, snapshot.owner) || snapshot.scope != scope_)
        return {false, "Measured scroll snapshot belongs to another owner or execution scope"};
    if (snapshot.panel_id.empty() || snapshot.geometry_revision == 0 ||
        snapshot.viewport_width <= 0 || snapshot.viewport_height <= 0 ||
        !snapshot.participants_complete)
        return {false, "Measured scroll participant enumeration or viewport is incomplete"};
    std::set<std::string> ids;
    for (const auto &child : snapshot.children) {
        if (child.id.empty() || !ids.insert(child.id).second || !child.visible)
            return {false, "Measured scroll participant ID or visibility is unresolved"};
        if (*child.visible && (!child.resolved_rect ||
            child.resolved_rect->width < 0 || child.resolved_rect->height < 0))
            return {false, "Visible scroll participant has unresolved or negative geometry"};
    }
    const auto found = panels_.find(snapshot.panel_id);
    if (found != panels_.end() &&
        snapshot.geometry_revision <= found->second.snapshot.geometry_revision)
        return {false, "Measured scroll geometry revision is stale"};
    const auto id = snapshot.panel_id;
    Panel panel{std::move(snapshot), {}, 0, 0};
    if (found != panels_.end()) {
        panel.retained_scroll_x = found->second.retained_scroll_x;
        panel.retained_scroll_y = found->second.retained_scroll_y;
    }
    panels_.insert_or_assign(id, std::move(panel));
    return {true, {}};
}
CivisMeasuredScrollResult CivisMeasuredScrollLayout::calculate_internal_size(
    const CivilopediaLayoutOwnerKey &current, std::string_view panel_id,
    std::uint64_t geometry_revision) {
    if (!valid_ || !civis_same_ui_owner(owner_, current))
        return blocked("Measured scroll geometry owner changed");
    const auto found = panels_.find(std::string(panel_id));
    if (found == panels_.end())
        return blocked("CalculateInternalSize requires resolved renderer measurements");
    auto &panel = found->second;
    if (geometry_revision != panel.snapshot.geometry_revision)
        return blocked("CalculateInternalSize geometry revision changed");
    std::int64_t min_x = 0, min_y = 0, max_x = 0, max_y = 0;
    std::size_t visible = 0;
    for (const auto &child : panel.snapshot.children) {
        if (!*child.visible) continue;
        const auto &rect = *child.resolved_rect;
        const auto right = static_cast<std::int64_t>(rect.x) + rect.width;
        const auto bottom = static_cast<std::int64_t>(rect.y) + rect.height;
        min_x = std::min(min_x, static_cast<std::int64_t>(rect.x));
        min_y = std::min(min_y, static_cast<std::int64_t>(rect.y));
        max_x = std::max(max_x, right);
        max_y = std::max(max_y, bottom);
        ++visible;
    }
    if (max_x > INT32_MAX || max_y > INT32_MAX ||
        max_x - min_x > INT32_MAX || max_y - min_y > INT32_MAX)
        return blocked("Measured scroll bounds exceed renderer integer range");
    CivisMeasuredScrollState state;
    state.geometry_revision = geometry_revision;
    state.min_x = static_cast<std::int32_t>(min_x);
    state.min_y = static_cast<std::int32_t>(min_y);
    state.content_width = static_cast<std::int32_t>(max_x - min_x);
    state.content_height = static_cast<std::int32_t>(max_y - min_y);
    state.max_scroll_x = std::max(0, state.content_width - panel.snapshot.viewport_width);
    state.max_scroll_y = std::max(0, state.content_height - panel.snapshot.viewport_height);
    state.scroll_x = std::clamp(panel.retained_scroll_x, 0, state.max_scroll_x);
    state.scroll_y = std::clamp(panel.retained_scroll_y, 0, state.max_scroll_y);
    state.visible_children = visible;
    panel.calculated = state;
    panel.retained_scroll_x = state.scroll_x;
    panel.retained_scroll_y = state.scroll_y;
    CivisMeasuredScrollResult result;
    result.applied = true;
    result.state = state;
    return result;
}
CivisMeasuredScrollResult CivisMeasuredScrollLayout::state(
    const CivilopediaLayoutOwnerKey &current, std::string_view panel_id) const {
    if (!valid_ || !civis_same_ui_owner(owner_, current))
        return blocked("Measured scroll geometry owner changed");
    const auto found = panels_.find(std::string(panel_id));
    if (found == panels_.end() || !found->second.calculated)
        return blocked("Measured scroll size has not been calculated for current geometry");
    CivisMeasuredScrollResult result;
    result.applied = true;
    result.state = *found->second.calculated;
    return result;
}
CivisUiStep CivisMeasuredScrollLayout::set_scroll(
    const CivilopediaLayoutOwnerKey &current, std::string_view panel_id,
    std::int32_t x, std::int32_t y) {
    const auto observed = state(current, panel_id);
    if (!observed.applied) return {false, observed.blocker};
    auto &panel = panels_.at(std::string(panel_id));
    panel.retained_scroll_x = std::clamp(x, 0, observed.state.max_scroll_x);
    panel.retained_scroll_y = std::clamp(y, 0, observed.state.max_scroll_y);
    panel.calculated->scroll_x = panel.retained_scroll_x;
    panel.calculated->scroll_y = panel.retained_scroll_y;
    return {true, {}};
}
void CivisMeasuredScrollLayout::invalidate() { valid_ = false; panels_.clear(); }

CivisUiContextLifecycle::CivisUiContextLifecycle(
    CivilopediaLayoutOwnerKey owner, CivisUiExecutionScope scope,
    std::string context_id, std::vector<std::string> available_events)
    : owner_(std::move(owner)), scope_(scope), context_id_(std::move(context_id)) {
    if (!valid_owner(owner_) || context_id_.empty())
        throw std::invalid_argument("Incomplete UI context owner");
    for (auto &event : available_events)
        if (event.empty() || !available_events_.insert(std::move(event)).second)
            throw std::invalid_argument("Empty or duplicate UI event API");
}
bool CivisUiContextLifecycle::owns(const CivilopediaLayoutOwnerKey &current) const {
    return valid_ && civis_same_ui_owner(owner_, current);
}
CivisUiListenerResult CivisUiContextLifecycle::add(
    const CivilopediaLayoutOwnerKey &current, std::string_view event,
    CivisUiListener listener) {
    CivisUiListenerResult result;
    if (!owns(current)) result.blocker = "UI listener owner changed or closed";
    else if (!available_events_.contains(std::string(event)))
        result.blocker = "UI event API is unavailable: " + std::string(event);
    else if (!listener || next_token_ == std::numeric_limits<std::uint64_t>::max())
        result.blocker = "UI listener callback or token is unavailable";
    else {
        const auto token = next_token_;
        listeners_.push_back({token, std::string(event), std::move(listener)});
        ++next_token_;
        result.applied = true;
        result.token = token;
    }
    return result;
}
CivisUiStep CivisUiContextLifecycle::remove(
    const CivilopediaLayoutOwnerKey &current, std::uint64_t token) {
    if (!owns(current)) return {false, "UI listener owner changed or closed"};
    const auto found = std::find_if(listeners_.begin(), listeners_.end(),
        [token](const auto &listener) { return listener.token == token; });
    if (found == listeners_.end()) return {false, "UI listener token is not registered"};
    listeners_.erase(found);
    return {true, {}};
}
CivisUiDispatchResult CivisUiContextLifecycle::dispatch(
    const CivilopediaLayoutOwnerKey &current, const CivisUiNotification &event) {
    CivisUiDispatchResult result;
    if (!owns(current)) result.blocker = "UI dispatch owner changed or closed";
    else if (!available_events_.contains(event.name)) result.blocker = "UI event API is unavailable";
    else if (dispatching_.contains(event.name)) result.blocker = "Reentrant UI notification dispatch";
    if (!result.blocker.empty()) return result;
    std::vector<std::uint64_t> tokens;
    for (const auto &listener : listeners_)
        if (listener.event == event.name) tokens.push_back(listener.token);
    dispatching_.insert(event.name);
    // Guard also clears dispatch state on allocation/callback-copy exceptions.
    struct Guard {
        std::set<std::string> &events; const std::string &name;
        ~Guard() { events.erase(name); }
    } guard{dispatching_, event.name};
    for (const auto token : tokens) {
        if (!owns(current)) { result.blocker = "UI context closed during dispatch"; break; }
        const auto found = std::find_if(listeners_.begin(), listeners_.end(),
            [token](const auto &listener) { return listener.token == token; });
        if (found == listeners_.end()) continue;
        // Copy before entering user code: Add/Remove can invalidate vector storage.
        const auto call = found->call;
        ++result.invoked;
        try { call(event); }
        catch (const std::exception &error) { result.errors.emplace_back(error.what()); }
        catch (...) { result.errors.emplace_back("UI listener raised a nonstandard exception"); }
    }
    if (!owns(current) && result.blocker.empty())
        result.blocker = "UI context closed during dispatch";
    result.applied = result.blocker.empty() && result.errors.empty();
    return result;
}
CivisUiStep CivisUiContextLifecycle::process_once(
    const CivilopediaLayoutOwnerKey &current, const CivisUiNotification &event,
    CivisUiListener processed_callback) {
    if (!owns(current)) return {false, "Processed event owner changed or closed"};
    if (!available_events_.contains(event.name) || !event.occurrence_id || !processed_callback)
        return {false, "Processed event API, occurrence or callback is unavailable"};
    for (const auto &attempt : processed_)
        if (attempt.event.name == event.name && attempt.event.occurrence_id == event.occurrence_id)
            return {false, "Processed event was already attempted"};
    const auto index = processed_.size();
    processed_.push_back({event, CivisUiProcessedStatus::attempted});
    try {
        processed_callback(event);
        if (!owns(current)) {
            processed_[index].status = CivisUiProcessedStatus::uncertain;
            return {false, "Processed callback invalidated its context owner"};
        }
        processed_[index].status = CivisUiProcessedStatus::completed;
        return {true, {}};
    } catch (const std::exception &error) {
        processed_[index].status = CivisUiProcessedStatus::uncertain;
        return {false, std::string("Processed callback attempted, completion uncertain: ") + error.what()};
    } catch (...) {
        processed_[index].status = CivisUiProcessedStatus::uncertain;
        return {false, "Processed callback attempted, completion uncertain"};
    }
}
CivisUiStep CivisUiContextLifecycle::close(const CivilopediaLayoutOwnerKey &current) {
    if (!owns(current)) return {false, "UI context owner changed or already closed"};
    valid_ = false;
    listeners_.clear();
    return {true, {}};
}
std::size_t CivisUiContextLifecycle::listener_count(std::string_view event) const {
    return static_cast<std::size_t>(std::count_if(listeners_.begin(), listeners_.end(),
        [event](const auto &listener) { return listener.event == event; }));
}
} // namespace civ5::ui
