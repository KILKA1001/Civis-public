#pragma once
#include "civ5/content/rules_database.hpp"
#include <array>
#include <span>

namespace civ5::runtime {
// VP's ID remapper assigns MinorCivilizations.ID in traversal order. The
// stable Type and other columns describe the same rules across activations.
inline std::span<const content::RulesFingerprintNormalization>
vp_rules_fingerprint_normalizations(){
    static constexpr std::array<content::RulesFingerprintNormalization,1> rules{{
        {"MinorCivilizations","Type","ID"}
    }};
    return rules;
}
} // namespace civ5::runtime
