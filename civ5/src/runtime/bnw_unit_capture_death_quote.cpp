#include "civ5/runtime/bnw_unit_capture_death_quote.hpp"
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *s) {
    return s?std::string_view(s):std::string_view();
}
std::int64_t core(const saves::SavedUnitObservation &unit,
                  std::string_view wanted,saves::UnitCoreEncoding encoding) {
    const saves::UnitCoreField *found=nullptr;
    require(unit.core.layout==bnw,"BNW capture unit core unavailable");
    for(const auto &field:unit.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW capture unit core field");
            found=&field;
        }
    require(found && found->encoding==encoding && found->values.size()==1 &&
            !found->wire_count,
            "BNW capture unit core field unavailable");
    const auto value=found->values[0];
    require(value>=INT32_MIN && value<=INT32_MAX,
            "BNW capture unit core value outside source i32");
    if(encoding==saves::UnitCoreEncoding::boolean)
        require(value==0 || value==1,"BNW capture boolean invalid");
    return value;
}
const saves::UnitTailNode &node(const std::vector<saves::UnitTailNode> &nodes,
                                std::string_view wanted,std::string_view encoding) {
    const saves::UnitTailNode *found=nullptr;
    for(const auto &candidate:nodes)
        if(name(candidate.name)==wanted) {
            require(!found,"Duplicate BNW capture religion node");
            found=&candidate;
        }
    require(found && name(found->encoding)==encoding &&
            !found->wire_count && found->bytes.empty() &&
            found->values.empty(),
            "BNW capture religion node unavailable");
    return *found;
}
std::int64_t religion_scalar(const saves::UnitTailNode &parent,
                             std::string_view wanted,std::string_view encoding) {
    const saves::UnitTailNode *found=nullptr;
    for(const auto &candidate:parent.children)
        if(name(candidate.name)==wanted) {
            require(!found,"Duplicate BNW capture religion field");
            found=&candidate;
        }
    require(found && name(found->encoding)==encoding &&
            found->values.size()==1 && found->children.empty() &&
            found->bytes.empty() && !found->wire_count,
            "BNW capture religion field unavailable");
    const auto value=found->values[0];
    require(value>=INT32_MIN && value<=INT32_MAX,
            "BNW capture religion field outside source i32");
    return value;
}
} // namespace

BnwUnitCaptureDeathQuote quote_bnw_unit_capture_death_no_player(
    const saves::SavedSessionSnapshot &source,
    const BnwDeathXpQuote &xp,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules,
    const BnwDeathXpRules &xp_rules) {
    // Requote the preceding source-ordered stage to reject a stale unit/rules
    // binding. We still cannot claim that earlier SDK effects were executed.
    const auto check=quote_bnw_unit_death_xp(source,xp.support,
        class_rules,support_rules,xp_rules,xp.killer);
    require(check.unit_experience==xp.unit_experience &&
            check.loss==xp.loss && check.triggered==xp.triggered &&
            check.naval==xp.naval &&
            check.setter_branch_unported==xp.setter_branch_unported &&
            check.current_before==xp.current_before &&
            check.current_after==xp.current_after &&
            check.lifetime_before==xp.lifetime_before &&
            check.lifetime_after==xp.lifetime_after,
            "BNW capture preceding XP quote is stale");
    const auto &removal=xp.support.class_death.removal;
    const auto &unit=source.all_players->players[removal.player_index]
        .units.units[removal.unit_index];
    BnwUnitCaptureDeathQuote result;result.xp=xp;
    result.old_player=core(unit,"m_eOwner",saves::UnitCoreEncoding::i32);
    result.original_owner=core(unit,"m_eOriginalOwner",
        saves::UnitCoreEncoding::i32);
    result.old_type=core(unit,"m_eUnitType",saves::UnitCoreEncoding::i32);
    result.capturing_player=core(unit,"m_eCapturingPlayer",
        saves::UnitCoreEncoding::i32);
    result.embarked=core(unit,"m_bEmbarked",
        saves::UnitCoreEncoding::boolean)!=0;
    result.captured_as_is=core(unit,"m_bCapturedAsIs",
        saves::UnitCoreEncoding::boolean)!=0;
    result.x=core(unit,"m_iX",saves::UnitCoreEncoding::i32);
    result.y=core(unit,"m_iY",saves::UnitCoreEncoding::i32);
    require(result.old_player==removal.owner &&
            result.original_owner>=0 && result.original_owner<64 &&
            result.old_type>=0 &&
            result.capturing_player==-1 &&
            result.x>=0 && result.y>=0 &&
            source.map_header.grid.width>0 &&
            source.map_header.grid.height>0 &&
            static_cast<std::uint64_t>(result.x)<source.map_header.grid.width &&
            static_cast<std::uint64_t>(result.y)<source.map_header.grid.height &&
            static_cast<std::uint64_t>(result.y)*
                source.map_header.grid.width+
                static_cast<std::uint64_t>(result.x)==removal.plot_index,
            "BNW capture identity/NO_PLAYER branch unavailable");
    require(unit.tail.layout==bnw,
            "BNW capture unit religion tail unavailable");
    const auto &religion=node(unit.tail.blocks,"religion","record");
    require(religion_scalar(religion,"version","u32")==2,
            "BNW capture religion wire version unsupported");
    result.religion=religion_scalar(religion,"religion","i32");
    result.religious_strength=religion_scalar(religion,"strength","i32");
    result.spreads_left=religion_scalar(religion,"spreads_left","i32");
    require(result.religion>=-1 && result.religious_strength>=0 &&
            result.spreads_left>=0,
            "BNW capture religion values unsupported");
    // With NO_PLAYER, getCaptureDefinition returns false even if IsCapturedAsIs.
    result.capture_type=result.captured_as_is?result.old_type:-1;
    result.capture_valid=false;
    return result;
}
} // namespace civ5::runtime
