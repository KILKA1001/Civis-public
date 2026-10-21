#include "civ5/ui/civis_civilopedia_geometry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>

namespace civ5::ui {
namespace {

bool same_owner(const CivilopediaLayoutOwnerKey &a,
                const CivilopediaLayoutOwnerKey &b) {
    return a.frame_number == b.frame_number && a.owner_token == b.owner_token &&
           a.player_id == b.player_id && a.game_turn == b.game_turn &&
           a.payload_sha256 == b.payload_sha256 &&
           a.rules_sha256 == b.rules_sha256 &&
           a.styles_sha256 == b.styles_sha256 &&
           a.civilopedia_sha256 == b.civilopedia_sha256 &&
           a.xml_sha256 == b.xml_sha256;
}

bool same_instruction(const CivilopediaLayoutInstruction &a,
                      const CivilopediaLayoutInstruction &b) {
    return a.operation == b.operation && a.control_id == b.control_id &&
           a.x == b.x && a.y == b.y && a.authored_line == b.authored_line;
}

const content::XmlNode *find_id(const content::XmlNode &node,
                                std::string_view id) {
    if (node.attribute("ID") == id) return &node;
    for (const auto &child : node.children)
        if (const auto *found = find_id(child, id)) return found;
    return nullptr;
}

const content::XmlNode *unique_style(const content::XmlNode &styles,
                                     std::string_view name) {
    const content::XmlNode *found = nullptr;
    for (const auto &child : styles.children)
        if (child.name == name) {
            if (found) return nullptr;
            found = &child;
        }
    return found;
}

} // namespace

CivisCivilopediaControlTree::CivisCivilopediaControlTree(
    CivilopediaLayoutOwnerKey owner,
    std::vector<CivilopediaLayoutInstruction> instructions,
    std::array<CivisControlGeometry, 8> controls)
    : owner_(std::move(owner)), instructions_(std::move(instructions)),
      controls_(std::move(controls)) {}

std::unique_ptr<CivisCivilopediaControlTree> CivisCivilopediaControlTree::open(
    const assets::ResourceMountTable &selected_ui_resources,
    const SelectedBnwCivilopediaLayoutSources &sources,
    const SelectedBnwCivilopediaLayoutPreflight &plan,
    const CivilopediaMeasuredLayout &measured,
    std::string &blocker) {
    blocker.clear();
    try {
        const auto style = resolve_bnw_civilopedia_vert_slider_style(
            selected_ui_resources, sources);
        if (!style.selected_source_verified)
            throw std::runtime_error("Selected Civilopedia style is unavailable: " +
                                     style.blocker);
        const runtime::SessionCheckpointIdentity identity{
            measured.payload_sha256, measured.rules_sha256};
        const auto canonical = plan_bnw_civilopedia_native_layout(
            sources, identity, measured.player_id, measured.game_turn,
            measured.frame_number, measured);
        if (!canonical.source_and_owner_verified ||
            !plan.source_and_owner_verified ||
            !same_owner(plan.owner, canonical.owner) ||
            plan.instructions.size() != canonical.instructions.size() ||
            measured.scrollbar_width != style.authored_vert_slider_width ||
            measured.scroll_panel_width != 1002 ||
            measured.scroll_panel_height != 980 ||
            measured.left_panel_width != 294 ||
            measured.left_panel_height != 980)
            throw std::runtime_error("Civilopedia owner frame, VFS style or initial sizes are inconsistent");
        for (std::size_t i = 0; i < plan.instructions.size(); ++i)
            if (!same_instruction(plan.instructions[i], canonical.instructions[i]))
                throw std::runtime_error("Civilopedia layout plan differs from authored order");
        if (runtime::identify_file(sources.styles_path).sha256 != style.sha256)
            throw std::runtime_error("Selected Civilopedia style changed after VFS resolution");
        const auto styles = content::read_xml(sources.styles_path);
        if (runtime::identify_file(sources.styles_path).sha256 != style.sha256 ||
            styles.name != "StyleSheet")
            throw std::runtime_error("Selected Civilopedia style changed during read");
        for (const auto name : {"VertSlider", "ScrollBarUp", "ScrollBarDown"}) {
            const auto *node = unique_style(styles, name);
            if (!node || node->attribute("Size") != "18,18")
                throw std::runtime_error("Selected Civilopedia control style size is unresolved");
        }
        const auto xml_path = std::filesystem::path(sources.styles_path)
            .parent_path().parent_path().parent_path() /
            "Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";
        if (runtime::identify_file(xml_path).sha256 != sources.xml_sha256)
            throw std::runtime_error("Selected Civilopedia XML changed after source preflight");
        const auto xml = content::read_xml(xml_path);
        if (runtime::identify_file(xml_path).sha256 != sources.xml_sha256 ||
            !inspect_bnw_civilopedia_native_control_tree(xml).verified)
            throw std::runtime_error("Selected Civilopedia XML ownership changed during read");
        std::array<CivisControlGeometry, 8> controls;
        for (std::size_t i = 0; i < controls.size(); ++i) {
            const auto &owned = sources.control_tree.controls.at(i);
            const auto *node = find_id(xml, owned.id);
            if (!node || node->attribute("Offset") != "0,0" ||
                (i != 0 && i != 4 && !node->attribute("Size").empty()))
                throw std::runtime_error("Selected Civilopedia initial control geometry changed");
            const bool main_panel = i == 0;
            const bool left_panel = i == 4;
            controls[i] = {owned.id, owned.parent_id,
                           main_panel ? 1002 : left_panel ? 294 : 18,
                           main_panel || left_panel ? 980 : 18, 0, 0};
        }
        return std::unique_ptr<CivisCivilopediaControlTree>(
            new CivisCivilopediaControlTree(plan.owner, plan.instructions,
                                            std::move(controls)));
    } catch (const std::exception &error) {
        blocker = error.what();
        return {};
    }
}

const CivisControlGeometry *CivisCivilopediaControlTree::find(
    std::string_view control_id) const {
    for (const auto &control : controls_)
        if (control.id == control_id) return &control;
    return nullptr;
}

CivisControlGeometry *CivisCivilopediaControlTree::find(
    std::string_view control_id) {
    return const_cast<CivisControlGeometry *>(
        static_cast<const CivisCivilopediaControlTree *>(this)->find(control_id));
}

CivisControlGeometryResult CivisCivilopediaControlTree::get_geometry(
    const CivilopediaLayoutOwnerKey &current,
    std::string_view control_id) const {
    if (!same_owner(owner_, current))
        return {false, {}, "Civilopedia control belongs to another frame or owner"};
    const auto *control = find(control_id);
    if (!control)
        return {false, {}, "Civilopedia control ID is absent from selected XML"};
    return {true, *control, {}};
}

CivisControlSizeResult CivisCivilopediaControlTree::get_size(
    const CivilopediaLayoutOwnerKey &current,
    std::string_view control_id) const {
    const auto geometry = get_geometry(current, control_id);
    return {geometry.available, geometry.geometry.width,
            geometry.geometry.height, geometry.blocker};
}

CivisGeometryStepResult CivisCivilopediaControlTree::apply_mutation(
    const CivilopediaLayoutOwnerKey &current,
    CivilopediaLayoutOperation operation,
    std::string_view control_id, std::int32_t x, std::int32_t y) {
    if (!same_owner(owner_, current))
        return {false, next_operation_, "Civilopedia layout owner/frame changed"};
    if (next_operation_ >= instructions_.size())
        return {false, next_operation_, "Civilopedia layout plan is complete"};
    const auto &expected = instructions_[next_operation_];
    if (expected.operation != operation || expected.control_id != control_id ||
        expected.x != x || expected.y != y)
        return {false, next_operation_, "Civilopedia setter differs from authored order"};
    auto *control = find(control_id);
    if (!control)
        return {false, next_operation_, "Civilopedia control ID is absent from selected XML"};
    if (operation == CivilopediaLayoutOperation::set_size) {
        if (x <= 0 || y <= 0)
            return {false, next_operation_, "Civilopedia control size is nonpositive"};
        control->width = x;
        control->height = y;
    } else if (operation == CivilopediaLayoutOperation::set_offset) {
        control->offset_x = x;
        control->offset_y = y;
    } else {
        return {false, next_operation_, "CalculateInternalSize has no verified Civis algorithm"};
    }
    ++next_operation_;
    return {true, next_operation_, {}};
}

CivisGeometryStepResult CivisCivilopediaControlTree::set_size(
    const CivilopediaLayoutOwnerKey &current,
    std::string_view control_id, std::int32_t width, std::int32_t height) {
    return apply_mutation(current, CivilopediaLayoutOperation::set_size,
                          control_id, width, height);
}

CivisGeometryStepResult CivisCivilopediaControlTree::set_offset(
    const CivilopediaLayoutOwnerKey &current,
    std::string_view control_id, std::int32_t x, std::int32_t y) {
    return apply_mutation(current, CivilopediaLayoutOperation::set_offset,
                          control_id, x, y);
}

CivisGeometryStepResult CivisCivilopediaControlTree::calculate_internal_size(
    const CivilopediaLayoutOwnerKey &current,
    std::string_view control_id) {
    if (!same_owner(owner_, current))
        return {false, next_operation_, "Civilopedia layout owner/frame changed"};
    if (next_operation_ >= instructions_.size() ||
        instructions_[next_operation_].operation !=
            CivilopediaLayoutOperation::calculate_internal_size ||
        instructions_[next_operation_].control_id != control_id)
        return {false, next_operation_, "Civilopedia CalculateInternalSize call is out of order"};
    return {false, next_operation_,
            "Native ScrollPanel CalculateInternalSize/content anchoring rules are unavailable"};
}

CivisGeometryStepResult CivisCivilopediaControlTree::apply_ordered_plan(
    const CivilopediaLayoutOwnerKey &current) {
    while (next_operation_ < instructions_.size()) {
        const auto instruction = instructions_[next_operation_];
        CivisGeometryStepResult step;
        if (instruction.operation == CivilopediaLayoutOperation::set_size)
            step = set_size(current, instruction.control_id,
                            instruction.x, instruction.y);
        else if (instruction.operation == CivilopediaLayoutOperation::set_offset)
            step = set_offset(current, instruction.control_id,
                              instruction.x, instruction.y);
        else step = calculate_internal_size(current, instruction.control_id);
        if (!step.applied) return step;
    }
    return {true, next_operation_, {}};
}

} // namespace civ5::ui
