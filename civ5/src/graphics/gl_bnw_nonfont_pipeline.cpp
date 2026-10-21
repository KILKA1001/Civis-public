#include "civ5/graphics/gl_bnw_nonfont_pipeline.hpp"
#include "civ5/graphics/detail/bnw_nonfont_native_record_projection.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace civ5::graphics {

struct GlBnwNonFontPipeline::PreparedNativeRecord::Impl final {
    const ui::detail::BnwNormalNonFontItemOccurrence::Guard item;
    const ui::detail::BnwNormalNonFontDrainPermission::Guard drain;
    const std::shared_ptr<const ui::detail::BnwNormalNonFontMaterialLoan> material;
    const std::array<std::shared_ptr<const ui::detail::BnwNormalNonFontResourceLoan>, 4> resources;
    const detail::BnwNonFontNativeRecordProjection projection;

    Impl(const ui::detail::BnwNormalNonFontItemOccurrence::Guard& original_item,
         const ui::detail::BnwNormalNonFontDrainPermission::Guard& original_drain,
         std::shared_ptr<const ui::detail::BnwNormalNonFontMaterialLoan> original_material,
         std::array<std::shared_ptr<const ui::detail::BnwNormalNonFontResourceLoan>, 4> original_resources,
         detail::BnwNonFontNativeRecordProjection original_projection)
        : item(original_item), drain(original_drain), material(std::move(original_material)),
          resources(std::move(original_resources)), projection(std::move(original_projection)) {}

    void first() const {
        item.require_physical_current_without_callbacks();
        drain.require_physical_current_without_callbacks();
        drain.require_same_item_without_callbacks(item);
        if (!material) {
            throw std::logic_error("nonFont projection: original material retired");
        }
        material->require_physical_current_without_callbacks();
        material->require_same_item_without_callbacks(item);
        const auto material_item = material->captured_item_without_callbacks();
        item.require_same_item_without_callbacks(material_item);
        if (projection.registration_dword00_index_ != item.index_without_callbacks()) {
            throw std::logic_error("nonFont projection: different issued record index");
        }
        for (const auto& resource : resources) {
            if (resource) resource->require_physical_without_callbacks();
        }
    }

    void full(const runtime::BnwWorldViewSourceOperationLoan& operation) const {
        first();
        item.require_source_owned(operation);
        first();
        drain.require_source_owned(operation);
        first();
        material->require_source_owned(operation);
        first();
        for (const auto& resource : resources) {
            if (resource) {
                first();
                resource->require_source_owned(operation);
                first();
            }
        }
        first();
    }
};

GlBnwNonFontPipeline::PreparedNativeRecord::PreparedNativeRecord(
    std::shared_ptr<const Impl> original_control) : impl_(std::move(original_control)) {}
GlBnwNonFontPipeline::PreparedNativeRecord::~PreparedNativeRecord() = default;

void GlBnwNonFontPipeline::PreparedNativeRecord::require_selected_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation) const {
    // The physical control is owned before a supplier can call Source callbacks.
    const auto original = impl_;
    if (!original) throw std::logic_error("nonFont projection: missing original control");
    original->full(operation);
}

std::unique_ptr<const GlBnwNonFontPipeline::PreparedNativeRecord>
GlBnwNonFontPipeline::prepare_selected_native_record_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const ui::detail::BnwNormalNonFontItemOccurrence::Guard& item,
    const ui::detail::BnwNormalNonFontDrainPermission::Guard& drain,
    const std::shared_ptr<const ui::detail::BnwNormalNonFontMaterialLoan>& material) {
    // Capture controls and the SAME original Material facade before any callback.
    const auto saved_item = item;
    const auto saved_drain = drain;
    const auto saved_material = material;
    if (!saved_material) throw std::logic_error("nonFont projection: missing material owner");
    saved_item.require_physical_current_without_callbacks();
    saved_drain.require_physical_current_without_callbacks();
    saved_drain.require_same_item_without_callbacks(saved_item);
    saved_material->require_physical_current_without_callbacks();
    saved_material->require_same_item_without_callbacks(saved_item);
    const auto material_item = saved_material->captured_item_without_callbacks();
    saved_item.require_same_item_without_callbacks(material_item);
    const auto saved_resources = saved_item.resources_owned_without_callbacks();

    const auto first = [&] {
        saved_item.require_physical_current_without_callbacks();
        saved_drain.require_physical_current_without_callbacks();
        saved_drain.require_same_item_without_callbacks(saved_item);
        saved_material->require_physical_current_without_callbacks();
        saved_material->require_same_item_without_callbacks(saved_item);
        saved_item.require_same_item_without_callbacks(material_item);
        for (const auto& resource : saved_resources) {
            if (resource) resource->require_physical_without_callbacks();
        }
    };
    const auto full = [&] {
        first();
        saved_item.require_source_owned(operation);
        first();
        saved_drain.require_source_owned(operation);
        first();
        saved_material->require_source_owned(operation);
        first();
        for (const auto& resource : saved_resources) {
            if (resource) {
                first();
                resource->require_source_owned(operation);
                first();
            }
        }
        first();
    };

    full();
    const auto saved_payload = saved_item.payload_without_callbacks();
    const auto saved_coverage = saved_item.write_coverage_without_callbacks();
    const auto saved_descriptor = saved_item.descriptor_without_callbacks();
    const auto saved_registration = saved_item.registration_without_callbacks();
    first();
    auto projection = detail::BnwNonFontNativeRecordProjection::decode_without_callbacks(
        saved_payload, saved_coverage, saved_descriptor, saved_registration);
    if (projection.registration_dword00_index_ != saved_item.index_without_callbacks()) {
        throw std::logic_error("nonFont projection: registration index does not match item");
    }
    full();
    const auto control = std::make_shared<PreparedNativeRecord::Impl>(
        saved_item, saved_drain, saved_material, saved_resources, std::move(projection));
    first();
    control->full(operation);
    auto prepared = std::unique_ptr<const PreparedNativeRecord>(
        new PreparedNativeRecord(control));
    first();
    control->full(operation);
    // No native table lookup, codec admission, GL effect, queue advance or Draw
    // completion is performed. All selected suppliers remain genuine external bodies.
    return prepared;
}

} // namespace civ5::graphics
