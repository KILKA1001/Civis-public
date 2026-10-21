#pragma once
#include "civ5/runtime/bnw_fresh_feature_setter.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

// This is a Civis headless observation record, not an emulation of the
// closed-engine ICvDLLUtility::GameplayFeatureChanged implementation.
class BnwFreshFeatureChangedEvent {
public:
    BnwFreshFeatureChangedEvent(std::uint64_t sequence, std::size_t plot_index,
        BnwSetupPlot before, std::int32_t old_feature,
        std::int32_t new_feature, std::int32_t variety,
        const content::RulesDatabase *source_rules,
        std::string rules_sha256, std::string map_source_sha256);

    std::uint64_t sequence() const noexcept { return sequence_; }
    std::size_t plot_index() const noexcept { return plot_index_; }
    const BnwSetupPlot &before() const noexcept { return before_; }
    std::int32_t old_feature() const noexcept { return old_feature_; }
    std::int32_t new_feature() const noexcept { return new_feature_; }
    std::int32_t variety() const noexcept { return variety_; }
    const content::RulesDatabase *source_rules() const noexcept { return source_rules_; }
    std::string_view rules_sha256() const noexcept { return rules_sha256_; }
    std::string_view map_source_sha256() const noexcept { return map_source_sha256_; }

private:
    const std::uint64_t sequence_;
    const std::size_t plot_index_;
    const BnwSetupPlot before_;
    const std::int32_t old_feature_, new_feature_, variety_;
    const content::RulesDatabase *const source_rules_;
    const std::string rules_sha256_, map_source_sha256_;
};

enum class BnwFreshFeatureEventAck {
    callback_effect_unverified,
    invalid_sequence
};

// A detached map transaction stages notifications at the SDK call position,
// before changing m_eFeatureType. No consumer may acknowledge or publish one
// until the engine callback's observable effects have been established. A
// future verified consumer must durably acknowledge increasing sequence IDs
// only after the matching world transaction commits; an unacknowledged ID is
// replayed from the same immutable snapshot. Map source SHA is supplied by
// the selected VFS caller; the journal verifies selected rules bytes itself.
class BnwFreshFeatureEventJournal {
public:
    explicit BnwFreshFeatureEventJournal(std::size_t maximum_events);
    const BnwFreshFeatureChangedEvent &stage_prewrite(
        const BnwFreshFeatureSetPreflight &plan,
        const BnwSetupPlotGrid &grid,
        std::span<const std::int32_t> feature_types,
        const BnwFreshFeatureSetInfo &info,
        std::string_view rules_sha256,
        std::string_view map_source_sha256);
    std::span<const BnwFreshFeatureChangedEvent> replay_from(
        std::uint64_t next_sequence) const;
    BnwFreshFeatureEventAck acknowledge_headless(
        std::uint64_t sequence) const noexcept;
    void rollback() noexcept;
    std::size_t size() const noexcept { return events_.size(); }

private:
    std::size_t maximum_events_;
    std::vector<BnwFreshFeatureChangedEvent> events_;
};

} // namespace civ5::runtime
