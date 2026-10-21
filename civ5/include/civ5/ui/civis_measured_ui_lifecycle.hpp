#pragma once

#include "civ5/ui/selected_bnw_civilopedia_layout.hpp"
#include <functional>
#include <map>
#include <optional>
#include <set>

namespace civ5::ui {

// Caller supplies resolved renderer geometry, never authored XML extents.
// Diagnostic execution cannot produce a production or native-parity receipt.
enum class CivisUiExecutionScope { diagnostic, production };
struct CivisUiStep {
    bool applied = false;
    std::string blocker;
};
struct CivisMeasuredRect {
    std::int32_t x = 0, y = 0, width = 0, height = 0;
};
struct CivisMeasuredScrollChild {
    std::string id;
    std::optional<bool> visible;
    std::optional<CivisMeasuredRect> resolved_rect;
};
struct CivisMeasuredScrollSnapshot {
    CivilopediaLayoutOwnerKey owner;
    CivisUiExecutionScope scope = CivisUiExecutionScope::diagnostic;
    std::string panel_id;
    std::uint64_t geometry_revision = 0;
    std::int32_t viewport_width = 0, viewport_height = 0;
    // The renderer must enumerate every participating child, including hidden
    // children. Empty complete enumeration represents a genuinely empty panel.
    bool participants_complete = false;
    std::vector<CivisMeasuredScrollChild> children;
};
struct CivisMeasuredScrollState {
    std::uint64_t geometry_revision = 0;
    std::int32_t min_x = 0, min_y = 0, content_width = 0, content_height = 0;
    std::int32_t max_scroll_x = 0, max_scroll_y = 0;
    std::int32_t scroll_x = 0, scroll_y = 0;
    std::size_t visible_children = 0;
};
struct CivisMeasuredScrollResult : CivisUiStep {
    CivisMeasuredScrollState state;
    bool native_parity_verified = false;
};

// Civis policy: bounds include content origin and visible resolved rectangles;
// scrolling normalizes negative bounds and clamps offsets to content extent.
// This executes layout. It does not attest the original ScrollPanel algorithm.
class CivisMeasuredScrollLayout {
  public:
    CivisMeasuredScrollLayout(CivilopediaLayoutOwnerKey owner,
                              CivisUiExecutionScope scope);
    CivisUiStep publish(const CivilopediaLayoutOwnerKey &current,
                        CivisMeasuredScrollSnapshot snapshot);
    CivisMeasuredScrollResult calculate_internal_size(
        const CivilopediaLayoutOwnerKey &current, std::string_view panel_id,
        std::uint64_t geometry_revision);
    CivisMeasuredScrollResult state(const CivilopediaLayoutOwnerKey &current,
                                     std::string_view panel_id) const;
    CivisUiStep set_scroll(const CivilopediaLayoutOwnerKey &current,
                            std::string_view panel_id, std::int32_t x,
                            std::int32_t y);
    void invalidate();
  private:
    struct Panel {
        CivisMeasuredScrollSnapshot snapshot;
        std::optional<CivisMeasuredScrollState> calculated;
        std::int32_t retained_scroll_x = 0, retained_scroll_y = 0;
    };
    CivilopediaLayoutOwnerKey owner_;
    CivisUiExecutionScope scope_;
    bool valid_ = true;
    std::map<std::string, Panel> panels_;
};

struct CivisUiNotification {
    std::string name;
    std::uint64_t occurrence_id = 0;
};
using CivisUiListener = std::function<void(const CivisUiNotification &)>;
struct CivisUiListenerResult : CivisUiStep {
    std::uint64_t token = 0;
};
struct CivisUiDispatchResult : CivisUiStep {
    std::size_t invoked = 0;
    std::vector<std::string> errors;
};
enum class CivisUiProcessedStatus { attempted, completed, uncertain };
struct CivisUiProcessedAttempt {
    CivisUiNotification event;
    CivisUiProcessedStatus status = CivisUiProcessedStatus::attempted;
};

// Single UI-thread owner. Add order is execution/source order; dispatch takes
// a token snapshot, skips removed listeners, and defers newly added listeners.
// Invalidation unregisters callbacks. Processed attempts reserve identity before
// external code; uncertain attempts are one-shot and are never auto-retried.
class CivisUiContextLifecycle {
  public:
    CivisUiContextLifecycle(CivilopediaLayoutOwnerKey owner,
        CivisUiExecutionScope scope, std::string context_id,
        std::vector<std::string> available_events);
    CivisUiListenerResult add(const CivilopediaLayoutOwnerKey &current,
        std::string_view event, CivisUiListener listener);
    CivisUiStep remove(const CivilopediaLayoutOwnerKey &current,
                        std::uint64_t token);
    CivisUiDispatchResult dispatch(const CivilopediaLayoutOwnerKey &current,
                                    const CivisUiNotification &event);
    CivisUiStep process_once(const CivilopediaLayoutOwnerKey &current,
        const CivisUiNotification &event, CivisUiListener processed_callback);
    CivisUiStep close(const CivilopediaLayoutOwnerKey &current);
    std::size_t listener_count(std::string_view event) const;
    const std::vector<CivisUiProcessedAttempt> &processed_attempts() const {
        return processed_;
    }
    CivisUiExecutionScope scope() const { return scope_; }
  private:
    struct Listener { std::uint64_t token; std::string event; CivisUiListener call; };
    bool owns(const CivilopediaLayoutOwnerKey &current) const;
    CivilopediaLayoutOwnerKey owner_;
    CivisUiExecutionScope scope_;
    std::string context_id_;
    std::set<std::string> available_events_, dispatching_;
    bool valid_ = true;
    std::uint64_t next_token_ = 1;
    std::vector<Listener> listeners_;
    std::vector<CivisUiProcessedAttempt> processed_;
};

bool civis_same_ui_owner(const CivilopediaLayoutOwnerKey &a,
                          const CivilopediaLayoutOwnerKey &b);
} // namespace civ5::ui
