#include "civ5/runtime/normalised_bnw_state.hpp"
#include "civ5/runtime/live_action_transaction.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace civ5;
namespace {
void need(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
void actual_fixture(ui::SelectedBnwPreviewHost& host,bool dump){
    auto& session=host.session();
    const auto generation=session.state_generation();
    const auto original_rng=session.state().random.gameplay.state();
    const auto before=runtime::normalise_bnw_state(session);
    need(!before.complete&&!before.unresolved.empty(),"Partial source projection falsely became complete");
    need(session.state_generation()==generation&&session.state().random.gameplay.state()==original_rng,
         "Read-only projection changed actual session/RNG");
    if(dump){std::cout<<before.json();return;}
    {
        auto reordered=session.state();
        auto original=std::move(reordered.setup.players);
        reordered.setup.players.clear();
        for(auto row=original.rbegin();row!=original.rend();++row)reordered.setup.players.emplace(row->first,row->second);
        auto original_gold=std::move(reordered.treasury.players);
        reordered.treasury.players.clear();
        for(auto row=original_gold.rbegin();row!=original_gold.rend();++row)reordered.treasury.players.emplace(row->first,row->second);
        // Real decoded state + genuine public diagnostic constructor. No
        // activation flag, fake acquisition, static Session or empty callback.
        auto other=runtime::LiveSession::from_restored_bnw(session.source(),std::move(reordered));
        const auto equal=runtime::normalise_bnw_state(other);
        need(equal.json()==before.json(),"Allocation/insertion history leaked into normalised state");
        const auto comparison=runtime::diff_normalised_bnw_state(before,equal);
        need(comparison.known_values_equal&&comparison.values.empty()&&!comparison.complete_comparison,
             "Same semantic diagnostic world did not retain partial-equality distinction");
    }
    need(original_rng.calls<std::numeric_limits<std::uint64_t>::max(),"Fixture RNG counter exhausted");
    const auto committed=runtime::apply_live_action(session,[](runtime::LiveSessionState& state){
        (void)state.random.gameplay.draw(65536);
        return runtime::LiveActionResult{runtime::LiveActionOutcome::committed,{}};
    });
    need(committed.outcome==runtime::LiveActionOutcome::committed,"Real owned RNG field change was not committed");
    const auto after=runtime::normalise_bnw_state(session);
    const auto diff=runtime::diff_normalised_bnw_state(before,after);
    const auto expected=std::to_string(original_rng.calls+1);
    const auto changed=std::find_if(diff.values.begin(),diff.values.end(),[](const auto& item){return item.path=="/owned/rng/gameplay/calls";});
    need(changed!=diff.values.end()&&changed->before==std::to_string(original_rng.calls)&&changed->after==expected,
         "Projection/diff lost an actual committed random-stream change");
    need(!diff.known_values_equal&&!diff.complete_comparison&&!after.complete,
         "Changed partial world falsely reported complete equality/parity");
    const auto rng_after=session.state().random.gameplay.state();
    need(rng_after.calls==original_rng.calls+1,"Projection consumed another random draw");
    std::cout<<diff.json();
}
}
int main(int argc,char** argv){
    try{
        if(argc!=3&&argc!=4)throw std::invalid_argument("Usage: normalised_bnw_state_tests <actual BNW save> <actual install> [--dump]");
        const bool dump=argc==4;
        if(dump&&std::string_view(argv[3])!="--dump")throw std::invalid_argument("Unknown mode");
        auto bootstrap=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0,true);
        if(bootstrap.preview.blocker)throw std::runtime_error(bootstrap.preview.blocker->reason);
        need(bool(bootstrap.preview.host),"Genuine BNW fixture Host absent");
        actual_fixture(*bootstrap.preview.host,dump);
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
