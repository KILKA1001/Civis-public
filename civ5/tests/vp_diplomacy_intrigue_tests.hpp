#pragma once
#include "civ5/runtime/vp_diplomacy_intrigue.hpp"
#include "civ5/runtime/vp_espionage_turn.hpp"

void vp_diplomacy_intrigue_tests() {
    using namespace civ5;
    using namespace civ5::runtime;
    constexpr auto vp=content::TechnologyProfile::vp542;
    constexpr auto layout=saves::GameStateLayout::vp542_corpus;
    constexpr std::int64_t third=5000000002LL;
    saves::SavedSessionSnapshot snapshot;
    snapshot.all_players.emplace();snapshot.all_players->layout=layout;
    snapshot.all_players->context.diplomacy.major=3;
    snapshot.all_players->context.diplomacy.civ_players=3;
    for(std::size_t slot=0;slot<3;++slot) {
        saves::SavedPlayerObservation saved{};
        saved.index=slot;saved.saved_player_id=slot==2?third:static_cast<std::int64_t>(slot);
        saved.diplomacy.layout=layout;saved.diplomacy.dimensions.major=3;
        saved.diplomacy.dimensions.civ_players=3;
        const auto add=[&](const char *name,std::uint64_t rows,std::uint64_t columns,
                           std::vector<std::int64_t> values) {
            saves::PlayerDiplomacyField field{};
            field.name=name;field.rows=rows;field.columns=columns;
            field.values=std::move(values);
            saved.diplomacy.fields.push_back(std::move(field));
        };
        add("m_aeCivApproach",1,3,{5,5,slot==1?2:5});
        add("m_aeCachedSurfaceApproach",1,3,{-1,-1,-1});
        add("m_aiDoFAcceptedTurn",1,3,{-1,-1,-1});
        add("m_aaeCoopWarState",3,3,std::vector<std::int64_t>(9,-1));
        snapshot.all_players->players.push_back(std::move(saved));
    }
    ResearchContext setup;setup.profile=vp;setup.game_turn=200;setup.map_random_seed=123;
    for(const auto id:{std::int64_t{0},std::int64_t{1},third}) {
        setup.players[id].team=id;setup.players[id].alive=true;
        setup.players[id].slot_status=id==0?3:1;
    }
    restore_vp_diplomacy_intrigue_profile(snapshot,setup);
    const auto *profile=find_vp_diplomacy_intrigue_profile(setup);
    check(profile&&profile->major_order==std::vector<std::int64_t>{0,1,third}&&
          profile->players.at(1).relations.at(third).approach==2,
          "VP diplomacy import must map source slots to wide live player IDs");
    PlayerResearchContext diplomacy;diplomacy.profile=vp;
    diplomacy.teams[0].met.insert(third);
    diplomacy.teams[1];diplomacy.teams[third];
    content::CustomModOptions options;
    const auto scan=[&] {return vp_scan_diplomatic_intrigue(setup,diplomacy,options,0,1,12,7);};
    auto result=scan();
    check(!result.target_human_diplomacy&&result.active_major_count==3&&
          result.candidate&&result.candidate->kind==VpDiplomaticIntrigueKind::deception&&
          result.candidate->other_player==third&&result.candidate->reported_player==third,
          "VP AI deception must use saved honest approach and source surface rule");
    diplomacy.teams[0].met.clear();
    result=scan();
    check(result.candidate&&result.candidate->reported_player==-1,
          "Deception about an unmet major must hide that player's identity");
    diplomacy.teams[0].met.insert(third);
    diplomacy.teams[1].at_war.insert(third);
    check(!scan().candidate,"Actual war must suppress deception intrigue");
    diplomacy.teams[1].at_war.clear();
    auto &live=ensure_vp_diplomacy_intrigue_profile(setup).players.at(1);
    live.relations.at(third).approach=5;
    check(!scan().candidate,"A live neutral approach must suppress VP deception");
    live.relations.at(third).approach=2;
    setup.players[1].slot_status=3;
    result=scan();
    check(result.target_human_diplomacy&&!result.candidate,
          "Human diplomacy must skip the shadow-AI deception branch");
    live.coop_wars[{0,third}]=2;
    result=scan();
    check(result.candidate&&result.candidate->kind==VpDiplomaticIntrigueKind::coop_war&&
          result.candidate->other_player==third&&result.candidate->diplomacy_player==0,
          "VP human diplomacy must inspect agreed cooperative-war state");
    options.entries["AI_CONTROL_DIPLOMACY"]={1,1};
    result=scan();
    check(!result.target_human_diplomacy&&result.candidate&&
          result.candidate->kind==VpDiplomaticIntrigueKind::deception,
          "Selected AI_CONTROL_DIPLOMACY option must switch to AI intrigue behavior");
    auto &history=ensure_vp_espionage_profile(setup).players[0].intrigue;
    VpIntrigue repeated{};repeated.source_player=1;repeated.target_player=third;
    repeated.type=0;repeated.turn=171;history.push_back(repeated);
    check(vp_recent_diplomatic_intrigue(setup,0,1,third,false,12,7)&&
          vp_recent_diplomatic_intrigue(setup,0,1,third,false,99,99),
          "VP deception duplicate must ignore city and suppress an event within 30 turns");
    history.back().turn=170;
    check(!vp_recent_diplomatic_intrigue(setup,0,1,third,false,12,7),
          "VP deception duplicate expires at exactly 30 turns");
    history.back().type=6;history.back().turn=171;
    history.back().city_x=12;history.back().city_y=7;
    check(vp_recent_diplomatic_intrigue(setup,0,1,third,true,12,7)&&
          !vp_recent_diplomatic_intrigue(setup,0,1,third,true,12,8),
          "VP cooperative-war duplicate must match the city coordinates");
    auto bad=snapshot;
    bad.all_players->players[1].diplomacy.fields.pop_back();
    rejects([&]{restore_vp_diplomacy_intrigue_profile(bad,setup);});
    check(find_vp_diplomacy_intrigue_profile(setup)->players.at(1).relations.at(third).approach==2,
          "Failed VP diplomacy import must preserve the previous live profile");
    ResearchContext vanilla;
    rejects([&]{(void)ensure_vp_diplomacy_intrigue_profile(vanilla);});
    check(!find_vp_diplomacy_intrigue_profile(vanilla),
          "Vanilla runtime must not require VP diplomacy semantics");
}
