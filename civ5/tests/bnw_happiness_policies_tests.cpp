#include "civ5/runtime/bnw_happiness_policies.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <stdexcept>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool ok,const char *why) {if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F &&f,const char *why) {try{f();}catch(const std::exception &){return;}throw std::runtime_error(why);}
auto &named(auto &rows,std::string_view name) {
    for(auto &row:rows)if(row.name && name==row.name)return row;
    throw std::runtime_error("Missing policy fixture field");
}
void city(LiveSessionState &state,std::int64_t player,std::int64_t id,std::int64_t population,bool puppet) {
    saves::SavedCityObservation raw{};raw.core.layout=saves::GameStateLayout::expansion2_version1;
    raw.core.start_offset=400+id*100;raw.core.end_offset=raw.core.start_offset+50;
    for(const auto &[name,value]:std::vector<std::pair<const char *,std::int64_t>>{
        {"m_iID",id},{"m_eOwner",player},{"m_iPopulation",population},{"m_bPuppet",puppet}}) {
        saves::CityCoreField f{};f.name=name;f.encoding=saves::CityCoreEncoding::i32;f.values={value};raw.core.fields.push_back(f);
    }
    CityYieldState live;live.owner=player;live.id=id;live.population=population;live.puppet=puppet;
    live.source_core=std::pair{raw.core.start_offset,raw.core.end_offset};
    state.cities.players.at(player).cities.emplace(id,live);
    auto &collection=state.world.all_players->players.at(player).cities;
    collection.cities.push_back(raw);collection.header.wire_city_count=collection.cities.size();
}
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected selected save, installation and optional SDK source root");
    if(argc==4) {
        const auto root=std::filesystem::path(argv[3])/"CvGameCoreDLL_Expansion2";
        check(identify_file(root/"CvPlayer.cpp").sha256=="9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d","Policy happiness player source pin differs");
        check(identify_file(root/"CvPolicyClasses.cpp").sha256=="3fe2aca3c99961746d60346913fa59f72315e323285f8594f1cbed22f4eddb5d","Policy class source pin differs");
    }
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(static_cast<bool>(selected.preview),"Selected policy bootstrap failed");auto &host=*selected.preview.host;
    const auto player=host.active_player_id();const auto &source=host.session().state();
    const auto rules=BnwPolicyHappinessRules::load(host.database());
    const auto baseline=quote_bnw_happiness_from_policies(source,rules,host.identity(),player);
    for(const auto &[id,cache]:source.setup.players) {
        const auto &block=named(source.world.all_players->players.at(id).strategy.blocks,"policies");
        const auto flags=rules.flags(named(block.hashes,"has_policy"),false);std::set<std::int64_t> expected;
        for(std::size_t slot=0;slot<flags.size();++slot)if(flags[slot])expected.insert(static_cast<std::int64_t>(slot));
        check(cache.policies_resolved && cache.policies==expected,"Actual imported shared policy membership differs");
    }
    auto cold=restore_research_context(source.world,host.database());
    auto malformed=source.world;
    named(named(malformed.all_players->players.back().strategy.blocks,"policies").hashes,"has_policy").rows.push_back({0x12345678,{1}});
    auto &badflags=named(named(malformed.all_players->players.back().strategy.blocks,"policies").hashes,"has_policy");++badflags.wire_count;
    rejects([&]{restore_bnw_policy_membership(malformed,cold,rules);},"Late malformed player policy import accepted");
    for(const auto &[id,cache]:cold.players)check(!cache.policies_resolved && cache.policies.empty(),"Late policy import mutated earlier owners");
    restore_bnw_policy_membership(source.world,cold,rules);
    rejects([&]{restore_bnw_policy_membership(source.world,cold,rules);},"Import reset an already owned live policy cache");
    auto database=host.database().clone_gameplay();
    database->apply_sql("UPDATE Policies SET ExtraHappiness=0,ExtraHappinessPerCity=0;"
        "UPDATE Policies SET ExtraHappiness=7,ExtraHappinessPerCity=2,PolicyBranchType=NULL WHERE Type='POLICY_TRADITION';"
        "UPDATE Policies SET ExtraHappiness=5,ExtraHappinessPerCity=3 WHERE Type='POLICY_OLIGARCHY';"
        "UPDATE Policies SET ExtraHappiness=11,ExtraHappinessPerCity=4,PolicyBranchType=NULL WHERE Type='POLICY_TRADITION_FINISHER';");
    const auto altered=BnwPolicyHappinessRules::load(*database);auto identity=host.identity();identity.rules_sha256=altered.rules_sha256();
    const auto types=database->cache_info_types();const auto lookup=[&](const char *name)->const content::InfoTypeEntry & {
        const auto *entry=types.find(name);check(entry,"Fixture policy/branch absent");return *entry;
    };
    const auto &opener=lookup("POLICY_TRADITION");const auto &ordinary=lookup("POLICY_OLIGARCHY");
    const auto &finisher=lookup("POLICY_TRADITION_FINISHER");const auto &branch=lookup("POLICY_BRANCH_TRADITION");
    check(altered.policies().at(opener.id).blocked_branch==branch.id && altered.policies().at(ordinary.id).blocked_branch==branch.id &&
          altered.policies().at(finisher.id).blocked_branch==-1,"Native reset free opener/finisher blocked branch cache differs");
    auto state=source;auto &raw=state.world.all_players->players.at(player);auto &policies=named(raw.strategy.blocks,"policies");
    auto &owned=named(policies.hashes,"has_policy");owned.rows={{opener.hash,{1}},{ordinary.hash,{1}},{finisher.hash,{1}},{0,{}}};owned.wire_count=owned.rows.size();
    state.setup.players.at(player).policies={opener.id,ordinary.id,finisher.id};
    auto &blocked=named(policies.hashes,"branch_blocked");blocked.rows={{branch.hash,{0}}};blocked.wire_count=blocked.rows.size();
    raw.cities.cities.clear();state.cities.players.at(player).cities.clear();raw.cities.header.wire_city_count=0;
    city(state,player,3,11,false);city(state,player,5,7,false);city(state,player,8,40,true);
    named(raw.core.fields,"m_iHappinessPerXPopulation").values={5};
    auto q=quote_bnw_happiness_from_policies(state,altered,identity,player);
    check(q.owned_policies==3 && q.active_policies==3 && q.flat==23 && q.from_city_count==27 && q.from_population==3 &&
          q.population_terms.size()==2 && q.total==53,"Native policy sums/city count/per-city population truncation differ");
    blocked.rows[0].values={1};q=quote_bnw_happiness_from_policies(state,altered,identity,player);
    check(q.active_policies==1 && q.flat==11 && q.from_city_count==12 && q.total==26,"Blocked free opener or active finisher differs");
    owned.rows.push_back({finisher.hash,{0}});owned.wire_count=owned.rows.size();
    state.setup.players.at(player).policies.erase(finisher.id);
    check(quote_bnw_happiness_from_policies(state,altered,identity,player).total==3,"Native repeated hash last value differs");
    named(raw.core.fields,"m_iHappinessPerXPopulation").values={-5};
    check(quote_bnw_happiness_from_policies(state,altered,identity,player).total==0,"Nonpositive population divisor entered city loop");
    named(raw.core.fields,"m_iHappinessPerXPopulation").values={5};
    auto wrong=state;named(named(wrong.world.all_players->players.at(player).strategy.blocks,"policies").hashes,"has_policy").rows[0].values={2};
    rejects([&]{quote_bnw_happiness_from_policies(wrong,altered,identity,player);},"Nonboolean owned flag accepted");
    wrong=state;named(named(wrong.world.all_players->players.at(player).strategy.blocks,"policies").hashes,"has_policy").rows[0].type_hash=0x12345678;
    rejects([&]{quote_bnw_happiness_from_policies(wrong,altered,identity,player);},"Unknown policy identity accepted");
    wrong=state;wrong.setup.players.at(player).policies.clear();
    rejects([&]{quote_bnw_happiness_from_policies(wrong,altered,identity,player);},"Stale shared policy owner accepted");
    wrong=state;wrong.setup.players.at(player).policies_resolved=false;
    rejects([&]{quote_bnw_happiness_from_policies(wrong,altered,identity,player);},"Unresolved shared policy owner accepted");
    wrong=state;wrong.cities.players.at(player).cities.at(3).population=12;
    rejects([&]{quote_bnw_happiness_from_policies(wrong,altered,identity,player);},"Stale population binding accepted");
    wrong=state;wrong.world.all_players->players.at(player).cities.header.wire_city_count=2;
    rejects([&]{quote_bnw_happiness_from_policies(wrong,altered,identity,player);},"Incomplete city collection accepted");
    wrong=state;named(named(wrong.world.all_players->players.at(player).strategy.blocks,"policies").fields,"version").value=3;
    rejects([&]{quote_bnw_happiness_from_policies(wrong,altered,identity,player);},"Unknown saved policy version accepted");
    rejects([&]{quote_bnw_happiness_from_policies(source,altered,host.identity(),player);},"Different rules accepted");
    auto overflow_db=database->clone_gameplay();overflow_db->apply_sql("UPDATE Policies SET ExtraHappiness=2147483647,ExtraHappinessPerCity=0;");
    const auto overflow=BnwPolicyHappinessRules::load(*overflow_db);auto overflow_identity=identity;overflow_identity.rules_sha256=overflow.rules_sha256();
    blocked.rows[0].values={0};
    rejects([&]{quote_bnw_happiness_from_policies(state,overflow,overflow_identity,player);},"Native int sum overflow accepted");
    check(quote_bnw_happiness_from_policies(source,rules,host.identity(),player).total==baseline.total && !host.session().readiness().can_continue(),
          "Read-only policy query changed source/readiness");
    std::cout<<"PASS selected policy happiness="<<baseline.total<<"; source branch opener cache, blocked policies, per-city and nonpuppet population terms, malformed states\n";
}
