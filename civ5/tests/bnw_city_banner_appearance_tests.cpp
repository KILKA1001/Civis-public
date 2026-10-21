#include "civ5/ui/bnw_city_banner_appearance.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/pregame_roster.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/ui/live_session_frame.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include <fstream>
#include <iostream>
#include <thread>

namespace {
using namespace civ5;
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
runtime::LiveSession live() {
    using namespace runtime;
    saves::SavedSession source;source.snapshot.game_state.layout=saves::GameStateLayout::expansion2_version1;source.source.bytes={1,2,3};
    LiveSessionState state{source.snapshot,ResearchContext{}, {},PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},
        MaintenanceContext{},TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{5,0,0}),SimulationRandom(RandomAlgorithm::bnw,{7,0,0})}};
    state.setup.game_turn=12;state.setup.players[7].team=3;state.setup.players[7].alive=true;
    auto &city=state.cities.players[7].cities[9];city.owner=7;city.id=9;city.population=5;city.x=2;city.y=4;
    return LiveSession::from_restored_bnw(std::move(source),std::move(state));
}
void apply(content::RulesDatabase &rules,const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("Actual color XML unavailable");
    rules.apply_xml(content::read_xml_metadata(std::string(std::istreambuf_iterator<char>(in),{})));
}
void actual(const std::filesystem::path &root) {
    auto session=live();content::RulesDatabase rules;
    apply(rules,root/"Assets/Gameplay/XML/Interface/CIV5Colors.xml");
    apply(rules,root/"Assets/Gameplay/XML/Interface/CIV5PlayerColors.xml");
    runtime::PregameRoster roster;roster.active_player=7;auto &slot=roster.slots[7];slot.status=3;slot.player_color_key="PLAYERCOLOR_INDIA";
    rules.query_rows({"SELECT ID FROM PlayerColors WHERE Type=?",{slot.player_color_key}},[&](const auto &row){slot.player_color=row.integer("ID");},2);
    auto loose=std::make_shared<assets::LooseResourceIndex>(root);
    for(const auto *file:{"Assets/DLC/Expansion2/UI/InGame/CityBannerManager.lua","Assets/DLC/Expansion2/UI/InGame/CityBannerManager.xml"})loose->add_file(root/file);
    // Actual caller-selected BNW mount redirects the base UI logical key to
    // Expansion2, as production's selected mount plan does.
    assets::ResourceMountTable resources([&](const assets::AssetKey &key) {
        const auto name=assets::resource_basename(key.original_name());
        return assets::ResourceMountHit{loose->find(assets::AssetKey("Assets/DLC/Expansion2/UI/InGame/"+
            std::string(name=="citybannermanager.lua"?"CityBannerManager.lua":"CityBannerManager.xml"))),true};
    });
    ui::BnwCityBannerAppearanceOwner owner(session,roster,rules,resources);
    auto major=owner.capture(7,7,9,1);
    check(major&&owner.matches(*major),"Current actual source/roster/rules appearance unavailable");
    ui::LiveSessionUiSources ui_sources;
    ui_sources.city_banner_appearance=[&](const runtime::LiveSession &current,std::int64_t active,
        std::int64_t player,std::int64_t city,std::uint64_t frame) {
        check(&current==&session,"Frame queried a copied live owner");
        return owner.capture(active,player,city,frame);
    };
    ui_sources.city_banner_appearance_current=[&](const runtime::LiveSession &current,
        const ui::BnwCityBannerAppearance &feed) {return &current==&session&&owner.matches(feed);};
    const auto composed=ui::capture_live_session_ui_frame(session,1,7,ui_sources);
    const auto *captured=composed.find<ui::LiveCityBannerAppearances>();
    check(captured&&captured->cities.size()==1&&ui::ui_owners_current(composed),
        "Shared frame did not retain current city appearance and owner fence");
    check(major->foreground==ui::UiFloatColor{1.f,0.6f,0.196f,1.f}&&
        major->background==ui::UiFloatColor{0.071f,0.533f,0.027f,0.7f}&&major->shadow[3]==0.5f,
        "Selected India primary/secondary channels or Lua float alpha changed");
    check(major->background[3]!=float(178)/255&&major->shadow[3]!=float(128)/255,
        "City appearance was silently routed through byte quantization");
    auto tampered=*major;tampered.background[0]=0;check(!owner.matches(tampered),"Mutated float appearance accepted issued seal");
    tampered=*major;tampered.frame_number=2;check(!owner.matches(tampered),"Unissued appearance frame accepted");
    ui::BnwCityBannerAppearanceOwner foreign(session,roster,rules,resources);check(!foreign.matches(*major),"Foreign owner accepted feed");
    bool cross_thread=true;std::thread other([&]{cross_thread=owner.matches(*major);});other.join();check(!cross_thread,"Foreign thread read live owners");
    rules.apply_sql("INSERT INTO Colors(Type,Red,Green,Blue,Alpha) VALUES('TEST_FIRST',0.2,0.3,0.4,1),('TEST_SECOND',0.5,0.6,0.7,1);"
        "INSERT INTO PlayerColors(ID,Type,PrimaryColor,SecondaryColor,TextColor) VALUES(777,'TEST_PLAYER','TEST_FIRST','TEST_SECOND','TEST_FIRST');");
    auto pregame=runtime::lua_pregame_services(roster,rules);pregame.set_player_color(7,777);
    check(!owner.matches(*major),"Same-turn current Lua recolor did not expire old appearance");
    check(!ui::ui_owners_current(composed),"Shared composed frame retained stale same-frame roster colors");
    auto recolored=owner.capture(7,7,9,1);check(recolored&&recolored->foreground[0]==0.2f&&recolored->background[0]==0.5f,
        "Appearance owner restored stale save colors instead of current shared roster");
    check(!ui::ui_owners_current(composed),"Reissued same-frame appearance revived old composed owner fence");
    slot.minor=true;session.mutable_state().cities.players[7].minor=true;
    check(!owner.matches(*recolored),"Current minor flag mutation retained old appearance");
    auto minor=owner.capture(7,7,9,3);check(minor&&minor->foreground[0]==0.5f&&minor->background[0]==0.2f,
        "Current minor owner did not swap native color pair");
    session.mutable_state().setup.players[7].team=4;check(!owner.matches(*minor),"Same-turn team change accepted stale appearance");
    auto team=owner.capture(7,7,9,4);check(team&&team->team_id==4&&team->active_team_id==4,"New current team was not observed");
    session.mutable_state().cities.players[7].cities[9].owner=8;
    check(!owner.matches(*team)&&!owner.capture(7,7,9,5),"Transferred city accepted old owner appearance");
    session.mutable_state().cities.players[7].cities[9].owner=7;
    slot.player_color_key="WRONG";check(!owner.capture(7,7,9,5),"Conflicting color ID/key selected a default");slot.player_color_key="TEST_PLAYER";
    rules.apply_sql("UPDATE Colors SET Red=2 WHERE Type='TEST_FIRST'");check(!owner.capture(7,7,9,5),"Out-of-range DB channel was accepted");
    rules.apply_sql("UPDATE Colors SET Red=NULL WHERE Type='TEST_FIRST'");check(!owner.capture(7,7,9,5),"NULL DB channel was treated as zero");
    rules.apply_sql("UPDATE Colors SET Red=0.2 WHERE Type='TEST_FIRST'");
    auto current=owner.capture(7,7,9,5);check(current&&owner.matches(*current),"Restored valid live color row was not observed");
    const auto rules_frame=ui::capture_live_session_ui_frame(session,5,7,ui_sources);
    check(ui::ui_owners_current(rules_frame),"Current rules frame failed shared fence");
    rules.apply_sql("UPDATE Colors SET Blue=0.8 WHERE Type='TEST_FIRST'");check(!owner.matches(*current),"Current activated rules row change retained old appearance");
    check(!ui::ui_owners_current(rules_frame),"Shared frame fence accepted changed current rules colors");
    bool mutated=false;
    {
        assets::ScopedResourceResolveObserver observer(resources,[&](const auto &,std::span<const assets::MountedResource>,bool) {
            if(!mutated){mutated=true;session.mutable_state().cities.players[7].cities[9].owner=8;}
        });
        check(!owner.capture(7,7,9,6),"Source resolve callback mutation published stale live owner");
    }
    session.mutable_state().cities.players[7].cities[9].owner=7;
    bool rejected_reentry=false;
    {
        assets::ScopedResourceResolveObserver observer(resources,[&](const auto &,std::span<const assets::MountedResource>,bool) {
            rejected_reentry=!owner.capture(7,7,9,7).has_value();
        });
        check(owner.capture(7,7,9,7).has_value()&&rejected_reentry,"Read-only appearance reentry was not bounded");
    }
    roster.active_player=8;check(!owner.capture(7,7,9,8),"Stale active player accepted appearance");roster.active_player=7;
    roster.slots.erase(7);check(!owner.capture(7,7,9,8),"Missing current roster fell back to saved colors");

    // Separate real checkpoint lifecycle fixture: equal projected fields and
    // allocator-reused addresses must not revive an earlier generation.
    auto replaced=live();runtime::PregameRoster lifetime_roster; lifetime_roster.active_player=7;
    auto &lifetime_slot=lifetime_roster.slots[7];lifetime_slot.status=3;lifetime_slot.player_color_key="PLAYERCOLOR_INDIA";
    rules.query_rows({"SELECT ID FROM PlayerColors WHERE Type=?",{lifetime_slot.player_color_key}},
        [&](const auto &row){lifetime_slot.player_color=row.integer("ID");},2);
    const auto checkpoint=replaced.checkpoint();
    auto original=std::make_shared<ui::BnwCityBannerAppearanceOwner>(replaced,lifetime_roster,rules,resources);
    const auto original_feed=original->capture(7,7,9,1);
    check(original_feed&&original->matches(*original_feed),"Initial lifecycle generation did not bind");
    const auto lifetime_sources=[&](const auto &bound) {
        ui::LiveSessionUiSources sources;
        sources.city_banner_appearance=[bound](const auto &,auto active,auto player,auto city,auto frame) {
            return bound->capture(active,player,city,frame);
        };
        sources.city_banner_appearance_current=[bound](const auto &,const auto &feed) {return bound->matches(feed);};
        return sources;
    };
    const auto original_frame=ui::capture_live_session_ui_frame(replaced,1,7,lifetime_sources(original));
    check(ui::ui_owners_current(original_frame),"Original generation failed shared frame fence");
    auto prior_generation=replaced.state_generation();
    auto foreign_world=live();bool foreign_rejected=false;
    try {replaced.restore(foreign_world.checkpoint());}catch(const std::exception &){foreign_rejected=true;}
    check(foreign_rejected&&replaced.state_generation()==prior_generation&&original->matches(*original_feed)&&
        ui::ui_owners_current(original_frame),"Rejected foreign restore invalidated unchanged appearance generation");
    replaced.restore(checkpoint);
    check(replaced.state_generation()>prior_generation&&!original->matches(*original_feed)&&
        !original->capture(7,7,9,2)&&!ui::ui_owners_current(original_frame),
        "Identical checkpoint restore retained old bound owner or shared frame");
    for(std::uint64_t frame=2;frame<18;++frame) {
        auto rebound=std::make_shared<ui::BnwCityBannerAppearanceOwner>(replaced,lifetime_roster,rules,resources);
        const auto feed=rebound->capture(7,7,9,frame);
        check(feed&&rebound->matches(*feed)&&feed->background==original_feed->background&&feed->foreground==original_feed->foreground,
            "Explicit appearance rebind changed identical source colors or rejected new generation");
        const auto rebound_frame=ui::capture_live_session_ui_frame(replaced,frame,7,lifetime_sources(rebound));
        prior_generation=replaced.state_generation();replaced.restore(checkpoint);
        check(replaced.state_generation()>prior_generation&&!rebound->matches(*feed)&&
            !rebound->capture(7,7,9,frame+1)&&!ui::ui_owners_current(rebound_frame)&&
            !original->matches(*original_feed),"Repeated restore revived stale appearance via address ABA");
    }
    auto during_lookup=std::make_shared<ui::BnwCityBannerAppearanceOwner>(replaced,lifetime_roster,rules,resources);
    bool restored_during_lookup=false;
    {
        assets::ScopedResourceResolveObserver observer(resources,[&](const auto &,std::span<const assets::MountedResource>,bool) {
            if(!restored_during_lookup){restored_during_lookup=true;replaced.restore(checkpoint);}
        });
        check(!during_lookup->capture(7,7,9,18),"Identical state replacement during source lookup was accepted");
    }
    auto final_rebind=std::make_shared<ui::BnwCityBannerAppearanceOwner>(replaced,lifetime_roster,rules,resources);
    check(final_rebind->capture(7,7,9,19).has_value(),"Rebind after lookup-time restore failed");
}
}
int main(int argc,char **argv){try {
    if(argc>1)actual(argv[1]);else {
        auto session=live();runtime::PregameRoster roster;content::RulesDatabase rules;
        assets::ResourceMountTable resources([](const assets::AssetKey&){return assets::ResourceMountHit{assets::AssetLookup{},false};});
        ui::BnwCityBannerAppearanceOwner owner(session,roster,rules,resources);
        check(!owner.capture(7,7,9,1)&&!owner.blocker().empty(),"Missing actual authorities were not rejected");
    }
    std::cout<<"PASS current borrowed CityBanner appearance owners and float source colors\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
