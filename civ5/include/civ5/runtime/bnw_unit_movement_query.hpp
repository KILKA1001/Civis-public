#pragma once
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/content/info_types.hpp"
namespace civ5::runtime {
inline constexpr std::array<BnwUnitOrderSourceFile,6> kBnwUnitMovementSources{{
    {"CvGameCoreDLL_Expansion2/CvUnit.cpp","646e6c26ffb5ca20b77320d1bb7d78af440072eec210a760c2c76275f5e80b82"},
    {"CvGameCoreDLL_Expansion2/CvTeam.cpp","4aacb3b0c3b96e97d80d215155341752417ce962a102f6bc8916e6d04bb723cc"},
    {"CvGameCoreDLL_Expansion2/CvTraitClasses.cpp","8e5c3fe9ad40e44e2ad9a16e9ea590ff13ced2b51925f96b280495d82fec8edc"},
    {"CvGameCoreDLL_Expansion2/CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"},
    {"CvGameCoreDLL_Expansion2/CvUnitClasses.cpp","c9695bae81d1d8f67c8e8acfd41e6f1e19aa76d8d51d4ab36883fbb190c1db36"},
    {"CvGameCoreDLLUtil/include/CvEnums.h","6781f34a6a6a9fac1cbaad3a3477afabf540669af9f860f57ad54b967c22da9b"}
}};
struct BnwUnitMovementTerms {
    std::int64_t unit_type=-1,domain=-1,unit_combat=-1,team=-1;
    std::int64_t info_moves=0,extra_moves=0,team_extra_moves=0,naval_moves=0;
    std::int64_t golden_age_moves=0,combat_trait_moves=0,denominator=0;
    bool golden_age=false;
};
struct BnwUnitMovementQuote {
    std::optional<std::int32_t> base_moves,max_moves;
    std::optional<bool> has_moved;
    BnwUnitMovementTerms terms;
    std::string blocker;
};
// Actual current-world default-domain baseMoves/maxMoves/hasMoved. Units info
// comes from the same selected rules, other operands from the fields read by
// native getters. No m_iMoves0 inference, snapshot memo or state mutation.
// Great-General stack movement and embarked policy modifier are explicit
// unsupported branches until their actual owners are implemented.
BnwUnitMovementQuote quote_bnw_unit_movement(const LiveSessionState &,const content::RulesDatabase &,
    const content::InfoTypeCatalog &,const SessionCheckpointIdentity &,BnwUnitOrderKey);
} // namespace civ5::runtime
