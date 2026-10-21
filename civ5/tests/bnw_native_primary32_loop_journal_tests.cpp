#include "bnw_native_primary32_loop_journal.hpp"
#include <iostream>
#include <type_traits>
using namespace civ5::ui::detail;
namespace {void need(bool b,const char*m){if(!b)throw std::runtime_error(m);}template<class F>void refuses(F&&f){bool no=false;try{f();}catch(const std::exception&){no=true;}need(no,"Unsupported source journal transition accepted");}
template<class T>concept PublicAcquire=requires{T::acquire({});};}
int main(){try{
    static_assert(!std::is_default_constructible_v<BnwNativePrimary32SourceBindings> &&
                  !std::is_copy_constructible_v<BnwNativePrimary32SourceBindings> &&
                  !PublicAcquire<BnwNativePrimary32LoopJournalOwner>);
    BnwPrimary32JournalPolicyDiagnostic unstarted;
    refuses([&]{unstarted.complete_diagnostic(0,bnw_primary32_source_order,1);});
    need(unstarted.failed(),"Unstarted completion did not retain terminal failure");
    BnwPrimary32JournalPolicyDiagnostic journal;
    journal.begin_attempt();journal.complete_diagnostic(0,bnw_primary32_source_order,66);
    need(journal.next()==1&&journal.total()==66&&!journal.complete(),"Only first actualstep became fullprimary32");
    journal.begin_attempt();refuses([&]{journal.complete_diagnostic(0,bnw_primary32_source_order,66);});
    need(journal.failed()&&journal.next()==1&&journal.total()==66,"Failure erased prior publish effects");
    refuses([&]{journal.begin_attempt();});
    BnwPrimary32JournalPolicyDiagnostic reordered;reordered.begin_attempt();auto wrong=bnw_primary32_source_order;std::swap(wrong[0],wrong[1]);
    refuses([&]{reordered.complete_diagnostic(0,wrong,1);});
    BnwPrimary32JournalPolicyDiagnostic zero;zero.begin_attempt();refuses([&]{zero.complete_diagnostic(0,bnw_primary32_source_order,0);});
    need(zero.next()==0&&zero.failed(),"Zero was counted as completed cache publication");
    BnwPrimary32JournalPolicyDiagnostic partial;partial.begin_attempt();refuses([&]{partial.complete_diagnostic(0,std::span(bnw_primary32_source_order).first(6),1);});
    need(reordered.failed()&&partial.failed(),"Caught malformed completion failed to poison journal");
    BnwPrimary32JournalPolicyDiagnostic reentry;reentry.begin_attempt();refuses([&]{reentry.begin_attempt();});
    refuses([&]{reentry.complete_diagnostic(0,bnw_primary32_source_order,1);});
    BnwPrimary32JournalPolicyDiagnostic all;
    for(std::size_t ordinal=0;ordinal<32;++ordinal){all.begin_attempt();all.complete_diagnostic(ordinal,bnw_primary32_source_order,1);}
    need(all.complete()&&all.next()==32&&all.total()==32,"Exact completed source32 differs");
    refuses([&]{all.begin_attempt();});
    std::cout<<"PASS primary32 source-order/adopt0/replay/zero/incomplete/reentry/stickyfailedprefix policy; genuine privateRootstep hooks pending\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
