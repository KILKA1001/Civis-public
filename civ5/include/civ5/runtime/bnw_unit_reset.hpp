#pragma once
#include "civ5/runtime/bnw_unit_movement_query.hpp"
namespace civ5::runtime {
inline constexpr std::array<BnwUnitOrderSourceFile,3> kBnwUnitResetSources{{
 {"CvGameCoreDLL_Expansion2/CvGameCoreUtils.h","8fbfd757ebfbbbbcce3bf260bd423598385a30733c43923c37624b02e9290f26"},
 {"CvGameCoreDLL_Expansion2/CvPlot.cpp","e0723e57acb5536d7d12cac26500cca005d1710465f921eb0a852b138c39ca9e"},
 {"CvGameCoreDLL_Expansion2/CvImprovementClasses.cpp","66ace0219a214f9f6ac9de1b92b37ed2d9381c3e1c5c9b37b1ab782323354ef7"}
}};
struct BnwEnemyCitadelQuote {
 std::optional<bool> near_enemy_citadel;
 std::optional<std::int32_t> damage;
 std::optional<std::uint64_t> damaging_plot;
 std::vector<std::uint64_t> visited_plots;
 std::string blocker;
};
// Source iX outer/iY inner hex loop, first damaging improvement wins. Reads
// current raw/projected map and only reaches owner/war when damage is nonzero.
BnwEnemyCitadelQuote quote_bnw_near_enemy_citadel(const LiveSessionState &,const content::RulesDatabase &,
 const content::InfoTypeCatalog &,const SessionCheckpointIdentity &,BnwUnitOrderKey);
struct BnwHumanUnitResetDraft {
 BnwEndTurnRequestDraft request;
 BnwUnitPresentationOwner presentation;
 BnwUnitPresentationBatch batch;
 std::vector<BnwUnitOrderKey> reset_units;
};
struct BnwHumanUnitResetExecution {std::unique_ptr<BnwHumanUnitResetDraft> draft;std::string blocker;};
// Exact bounded DoUnitReset component on a private current-human EndTurn draft
// whose SAME retained owner has completed automoves. Does not deactivate,
// publish a full turn, run notifications or execute DoTurn. Healing, positive
// citadel damage and archaeologist culture effects require actual owners.
BnwHumanUnitResetExecution execute_bnw_human_unit_reset_draft(const BnwEndTurnRequestDraft &,
 const BnwEndTurnInputBinding &,const BnwUnitPresentationOwner &,const content::RulesDatabase &,
 const content::InfoTypeCatalog &,std::int64_t player);
namespace bnw_unit_reset_detail {
// Internal SetTurnActive prefix entry: original_active is validated before
// flag change; current input must be the same unit epoch with active=false.
// Current serialized blocking fields must already be cleared by the caller.
// Returns no valid active-human input revision. No live publication.
BnwHumanUnitResetExecution execute_after_turn_flag_draft(const BnwEndTurnRequestDraft &original_active,
 const BnwEndTurnRequestDraft &current,const BnwEndTurnInputBinding &,const BnwUnitPresentationOwner &,
 const content::RulesDatabase &,const content::InfoTypeCatalog &,std::int64_t);
}
} // namespace civ5::runtime
