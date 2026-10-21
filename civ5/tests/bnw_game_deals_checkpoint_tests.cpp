#include "civ5/runtime/bnw_game_deals_checkpoint.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace civ5;
void check(bool valid,const char *why) {
    if(!valid) throw std::runtime_error(why);
}
template<class F> void rejects(F &&run) {
    bool failed=false;
    try {run();} catch(const std::exception &) {failed=true;}
    check(failed,"Invalid BNW deals checkpoint was accepted");
}
saves::TradedItemObservation item(std::optional<std::uint64_t> version) {
    saves::TradedItemObservation value;
    value.version=version;
    value.type=7;
    value.duration=20;
    value.final_turn=25;
    value.from_player=0;
    value.data={3,4,0};
    if(version && *version==2) {
        value.data[2]=5;
        value.flag1=true;
    }
    value.from_renewed=false;
    value.to_renewed=false;
    return value;
}
saves::DealObservation deal(std::uint64_t version) {
    saves::DealObservation value;
    value.version=version;
    value.from_player=0;
    value.to_player=1;
    value.final_turn=25;
    value.duration=20;
    value.start_turn=5;
    value.considering_renewal=true;
    value.checked_renewal=version==3;
    value.cancelled=false;
    value.peace_treaty=-1;
    value.surrendering_player=-1;
    value.demanding_player=-1;
    value.requesting_player=-1;
    value.items.push_back(item(version==1?
        std::nullopt:std::optional<std::uint64_t>(version==2?1:2)));
    return value;
}
saves::SavedSessionSnapshot fixture() {
    saves::SavedSessionSnapshot world;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    world.game_state.layout=bnw;
    world.history.layout=bnw;
    world.history.deals.version=1;
    world.history.deals.groups[0].push_back(deal(1));
    world.history.deals.groups[1].push_back(deal(2));
    world.history.deals.groups[2].push_back(deal(3));
    world.all_players.emplace();
    world.all_players->layout=bnw;
    world.all_players->context.player_count=64;
    world.all_players->players.resize(64);
    for(std::size_t p=0;p<64;++p) {
        world.all_players->players[p].index=p;
        world.all_players->players[p].saved_player_id=
            static_cast<std::int64_t>(p);
    }
    return world;
}
void synthetic() {
    const runtime::SessionCheckpointIdentity identity{
        std::string(64,'a'),std::string(64,'b')};
    const auto source=fixture();
    auto live=source;
    auto &groups=live.history.deals.groups;
    groups[0].push_back(deal(3));
    groups[0].back().items.push_back(item(2));
    groups[1].clear();
    groups[2][0].items[0].data[2]=44;
    groups[2][0].items[0].to_renewed=true;
    const auto section=runtime::make_bnw_game_deals_checkpoint(
        live,source,identity);
    check(section.id==runtime::bnw_game_deals_checkpoint_section_id &&
          section.flags==0,"BNW deals section identity changed");
    auto restored=source;
    runtime::restore_bnw_game_deals_checkpoint(
        section,restored,source,identity);
    check(restored.history.deals.groups[0].size()==2 &&
          restored.history.deals.groups[0][1].items.size()==2 &&
          restored.history.deals.groups[1].empty() &&
          restored.history.deals.groups[2][0].items[0].data[2]==44 &&
          *restored.history.deals.groups[2][0].items[0].to_renewed,
          "BNW deals groups or item fields did not round-trip");
    check(runtime::make_bnw_game_deals_checkpoint(
              restored,source,identity).bytes==section.bytes,
          "BNW deals checkpoint changed after reload");
    auto corrupt=section;
    corrupt.bytes.pop_back();
    auto unchanged=source;
    rejects([&] {runtime::restore_bnw_game_deals_checkpoint(
        corrupt,unchanged,source,identity);});
    check(unchanged.history.deals.groups[0].size()==1 &&
          unchanged.history.deals.groups[1].size()==1,
          "Rejected BNW deals checkpoint mutated destination");
    corrupt=section;corrupt.bytes[12]^=1;
    rejects([&] {auto state=source;
        runtime::restore_bnw_game_deals_checkpoint(
            corrupt,state,source,identity);});
    corrupt=section;corrupt.bytes[8+4+128+4+7]^=1;
    rejects([&] {auto state=source;
        runtime::restore_bnw_game_deals_checkpoint(
            corrupt,state,source,identity);});
    corrupt=section;corrupt.bytes.back()=2;
    auto bad_live=source;
    rejects([&] {runtime::restore_bnw_game_deals_checkpoint(
        corrupt,bad_live,source,identity);});
    check(bad_live.history.deals.groups[1].size()==1,
          "Rejected BNW deal boolean partially changed destination");
    auto changed_source=source;
    changed_source.history.deals.groups[0][0].duration++;
    rejects([&] {runtime::restore_bnw_game_deals_checkpoint(
        section,changed_source,source,identity);});
    auto invalid=live;
    invalid.history.deals.groups[0][0].items[0].version=2;
    rejects([&] {(void)runtime::make_bnw_game_deals_checkpoint(
        invalid,source,identity);});
    invalid=live;
    invalid.history.deals.groups[2][0].items[0].data[2]=INT64_MAX;
    rejects([&] {(void)runtime::make_bnw_game_deals_checkpoint(
        invalid,source,identity);});
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &root) {
    auto boot=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(boot.preview.blocker)
        throw std::runtime_error(boot.preview.blocker->reason);
    check(boot.preview.host!=nullptr,"Selected BNW host unavailable");
    const auto &host=*boot.preview.host;
    const auto &source=host.session().source().snapshot;
    for(const auto *world:{&source,&host.session().state().world}) {
        const auto section=runtime::make_bnw_game_deals_checkpoint(
            *world,source,host.identity());
        auto restored=source;
        runtime::restore_bnw_game_deals_checkpoint(
            section,restored,source,host.identity());
        check(runtime::make_bnw_game_deals_checkpoint(
                  restored,source,host.identity()).bytes==section.bytes,
              "Selected BNW deals failed repeated roundtrip");
        std::cout<<"selected BNW deals "
                 <<world->history.deals.groups[0].size()<<'/'
                 <<world->history.deals.groups[1].size()<<'/'
                 <<world->history.deals.groups[2].size()
                 <<" section_bytes="<<section.bytes.size()<<'\n';
    }
}
} // namespace

int main(int argc,char **argv) {
    try {
        synthetic();
        if(argc==3) selected(argv[1],argv[2]);
        return 0;
    } catch(const std::exception &e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
}
