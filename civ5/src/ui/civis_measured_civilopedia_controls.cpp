#include "civ5/ui/civis_measured_civilopedia_controls.hpp"
#include <utility>

namespace civ5::ui {
CivisMeasuredCivilopediaControlTree::CivisMeasuredCivilopediaControlTree(
    std::unique_ptr<CivisCivilopediaControlTree> prefix,
    std::vector<CivilopediaLayoutInstruction> instructions, CivisUiExecutionScope scope)
    : prefix_(std::move(prefix)), instructions_(std::move(instructions)),
      scope_(scope), scroll_(prefix_->owner(), scope) {}
std::unique_ptr<CivisMeasuredCivilopediaControlTree>
CivisMeasuredCivilopediaControlTree::open(const assets::ResourceMountTable &resources,
    const SelectedBnwCivilopediaLayoutSources &sources,
    const SelectedBnwCivilopediaLayoutPreflight &plan,
    const CivilopediaMeasuredLayout &measured, CivisUiExecutionScope scope,
    std::string &blocker) {
    auto prefix = CivisCivilopediaControlTree::open(resources, sources, plan, measured, blocker);
    if (!prefix) return {};
    if (plan.instructions.size() != 11 ||
        plan.instructions[9].operation != CivilopediaLayoutOperation::calculate_internal_size ||
        plan.instructions[9].control_id != "ScrollPanel" ||
        plan.instructions[10].operation != CivilopediaLayoutOperation::calculate_internal_size ||
        plan.instructions[10].control_id != "LeftScrollPanel") {
        blocker = "Selected Civilopedia measured layout source block changed";
        return {};
    }
    return std::unique_ptr<CivisMeasuredCivilopediaControlTree>(
        new CivisMeasuredCivilopediaControlTree(std::move(prefix), plan.instructions, scope));
}
std::size_t CivisMeasuredCivilopediaControlTree::completed_operations() const {
    return prefix_->completed_operations() + internal_completed_;
}
CivisControlSizeResult CivisMeasuredCivilopediaControlTree::get_size(
    const CivilopediaLayoutOwnerKey &current, std::string_view id) const {
    return prefix_->get_size(current, id);
}
CivisControlGeometryResult CivisMeasuredCivilopediaControlTree::get_geometry(
    const CivilopediaLayoutOwnerKey &current, std::string_view id) const {
    return prefix_->get_geometry(current, id);
}
CivisGeometryStepResult CivisMeasuredCivilopediaControlTree::set_size(
    const CivilopediaLayoutOwnerKey &current, std::string_view id,
    std::int32_t width, std::int32_t height) {
    return prefix_->set_size(current, id, width, height);
}
CivisGeometryStepResult CivisMeasuredCivilopediaControlTree::set_offset(
    const CivilopediaLayoutOwnerKey &current, std::string_view id,
    std::int32_t x, std::int32_t y) {
    return prefix_->set_offset(current, id, x, y);
}
CivisUiStep CivisMeasuredCivilopediaControlTree::publish(
    const CivilopediaLayoutOwnerKey &current, CivisMeasuredScrollSnapshot snapshot) {
    if (prefix_->completed_operations() != 9 ||
        (snapshot.panel_id != "ScrollPanel" && snapshot.panel_id != "LeftScrollPanel"))
        return {false, "Measured Civilopedia scroll snapshot requires the nine source setters"};
    const auto size = prefix_->get_size(current, snapshot.panel_id);
    if (!size.available || size.width != snapshot.viewport_width ||
        size.height != snapshot.viewport_height)
        return {false, "Measured Civilopedia viewport differs from current control size"};
    const auto id = snapshot.panel_id;
    const auto revision = snapshot.geometry_revision;
    const auto result = scroll_.publish(current, std::move(snapshot));
    if (result.applied) revisions_.insert_or_assign(id, revision);
    return result;
}
CivisGeometryStepResult CivisMeasuredCivilopediaControlTree::calculate_internal_size(
    const CivilopediaLayoutOwnerKey &current, std::string_view id) {
    if (!civis_same_ui_owner(owner(), current))
        return {false, completed_operations(), "Measured Civilopedia owner changed"};
    if (prefix_->completed_operations() != 9 ||
        (internal_completed_ < 2 && instructions_[9 + internal_completed_].control_id != id))
        return {false, completed_operations(), "Measured Civilopedia CalculateInternalSize is out of source order"};
    if (id != "ScrollPanel" && id != "LeftScrollPanel")
        return {false, completed_operations(), "Measured Civilopedia control is not a supported scroll panel"};
    const auto revision = revisions_.find(std::string(id));
    const auto result = scroll_.calculate_internal_size(current, id,
        revision == revisions_.end() ? 0 : revision->second);
    if (!result.applied) return {false, completed_operations(), result.blocker};
    if (internal_completed_ < 2) ++internal_completed_;
    return {true, completed_operations(), {}};
}
CivisGeometryStepResult CivisMeasuredCivilopediaControlTree::apply_ordered_plan(
    const CivilopediaLayoutOwnerKey &current) {
    if (!civis_same_ui_owner(owner(), current))
        return {false, completed_operations(), "Measured Civilopedia owner changed"};
    while (completed_operations() < instructions_.size()) {
        const auto &instruction = instructions_[completed_operations()];
        CivisGeometryStepResult step;
        if (instruction.operation == CivilopediaLayoutOperation::set_size)
            step = set_size(current, instruction.control_id, instruction.x, instruction.y);
        else if (instruction.operation == CivilopediaLayoutOperation::set_offset)
            step = set_offset(current, instruction.control_id, instruction.x, instruction.y);
        else step = calculate_internal_size(current, instruction.control_id);
        if (!step.applied) return step;
    }
    return {true, completed_operations(), {}};
}
CivisMeasuredScrollResult CivisMeasuredCivilopediaControlTree::scroll_state(
    const CivilopediaLayoutOwnerKey &current, std::string_view id) const {
    return scroll_.state(current, id);
}
CivisUiStep CivisMeasuredCivilopediaControlTree::set_scroll(
    const CivilopediaLayoutOwnerKey &current, std::string_view id,
    std::int32_t x, std::int32_t y) { return scroll_.set_scroll(current, id, x, y); }
} // namespace civ5::ui
