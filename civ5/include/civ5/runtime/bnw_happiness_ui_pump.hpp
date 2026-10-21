#pragma once
#include "civ5/runtime/bnw_happiness_update_suffix.hpp"
namespace civ5::runtime {
struct BnwHappinessUiPumpExecution {
    // Takes ownership of the SAME staged VM/world/client. A callback failure
    // discards the consumed transaction; no original world/client is changed.
    std::unique_ptr<BnwHappinessSuffixExecution> staged;
    // Before any event dispatch, a refusal retains the SAME accepted native
    // transaction for recovery. It is not a delivered UI receipt. A failure
    // after dispatch still discards the consumed, nonrepeatable attempt.
    std::unique_ptr<BnwHappinessSuffixExecution> pending;
    std::uint64_t listeners=0;
    bool dispatch_attempted=false;
    std::string frontier;
};
// Standalone Civis scheduling contract: native setDirty records an owned dirty
// packet; an explicit later client UI pump consumes it once. This is not proof
// of proprietary original engine timing. Full selected context/listener/loading
// and escrow verification is mandatory before real emit_ui_event. No extra VM
// fork, native happiness calculation, or gameplay-world publication occurs.
BnwHappinessUiPumpExecution pump_bnw_happiness_game_data(
    BnwHappinessSuffixExecution &&,const content::RulesDatabase &);
}
