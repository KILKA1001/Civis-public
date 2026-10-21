#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace civ5::runtime {
class LiveSession;

// Detached diagnostic values, never an action, persistence or readiness grant.
// Keys are stable escaped JSON-pointer paths. Values are complete JSON tokens.
struct NormalisedBnwState {
    static constexpr unsigned schema_version=1;
    std::map<std::string,std::string,std::less<>> values;
    std::map<std::string,std::string,std::less<>> unresolved;
    bool complete=false;
    std::string json() const;
};
struct NormalisedBnwChange {
    std::string path;
    std::optional<std::string> before,after;
};
struct NormalisedBnwDiff {
    std::vector<NormalisedBnwChange> values,unresolved;
    bool known_values_equal=false;
    // Equal partial projections cannot establish restart or original parity.
    bool complete_comparison=false;
    std::string json() const;
};

// Synchronous owner-thread call with no concurrent mutation of the session.
// No callbacks, query side effects, RNG draws, reset, cache rebuild or rules
// activation run here. The genuine state allocation is pinned and currentness
// is checked before/after. This does not enforce a new gameplay/thread owner.
NormalisedBnwState normalise_bnw_state(const LiveSession&);
NormalisedBnwDiff diff_normalised_bnw_state(const NormalisedBnwState&,const NormalisedBnwState&);
} // namespace civ5::runtime
