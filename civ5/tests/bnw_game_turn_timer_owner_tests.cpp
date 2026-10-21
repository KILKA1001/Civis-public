#include "civ5/runtime/bnw_game_turn_timer_owner.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <functional>
#include <iostream>
#include <thread>
#include <type_traits>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

using namespace civ5;
using namespace civ5::runtime;
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F> void rejects(F&& f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected,"Invalid timer use accepted");}
std::int64_t qpc(){LARGE_INTEGER v{};need(QueryPerformanceCounter(&v)!=0,"Actual test QPC unavailable");return v.QuadPart;}
std::int64_t qpf(){LARGE_INTEGER v{};need(QueryPerformanceFrequency(&v)!=0&&v.QuadPart>0,"Actual test QPF unavailable");return v.QuadPart;}
std::shared_ptr<LiveSession> make_session(const saves::SavedSession& source){
    need(source.snapshot.pregame.has_value(),"Actual BNW source pregame missing");
    ResearchContext setup;
    setup.game_turn=source.snapshot.pregame->archive.integers.at("game_turn");
    setup.game_type=source.snapshot.pregame->archive.integers.at("game_type");
    // Only timer-scope setup mirrors are derived. This diagnostic fixture does
    // not manufacture rules/cache/load-hook or normal-engine readiness.
    LiveSessionState state{.world=source.snapshot,.setup=setup,.technologies={},
        .player_research={},.cities={},.unhappiness={},.maintenance={},.treasury={},
        .science={},.research_turn={},.random=restore_session_random(source.snapshot.progression),
        .research_selection={},.game_clock={},.player_gold_publication={},
        .player_research_publication={},.player_early_publication={},.nearby_cities={},
        .city_flavor_recipients={},.city_strategy_runtime={},.player_flavor_recipients={},.plot_impassability={},
        .adjacent_land={},.plot_visibility={},.diplomacy_scratchpad={},
        .notification_load_transients={},.player_plot_lists={},.player_read_tail={},
        .game_plot_extras={},.player_culture_bomb_timers={},.military_request_counters={},
        .city_dirty_events={},.city_dirty_next_occurrence=1,.specific_city_info_events={},.ui_source_revision=1,
        .plot_gameplay_events={},.plot_gameplay_next_occurrence=1,
        .standalone_services={},.bnw_unit_mission_continuation_quarantine={}};
    return std::make_shared<LiveSession>(LiveSession::from_restored_bnw(source,std::move(state)));
}
void bracket(const std::shared_ptr<BnwGameTurnTimerOwner>& owner,const BnwGameTurnTimerUse& use){
    const auto before=qpc();const auto sample=owner->read(use);const auto after=qpc();
    need(sample.frequency==qpf(),"Timer does not use actual platform frequency");
    const auto inside=[&](float value,std::int64_t start){
        const double reciprocal=1.0/static_cast<double>(sample.frequency);
        return value>=static_cast<float>(static_cast<double>(before-start)*reciprocal)&&
               value<=static_cast<float>(static_cast<double>(after-start)*reciprocal);
    };
    need(inside(sample.current_elapsed,sample.current_start)&&inside(sample.game_turn_elapsed,sample.game_turn_start),
         "Timer elapsed is not bracketed by actual QPC delta/float arithmetic");
}
void actual(const saves::SavedSession& source){
    static_assert(!std::is_default_constructible_v<BnwGameTurnTimerUse>);
    static_assert(!std::is_constructible_v<BnwGameTurnTimerUse,std::int64_t>);
    static_assert(!std::is_copy_constructible_v<BnwGameTurnTimerOwner>);
    auto session=make_session(source);auto owner=BnwGameTurnTimerOwner::create(session);
    const auto map_rng=session->state().random.map.state();
    const auto game_rng=session->state().random.gameplay.state();
    const auto initial=owner->current_use();const auto initial_value=owner->read(initial);
    need(initial_value.operation==0&&initial_value.reset==BnwGameTurnTimerReset::construction,
         "Timer construction did not own an initial actual Start");
    bracket(owner,initial);
    const auto before=qpc();const auto begin=owner->reset_for_engine_begin();const auto after=qpc();
    const auto b=owner->read(begin);
    need(b.current_start>=before&&b.current_start<=b.game_turn_start&&b.game_turn_start<=after,
         "reset(true) did not start both actual timers in source order");
    need(b.pause_delta==0.0f&&b.current_stopped_elapsed==0.0f&&b.game_turn_stopped_elapsed==0.0f&&
         b.operation==1&&b.reset==BnwGameTurnTimerReset::engine_begin,"True reset state not owned");
    rejects([&]{(void)owner->read(initial);});
    rejects([&]{(void)owner->reset_for_engine_begin();});
    need(owner->read(begin).operation==1,"Rejected repeat reset changed current operation");
    // Real elapsed CPU time, no injected timestamps, clock callbacks or sleeps.
    const auto until=qpc()+qpf()/1000;
    while(qpc()<until){}
    const auto player=owner->reset_for_current_player();const auto p=owner->read(player);
    need(p.current_start>b.current_start&&p.game_turn_start==b.game_turn_start,
         "reset(false) restarted game-turn timer or failed to restart current timer");
    need(p.current_elapsed<p.game_turn_elapsed&&p.operation==2&&p.pause_delta==0.0f&&
         p.reset==BnwGameTurnTimerReset::current_player,"False reset elapsed/state wrong");
    bracket(owner,player);
    rejects([&]{(void)owner->read(begin);});
    auto peer=BnwGameTurnTimerOwner::create(session);
    rejects([&]{(void)peer->read(player);});
    (void)peer->read(peer->current_use());
    const auto second=owner->reset_for_current_player();
    need(owner->read(second).operation==3&&owner->read(second).game_turn_start==b.game_turn_start,
         "Repeated current-player reset lost game start");
    need(session->state().random.map.state()==map_rng&&session->state().random.gameplay.state()==game_rng,
         "Timer consumed session RNG");
    need(!session->readiness().can_continue(),"Timer incorrectly granted full continuation");
    owner->retire();rejects([&]{owner->require_current();});rejects([&]{(void)owner->read(second);});
}
void identity(const saves::SavedSession& source){
    rejects([&]{(void)BnwGameTurnTimerOwner::create({});});
    const auto changed=[&](const std::function<void(LiveSessionState&)>& mutate){
        auto s=make_session(source);auto o=BnwGameTurnTimerOwner::create(s);const auto use=o->current_use();
        const auto checkpoint=s->checkpoint();mutate(s->mutable_state());
        rejects([&]{(void)o->reset_for_engine_begin();});
        s->restore(checkpoint);
        rejects([&]{(void)o->read(use);}); // rejected binding cannot resurrect
    };
    changed([](auto& s){++s.setup.game_turn;});
    changed([](auto& s){s.setup.game_type=1;});
    changed([](auto& s){s.world.pregame.reset();});
    changed([](auto& s){s.world.map_header.guid[0]^=1;});
    changed([](auto& s){s.world.pregame->gamecore_guid[0]^=1;});
    changed([](auto& s){s.world.pregame->engine_guid[0]^=1;});
    changed([](auto& s){s.world.pregame->archive.integers.erase("game_turn");});
    changed([](auto& s){s.world.pregame->archive.integers.at("game_type")=1;});
    changed([](auto& s){s.world.pregame->layout=saves::GameStateLayout::vp542_corpus;});
    changed([](auto& s){s.world.pregame->archive_version=5;});
    changed([](auto& s){std::get<saves::Expansion2PrefixObservation>(s.world.game_state.prefix).version=2;});
    {
        auto s=make_session(source);auto o=BnwGameTurnTimerOwner::create(s);const auto use=o->current_use();
        const auto cp=s->checkpoint();s->restore(cp);
        rejects([&]{(void)o->read(use);});
    }
    {
        auto s=make_session(source);auto o=BnwGameTurnTimerOwner::create(s);const auto use=o->current_use();
        *s=std::move(*make_session(source));
        rejects([&]{(void)o->read(use);}); // equal source/turn is not same allocation
    }
    {
        auto s=make_session(source);auto o=BnwGameTurnTimerOwner::create(s);
        auto moved=std::make_shared<LiveSession>(std::move(*s));
        rejects([&]{o->require_current();});
        rejects([&]{(void)BnwGameTurnTimerOwner::create(s);});
        *s=std::move(*moved);rejects([&]{o->require_current();});
    }
    {
        auto s=make_session(source);s->mutable_state().world.pregame->gamecore_guid[0]^=1;
        rejects([&]{(void)BnwGameTurnTimerOwner::create(s);});
    }
    {
        auto s=make_session(source);s->mutable_state().world.pregame->archive.integers.erase("game_type");
        rejects([&]{(void)BnwGameTurnTimerOwner::create(s);});
    }
}
void lifetime_thread(const saves::SavedSession& source){
    {
        auto s=make_session(source);std::weak_ptr<LiveSession> lease=s;
        auto o=BnwGameTurnTimerOwner::create(s);const auto use=o->current_use();
        s.reset();need(!lease.expired(),"Owner lost actual retained session lifetime");
        (void)o->read(use);o.reset();need(lease.expired(),"Sealed read handle retained retired session");
    }
    for(int mode=0;mode<3;++mode){
        auto s=make_session(source);auto o=BnwGameTurnTimerOwner::create(s);const auto use=o->current_use();
        bool rejected=false;
        std::thread other([&]{try{
            if(mode==0)o->require_current();else if(mode==1)(void)o->read(use);else(void)o->reset_for_engine_begin();
        }catch(const std::exception&){rejected=true;}});other.join();
        need(rejected,"Foreign thread timer operation accepted");
        rejects([&]{o->require_current();}); // permanently poisoned on owner thread
    }
}
}
int main(int argc,char** argv){
    try{
        need(argc==2,"Usage: bnw_game_turn_timer_owner_tests selected-bnw.Civ5Save");
        const std::filesystem::path path=argv[1];const auto before=identify_file(path);
        const auto source=saves::read_saved_session(path,saves::SavedSessionDecodeContext{});
        actual(source);identity(source);lifetime_thread(source);
        need(identify_file(path).sha256==before.sha256,"Original save changed");
        std::cout<<"BNW actual QPC timer owner CPU tests PASS; engine/full turn unavailable\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
