#include "civ5/runtime/bnw_fresh_feature_events.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/rules_database.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
bool sha256_hex(std::string_view value) {
    return value.size() == 64 &&
        std::all_of(value.begin(), value.end(), [](char c) {
            return (c >= '0' && c <= '9') ||
                   (c >= 'a' && c <= 'f') ||
                   (c >= 'A' && c <= 'F');
        });
}
}

BnwFreshFeatureChangedEvent::BnwFreshFeatureChangedEvent(
    std::uint64_t sequence, std::size_t plot_index, BnwSetupPlot before,
    std::int32_t old_feature, std::int32_t new_feature, std::int32_t variety,
    const content::RulesDatabase *source_rules,
    std::string rules_sha256, std::string map_source_sha256)
    : sequence_(sequence), plot_index_(plot_index), before_(before),
      old_feature_(old_feature), new_feature_(new_feature), variety_(variety),
      source_rules_(source_rules), rules_sha256_(std::move(rules_sha256)),
      map_source_sha256_(std::move(map_source_sha256)) {}

BnwFreshFeatureEventJournal::BnwFreshFeatureEventJournal(
    std::size_t maximum_events) : maximum_events_(maximum_events) {
    if (!maximum_events)
        throw std::invalid_argument("Fresh feature event budget is invalid");
}

const BnwFreshFeatureChangedEvent &BnwFreshFeatureEventJournal::stage_prewrite(
    const BnwFreshFeatureSetPreflight &plan, const BnwSetupPlotGrid &grid,
    std::span<const std::int32_t> feature_types,
    const BnwFreshFeatureSetInfo &info,
    std::string_view rules_sha256, std::string_view map_source_sha256) {
    if (events_.size() >= maximum_events_)
        throw std::length_error("Fresh feature event budget exceeded");
    if (!sha256_hex(rules_sha256) || !sha256_hex(map_source_sha256) ||
        plan.blocker != BnwFreshFeatureSetBlocker::required_services_pending ||
        plan.plot_index >= grid.size() ||
        feature_types.size() != grid.size() ||
        feature_types[plan.plot_index] != -1 ||
        plan.new_feature_id < 0 || plan.new_feature_id != info.id ||
        !info.source_rules || plan.source_rules != info.source_rules ||
        !plan.before || *plan.before != grid.at(plan.plot_index) ||
        !std::all_of(feature_types.begin(), feature_types.end(),
                     [](std::int32_t value) { return value == -1; }) ||
        plan.effective_variety != 0 ||
        plan.source_order.size() < 2 ||
        plan.source_order[0] != BnwFreshFeatureSetStep::remove_empty_sight ||
        plan.source_order[1] !=
            BnwFreshFeatureSetStep::gameplay_feature_changed_pending)
        throw std::invalid_argument("Fresh feature event pre-write contract is incomplete");
    if (std::any_of(events_.begin(), events_.end(), [&](const auto &event) {
            return event.plot_index() == plan.plot_index;
        }))
        throw std::invalid_argument("Fresh feature pre-write event is duplicated");
    if (events_.empty()) {
        if (identify_bytes(info.source_rules->gameplay_fingerprint_data()).sha256 !=
            rules_sha256)
            throw std::invalid_argument("Fresh feature event selected rules hash differs");
    } else if (events_.front().source_rules() != info.source_rules ||
               events_.front().rules_sha256() != rules_sha256 ||
               events_.front().map_source_sha256() != map_source_sha256) {
        throw std::invalid_argument("Fresh feature event provenance changed within transaction");
    }
    const auto &plot = *plan.before;
    if (!plot.terrain_type_assigned_by_script || plot.terrain_type < 0 ||
        plot.area_id < 0 || plot.landmass_id < 0)
        throw std::invalid_argument("Fresh feature event plot state is incomplete");
    events_.emplace_back(static_cast<std::uint64_t>(events_.size()),
        plan.plot_index, plot, -1, plan.new_feature_id, 0,
        info.source_rules, std::string(rules_sha256),
        std::string(map_source_sha256));
    return events_.back();
}

std::span<const BnwFreshFeatureChangedEvent>
BnwFreshFeatureEventJournal::replay_from(std::uint64_t next_sequence) const {
    if (next_sequence > events_.size())
        throw std::out_of_range("Fresh feature event replay cursor is invalid");
    return std::span<const BnwFreshFeatureChangedEvent>(events_).subspan(
        static_cast<std::size_t>(next_sequence));
}

BnwFreshFeatureEventAck BnwFreshFeatureEventJournal::acknowledge_headless(
    std::uint64_t sequence) const noexcept {
    if (sequence >= events_.size())
        return BnwFreshFeatureEventAck::invalid_sequence;
    // The installed executable provides no source for the callback body.
    // Even an event recorded before the write cannot claim delivery of its
    // synchronous ICvPlot1 interactions after an atomic grid commit.
    return BnwFreshFeatureEventAck::callback_effect_unverified;
}

void BnwFreshFeatureEventJournal::rollback() noexcept {
    events_.clear();
}

} // namespace civ5::runtime
