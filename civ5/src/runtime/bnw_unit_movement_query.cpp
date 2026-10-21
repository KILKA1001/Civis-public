#include "civ5/runtime/bnw_unit_movement_query.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <limits>
#include <stdexcept>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
std::int64_t native(std::int64_t value){need(value>=INT32_MIN && value<=INT32_MAX,"Unit movement operand exceeds native int");return value;}
template<class Core>std::int64_t raw_scalar(const Core &core,std::string_view name) {
    std::optional<std::int64_t> value;
    for(const auto &field:core.fields)if(field.name && std::string_view(field.name)==name){need(!value && field.values.size()==1,"Unknown/duplicate unit movement scalar");value=field.values[0];}
    need(value.has_value(),"Missing unit movement source scalar");return *value;
}
template<class Core>std::int64_t scalar(const Core &core,std::string_view name){return native(raw_scalar(core,name));}
const saves::PlayerTraitsField &trait(const saves::PlayerTraitsObservation &traits,std::string_view name) {
    need(traits.layout==bnw && traits.version && *traits.version>=1 && *traits.version<=19,"Unit movement trait cache layout is unknown");
    const saves::PlayerTraitsField *found=nullptr;
    for(const auto &field:traits.fields)if(field.name && std::string_view(field.name)==name){need(!found,"Duplicate unit movement trait cache");found=&field;}
    need(found,"Missing native unit movement trait cache");return *found;
}
std::int64_t trait_scalar(const saves::PlayerTraitsObservation &traits,std::string_view name) {
    const auto &field=trait(traits,name);need(field.encoding==saves::PlayerTraitsEncoding::i32 && field.values.size()==1,"Wrong native movement trait scalar shape");return native(field.values.front());
}
struct UnitInfo {std::int64_t moves=0,domain=-1,combat=-1;std::string type;};
UnitInfo load_info(const content::RulesDatabase &rules,const content::InfoTypeCatalog &types,const content::InfoTypeEntry &entry) {
    std::optional<UnitInfo> result;
    const auto query=rules.query_rows({"SELECT Type,Moves,Domain,CombatClass FROM Units WHERE ID=?",{entry.id}},[&](const content::RulesRowView &row){
        need(!result,"Duplicate unit movement info row");UnitInfo value;value.type=row.text("Type").value_or("");
        need(value.type==entry.type && content::saved_info_type_hash(value.type)==entry.hash,"Selected unit movement rules/hash identity differs");
        value.moves=native(row.integer("Moves"));const auto domain=row.text("Domain");need(domain.has_value(),"Selected movement domain is absent");
        const auto *domain_info=types.find(*domain);need(domain_info && domain_info->table=="Domains" && domain_info->id>=0 && domain_info->id<5,"Selected movement domain mapping unknown");
        constexpr std::array<std::string_view,5> names{"DOMAIN_SEA","DOMAIN_AIR","DOMAIN_LAND","DOMAIN_IMMOBILE","DOMAIN_HOVER"};
        need(domain_info->type==names.at(domain_info->id),"Selected movement domain IDs differ from source enums");value.domain=domain_info->id;
        const auto combat=row.text("CombatClass");if(combat && !combat->empty()){const auto *info=types.find(*combat);need(info && info->table=="UnitCombatInfos" && info->id>=0,"Selected movement combat class mapping unknown");value.combat=info->id;}
        result=value;
    },2);
    need(query.all_succeeded() && result.has_value(),"Selected native unit movement rules unavailable");return *result;
}
std::int64_t combat_trait(const saves::PlayerTraitsObservation &traits,const content::RulesDatabase &rules,std::int64_t combat) {
    if(combat==-1)return 0; // SDK NO_UNITCLASS branch returns before vector access.
    const auto &field=trait(traits,"m_paiMovesChangeUnitCombat");
    need(field.encoding==saves::PlayerTraitsEncoding::combat_i32 && field.wire_count && *field.wire_count>=0 &&
         static_cast<std::uint64_t>(*field.wire_count)==field.values.size(),"Native combat movement vector is incomplete");
    std::int64_t count=0;const auto classes=rules.query_rows({"SELECT ID FROM UnitCombatInfos ORDER BY ID",{}},[&](const auto &row){need(row.integer_at(0)==count,"Native combat movement class IDs are not dense");++count;});
    need(classes.all_succeeded() && count==*field.wire_count && combat>=0 && combat<count,"Native movement trait vector/source rules span differs");
    return native(field.values.at(combat));
}
}
BnwUnitMovementQuote quote_bnw_unit_movement(const LiveSessionState &state,const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,const SessionCheckpointIdentity &identity,BnwUnitOrderKey key) {
    BnwUnitMovementQuote result;
    try {
        (void)bnw_unit_order_world_revision(state,identity,rules);(void)bnw_unit_order_plot(state,key);
        need(types.report().all_succeeded() && state.world.teams && state.world.teams->layout==bnw &&
             state.world.teams->teams.size()==64 && state.world.teams->dimensions.domains==5,"Native unit movement team/rules roster is unknown");
        const auto &player=state.world.all_players->players.at(key.first);const saves::SavedUnitObservation *unit=nullptr;
        for(const auto &candidate:player.units.units)if(scalar(candidate.core,"m_iID")==key.second){need(!unit,"Duplicate native movement unit");unit=&candidate;}
        need(unit && scalar(unit->core,"m_eOwner")==key.first,"Native movement unit owner is unknown");
        const auto great_general=scalar(unit->core,"m_iGreatGeneralCount");
        need(great_general<=0,"Native maxMoves requires actual GetGreatGeneralStackMovement owner");
        const auto hash=raw_scalar(unit->core,"idUnitType");need(hash>=0 && hash<=UINT32_MAX,"Native unit movement type hash is invalid");
        const auto *type=types.find_hash(hash);
        need(type && type->table=="Units" && state.maintenance.players.at(key.first).units.at(key.second).type==type->id,"Native movement unit info binding differs");
        const auto info=load_info(rules,types,*type);auto &terms=result.terms;terms.unit_type=type->id;terms.domain=info.domain;terms.unit_combat=info.combat;terms.info_moves=info.moves;
        terms.team=state.setup.players.at(key.first).team;need(terms.team>=0 && terms.team<64,"Native unit movement team is invalid");
        const auto &team=state.world.teams->teams.at(terms.team);need(team.index==static_cast<std::uint64_t>(terms.team),"Native unit movement team index differs");
        const saves::PlotArrayObservation *extra=nullptr;
        for(const auto &array:team.arrays)if(array.name && std::string_view(array.name)=="extra_moves"){need(!extra,"Duplicate native team movement array");extra=&array;}
        need(extra && extra->columns==1 && extra->values.size()==5,"Native team movement array incomplete");terms.team_extra_moves=native(extra->values.at(info.domain));
        const auto embarked=scalar(unit->core,"m_bEmbarked");need(embarked==0 || embarked==1,"Native embark state is unknown");
        need(!embarked,"Native embarked baseMoves requires actual POLICYMOD_EMBARKED_EXTRA_MOVES owner");
        terms.extra_moves=scalar(unit->core,"m_iExtraMoves");
        if(info.domain==0){terms.naval_moves=scalar(unit->core,"m_iExtraNavalMoves");
            if(scalar(unit->core,"m_iBaseCombat")==0)terms.naval_moves=native(terms.naval_moves+trait_scalar(player.traits,"m_iExtraEmbarkMoves"));}
        terms.golden_age=scalar(player.core,"m_iGoldenAgeTurns")>0;
        if(terms.golden_age)terms.golden_age_moves=trait_scalar(player.traits,"m_iGoldenAgeMoveChange");
        terms.combat_trait_moves=combat_trait(player.traits,rules,info.combat);
        auto base=native(terms.info_moves+terms.extra_moves);base=native(base+terms.team_extra_moves);base=native(base+terms.naval_moves);base=native(base+terms.golden_age_moves);base=native(base+terms.combat_trait_moves);
        const auto denominator=rules.global_define("MOVE_DENOMINATOR",content::GlobalDefineKind::integer);
        need(denominator.found && std::holds_alternative<std::int64_t>(denominator.value) && denominator.error.empty(),"Native MOVE_DENOMINATOR unavailable");
        terms.denominator=native(std::get<std::int64_t>(denominator.value));
        result.base_moves=static_cast<std::int32_t>(base);result.max_moves=static_cast<std::int32_t>(native(base*terms.denominator));
        result.has_moved=scalar(unit->core,"m_iMoves")<*result.max_moves;
    } catch(const std::exception &error){result.base_moves.reset();result.max_moves.reset();result.has_moved.reset();result.blocker=error.what();}
    return result;
}
} // namespace civ5::runtime
