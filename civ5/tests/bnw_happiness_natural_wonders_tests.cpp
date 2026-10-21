#include "civ5/runtime/bnw_happiness_natural_wonders.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool ok,const char *why) {if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F &&f,const char *why) {try{f();}catch(const std::exception &){return;}throw std::runtime_error(why);}
void trait(LiveSessionState &state,std::int64_t player,std::string_view name,std::int64_t value) {
    for(auto &field:state.world.all_players->players.at(player).traits.fields)
        if(field.name && std::string_view(field.name)==name) {field.values={value};return;}
    throw std::runtime_error("Wonder trait fixture is absent");
}
void discovered(LiveSessionState &state,std::int64_t player,std::int64_t value) {
    const auto team=state.setup.players.at(player).team;
    for(auto &field:state.world.teams->teams.at(team).fields)
        if(field.name && std::string_view(field.name)=="natural_wonders") {field.value=value;return;}
    throw std::runtime_error("Wonder discovery fixture is absent");
}
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected selected save, installation and optional SDK source root");
    if(argc==4) {
        const auto root=std::filesystem::path(argv[3])/"CvGameCoreDLL_Expansion2";
        check(identify_file(root/"CvPlayer.cpp").sha256=="9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d","Wonder CvPlayer source pin differs");
        check(identify_file(root/"CvTeam.cpp").sha256=="4aacb3b0c3b96e97d80d215155341752417ce962a102f6bc8916e6d04bb723cc","Wonder team source pin differs");
    }
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(static_cast<bool>(selected.preview),"Selected wonder bootstrap failed");auto &host=*selected.preview.host;
    const auto player=host.active_player_id();const auto &source=host.session().state();
    const auto rules=BnwNaturalWonderHappinessRules::load(host.database());
    const auto baseline=quote_bnw_happiness_from_natural_wonders(source,rules,host.identity(),player);
    check(rules.per_discovered_wonder()==1,"Stock happiness-per-wonder differs");
    // Ordinary owned features also count when their InBorderHappiness is positive.
    // This tests the actual source predicate rather than a NaturalWonder filter.
    auto database=host.database().clone_gameplay();database->apply_sql("UPDATE Features SET InBorderHappiness=3 WHERE Type='FEATURE_FOREST';");
    const auto altered=BnwNaturalWonderHappinessRules::load(*database);auto identity=host.identity();identity.rules_sha256=altered.rules_sha256();
    const auto catalog=database->cache_info_types();const auto *forest=catalog.find("FEATURE_FOREST");check(forest,"Forest fixture identity absent");
    auto state=source;for(auto &plot:state.world.map_projection->cells)plot.owner=-1;
    auto &cell=state.world.map_projection->cells.front();cell.owner=player;cell.feature_type.reset();cell.feature_type_hash=forest->hash;
    discovered(state,player,2);trait(state,player,"m_iNaturalWonderHappinessModifier",50);trait(state,player,"m_iNaturalWonderYieldModifier",50);
    auto q=quote_bnw_happiness_from_natural_wonders(state,altered,identity,player);
    check(q.discovered_base==2 && q.discovered_after_trait==3 && q.owned_features==4 && q.total==7 &&
          q.owned_positive_features.size()==1,"Separate discovery/owned-feature modifiers or native truncation differ");
    cell.feature_type=forest->id;cell.feature_type_hash.reset();
    check(quote_bnw_happiness_from_natural_wonders(state,altered,identity,player).total==7,"Numeric/hash native feature identities differ");
    trait(state,player,"m_iNaturalWonderHappinessModifier",-50);trait(state,player,"m_iNaturalWonderYieldModifier",-50);
    check(quote_bnw_happiness_from_natural_wonders(state,altered,identity,player).total==5,"Nonpositive modifiers did not retain native base values");
    cell.owner=(player+1)%63;cell.feature_type.reset();cell.feature_type_hash=0x12345678U;
    check(quote_bnw_happiness_from_natural_wonders(state,altered,identity,player).total==2,"Foreign plot was resolved or included before owner predicate");
    cell.owner=player;
    rejects([&]{quote_bnw_happiness_from_natural_wonders(state,altered,identity,player);},"Unknown owned feature accepted");
    cell.feature_type_hash=0;
    check(quote_bnw_happiness_from_natural_wonders(state,altered,identity,player).total==2,"Native NO_FEATURE hash gave owned happiness");
    cell.feature_type=forest->id;cell.feature_type_hash=forest->hash;
    rejects([&]{quote_bnw_happiness_from_natural_wonders(state,altered,identity,player);},"Ambiguous owned feature accepted");
    cell.feature_type.reset();cell.feature_type_hash=forest->hash;cell.owner.reset();
    rejects([&]{quote_bnw_happiness_from_natural_wonders(state,altered,identity,player);},"Unknown plot owner accepted");
    cell.owner=player;trait(state,player,"m_iNaturalWonderHappinessModifier",INT32_MAX);
    rejects([&]{quote_bnw_happiness_from_natural_wonders(state,altered,identity,player);},"Native wonder boost overflow accepted");
    rejects([&]{quote_bnw_happiness_from_natural_wonders(source,altered,host.identity(),player);},"Wrong selected rules accepted");
    check(quote_bnw_happiness_from_natural_wonders(source,rules,host.identity(),player).total==baseline.total &&
          !host.session().readiness().can_continue(),"Read-only wonder query changed source/readiness");
    std::cout<<"PASS selected wonder happiness: discovered="<<baseline.discovered_count<<", total="<<baseline.total
        <<"; owned feature predicate, separate trait modifiers, native truncation and malformed states\n";
}
