#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "../src/ui/bnw_native_common13_source.hpp"
#include <array>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui::detail {unsigned bnw_native_initial_descriptor_retained_metadata_phase_for_test()noexcept;}
namespace civ5::ui {bool bnw_native_primary32_snapshot_leaf_observer_active_for_test()noexcept;}
namespace civ5::ui {bool bnw_native_primary32_default_projection_tail_observer_active_for_test()noexcept;}
namespace civ5::ui {bool bnw_native_primary32_publication_prefix_observer_active_for_test()noexcept;}
namespace civ5::ui {bool bnw_native_firstarchive_post_prefix_observer_active_for_test()noexcept;}
namespace civ5::ui {bool bnw_native_primary32_default_publishvalidation_observer_active_for_test()noexcept;}
namespace civ5::ui {bool bnw_native_primary32_postpublished_prefix_observer_active_for_test()noexcept;}
namespace civ5::ui {bool bnw_native_postpublisher_prefix_prelude_observer_active_for_test()noexcept;}
namespace civ5::ui {bool bnw_native_installed_boot_construction_observer_active_for_test()noexcept;}
using namespace civ5;
namespace {
void need(bool value,const char* text){if(!value)throw std::runtime_error(text);}
template<class F>void mandatory(F&& call,std::string_view frontier="Mandatory installation-aware retained UISkin occurrences/Cold/Stat"){bool stopped=false;try{call();}catch(const std::exception& e){
 if(std::string_view(e.what()).find(frontier)==std::string_view::npos)
  throw std::runtime_error(std::string("Unexpected installation source boundary; expected=")+std::string(frontier)+"; actual="+e.what());
 stopped=true;
}need(stopped,"Actual zero-save Parent bypassed missing installation source supplier");}
template<class F>void exact_refusal(F&& call,std::string_view expected){
 try{call();}catch(const std::exception& e){
  if(std::string_view(e.what())!=expected)
   throw std::runtime_error(std::string("Unexpected exact source refusal; expected=")+std::string(expected)+"; actual="+e.what());
  return;
 }
 throw std::runtime_error(std::string("Expected exact source refusal; actual=normal return; expected=")+std::string(expected));
}
struct Phase {
 const char* const name;const std::chrono::steady_clock::time_point started=std::chrono::steady_clock::now();
 explicit Phase(const char* value):name(value){std::cout<<"installation_prefix phase="<<name<<" event=begin elapsed_ms=0\n";}
 void end()const{const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();
  std::cout<<"installation_prefix phase="<<name<<" event=end elapsed_ms="<<elapsed<<'\n';}
};
struct Window{HWND hwnd=CreateWindowExW(0,L"STATIC",L"Actual installation Parent prefix",WS_OVERLAPPEDWINDOW,
 0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(hwnd)DestroyWindow(hwnd);}};
#include "bnw_native_installation_skin_pc_benchmark.inc"
void run_installed_startup_benchmark(const char* original_install,const char* canonical_assets,std::string_view record_journal_case={},std::string_view snapshot_leaf_case={},std::string_view projection_tail_case={},std::string_view publication_prefix_case={},bool installed_skin_pc_benchmark=false,std::string_view publishvalidation_case={},std::string_view postpublished_prefix_case={},bool installed_skin_global_benchmark=false,std::string_view postpublisher_prelude_case={},bool installed_late_speech_benchmark=false,std::string_view boot_construction_case={}){
 Phase total("benchmark.total");
 std::uint64_t revision=1;std::size_t revision_callbacks=0;
 const ui::BnwNativePackageDiscoveryOwner* callback_parent=nullptr;
 const ui::BnwNativeInstallationInputOwner* callback_input=nullptr;
 const unsigned requested_record_phase=record_journal_case=="pre-brand"?1U:2U;
 bool record_armed=false;std::size_t record_callbacks=0,record_callback_ordinal=0;unsigned record_seen_phase=0;
 std::string record_inner_reason;
 std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner>* callback_parent_slot=nullptr;
 const unsigned requested_leaf_phase=snapshot_leaf_case.starts_with("construction-")?1U:2U;
 const bool leaf_reentry=snapshot_leaf_case.ends_with("-reentry");
 bool leaf_armed=false;std::size_t leaf_callbacks=0,leaf_callback_ordinal=0,leaf_releases=0;unsigned leaf_seen_phase=0;
 std::string leaf_inner_reason;
 const unsigned requested_tail_phase=projection_tail_case.starts_with("pending-")?2U:3U;
 const bool tail_reentry=projection_tail_case.ends_with("-reentry");
 bool tail_armed=false;std::size_t tail_callbacks=0,tail_callback_ordinal=0,tail_releases=0;
 unsigned tail_seen_phase=0,tail_seen_journal_phase=~0U;std::string tail_inner_reason;
 const unsigned requested_publication_phase=publication_prefix_case.starts_with("prerequisite-")?1U:2U;
 const bool publication_reentry=publication_prefix_case.ends_with("-reentry");
 bool publication_armed=false;std::size_t publication_callbacks=0,publication_callback_ordinal=0,publication_releases=0;
 unsigned publication_seen_phase=0,publication_seen_journal=~0U,publication_seen_leaf=~0U,publication_seen_tail=~0U;
 std::string publication_inner_reason;
 const unsigned requested_publishvalidation_phase=publishvalidation_case.starts_with("a-")?1U:2U;
 const bool publishvalidation_reentry=publishvalidation_case.ends_with("-reentry");
 bool publishvalidation_armed=false,publishvalidation_prior_tags_clear=false;
 std::size_t publishvalidation_callbacks=0,publishvalidation_callback_ordinal=0,publishvalidation_releases=0;
 unsigned publishvalidation_seen_phase=0;std::string publishvalidation_inner_reason;
 const unsigned requested_postpublished_phase=postpublished_prefix_case.starts_with("journal-")?1U:
  (postpublished_prefix_case.starts_with("snapshot-")?2U:3U);
 const bool postpublished_reentry=postpublished_prefix_case.ends_with("-reentry");
 const char* const postpublished_owner_refusal=requested_postpublished_phase==1?
  "Native primary32 retained source lost its genuine external Package facade":(requested_postpublished_phase==2?
  "Native snapshot authentication lost actual source brand/scope/outside Package facade":
  "Default projection is not the privately retained SAME parent/source/index operand");
 bool postpublished_armed=false,postpublished_prior_tags_clear=false;
 std::size_t postpublished_callbacks=0,postpublished_callback_ordinal=0,postpublished_releases=0;
 unsigned postpublished_seen_phase=0;std::string postpublished_inner_reason;
 const bool prelude_reentry=postpublisher_prelude_case=="reentry";
 bool prelude_armed=false,prelude_prior_tags_clear=false;
 std::size_t prelude_callbacks=0,prelude_callback_ordinal=0,prelude_releases=0;
 unsigned prelude_seen_phase=0;std::string prelude_inner_reason;
 const bool boot_reentry=boot_construction_case=="reentry";
 bool boot_armed=false,boot_prior_tags_clear=false;
 std::size_t boot_callbacks=0,boot_callback_ordinal=0,boot_releases=0;
 unsigned boot_seen_phase=0;std::string boot_inner_reason;
 const auto report_callbacks=[&](const char* name,std::size_t before){
  std::cout<<"installation_prefix phase="<<name<<" revision_callbacks="<<revision_callbacks
   <<" callback_delta="<<(revision_callbacks-before)<<'\n';
 };
 const auto method=[&](const char* name,const auto& call){
  Phase phase(name);const auto before=revision_callbacks;call();phase.end();report_callbacks(name,before);
 };
 const auto value=[&](const char* name,const auto& call){
  Phase phase(name);const auto before=revision_callbacks;auto result=call();phase.end();report_callbacks(name,before);return result;
 };
 Phase acquire("benchmark.method.acquire");
 Window window;need(window.hwnd&&!IsWindowVisible(window.hwnd),"Benchmark actual current-process hidden HWND missing");
 const auto hwnd=reinterpret_cast<std::uintptr_t>(window.hwnd);
 auto installation=ui::BnwNativeInstallationInputOwner::acquire(original_install,canonical_assets,hwnd,
  runtime::bnw_stock_lua_win32_client_extent(hwnd,"real-zero-save-startup-benchmark",[&]{
   ++revision_callbacks;
   if(record_armed&&callback_input&&callback_parent&&bnw_native_retained_primary32_borrower_active_for_test(*callback_input)){
    const auto actual_phase=bnw_native_primary32_record_projection_journal_phase_for_test(*callback_parent);
    if(actual_phase==requested_record_phase){
     record_armed=false;++record_callbacks;record_callback_ordinal=revision_callbacks;record_seen_phase=actual_phase;
     try{(void)callback_parent->current();}catch(const std::exception& e){record_inner_reason=e.what();}
     need(record_inner_reason=="Retained installation Parent.current reentry","Actual record-journal callback caught the wrong Parent.current refusal");
    }
   }
   if(leaf_armed&&callback_input&&callback_parent&&bnw_native_retained_primary32_borrower_active_for_test(*callback_input)){
    const auto actual_phase=bnw_native_primary32_snapshot_leaf_phase_for_test(*callback_parent);
    if(actual_phase==requested_leaf_phase){
     leaf_armed=false;++leaf_callbacks;leaf_callback_ordinal=revision_callbacks;leaf_seen_phase=actual_phase;
     if(leaf_reentry){
      try{(void)callback_parent->current();}catch(const std::exception& e){leaf_inner_reason=e.what();}
      need(leaf_inner_reason=="Retained installation Parent.current reentry","Actual Snapshot leaf callback caught the wrong Parent.current refusal");
     }else{
      need(callback_parent_slot&&callback_parent_slot->get()==callback_parent,"Snapshot leaf release lost the actual outside Parent slot");
      ++leaf_releases;callback_parent_slot->reset();callback_parent=nullptr;
     }
    }
   }
   if(tail_armed&&callback_input&&callback_parent&&bnw_native_retained_primary32_borrower_active_for_test(*callback_input)){
    const auto actual_phase=bnw_native_primary32_default_projection_tail_phase_for_test(*callback_parent);
    if(actual_phase==requested_tail_phase){
     tail_seen_journal_phase=bnw_native_primary32_record_projection_journal_phase_for_test(*callback_parent);
     need(tail_seen_journal_phase==0,"Actual projection tail callback ran before the Journal helper fully returned");
     tail_armed=false;++tail_callbacks;tail_callback_ordinal=revision_callbacks;tail_seen_phase=actual_phase;
     if(tail_reentry){
      try{(void)callback_parent->current();}catch(const std::exception& e){tail_inner_reason=e.what();}
      need(tail_inner_reason=="Retained installation Parent.current reentry","Actual projection tail callback caught the wrong Parent.current refusal");
     }else{
      need(callback_parent_slot&&callback_parent_slot->get()==callback_parent,"Projection tail release lost the actual outside Parent slot");
      ++tail_releases;callback_parent_slot->reset();callback_parent=nullptr;
     }
    }
   }
   if(publication_armed&&callback_input&&callback_parent&&bnw_native_retained_primary32_borrower_active_for_test(*callback_input)){
    const auto actual_phase=bnw_native_primary32_publication_prefix_phase_for_test(*callback_parent);
    if(actual_phase==requested_publication_phase){
     publication_seen_journal=bnw_native_primary32_record_projection_journal_phase_for_test(*callback_parent);
     publication_seen_leaf=bnw_native_primary32_snapshot_leaf_phase_for_test(*callback_parent);
     publication_seen_tail=bnw_native_primary32_default_projection_tail_phase_for_test(*callback_parent);
     need(publication_seen_journal==0&&publication_seen_leaf==0&&publication_seen_tail==0,"Publication prefix injection reached an earlier Journal/leaf/tail tag");
     publication_armed=false;++publication_callbacks;publication_callback_ordinal=revision_callbacks;publication_seen_phase=actual_phase;
     if(publication_reentry){
      try{(void)callback_parent->current();}catch(const std::exception& e){publication_inner_reason=e.what();}
      need(publication_inner_reason=="Retained installation Parent.current reentry","Actual publication prefix callback caught the wrong Parent.current refusal");
     }else{
      need(callback_parent_slot&&callback_parent_slot->get()==callback_parent,"Publication prefix release lost the actual outside Parent slot");
      ++publication_releases;callback_parent_slot->reset();callback_parent=nullptr;
     }
    }
   }
   if(publishvalidation_armed&&callback_input&&callback_parent&&bnw_native_retained_primary32_borrower_active_for_test(*callback_input)){
    const auto actual_phase=bnw_native_primary32_default_publishvalidation_phase_for_test(*callback_parent);
    if(actual_phase==requested_publishvalidation_phase){
     publishvalidation_prior_tags_clear=bnw_native_primary32_record_projection_journal_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_snapshot_leaf_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_default_projection_tail_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_publication_prefix_phase_for_test(*callback_parent)==0&&
      !bnw_native_primary32_completed_pass_active_for_test(*callback_parent)&&
      !bnw_native_primary32_new_step_acceptance_active_for_test(*callback_parent)&&
      !bnw_native_firstarchive_post_prefix_active_for_test(*callback_parent);
     need(publishvalidation_prior_tags_clear,"Publishvalidation injection reached a different actual borrower");
     publishvalidation_armed=false;++publishvalidation_callbacks;publishvalidation_callback_ordinal=revision_callbacks;
     publishvalidation_seen_phase=actual_phase;
     if(publishvalidation_reentry){
      try{(void)callback_parent->current();}catch(const std::exception& e){publishvalidation_inner_reason=e.what();}
      need(publishvalidation_inner_reason=="Retained installation Parent.current reentry","Actual publishvalidation callback caught the wrong Parent.current refusal");
     }else{
      need(callback_parent_slot&&callback_parent_slot->get()==callback_parent,"Publishvalidation release lost the actual outside Parent slot");
      ++publishvalidation_releases;callback_parent=nullptr;callback_parent_slot->reset();
     }
    }
   }
   if(boot_armed&&callback_input&&callback_parent&&!bnw_native_retained_primary32_borrower_active_for_test(*callback_input)){
    const auto actual_phase=bnw_native_installed_boot_construction_phase_for_test(*callback_parent);
    if(actual_phase==1U){
     boot_prior_tags_clear=bnw_native_postpublisher_prefix_prelude_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_postpublished_prefix_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_record_projection_journal_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_snapshot_leaf_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_default_projection_tail_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_publication_prefix_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_default_publishvalidation_phase_for_test(*callback_parent)==0&&
      !bnw_native_primary32_completed_pass_active_for_test(*callback_parent)&&
      !bnw_native_primary32_new_step_acceptance_active_for_test(*callback_parent)&&
      !bnw_native_firstarchive_post_prefix_active_for_test(*callback_parent)&&
      ui::detail::bnw_native_initial_descriptor_retained_metadata_phase_for_test()==0;
     need(boot_prior_tags_clear,"Boot construction injection reached a different actual scope");
     boot_armed=false;++boot_callbacks;boot_callback_ordinal=revision_callbacks;boot_seen_phase=actual_phase;
     if(boot_reentry){
      try{(void)callback_parent->current();}catch(const std::exception& e){boot_inner_reason=e.what();}
      need(boot_inner_reason=="Installation client source reentry","Actual Boot construction callback caught the wrong Parent.current refusal");
     }else{
      need(callback_parent_slot&&callback_parent_slot->get()==callback_parent,"Boot construction release lost the actual outside Parent slot");
      ++boot_releases;callback_parent=nullptr;callback_parent_slot->reset();
     }
    }
   }
   if(prelude_armed&&callback_input&&callback_parent&&bnw_native_retained_primary32_borrower_active_for_test(*callback_input)){
    const auto actual_phase=bnw_native_postpublisher_prefix_prelude_phase_for_test(*callback_parent);
    if(actual_phase==1U){
     prelude_prior_tags_clear=bnw_native_primary32_postpublished_prefix_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_record_projection_journal_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_snapshot_leaf_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_default_projection_tail_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_publication_prefix_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_default_publishvalidation_phase_for_test(*callback_parent)==0&&
      !bnw_native_primary32_completed_pass_active_for_test(*callback_parent)&&
      !bnw_native_primary32_new_step_acceptance_active_for_test(*callback_parent)&&
      !bnw_native_firstarchive_post_prefix_active_for_test(*callback_parent);
     need(prelude_prior_tags_clear,"Postpublisher prelude injection reached a different actual borrower");
     prelude_armed=false;++prelude_callbacks;prelude_callback_ordinal=revision_callbacks;prelude_seen_phase=actual_phase;
     if(prelude_reentry){
      try{(void)callback_parent->current();}catch(const std::exception& e){prelude_inner_reason=e.what();}
      need(prelude_inner_reason=="Retained installation Parent.current reentry","Actual postpublisher prelude callback caught the wrong Parent.current refusal");
     }else{
      need(callback_parent_slot&&callback_parent_slot->get()==callback_parent,"Postpublisher prelude release lost the actual outside Parent slot");
      ++prelude_releases;callback_parent=nullptr;callback_parent_slot->reset();
     }
    }
   }
   if(postpublished_armed&&callback_input&&callback_parent&&bnw_native_retained_primary32_borrower_active_for_test(*callback_input)){
    const auto actual_phase=bnw_native_primary32_postpublished_prefix_phase_for_test(*callback_parent);
    if(actual_phase==requested_postpublished_phase){
     postpublished_prior_tags_clear=bnw_native_primary32_record_projection_journal_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_snapshot_leaf_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_default_projection_tail_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_publication_prefix_phase_for_test(*callback_parent)==0&&
      bnw_native_primary32_default_publishvalidation_phase_for_test(*callback_parent)==0&&
      !bnw_native_primary32_completed_pass_active_for_test(*callback_parent)&&
      !bnw_native_primary32_new_step_acceptance_active_for_test(*callback_parent)&&
      !bnw_native_firstarchive_post_prefix_active_for_test(*callback_parent);
     need(postpublished_prior_tags_clear,"Postpublished injection reached an earlier actual borrower");
     postpublished_armed=false;++postpublished_callbacks;postpublished_callback_ordinal=revision_callbacks;postpublished_seen_phase=actual_phase;
     if(postpublished_reentry){
      try{(void)callback_parent->current();}catch(const std::exception& e){postpublished_inner_reason=e.what();}
      need(postpublished_inner_reason=="Retained installation Parent.current reentry","Actual postpublished callback caught the wrong Parent.current refusal");
     }else{
      need(callback_parent_slot&&callback_parent_slot->get()==callback_parent,"Postpublished release lost the actual outside Parent slot");
      ++postpublished_releases;callback_parent=nullptr;callback_parent_slot->reset();
     }
    }
   }return revision;}));
 callback_input=installation.get();
 acquire.end();report_callbacks("benchmark.method.acquire",0);
 Phase create("benchmark.method.create");const auto before_create=revision_callbacks;
 auto parent=ui::BnwNativePackageDiscoveryOwner::create_pre_dx9_installation(installation);
 callback_parent=parent.get();callback_parent_slot=&parent;
 create.end();report_callbacks("benchmark.method.create",before_create);
 method("benchmark.method.pre_dx9",[&]{parent->advance_pre_dx9_discovery_before_primary32();});
 method("benchmark.method.category2",[&]{exact_refusal([&]{parent->advance_installation_category2_first_prefix();},
  "Mandatory installation-aware retained UISkin occurrences/Cold/Stat before Boot/Skin");});
 method("benchmark.method.retainedUI",[&]{exact_refusal([&]{parent->advance_installation_retained_ui_prefix();},
  "Mandatory actual installation first-header Cold/Stat constructors before Boot/Skin");});
 method("benchmark.method.coldStat",[&]{exact_refusal([&]{parent->advance_installation_cold_stat_prefix();},
  "Mandatory installation-aware initial PrefixRegistry/Arena/cache/first archive constructor before Boot/Skin");});
 method("benchmark.method.namespace",[&]{exact_refusal([&]{parent->advance_installation_prefix_arena_cache();},
  "Mandatory installation-aware SAME CurrentDefaultSource assignment before first archive request/constructor/Header/Boot");});
 method("benchmark.method.firstHeader",[&]{exact_refusal([&]{parent->advance_installation_current_first_archive_header();},
  "Mandatory actual installed Prefix transaction brand before shared prefix attempt/store/entry metadata/Boot");});
 method("benchmark.method.firstarchive",[&]{exact_refusal([&]{parent->advance_installation_first_archive_publication();},
  "Mandatory actual installed primary32 journal/capture bindings before remaining archive constructors/Boot");});
 if(!projection_tail_case.empty()){
  // Tail-only prelude skips the two diagnostic inspectors. The real whole32
  // producer and target authenticate actual completed sources/counts before
  // the post-Journal phase; no unobserved diagnostic counts are printed.
  method("benchmark.method.whole32",[&]{exact_refusal([&]{parent->advance_installation_primary32_archives();},
   "Mandatory actual installed primary32 record snapshot/Default projection/publication before DX9/Boot");});
  Phase negative("default_projection_tail.callback_refusal");
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
   !ui::bnw_native_primary32_default_projection_tail_observer_active_for_test(),"Projection tail scopes active before actual record operation");
  // Only a weak witness: no lock or replacement outside Parent owner.
  const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> witness=parent;
  tail_armed=true;
  method("benchmark.method.recordsnapshot",[&]{exact_refusal([&]{(void)parent->advance_installation_record_snapshot_default_projection();},
   tail_reentry?"Installation creator process/thread/source retired":"Default projection is not the privately retained SAME parent/source/index operand");});
  tail_armed=false;
  need(tail_callbacks==1&&tail_seen_phase==requested_tail_phase&&tail_seen_journal_phase==0,
   "Actual requested projection tail did not invoke exactly one callback after Journal return");
  need(revision_callbacks==tail_callback_ordinal+1,"Projection tail negative did not return through both actual unchanged provider samples");
  need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Projection tail negative changed the actual HWND/client source");
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
   !ui::bnw_native_primary32_default_projection_tail_observer_active_for_test(),"Projection tail unwind left a borrower/TLS observer active");
  if(tail_reentry){
   need(parent&&parent.get()==callback_parent&&!witness.expired()&&tail_releases==0&&
    tail_inner_reason=="Retained installation Parent.current reentry","Caught projection tail reentry changed the actual outside Parent");
  }else{
   need(!parent&&witness.expired()&&callback_parent==nullptr&&tail_releases==1&&tail_inner_reason.empty(),
    "Projection tail callback did not release exactly the last outside Parent through unwind");
  }
  const auto before_sticky=revision_callbacks;
  exact_refusal([&]{installation->require_current();},"Installation input retired");
  need(revision_callbacks==before_sticky,"Sticky projection tail retirement invoked another provider callback");
  negative.end();total.end();report_callbacks("benchmark.total",0);
  std::cout<<"SUBSET projection_tail_case="<<projection_tail_case<<" actual_phase="<<tail_seen_phase<<" Journal_at_injection="<<tail_seen_journal_phase
   <<" injected_callbacks="<<tail_callbacks<<" whole32_frontier=exact diagnostic_primary32_counts=unobserved"
   <<" caught_current="<<(tail_reentry?"exact":"unissued")<<" outside_Parent="<<(tail_reentry?"live":"expired")
   <<" outer_refusal=exact sticky_input=refused borrower_cleanup=clear tail_observer_cleanup=clear"
   <<" record_return=unissued Default=unissued Common=unissued Skin=unissued full_game=0 can_continue=0\n";
  return;
 }
 if(!publication_prefix_case.empty()||!publishvalidation_case.empty()||!postpublished_prefix_case.empty()||!postpublisher_prelude_case.empty()||!boot_construction_case.empty()){
  // Publication negatives keep the real whole32 and record returns while
  // skipping two repeated diagnostic inspectors. The returned capture count
  // is already checked against actual admitted totals by production.
  method("benchmark.method.whole32",[&]{exact_refusal([&]{parent->advance_installation_primary32_archives();},
   "Mandatory actual installed primary32 record snapshot/Default projection/publication before DX9/Boot");});
  const auto actual_records=value("benchmark.method.recordsnapshot",[&]{return parent->advance_installation_record_snapshot_default_projection();});
  if(!boot_construction_case.empty()){
   method("benchmark.method.Default",[&]{exact_refusal([&]{parent->advance_installation_default_publication();},
    "Mandatory actual installed low-resolution source CVarD80DE0 before Common/default/Skin/Boot");});
   const auto scopes_clear=[&]{return !bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
    !ui::bnw_native_installed_boot_construction_observer_active_for_test()&&
    !ui::bnw_native_postpublisher_prefix_prelude_observer_active_for_test()&&
    !ui::bnw_native_primary32_postpublished_prefix_observer_active_for_test()&&
    !ui::bnw_native_primary32_default_publishvalidation_observer_active_for_test()&&
    !ui::bnw_native_primary32_publication_prefix_observer_active_for_test()&&
    !ui::bnw_native_primary32_snapshot_leaf_observer_active_for_test()&&
    !ui::bnw_native_primary32_default_projection_tail_observer_active_for_test()&&
    !ui::bnw_native_firstarchive_post_prefix_observer_active_for_test()&&
    ui::detail::bnw_native_initial_descriptor_retained_metadata_phase_for_test()==0;};
   need(scopes_clear(),"Boot construction armed with an already active borrower/observer");
   if(boot_construction_case=="returned"){
    const auto boot=value("benchmark.method.Boot",[&]{return parent->advance_installation_boot_settings();});
    need(boot.normal_launch_source_closed&&!boot.leader_viewer&&boot.effective_texture_quality.has_value(),
     "Benchmark actual installed Boot lacks genuine normal-launch texture source");
    need(parent&&parent.get()==callback_parent&&IsWindow(window.hwnd)!=FALSE&&revision==1&&
     boot_callbacks==0&&boot_releases==0&&boot_inner_reason.empty(),"Returned Boot subset changed the actual outside Parent/HWND/client");
    need(scopes_clear(),"Returned Boot subset left a borrower/TLS observer active");
    total.end();report_callbacks("benchmark.total",0);
    std::cout<<"SUBSET boot_construction_case=returned record_return=actual Default_frontier=actual actual_captured_records="<<actual_records
     <<" actual_Boot_TextureQuality="<<*boot.effective_texture_quality
     <<" injected_callbacks=0 Input_borrower=absent borrower_cleanup=clear boot_observer_cleanup=clear"
     <<" Boot_normal_return=actual Boot=actual_returned Texture_branch=unissued World32=unissued"
     <<" Common=unissued Skin=unissued full_game=0 can_continue=0\n";
    return;
   }
   Phase negative("boot_construction.callback_refusal");
   const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> witness=parent;
   bool boot_normal_returned=false;boot_armed=true;
   method("benchmark.method.Boot",[&]{exact_refusal([&]{(void)parent->advance_installation_boot_settings();boot_normal_returned=true;},
    boot_reentry?"Installation creator process/thread/source retired":"Installed Boot construction lost actual source/operation/outside Parent");});
   boot_armed=false;
   need(!boot_normal_returned&&boot_callbacks==1&&boot_seen_phase==1U&&boot_prior_tags_clear,
    "Actual Boot factory firstpoint did not invoke exactly one isolated callback before normal return");
   need(revision_callbacks==boot_callback_ordinal+1,"Boot construction firstpoint did not return through both actual provider samples");
   need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Boot construction negative changed the actual HWND/client source");
   need(scopes_clear(),"Boot construction unwind left a borrower/TLS observer active");
   if(boot_reentry){
    need(parent&&parent.get()==callback_parent&&!witness.expired()&&boot_releases==0&&boot_inner_reason=="Installation client source reentry",
     "Caught Boot construction reentry changed the actual outside Parent");
    const auto before_parent_control=revision_callbacks;
    exact_refusal([&]{(void)parent->current();},"Installation Parent prefix foreign/source domain/operation changed");
    need(revision_callbacks==before_parent_control,"Poisoned Boot construction Parent.current invoked another provider callback");
   }else{
    need(!parent&&witness.expired()&&callback_parent==nullptr&&boot_releases==1&&boot_inner_reason.empty(),
     "Boot construction callback did not release exactly the last outside Parent through unwind");
   }
   const auto before_input_control=revision_callbacks;
   if(boot_reentry){
    exact_refusal([&]{installation->require_current();},"Installation input retired");
    need(revision_callbacks==before_input_control,"Sticky Boot construction retirement invoked another provider callback");
   }else{
    installation->require_current();
    need(revision_callbacks==before_input_control+2,"Boot construction owner loss did not leave independent Input genuinely current");
   }
   need(scopes_clear(),"Boot construction control revived a retired borrower/TLS observer");
   negative.end();total.end();report_callbacks("benchmark.total",0);
   std::cout<<"SUBSET boot_construction_case="<<boot_construction_case<<" actual_phase="<<boot_seen_phase
    <<" injected_callbacks="<<boot_callbacks<<" Input_borrower=absent prior_tags=clear whole32_frontier=exact diagnostic_primary32_counts=unobserved"
    <<" actual_captured_records="<<actual_records<<" record_return=actual Default_frontier=actual"
    <<" caught_current="<<(boot_reentry?"exact":"unissued")<<" outside_Parent="<<(boot_reentry?"live":"expired")
    <<" outer_refusal=exact Parent_current="<<(boot_reentry?"poisoned-exact":"unissued-expired")
    <<" Input_after="<<(boot_reentry?"retired":"current")<<" Input_control_callbacks="<<(revision_callbacks-before_input_control)
    <<" borrower_cleanup=clear boot_observer_cleanup=clear Boot_normal_return=unissued Boot_adoption=unissued"
    <<" TextureQuality=unissued Common=unissued Skin=unissued full_game=0 can_continue=0\n";
   return;
  }
  if(!postpublisher_prelude_case.empty()){
   Phase negative("postpublisher_prelude.callback_refusal");
   const auto scopes_clear=[&]{return !bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
    !ui::bnw_native_postpublisher_prefix_prelude_observer_active_for_test()&&
    !ui::bnw_native_primary32_postpublished_prefix_observer_active_for_test()&&
    !ui::bnw_native_primary32_default_publishvalidation_observer_active_for_test()&&
    !ui::bnw_native_primary32_publication_prefix_observer_active_for_test()&&
    !ui::bnw_native_primary32_snapshot_leaf_observer_active_for_test()&&
    !ui::bnw_native_primary32_default_projection_tail_observer_active_for_test()&&
    !ui::bnw_native_firstarchive_post_prefix_observer_active_for_test();};
   need(scopes_clear(),"Postpublisher prelude scopes active before actual Default publication");
   const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> witness=parent;
   prelude_armed=true;
   method("benchmark.method.Default",[&]{exact_refusal([&]{parent->advance_installation_default_publication();},
    prelude_reentry?"Installation creator process/thread/source retired":"Pre-DX9 lost last outside startup Parent owner");});
   prelude_armed=false;
   need(prelude_callbacks==1&&prelude_seen_phase==1U&&prelude_prior_tags_clear,
    "Actual postpublisher prelude did not invoke exactly one isolated callback");
   need(revision_callbacks==prelude_callback_ordinal+1,"Postpublisher prelude firstpoint did not return through both actual provider samples");
   need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Postpublisher prelude negative changed the actual HWND/client source");
   need(scopes_clear(),"Postpublisher prelude unwind left a borrower/TLS observer active");
   if(prelude_reentry){
    need(parent&&parent.get()==callback_parent&&!witness.expired()&&prelude_releases==0&&
     prelude_inner_reason=="Retained installation Parent.current reentry","Caught postpublisher prelude reentry changed the actual outside Parent");
    const auto before_parent_control=revision_callbacks;
    exact_refusal([&]{(void)parent->current();},"Installation Parent prefix foreign/source domain/operation changed");
    need(revision_callbacks==before_parent_control,"Poisoned postpublisher prelude Parent.current invoked another provider callback");
   }else{
    need(!parent&&witness.expired()&&callback_parent==nullptr&&prelude_releases==1&&prelude_inner_reason.empty(),
     "Postpublisher prelude callback did not release exactly the last outside Parent through unwind");
   }
   const auto before_input_control=revision_callbacks;
   if(prelude_reentry){
    exact_refusal([&]{installation->require_current();},"Installation input retired");
    need(revision_callbacks==before_input_control,"Sticky postpublisher prelude retirement invoked another provider callback");
   }else{
    installation->require_current();
    need(revision_callbacks==before_input_control+2,"Pre-DX9 owner loss did not leave the same independent Input genuinely current");
   }
   need(scopes_clear(),"Postpublisher prelude control revived a retired borrower/TLS observer");
   negative.end();total.end();report_callbacks("benchmark.total",0);
   std::cout<<"SUBSET postpublisher_prelude_case="<<postpublisher_prelude_case<<" actual_phase="<<prelude_seen_phase
    <<" injected_callbacks="<<prelude_callbacks<<" prior_tags=clear whole32_frontier=exact diagnostic_primary32_counts=unobserved"
    <<" actual_captured_records="<<actual_records<<" record_return=actual publication_before_fault=actual_scoped_commit"
    <<" caught_current="<<(prelude_reentry?"exact":"unissued")<<" outside_Parent="<<(prelude_reentry?"live":"expired")
    <<" outer_refusal=exact Parent_current="<<(prelude_reentry?"poisoned-exact":"unissued-expired")
    <<" Input_after="<<(prelude_reentry?"retired":"current")<<" Input_control_callbacks="<<(revision_callbacks-before_input_control)
    <<" borrower_cleanup=clear prelude_observer_cleanup=clear Default_method_frontier=unissued rollback=unclaimed"
    <<" Boot=unissued Common=unissued Skin=unissued full_game=0 can_continue=0\n";
   return;
  }
  if(!postpublished_prefix_case.empty()){
   Phase negative("postpublished_prefix.callback_refusal");
   const auto scopes_clear=[&]{return !bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
    !ui::bnw_native_primary32_postpublished_prefix_observer_active_for_test()&&
    !ui::bnw_native_primary32_default_publishvalidation_observer_active_for_test()&&
    !ui::bnw_native_primary32_publication_prefix_observer_active_for_test()&&
    !ui::bnw_native_primary32_snapshot_leaf_observer_active_for_test()&&
    !ui::bnw_native_primary32_default_projection_tail_observer_active_for_test()&&
    !ui::bnw_native_firstarchive_post_prefix_observer_active_for_test();};
   need(scopes_clear(),"Postpublished scopes active before actual Default publication");
   const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> witness=parent;
   postpublished_armed=true;
   method("benchmark.method.Default",[&]{exact_refusal([&]{parent->advance_installation_default_publication();},
    postpublished_reentry?"Installation creator process/thread/source retired":postpublished_owner_refusal);});
   postpublished_armed=false;
   need(postpublished_callbacks==1&&postpublished_seen_phase==requested_postpublished_phase&&postpublished_prior_tags_clear,
    "Actual requested postpublished interval did not invoke exactly one isolated callback");
   need(revision_callbacks==postpublished_callback_ordinal+1,"Postpublished firstpoint did not return through both actual provider samples");
   need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Postpublished negative changed the actual HWND/client source");
   need(scopes_clear(),"Postpublished unwind left a borrower/TLS observer active");
   if(postpublished_reentry){
    need(parent&&parent.get()==callback_parent&&!witness.expired()&&postpublished_releases==0&&
     postpublished_inner_reason=="Retained installation Parent.current reentry","Caught postpublished reentry changed the actual outside Parent");
    const auto before_parent_control=revision_callbacks;
    exact_refusal([&]{(void)parent->current();},"Installation Parent prefix foreign/source domain/operation changed");
    need(revision_callbacks==before_parent_control,"Poisoned postpublished Parent.current invoked another provider callback");
   }else{
    need(!parent&&witness.expired()&&callback_parent==nullptr&&postpublished_releases==1&&postpublished_inner_reason.empty(),
     "Postpublished callback did not release exactly the last outside Parent through unwind");
   }
   const auto before_sticky=revision_callbacks;
   exact_refusal([&]{installation->require_current();},"Installation input retired");
   need(revision_callbacks==before_sticky,"Sticky postpublished retirement invoked another provider callback");
   negative.end();total.end();report_callbacks("benchmark.total",0);
   std::cout<<"SUBSET postpublished_prefix_case="<<postpublished_prefix_case<<" actual_phase="<<postpublished_seen_phase
    <<" injected_callbacks="<<postpublished_callbacks<<" prior_tags=clear whole32_frontier=exact diagnostic_primary32_counts=unobserved"
    <<" actual_captured_records="<<actual_records<<" record_return=actual publication_before_fault=actual_scoped_commit"
    <<" caught_current="<<(postpublished_reentry?"exact":"unissued")<<" outside_Parent="<<(postpublished_reentry?"live":"expired")
    <<" outer_refusal=exact Parent_current="<<(postpublished_reentry?"poisoned-exact":"unissued-expired")
    <<" Input_after=retired Input_control_callbacks="<<(revision_callbacks-before_sticky)
    <<" borrower_cleanup=clear postpublished_observer_cleanup=clear Default_method_frontier=unissued rollback=unclaimed"
    <<" Boot=unissued Common=unissued Skin=unissued full_game=0 can_continue=0\n";
   return;
  }
  if(!publishvalidation_case.empty()){
   Phase negative("default_publishvalidation.callback_refusal");
   const auto scopes_clear=[&]{return !bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
    !ui::bnw_native_primary32_default_publishvalidation_observer_active_for_test()&&
    !ui::bnw_native_primary32_publication_prefix_observer_active_for_test()&&
    !ui::bnw_native_primary32_snapshot_leaf_observer_active_for_test()&&
    !ui::bnw_native_primary32_default_projection_tail_observer_active_for_test()&&
    !ui::bnw_native_firstarchive_post_prefix_observer_active_for_test();};
   need(scopes_clear(),"Publishvalidation scopes active before actual Default operation");
   // This weak witness never retains or locks the actual outside Parent.
   const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> witness=parent;
   publishvalidation_armed=true;
   method("benchmark.method.Default",[&]{exact_refusal([&]{parent->advance_installation_default_publication();},
    publishvalidation_reentry?"Installation creator process/thread/source retired":"Default projection is not the privately retained SAME parent/source/index operand");});
   publishvalidation_armed=false;
   need(publishvalidation_callbacks==1&&publishvalidation_seen_phase==requested_publishvalidation_phase&&publishvalidation_prior_tags_clear,
    "Actual requested publishvalidation did not invoke exactly one isolated callback");
   // First validator Input.current precedes Cold/Journal. Both real samples
   // return before the original Input POST controller refuses either case.
   need(revision_callbacks==publishvalidation_callback_ordinal+1,"Publishvalidation refusal did not return through both actual provider samples");
   need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Publishvalidation negative changed the actual HWND/client source");
   need(scopes_clear(),"Publishvalidation unwind left a borrower/TLS observer active");
   if(publishvalidation_reentry){
    need(parent&&parent.get()==callback_parent&&!witness.expired()&&publishvalidation_releases==0&&
     publishvalidation_inner_reason=="Retained installation Parent.current reentry","Caught publishvalidation reentry changed the actual outside Parent");
   }else{
    need(!parent&&witness.expired()&&callback_parent==nullptr&&publishvalidation_releases==1&&publishvalidation_inner_reason.empty(),
     "Publishvalidation callback did not release exactly the last outside Parent through unwind");
   }
   const auto before_sticky=revision_callbacks;
   exact_refusal([&]{installation->require_current();},"Installation input retired");
   need(revision_callbacks==before_sticky,"Sticky publishvalidation retirement invoked another provider callback");
   negative.end();total.end();report_callbacks("benchmark.total",0);
   std::cout<<"SUBSET publishvalidation_case="<<publishvalidation_case<<" actual_phase="<<publishvalidation_seen_phase
    <<" injected_callbacks="<<publishvalidation_callbacks<<" prior_tags=clear whole32_frontier=exact diagnostic_primary32_counts=unobserved"
    <<" actual_captured_records="<<actual_records<<" record_return=actual caught_current="<<(publishvalidation_reentry?"exact":"unissued")
    <<" outside_Parent="<<(publishvalidation_reentry?"live":"expired")<<" outer_refusal=exact Input_after=retired Input_control_callbacks="<<(revision_callbacks-before_sticky)
    <<" borrower_cleanup=clear publishvalidation_observer_cleanup=clear Default_publication=refused Common=unissued Skin=unissued full_game=0 can_continue=0\n";
   return;
  }
  Phase negative("publication_prefix.callback_refusal");
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
   !ui::bnw_native_primary32_publication_prefix_observer_active_for_test(),"Publication prefix scopes active before actual publication");
  // Witness has no strong owner, and is never locked after the real reset.
  const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> witness=parent;
  publication_armed=true;
  method("benchmark.method.Default",[&]{exact_refusal([&]{parent->advance_installation_default_publication();},
   publication_reentry?"Installation creator process/thread/source retired":"Native primary32 retained source lost its genuine external Package facade");});
  publication_armed=false;
  need(publication_callbacks==1&&publication_seen_phase==requested_publication_phase&&
   publication_seen_journal==0&&publication_seen_leaf==0&&publication_seen_tail==0,
   "Actual requested publication prefix did not invoke exactly one isolated callback");
  // Reentry fails first Input POST. Owner loss reaches original Journal check
  // only after the entire Cold FULL (two Input.current, four real samples).
  need(revision_callbacks==publication_callback_ordinal+(publication_reentry?1U:3U),
   "Publication prefix refusal did not preserve the actual Input/Cold callback boundary");
  need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Publication prefix negative changed the actual HWND/client source");
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
   !ui::bnw_native_primary32_publication_prefix_observer_active_for_test()&&
   !ui::bnw_native_primary32_snapshot_leaf_observer_active_for_test()&&
   !ui::bnw_native_primary32_default_projection_tail_observer_active_for_test(),"Publication prefix unwind left a borrower/TLS observer active");
  const auto before_input_control=revision_callbacks;
  if(publication_reentry){
   need(parent&&parent.get()==callback_parent&&!witness.expired()&&publication_releases==0&&
    publication_inner_reason=="Retained installation Parent.current reentry","Caught publication prefix reentry changed the actual outside Parent");
   exact_refusal([&]{installation->require_current();},"Installation input retired");
   need(revision_callbacks==before_input_control,"Sticky publication prefix retirement invoked another provider callback");
  }else{
   need(!parent&&witness.expired()&&callback_parent==nullptr&&publication_releases==1&&publication_inner_reason.empty(),
    "Publication prefix callback did not release exactly the last outside Parent through unwind");
   installation->require_current();
   need(revision_callbacks==before_input_control+2,"Publication owner loss did not leave the same independent Input genuinely current");
  }
  negative.end();total.end();report_callbacks("benchmark.total",0);
  std::cout<<"SUBSET publication_prefix_case="<<publication_prefix_case<<" actual_phase="<<publication_seen_phase
   <<" injected_callbacks="<<publication_callbacks<<" prior_Journal_tag="<<publication_seen_journal
   <<" prior_leaf_tag="<<publication_seen_leaf<<" prior_tail_tag="<<publication_seen_tail
   <<" actual_captured_records="<<actual_records<<" record_return=actual caught_current="<<(publication_reentry?"exact":"unissued")
   <<" outside_Parent="<<(publication_reentry?"live":"expired")<<" outer_refusal=exact Input_after="<<(publication_reentry?"retired":"current")
   <<" Input_control_callbacks="<<(revision_callbacks-before_input_control)<<" borrower_cleanup=clear publication_observer_cleanup=clear"
   <<" Default_publication=refused Common=unissued Skin=unissued full_game=0 can_continue=0\n";
  return;
 }
 const auto admission=value("benchmark.verify.firstarchive",[&]{
  const auto actual=parent->inspect_installation_first_archive_admission();
  need(actual.admitted_entry_count>0&&actual.admitted_entry_count<=actual.declared_entry_count,
   "Benchmark first archive substituted declared count for genuine positive admission");return actual;
 });
 method("benchmark.method.whole32",[&]{exact_refusal([&]{parent->advance_installation_primary32_archives();},
  "Mandatory actual installed primary32 record snapshot/Default projection/publication before DX9/Boot");});
 const auto primary=value("benchmark.verify.whole32",[&]{
  const auto actual=parent->inspect_installation_primary32_archives();
  need(actual.positive_primary32_complete&&actual.completed_archives==32&&actual.executed_admitted_total>=admission.admitted_entry_count,
   "Benchmark lacks actual complete32 positive source returns");return actual;
 });
 if(!record_journal_case.empty()){
  Phase negative(requested_record_phase==1?"record_journal.pre_brand.caught_parent_current":"record_journal.pending_projection.caught_parent_current");
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
   bnw_native_primary32_record_projection_journal_phase_for_test(*parent)==0,"Record-journal scopes active before actual record operation");
  const auto* actual_parent=parent.get();record_armed=true;
  method("benchmark.method.recordsnapshot",[&]{exact_refusal([&]{(void)parent->advance_installation_record_snapshot_default_projection();},
   "Installation creator process/thread/source retired");});
  record_armed=false;
  need(record_callbacks==1&&record_seen_phase==requested_record_phase&&record_inner_reason=="Retained installation Parent.current reentry",
   "Actual requested record-journal borrower did not run exactly one caught-current callback");
  need(revision_callbacks==record_callback_ordinal+1,"Caught record-journal current did not return through both actual unchanged provider samples");
  need(parent.get()==actual_parent&&IsWindow(window.hwnd)!=FALSE&&revision==1,"Record-journal negative lost the actual Parent/window/client source");
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
   bnw_native_primary32_record_projection_journal_phase_for_test(*parent)==0,"Record-journal unwind left an actual borrower/phase tag active");
  const auto before_sticky=revision_callbacks;
  exact_refusal([&]{installation->require_current();},"Installation input retired");
  need(revision_callbacks==before_sticky,"Sticky record-journal retirement invoked another actual provider callback");
  negative.end();total.end();report_callbacks("benchmark.total",0);
  std::cout<<"SUBSET record_journal_case="<<record_journal_case<<" actual_phase="<<record_seen_phase<<" injected_callbacks="<<record_callbacks
   <<" actual_primary32_completed="<<primary.completed_archives<<" actual_primary32_executed_admitted="<<primary.executed_admitted_total
   <<" caught_current=exact outer_refusal=exact sticky_input=refused borrower_cleanup=clear record_phase_cleanup=clear"
   <<" record_return=unissued Default=unissued Common=unissued Skin=unissued full_game=0 can_continue=0\n";
  return;
 }
 if(!snapshot_leaf_case.empty()){
  Phase negative("snapshot_leaf.callback_refusal");
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
   !ui::bnw_native_primary32_snapshot_leaf_observer_active_for_test(),"Snapshot leaf scopes active before actual record operation");
  // A weak witness never adds an outside owner. Do not lock it.
  const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> witness=parent;
  leaf_armed=true;
  method("benchmark.method.recordsnapshot",[&]{exact_refusal([&]{(void)parent->advance_installation_record_snapshot_default_projection();},
   leaf_reentry?"Installation creator process/thread/source retired":"Retained Snapshot leaf operation lost actual source/fence/outside Parent");});
  leaf_armed=false;
  need(leaf_callbacks==1&&leaf_seen_phase==requested_leaf_phase,"Actual requested Snapshot leaf borrower did not invoke exactly one callback");
  need(revision_callbacks==leaf_callback_ordinal+1,"Snapshot leaf negative did not return through both actual unchanged provider samples");
  need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Snapshot leaf negative changed the actual HWND/client source");
  // These two observers need no Parent facade after owner-loss unwind.
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
   !ui::bnw_native_primary32_snapshot_leaf_observer_active_for_test(),"Snapshot leaf unwind left a borrower/TLS observer active");
  if(leaf_reentry){
   need(parent&&parent.get()==callback_parent&&!witness.expired()&&leaf_releases==0&&
    leaf_inner_reason=="Retained installation Parent.current reentry","Caught Snapshot leaf reentry changed the actual outside Parent");
  }else{
   need(!parent&&witness.expired()&&callback_parent==nullptr&&leaf_releases==1&&leaf_inner_reason.empty(),
    "Snapshot leaf callback did not release exactly the last outside Parent through unwind");
  }
  const auto before_sticky=revision_callbacks;
  exact_refusal([&]{installation->require_current();},"Installation input retired");
  need(revision_callbacks==before_sticky,"Sticky Snapshot leaf retirement invoked another provider callback");
  negative.end();total.end();report_callbacks("benchmark.total",0);
  std::cout<<"SUBSET snapshot_leaf_case="<<snapshot_leaf_case<<" actual_phase="<<leaf_seen_phase<<" injected_callbacks="<<leaf_callbacks
   <<" actual_primary32_completed="<<primary.completed_archives<<" actual_primary32_executed_admitted="<<primary.executed_admitted_total
   <<" caught_current="<<(leaf_reentry?"exact":"unissued")<<" outside_Parent="<<(leaf_reentry?"live":"expired")
   <<" outer_refusal=exact sticky_input=refused borrower_cleanup=clear leaf_observer_cleanup=clear"
   <<" record_return=unissued Default=unissued Common=unissued Skin=unissued full_game=0 can_continue=0\n";
  return;
 }
 const auto actual_records=value("benchmark.method.recordsnapshot",[&]{return parent->advance_installation_record_snapshot_default_projection();});
 need(actual_records==primary.executed_admitted_total,"Benchmark actual captured records differ from whole32 entry returns");
 method("benchmark.method.Default",[&]{exact_refusal([&]{parent->advance_installation_default_publication();},
  "Mandatory actual installed low-resolution source CVarD80DE0 before Common/default/Skin/Boot");});
 const auto published=value("benchmark.verify.Default",[&]{
  const auto actual=parent->current();
  need(actual.primary_dx9_registration_complete&&actual.archives_registered==32&&actual.default_indexed_names==actual_records&&
   actual.resource_registration_order.size()==32&&!actual.native_skin_loading_complete&&actual.next_resource_dependency==
   "Mandatory actual installed low-resolution source CVarD80DE0 before Common/default/Skin/Boot",
   "Benchmark actual same32 Default publication differs from captured nodes or source frontier");return actual;
 });
 const auto boot=value("benchmark.method.Boot",[&]{return parent->advance_installation_boot_settings();});
 need(boot.normal_launch_source_closed&&!boot.leader_viewer&&boot.effective_texture_quality.has_value(),
  "Benchmark actual installed Boot lacks genuine normal-launch texture source");
 if(*boot.effective_texture_quality==0){
  method("benchmark.method.TextureQuality",[&]{exact_refusal([&]{parent->advance_installation_high_texture_quality_branch();},
   "Mandatory actual installed low22 archive constructor/cache source before zero TextureQuality branch");});
  throw std::runtime_error("Startup benchmark cannot reach52D590: actual zero TextureQuality requires unimplemented low22 source");
 }
 method("benchmark.method.TextureQuality",[&]{exact_refusal([&]{parent->advance_installation_high_texture_quality_branch();},
  "Mandatory actual installed Common13 constructor/cache source before default loose/Skin");});
 const auto common_declared=value("benchmark.method.Common0.Header",[&]{return parent->advance_installation_first_common_archive_header();});
 std::uint32_t common0_admitted=0;
 if(common_declared){
  method("benchmark.method.Common0.metadata",[&]{exact_refusal([&]{parent->advance_installation_first_common_archive_metadata();},
   "Mandatory actual installed first Common admission source transition/cache before remaining Common/default loose/Skin");});
  common0_admitted=value("benchmark.method.Common0.admission",[&]{return parent->advance_installation_first_common_archive_admission();});
  need(common0_admitted<=*common_declared,"Benchmark Common0 actual admission exceeded actual Header declaration");
 }
 const auto common13_admitted=value("benchmark.method.Common13",[&]{return parent->advance_installation_remaining_common_archives();});
 need(common13_admitted>=common0_admitted,"Benchmark actual whole Common13 count lost its first archive returns");
 method("benchmark.method.DefaultLoose",[&]{parent->advance_installation_default_loose_source();});
 method("benchmark.method.DefaultCommon.publication",[&]{parent->publish_installation_default_common_source();});
 method("benchmark.method.DefaultCommon.adoption",[&]{ui::detail::BnwNativeDefaultCommonProduction::adopt_installed_pre_dx9_receivers(*parent);});
 method("benchmark.method.StartupAdmission",[&]{parent->advance_installation_startup_admission();});
 // The genuine driver executes Source6a itself when no prefix was requested.
 method("benchmark.method.initial656440",[&]{parent->drive_installation_initial_categories();});
 const auto final=value("benchmark.verify.endpoint52D590",[&]{
  const auto actual=parent->current();
  need(actual.packages>0&&actual.original_descriptor_order.size()==actual.packages&&actual.descriptor_index_misses==0&&
   actual.descriptor_direct_reads==0&&actual.potential_package_color_maps_complete&&
   actual.primary_dx9_registration_complete&&actual.common_high_quality_registration_complete&&
   actual.low_resolution_source_branch_complete&&actual.high_texture_quality_source_branch&&!actual.low_resource_mount_present&&
   actual.archives_registered>=published.archives_registered&&actual.archives_registered<=published.archives_registered+13&&
   actual.default_indexed_names>=actual_records&&actual.loose_registration_requests==3&&actual.resource_registration_order.size()==48&&
   actual.resource_registration_order[45]=="Resource\\Overrides"&&actual.resource_registration_order[46]=="Assets"&&
   actual.resource_registration_order[47]=="Assets\\Sounds\\Speech\\Shared"&&!actual.native_skin_loading_complete&&actual.next_resource_dependency==
   "Actual initial656440 per-package0/1 effects returned; mandatory52D590 Color/Skin and subsequent completed startup suppliers",
   "Benchmark genuine installed category return changed source/count/order or did not stop at52D590");
  need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Benchmark actual HWND/client source changed");return actual;
 });
 if(installed_skin_pc_benchmark){
  const auto pc=run_installed_skin_pc_benchmark_tail(parent,final,method,value,[&]{
   need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Skin/PC benchmark changed the actual HWND/client source");
  });
  if(installed_skin_global_benchmark){
   method("benchmark.method.Source8k.global4691E0",[&]{parent->drive_installation_global_skin();});
   const auto global=value("benchmark.verify.endpointGlobal4691E0",[&]{
    const auto actual=parent->current();
    need(actual.packages==pc.packages&&actual.original_descriptor_order==pc.original_descriptor_order&&
     actual.primary_dx9_registration_complete&&actual.common_high_quality_registration_complete&&
     actual.low_resolution_source_branch_complete&&actual.high_texture_quality_source_branch&&
     !actual.low_resource_mount_present&&!actual.native_skin_loading_complete,
     "Global Skin benchmark changed genuine installed source lineage or claimed complete Skin");
    need(actual.next_resource_dependency==
     "Actual installed4691E0(-1), both actual slot map/list closes and full physical POST returned; SAME late Speech Default/624920 and terminal suppliers remain mandatory",
     "Global Skin benchmark did not reach its actual post-retirement late-tail frontier");
    need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Global Skin benchmark changed the actual HWND/client source");
    return actual;
   });
   if(installed_late_speech_benchmark){
    method("benchmark.method.Source9a.lateSpeechAndMappings",[&]{parent->drive_installation_late_speech_and_mappings();});
    const auto late=value("benchmark.verify.endpoint54CE60Returned",[&]{
     const auto actual=parent->current();
     need(actual.packages==global.packages&&actual.original_descriptor_order==global.original_descriptor_order&&
      actual.primary_dx9_registration_complete&&actual.common_high_quality_registration_complete&&
      actual.low_resolution_source_branch_complete&&actual.high_texture_quality_source_branch&&
      !actual.low_resource_mount_present&&!actual.native_skin_loading_complete,
      "Late Speech benchmark changed genuine installed lineage or claimed complete Skin");
     need(actual.next_resource_dependency==
      "Actual late Speech Default directory effects, SAME retained FS33 appends and specific54CE60 return1 closed; terminal/resource/window/World32 gates remain required",
      "Late Speech benchmark did not reach its actual outside returned frontier");
     need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Late Speech benchmark changed the actual HWND/client source");
     return actual;
    });
    total.end();report_callbacks("benchmark.total",0);
    std::cout<<"SUBSET installed_late_speech_benchmark=1 actual_packages="<<late.packages
     <<" actual_primary32_completed="<<primary.completed_archives<<" actual_primary32_executed_admitted="<<primary.executed_admitted_total
     <<" actual_captured_records="<<actual_records<<" actual_Common13_admitted="<<common13_admitted
     <<" actual_archives_at_late_return="<<late.archives_registered<<" actual_Default_names_at_late_return="<<late.default_indexed_names
     <<" revision_callbacks="<<revision_callbacks
     <<" Source7=normal_return Source8a=normal_return Source8b=normal_return Source8c=normal_return Source8d=normal_return Source8e=normal_return Source8k=normal_return Source9a=normal_return"
     <<" endpoint=54CE60_returned late_Speech=returned SAME_FS33=returned next=terminal_resource_window"
     <<" Skin=partial World32=unissued full_game=0 can_continue=0\n";
    return;
   }
   total.end();report_callbacks("benchmark.total",0);
   std::cout<<"SUBSET installed_skin_global_benchmark=1 actual_packages="<<global.packages
    <<" actual_primary32_completed="<<primary.completed_archives<<" actual_primary32_executed_admitted="<<primary.executed_admitted_total
    <<" actual_captured_records="<<actual_records<<" actual_Common13_admitted="<<common13_admitted
    <<" actual_archives_at_global_return="<<global.archives_registered<<" actual_Default_names_at_global_return="<<global.default_indexed_names
    <<" revision_callbacks="<<revision_callbacks
    <<" Source7=normal_return Source8a=normal_return Source8b=normal_return Source8c=normal_return Source8d=normal_return Source8e=normal_return Source8k=normal_return"
    <<" endpoint=global4691E0_returned next=late_Speech_Default_624920 actual_slot_closes=returned physical_POST=returned"
    <<" late_Default=pending Skin=partial full_game=0 can_continue=0\n";
   return;
  }
  total.end();report_callbacks("benchmark.total",0);
  std::cout<<"SUBSET installed_skin_pc_benchmark=1 actual_packages="<<pc.packages
   <<" actual_primary32_completed="<<primary.completed_archives<<" actual_primary32_executed_admitted="<<primary.executed_admitted_total
   <<" actual_captured_records="<<actual_records<<" actual_Common13_admitted="<<common13_admitted
   <<" actual_archives_at_initial_close="<<pc.archives_registered<<" actual_Default_names_at_initial_close="<<pc.default_indexed_names
   <<" revision_callbacks="<<revision_callbacks
   <<" Source7=normal_return Source8a=normal_return Source8b=normal_return Source8c=normal_return Source8d=normal_return Source8e=normal_return"
   <<" endpoint=initial6B9530_closed next=4691E0_global new_per_slot_owners=pending late_Default=pending"
   <<" Skin=partial full_game=0 can_continue=0\n";
  return;
 }
 total.end();report_callbacks("benchmark.total",0);
 std::cout<<"SUBSET installed_startup_benchmark=1 actual_packages="<<final.packages
  <<" actual_first_archive_declared="<<admission.declared_entry_count<<" actual_first_archive_admitted="<<admission.admitted_entry_count
  <<" actual_primary32_completed="<<primary.completed_archives<<" actual_primary32_executed_admitted="<<primary.executed_admitted_total
  <<" actual_captured_records="<<actual_records<<" actual_published_Default_records="<<published.default_indexed_names
  <<" actual_Boot_TextureQuality="<<*boot.effective_texture_quality<<" actual_Common0_present="<<bool(common_declared)
  <<" actual_Common0_declared="<<common_declared.value_or(0)<<" actual_Common0_admitted="<<common0_admitted
  <<" actual_Common13_admitted="<<common13_admitted<<" actual_final_archives="<<final.archives_registered
  <<" actual_final_Default_names="<<final.default_indexed_names<<" revision_callbacks="<<revision_callbacks
  <<" Source5=normal_return Source6a_prefix=normal_return initial656440=normal_return next=52D590"
  <<" ColorSkin=pending Skin=unissued full_game=0 can_continue=0\n";
}
void run_parent_loss_fixture(const char* original_install,const char* canonical_assets,
 std::uintptr_t hwnd,const std::uint64_t& revision){
 // Real creator/provider callback releases the ONLY external Parent during
 // the same genuine installed source method as the ordinary full fixture.
 Phase parent_loss_phase("parent_loss");
 std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner> doomed;bool drop=false;
 std::size_t revision_callbacks=0,release_count=0,release_ordinal=0;
 auto dropping_input=ui::BnwNativeInstallationInputOwner::acquire(original_install,canonical_assets,hwnd,
  runtime::bnw_stock_lua_win32_client_extent(hwnd,"real-zero-save-last-parent-loss",[&]{
   ++revision_callbacks;if(drop){if(doomed){++release_count;release_ordinal=revision_callbacks;}doomed.reset();}return revision;
  }));
 doomed=ui::BnwNativePackageDiscoveryOwner::create(dropping_input);
 mandatory([&]{doomed->advance_installation_category2_first_prefix();});
 mandatory([&]{doomed->advance_installation_retained_ui_prefix();},"Mandatory actual installation first-header Cold/Stat");
 // A weak witness never creates an outside owner. Do not lock it.
 const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> witness=doomed;
 const auto callbacks_before_arm=revision_callbacks;std::string loss_reason;bool loss_threw=false;
 const auto* actual_doomed=doomed.get();drop=true;
 try{actual_doomed->advance_installation_cold_stat_prefix();}catch(const std::exception& e){loss_threw=true;loss_reason=e.what();}
 // The raw facade is not accessed after the call has unwound.
 const auto callbacks_after_loss=revision_callbacks;const bool facade_released=!doomed,weak_expired=witness.expired();
 const bool live_hwnd=IsWindow(reinterpret_cast<HWND>(hwnd))!=FALSE;
 bool input_current=false;std::string input_reason;
 try{dropping_input->require_current();input_current=true;}catch(const std::exception& e){input_reason=e.what();}
 std::cout<<"installation_prefix parent_loss loss_threw="<<loss_threw<<" reason=\""<<(loss_threw?loss_reason:"<normal return>")
  <<"\" callbacks_before_arm="<<callbacks_before_arm<<" callbacks_after_loss="<<callbacks_after_loss
  <<" release_count="<<release_count<<" release_ordinal="<<release_ordinal<<" facade_released="<<facade_released
  <<" weak_expired="<<weak_expired<<" live_hwnd="<<live_hwnd<<" independent_input_current="<<input_current
  <<" independent_input_reason=\""<<input_reason<<"\" independent_input_callbacks="<<(revision_callbacks-callbacks_after_loss)<<'\n';
 need(release_count==1&&release_ordinal>callbacks_before_arm&&release_ordinal<=callbacks_after_loss,
  "Last-Parent fixture did not release exactly one real external facade in the armed callback");
 need(facade_released&&weak_expired,"Real lost external Parent was hidden by self/facade/HeaderGuard pins");
 need(loss_threw&&loss_reason=="Native shared FIRST persistent parent retired",
  "Last-Parent loss did not report exact FIRST persistent-parent refusal");
 need(live_hwnd&&input_current&&revision_callbacks>callbacks_after_loss,
  "Parent loss fixture did not preserve an independently current real Installation input/window/provider");
 parent_loss_phase.end();
}
#include "bnw_native_installation_held_metadata_fixture.inc"
#include "bnw_native_installation_firstarchive_post_prefix_fixture.inc"
}
int main(int argc,char** argv){try{
 std::cout << std::unitbuf;
 if(argc>1&&std::string_view(argv[1])=="--firstarchive-post-prefix-case"){
  need(argc==5,"Expected --firstarchive-post-prefix-case reentry|parent-loss ORIGINAL_INSTALL CANONICAL_ASSETS");
  firstarchive_post_prefix_fixture::run(argv[3],argv[4],argv[2]);return 0;
 }
 if(argc>1&&std::string_view(argv[1])=="--held-metadata-case"){
  need(argc==5,"Expected --held-metadata-case transient|persistent|unwind-selected ORIGINAL_INSTALL CANONICAL_ASSETS");
  held_metadata_fixture::run(argv[3],argv[4],argv[2]);return 0;
 }
 if(argc>1&&std::string_view(argv[1])=="--parent-loss-only"){
  need(argc==4,"Expected --parent-loss-only ORIGINAL_INSTALL CANONICAL_ASSETS");
  Window focused_window;need(focused_window.hwnd&&!IsWindowVisible(focused_window.hwnd),"Actual current-process hidden HWND missing");
  const auto focused_hwnd=reinterpret_cast<std::uintptr_t>(focused_window.hwnd);const std::uint64_t focused_revision=1;
  run_parent_loss_fixture(argv[2],argv[3],focused_hwnd,focused_revision);
  std::cout<<"SUBSET parent_loss_only=1 full_game=0 can_continue=0\n";
  return 0;
 }
 // Keep language on the real process argv: product_options reads GetCommandLineW.
 bool pre_dx9=false,pre_dx9_prefix_only=false,installed_default_common=false,installed_default_common_reentry=false;
 bool installed_startup_benchmark=false,installed_skin_pc_benchmark=false,installed_skin_global_benchmark=false,installed_late_speech_benchmark=false;
 bool retained_current_reentry=false,firstarchive_current_reentry=false,acceptance_current_reentry=false,completed32_current_reentry=false,primary32_timed=false;
 std::string_view ui_language,installed_startup_case,initial_metadata_case,record_journal_case,snapshot_leaf_case,projection_tail_case,publication_prefix_case,publishvalidation_case,postpublished_prefix_case,postpublisher_prelude_case,boot_construction_case;std::array<const char*,2> roots{};std::size_t root_count=0;
 for(int arg=1;arg<argc;++arg){const std::string_view value(argv[arg]);
  if(value=="--pre-dx9"){need(!pre_dx9,"Duplicate --pre-dx9");pre_dx9=true;}
  else if(value=="--pre-dx9-prefix-only"){need(!pre_dx9_prefix_only,"Duplicate --pre-dx9-prefix-only");pre_dx9_prefix_only=true;}
  else if(value=="--installed-default-common"){need(!installed_default_common,"Duplicate --installed-default-common");installed_default_common=true;}
  else if(value=="--installed-default-common-reentry"){need(!installed_default_common_reentry,"Duplicate --installed-default-common-reentry");installed_default_common_reentry=true;}
  else if(value=="--retained-current-reentry"){need(!retained_current_reentry,"Duplicate --retained-current-reentry");retained_current_reentry=true;}
  else if(value=="--firstarchive-current-reentry"){need(!firstarchive_current_reentry,"Duplicate --firstarchive-current-reentry");firstarchive_current_reentry=true;}
  else if(value=="--acceptance-current-reentry"){need(!acceptance_current_reentry,"Duplicate --acceptance-current-reentry");acceptance_current_reentry=true;}
  else if(value=="--completed32-current-reentry"){need(!completed32_current_reentry,"Duplicate --completed32-current-reentry");completed32_current_reentry=true;}
  else if(value=="--primary32-timed"){need(!primary32_timed,"Duplicate --primary32-timed");primary32_timed=true;}
  else if(value=="--installed-startup-benchmark"){need(!installed_startup_benchmark,"Duplicate --installed-startup-benchmark");installed_startup_benchmark=true;}
  else if(value=="--installed-skin-pc-benchmark"){need(!installed_skin_pc_benchmark,"Duplicate --installed-skin-pc-benchmark");installed_skin_pc_benchmark=true;}
  else if(value=="--installed-skin-global-benchmark"){need(!installed_skin_pc_benchmark,"Duplicate or conflicting installed Skin benchmark");installed_skin_pc_benchmark=true;installed_skin_global_benchmark=true;}
  else if(value=="--installed-late-speech-benchmark"){need(!installed_skin_pc_benchmark,"Duplicate or conflicting installed Skin benchmark");installed_skin_pc_benchmark=true;installed_skin_global_benchmark=true;installed_late_speech_benchmark=true;}
  else if(value=="--boot-construction-case"){
   need(boot_construction_case.empty()&&arg+1<argc,"Missing or duplicate --boot-construction-case value");boot_construction_case=argv[++arg];
   need(boot_construction_case=="reentry"||boot_construction_case=="parent-loss"||boot_construction_case=="returned","Expected Boot construction reentry, parent-loss or returned case");
  }
  else if(value=="--postpublisher-prelude-case"){
   need(postpublisher_prelude_case.empty()&&arg+1<argc,"Missing or duplicate --postpublisher-prelude-case value");postpublisher_prelude_case=argv[++arg];
   need(postpublisher_prelude_case=="reentry"||postpublisher_prelude_case=="parent-loss","Expected postpublisher prelude reentry or parent-loss case");
  }
  else if(value=="--postpublished-prefix-case"){
   need(postpublished_prefix_case.empty()&&arg+1<argc,"Missing or duplicate --postpublished-prefix-case value");postpublished_prefix_case=argv[++arg];
   need(postpublished_prefix_case=="journal-reentry"||postpublished_prefix_case=="snapshot-reentry"||postpublished_prefix_case=="projection-reentry"||
    postpublished_prefix_case=="journal-parent-loss"||postpublished_prefix_case=="snapshot-parent-loss"||postpublished_prefix_case=="projection-parent-loss",
    "Expected postpublished journal/snapshot/projection reentry/parent-loss case");
  }
  else if(value=="--publishvalidation-case"){
   need(publishvalidation_case.empty()&&arg+1<argc,"Missing or duplicate --publishvalidation-case value");publishvalidation_case=argv[++arg];
   need(publishvalidation_case=="a-reentry"||publishvalidation_case=="b-reentry"||
    publishvalidation_case=="a-parent-loss"||publishvalidation_case=="b-parent-loss","Expected publishvalidation a/b reentry/parent-loss case");
  }
  else if(value=="--publication-prefix-case"){
   need(publication_prefix_case.empty()&&arg+1<argc,"Missing or duplicate --publication-prefix-case value");publication_prefix_case=argv[++arg];
   need(publication_prefix_case=="prerequisite-reentry"||publication_prefix_case=="projection-reentry"||
    publication_prefix_case=="prerequisite-parent-loss"||publication_prefix_case=="projection-parent-loss","Expected publication prefix prerequisite/projection reentry/parent-loss case");
  }
  else if(value=="--projection-tail-case"){
   need(projection_tail_case.empty()&&arg+1<argc,"Missing or duplicate --projection-tail-case value");projection_tail_case=argv[++arg];
   need(projection_tail_case=="pending-reentry"||projection_tail_case=="prepared-reentry"||
    projection_tail_case=="pending-parent-loss"||projection_tail_case=="prepared-parent-loss","Expected projection tail pending/prepared reentry/parent-loss case");
  }
  else if(value=="--snapshot-leaf-case"){
   need(snapshot_leaf_case.empty()&&arg+1<argc,"Missing or duplicate --snapshot-leaf-case value");snapshot_leaf_case=argv[++arg];
   need(snapshot_leaf_case=="construction-reentry"||snapshot_leaf_case=="stored-reentry"||
    snapshot_leaf_case=="construction-parent-loss"||snapshot_leaf_case=="stored-parent-loss","Expected Snapshot leaf construction/stored reentry/parent-loss case");
  }
  else if(value=="--record-journal-case"){
   need(record_journal_case.empty()&&arg+1<argc,"Missing or duplicate --record-journal-case value");record_journal_case=argv[++arg];
   need(record_journal_case=="pre-brand"||record_journal_case=="pending-projection","Expected record-journal case pre-brand or pending-projection");
  }
  else if(value=="--installed-startup-case"){
   need(installed_startup_case.empty()&&arg+1<argc,"Missing or duplicate --installed-startup-case value");installed_startup_case=argv[++arg];
   need(installed_startup_case=="prefix"||installed_startup_case=="reentry"||installed_startup_case=="parent-loss"||installed_startup_case=="categories",
    "Expected installed startup case prefix, reentry, parent-loss or categories");
  }
  else if(value=="--initial-metadata-case"){
   need(initial_metadata_case.empty()&&arg+1<argc,"Missing or duplicate --initial-metadata-case value");initial_metadata_case=argv[++arg];
   need(initial_metadata_case=="reentry-pre"||initial_metadata_case=="reentry-post"||
    initial_metadata_case=="parent-loss-pre"||initial_metadata_case=="parent-loss-post","Expected metadata reentry/parent-loss pre/post case");
   // Reserved Snapshot layer: remain exclusive after its independent85977826 composition.
   for(int other=1;other<argc;++other)need(std::string_view(argv[other])!="--completed32-current-reentry",
    "Initial metadata and completed32 reentry cases are mutually exclusive");
  }
  else if(value=="--ui-language"){
   need(ui_language.empty()&&arg+1<argc,"Missing or duplicate --ui-language value");ui_language=argv[++arg];
   need(!ui_language.empty()&&ui_language.size()<=63&&!ui_language.starts_with("--"),"Expected actual product UI language Type");
   for(const unsigned char byte:ui_language)need(byte>0&&byte<128,"Product UI language Type must be ASCII");
  }else{need(!value.starts_with("--")&&root_count<roots.size(),"Expected two paths and optional --pre-dx9 --ui-language TYPE --pre-dx9-prefix-only --installed-default-common --installed-default-common-reentry --retained-current-reentry --firstarchive-current-reentry --acceptance-current-reentry --completed32-current-reentry --primary32-timed --installed-startup-case CASE --installed-startup-benchmark --installed-skin-pc-benchmark --installed-skin-global-benchmark --installed-late-speech-benchmark --record-journal-case CASE --snapshot-leaf-case CASE --projection-tail-case CASE --publication-prefix-case CASE --publishvalidation-case CASE --postpublished-prefix-case CASE --postpublisher-prelude-case CASE --boot-construction-case CASE");
   roots[root_count++]=argv[arg];}
 }
 need(boot_construction_case.empty()||(pre_dx9&&!installed_startup_benchmark&&!installed_skin_pc_benchmark&&!installed_skin_global_benchmark&&!installed_late_speech_benchmark&&
  !pre_dx9_prefix_only&&!installed_default_common&&!installed_default_common_reentry&&!retained_current_reentry&&!firstarchive_current_reentry&&
  !acceptance_current_reentry&&!completed32_current_reentry&&!primary32_timed&&installed_startup_case.empty()&&initial_metadata_case.empty()&&
  record_journal_case.empty()&&snapshot_leaf_case.empty()&&projection_tail_case.empty()&&publication_prefix_case.empty()&&
  publishvalidation_case.empty()&&postpublished_prefix_case.empty()&&postpublisher_prelude_case.empty()),"Boot construction cases require a separate genuine --pre-dx9 benchmark setup");
 need(postpublisher_prelude_case.empty()||(pre_dx9&&!installed_startup_benchmark&&!installed_skin_pc_benchmark&&!installed_skin_global_benchmark&&
  !pre_dx9_prefix_only&&!installed_default_common&&!installed_default_common_reentry&&!retained_current_reentry&&!firstarchive_current_reentry&&
  !acceptance_current_reentry&&!completed32_current_reentry&&!primary32_timed&&installed_startup_case.empty()&&initial_metadata_case.empty()&&
  record_journal_case.empty()&&snapshot_leaf_case.empty()&&projection_tail_case.empty()&&publication_prefix_case.empty()&&
  publishvalidation_case.empty()&&postpublished_prefix_case.empty()),"Postpublisher prelude cases require a separate genuine --pre-dx9 benchmark setup");
 need(postpublished_prefix_case.empty()||(pre_dx9&&!installed_startup_benchmark&&!installed_skin_pc_benchmark&&!pre_dx9_prefix_only&&
  !installed_default_common&&!installed_default_common_reentry&&!retained_current_reentry&&!firstarchive_current_reentry&&
  !acceptance_current_reentry&&!completed32_current_reentry&&!primary32_timed&&installed_startup_case.empty()&&initial_metadata_case.empty()&&
  record_journal_case.empty()&&snapshot_leaf_case.empty()&&projection_tail_case.empty()&&publication_prefix_case.empty()&&publishvalidation_case.empty()),
  "Postpublished cases require a separate genuine --pre-dx9 benchmark setup");
 need(publishvalidation_case.empty()||(pre_dx9&&!installed_startup_benchmark&&!installed_skin_pc_benchmark&&!pre_dx9_prefix_only&&
  !installed_default_common&&!installed_default_common_reentry&&!retained_current_reentry&&!firstarchive_current_reentry&&
  !acceptance_current_reentry&&!completed32_current_reentry&&!primary32_timed&&installed_startup_case.empty()&&initial_metadata_case.empty()&&
  record_journal_case.empty()&&snapshot_leaf_case.empty()&&projection_tail_case.empty()&&publication_prefix_case.empty()),
  "Publishvalidation cases require a separate genuine --pre-dx9 benchmark setup");
 need(publication_prefix_case.empty()||(pre_dx9&&!installed_startup_benchmark&&!pre_dx9_prefix_only&&!installed_default_common&&!installed_default_common_reentry&&
  !retained_current_reentry&&!firstarchive_current_reentry&&!acceptance_current_reentry&&!completed32_current_reentry&&!primary32_timed&&
  record_journal_case.empty()&&snapshot_leaf_case.empty()&&projection_tail_case.empty()&&installed_startup_case.empty()&&initial_metadata_case.empty()),"Publication prefix cases require a separate genuine --pre-dx9 benchmark setup");
 need(!installed_skin_pc_benchmark||(pre_dx9&&!installed_startup_benchmark&&!pre_dx9_prefix_only&&!installed_default_common&&!installed_default_common_reentry&&
  !retained_current_reentry&&!firstarchive_current_reentry&&!acceptance_current_reentry&&!completed32_current_reentry&&!primary32_timed&&
  installed_startup_case.empty()&&initial_metadata_case.empty()&&record_journal_case.empty()&&snapshot_leaf_case.empty()&&
  projection_tail_case.empty()&&publication_prefix_case.empty()),"Skin/PC benchmark requires a separate genuine --pre-dx9 setup and no focused negative mode");
 need(projection_tail_case.empty()||(pre_dx9&&!installed_startup_benchmark&&!pre_dx9_prefix_only&&!installed_default_common&&!installed_default_common_reentry&&
  !retained_current_reentry&&!firstarchive_current_reentry&&!acceptance_current_reentry&&!completed32_current_reentry&&!primary32_timed&&
  record_journal_case.empty()&&snapshot_leaf_case.empty()&&installed_startup_case.empty()&&initial_metadata_case.empty()),"Projection tail cases require a separate genuine --pre-dx9 benchmark setup");
 need(snapshot_leaf_case.empty()||(pre_dx9&&!installed_startup_benchmark&&!pre_dx9_prefix_only&&!installed_default_common&&!installed_default_common_reentry&&
  !retained_current_reentry&&!firstarchive_current_reentry&&!acceptance_current_reentry&&!completed32_current_reentry&&!primary32_timed&&
  record_journal_case.empty()&&installed_startup_case.empty()&&initial_metadata_case.empty()),"Snapshot leaf cases require a separate genuine --pre-dx9 benchmark setup");
 need(record_journal_case.empty()||(pre_dx9&&!installed_startup_benchmark&&!pre_dx9_prefix_only&&!installed_default_common&&!installed_default_common_reentry&&
  !retained_current_reentry&&!firstarchive_current_reentry&&!acceptance_current_reentry&&!completed32_current_reentry&&!primary32_timed&&
  installed_startup_case.empty()&&initial_metadata_case.empty()),"Record-journal cases require a separate genuine --pre-dx9 benchmark setup");
 need(!installed_startup_benchmark||(pre_dx9&&!pre_dx9_prefix_only&&!installed_default_common&&!installed_default_common_reentry&&
  !retained_current_reentry&&!firstarchive_current_reentry&&!acceptance_current_reentry&&!completed32_current_reentry&&!primary32_timed&&
  installed_startup_case.empty()&&initial_metadata_case.empty()),"Startup benchmark requires separate genuine --pre-dx9 route");
 need(installed_startup_case.empty()||(!installed_default_common&&!installed_default_common_reentry&&!pre_dx9_prefix_only&&
  !retained_current_reentry&&!firstarchive_current_reentry&&!acceptance_current_reentry&&!completed32_current_reentry&&!primary32_timed),"Installed startup cases require a separate genuine full Common setup");
 if(!installed_startup_case.empty()){
  need(pre_dx9,"Installed startup cases require genuine --pre-dx9 adoption and --ui-language TYPE");
  installed_default_common=true; // Reuse every existing positive Common/adoption prerequisite.
 }
 need(initial_metadata_case.empty()||(pre_dx9&&!pre_dx9_prefix_only&&!installed_default_common&&!installed_default_common_reentry&&
  !retained_current_reentry&&!firstarchive_current_reentry&&!acceptance_current_reentry&&!primary32_timed&&installed_startup_case.empty()),
  "Initial metadata cases require a separate genuine pre-DX9 retained UI/text setup");
 need(root_count==roots.size(),"Expected ORIGINAL_INSTALL CANONICAL_ASSETS; no SAVE, header, profile or selected DLC input");
 need(!pre_dx9||!ui_language.empty(),"Pre-DX9 mode requires actual --ui-language TYPE on process argv");
 need(pre_dx9||ui_language.empty(),"--ui-language requires --pre-dx9 in this installation-prefix test");
 need(!pre_dx9_prefix_only||pre_dx9,"--pre-dx9-prefix-only requires --pre-dx9");
 need(!(installed_default_common&&installed_default_common_reentry),"Installed Default/Common positive and reentry modes are mutually exclusive");
 need(!(installed_default_common||installed_default_common_reentry)||pre_dx9,"Installed Default/Common modes require --pre-dx9");
 need(!pre_dx9_prefix_only||!(installed_default_common||installed_default_common_reentry),"Prefix-only cannot include installed Default/Common modes");
 need(!(retained_current_reentry&&primary32_timed),"Retained-current reentry and ordinary Primary32 timing are mutually exclusive");
 need(!firstarchive_current_reentry||!(retained_current_reentry||primary32_timed),"Firstarchive reentry excludes other focused Primary32 modes");
 need(!acceptance_current_reentry||!(retained_current_reentry||firstarchive_current_reentry||primary32_timed),
  "Acceptance reentry excludes other focused Primary32 modes");
 need(!completed32_current_reentry||!(retained_current_reentry||firstarchive_current_reentry||acceptance_current_reentry||primary32_timed),
  "Completed32 reentry excludes other focused Primary32 modes");
 need(!(retained_current_reentry||firstarchive_current_reentry||acceptance_current_reentry||completed32_current_reentry||primary32_timed)||!(pre_dx9_prefix_only||installed_default_common||installed_default_common_reentry),
  "Focused Primary32 modes cannot include prefix-only or Default/Common modes");
 if(!boot_construction_case.empty()){run_installed_startup_benchmark(roots[0],roots[1],{},{},{},{},false,{},{},false,{},false,boot_construction_case);return 0;}
 if(!postpublisher_prelude_case.empty()){run_installed_startup_benchmark(roots[0],roots[1],{},{},{},{},false,{},{},false,postpublisher_prelude_case);return 0;}
 if(!postpublished_prefix_case.empty()){run_installed_startup_benchmark(roots[0],roots[1],{},{},{},{},false,{},postpublished_prefix_case);return 0;}
 if(!publishvalidation_case.empty()){run_installed_startup_benchmark(roots[0],roots[1],{},{},{},{},false,publishvalidation_case);return 0;}
 if(!publication_prefix_case.empty()){run_installed_startup_benchmark(roots[0],roots[1],{},{},{},publication_prefix_case);return 0;}
 if(installed_skin_pc_benchmark){run_installed_startup_benchmark(roots[0],roots[1],{},{},{},{},true,{},{},installed_skin_global_benchmark,{},installed_late_speech_benchmark);return 0;}
 if(!projection_tail_case.empty()){run_installed_startup_benchmark(roots[0],roots[1],{},{},projection_tail_case);return 0;}
 if(!snapshot_leaf_case.empty()){run_installed_startup_benchmark(roots[0],roots[1],{},snapshot_leaf_case);return 0;}
 if(!record_journal_case.empty()){run_installed_startup_benchmark(roots[0],roots[1],record_journal_case);return 0;}
 if(installed_startup_benchmark){run_installed_startup_benchmark(roots[0],roots[1]);return 0;}
 const auto original_install=roots[0];const auto canonical_assets=roots[1];
 Phase acquire_phase("acquire");
 Window window;need(window.hwnd&&!IsWindowVisible(window.hwnd),"Actual current-process hidden HWND missing");
 const auto hwnd=reinterpret_cast<std::uintptr_t>(window.hwnd);std::uint64_t revision=1;
 std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner> parent;
 bool drop_join_parent=false;std::size_t join_loss_callbacks=0;
 bool inject_publisher_reentry=false;std::size_t publisher_reentry_callbacks=0;std::string publisher_reentry_reason;
 const ui::BnwNativeInstallationInputOwner* callback_input=nullptr;
 bool inject_retained_current=false;std::size_t revision_callbacks=0,retained_current_callbacks=0,retained_current_ordinal=0;
 std::string retained_current_reason;
 bool metadata_armed=false;std::size_t metadata_callbacks=0;unsigned metadata_seen_phase=0;std::string metadata_inner_reason;
 const unsigned metadata_requested_phase=initial_metadata_case.ends_with("-post")?2U:1U;
 const bool metadata_reentry=initial_metadata_case.starts_with("reentry-");
 bool inject_initial_reentry=false,drop_initial_parent=false;
 std::size_t initial_reentry_callbacks=0,initial_loss_callbacks=0,initial_callback_ordinal=0;
 std::string initial_reentry_reason;
 auto installation=ui::BnwNativeInstallationInputOwner::acquire(original_install,canonical_assets,hwnd,
  runtime::bnw_stock_lua_win32_client_extent(hwnd,"real-zero-save-Parent-prefix",[&]{
   ++revision_callbacks;
   if(metadata_armed&&ui::detail::bnw_native_initial_descriptor_retained_metadata_phase_for_test()==metadata_requested_phase){
    metadata_armed=false;++metadata_callbacks;metadata_seen_phase=metadata_requested_phase;
    if(metadata_reentry){
     try{(void)parent->current();}catch(const std::exception& e){metadata_inner_reason=e.what();}
     need(metadata_inner_reason=="Installation client source reentry","Actual metadata callback caught the wrong inner Input refusal");
    }else parent.reset();
   }
   if(drop_join_parent){++join_loss_callbacks;parent.reset();}
   if(inject_publisher_reentry){inject_publisher_reentry=false;++publisher_reentry_callbacks;
    try{parent->publish_installation_default_common_source();}catch(const std::exception& e){publisher_reentry_reason=e.what();}
   }
   if(inject_initial_reentry){
    inject_initial_reentry=false;++initial_reentry_callbacks;initial_callback_ordinal=revision_callbacks;
    try{parent->advance_installation_initial_category_prefix();}catch(const std::exception& e){initial_reentry_reason=e.what();}
    need(initial_reentry_reason=="Installed initial category prefix replay/reentry","Actual installed entry callback caught the wrong category refusal");
   }
   if(drop_initial_parent){drop_initial_parent=false;++initial_loss_callbacks;initial_callback_ordinal=revision_callbacks;parent.reset();}
   if(inject_retained_current&&callback_input&&bnw_native_retained_primary32_borrower_active_for_test(*callback_input)&&
    (!acceptance_current_reentry||(parent&&bnw_native_primary32_new_step_acceptance_active_for_test(*parent)))&&
    (!completed32_current_reentry||(parent&&bnw_native_primary32_completed_pass_active_for_test(*parent)))){
    inject_retained_current=false;++retained_current_callbacks;retained_current_ordinal=revision_callbacks;
    try{(void)parent->current();}catch(const std::exception& e){retained_current_reason=e.what();}
    need(retained_current_reason=="Retained installation Parent.current reentry",
     "Actual retained callback did not catch the exact Parent.current refusal");
   }return revision;}));
 callback_input=installation.get();
 acquire_phase.end();
 Phase create_phase("create");
 parent=pre_dx9?ui::BnwNativePackageDiscoveryOwner::create_pre_dx9_installation(installation):
  ui::BnwNativePackageDiscoveryOwner::create(installation);
 if(!pre_dx9){const auto before=parent->current();
  need(before.descriptor_index_misses==1&&before.descriptor_direct_reads==1&&before.packages==0&&
   before.original_descriptor_order.empty()&&!before.potential_package_color_maps_complete&&
   !before.primary_dx9_registration_complete&&!before.native_skin_loading_complete&&
   before.next_resource_dependency.find("Mandatory installation-aware Category2")!=std::string::npos,
   "Actual first pending descriptor falsely became completed Package/Boot/Skin");
 }
 create_phase.end();
 if(pre_dx9){Phase early_phase("pre_dx9");
  parent->advance_pre_dx9_discovery_before_primary32();
  early_phase.end();
 }
 Phase category2_phase("category2");
 mandatory([&]{parent->advance_installation_category2_first_prefix();});
 const auto actual=parent->current();
 need(actual.packages>0&&actual.original_descriptor_order.size()==actual.packages&&
  (pre_dx9?(actual.descriptor_index_misses==0&&actual.descriptor_direct_reads==0):
   (actual.descriptor_index_misses==actual.packages&&actual.descriptor_direct_reads==actual.packages))&&
  actual.potential_package_color_maps_complete&&!actual.primary_dx9_registration_complete&&
  !actual.native_skin_loading_complete&&actual.next_resource_dependency.find("retained UISkin occurrences/Cold/Stat")!=std::string::npos,
  "Actual installed constructor/append/FIRST sequence failed or forged next UI/Cold source");
  if(pre_dx9_prefix_only){
   need(actual.archive_registration_requests==0&&actual.archives_registered==0&&actual.default_indexed_names==0&&
    actual.loose_registration_requests==0&&actual.loose_files_registered==0&&actual.loose_file_registration_requests==0&&
    !actual.low_resolution_source_branch_complete&&!actual.common_high_quality_registration_complete&&
    actual.resource_registration_order.empty(),"Early pre-DX9 subset admitted Primary32/Boot/Common/loose effects");
   category2_phase.end();
   // Reuse the genuine returned prefix; the real input callback must revoke
   // its last outside facade before this wrapper reaches the replay guard.
   Phase join_loss_phase("pre_dx9.outside_loss.join");
   const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> join_weak=parent;
   const auto* actual_join=parent.get();drop_join_parent=true;bool join_lost=false;std::string join_reason;
   try{actual_join->advance_installation_category2_first_prefix();}catch(const std::exception& e){
    join_reason=e.what();join_lost=join_reason.find("Pre-DX9 lost last outside startup Parent owner")!=std::string::npos;
   }
   if(!join_lost)throw std::runtime_error(std::string("Pre-DX9 join did not report actual outside-parent loss; actual=")+join_reason);
   need(join_loss_callbacks>0&&!parent&&join_weak.expired(),"Pre-DX9 join callback did not retire the sole outside Parent facade");
   join_loss_phase.end();
   // A fresh actual factory drops on the first source drive; the receiver
   // pipeline cannot begin, so this does not repeat the positive package run.
   Phase drive_loss_phase("pre_dx9.outside_loss.drive");
   std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner> drive_parent;
   bool drop_drive_parent=false;std::size_t drive_loss_callbacks=0;
   auto drive_input=ui::BnwNativeInstallationInputOwner::acquire(original_install,canonical_assets,hwnd,
    runtime::bnw_stock_lua_win32_client_extent(hwnd,"real-pre-dx9-first-drive-parent-loss",[&]{
     if(drop_drive_parent){++drive_loss_callbacks;drive_parent.reset();}return revision;}));
   drive_parent=ui::BnwNativePackageDiscoveryOwner::create_pre_dx9_installation(drive_input);
   const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> drive_weak=drive_parent;
   const auto* actual_drive=drive_parent.get();drop_drive_parent=true;bool drive_lost=false;std::string drive_reason;
   try{actual_drive->advance_pre_dx9_discovery_before_primary32();}catch(const std::exception& e){
    drive_reason=e.what();drive_lost=drive_reason.find("Pre-DX9 lost last outside startup Parent owner")!=std::string::npos;
   }
   if(!drive_lost)throw std::runtime_error(std::string("Pre-DX9 drive did not report actual outside-parent loss; actual=")+drive_reason);
   need(drive_loss_callbacks>0&&!drive_parent&&drive_weak.expired(),"Pre-DX9 drive callback did not retire the sole outside Parent facade");
   drive_loss_phase.end();
   std::cout<<"SUBSET pre_dx9_prefix_only=1 actual_packages="<<actual.packages
    <<" last_outside_parent_join=refused last_outside_parent_drive=refused"
    <<" Primary32=unissued Boot=unissued Common=unissued full_game=0 can_continue=0\n";
   return 0;
  }
 bool replay=false;try{parent->advance_installation_category2_first_prefix();}catch(const std::exception& e){
  replay=std::string_view(e.what()).find(pre_dx9?
   "Initial prefix join requires actual returned pipeline and untouched initial operations":
   "Installation Category2/FIRST source replay")!=std::string_view::npos;
 }need(replay,"Actual installation constructor sequence replayed");
 category2_phase.end();
 Phase retained_ui_phase("retainedUI");
 mandatory([&]{parent->advance_installation_retained_ui_prefix();},"Mandatory actual installation first-header Cold/Stat");
 const auto ui_frame=parent->current();
 need(ui_frame.skin_contexts==actual.skin_contexts&&ui_frame.packages==actual.packages&&
  !ui_frame.primary_dx9_registration_complete&&!ui_frame.native_skin_loading_complete&&
  ui_frame.next_resource_dependency.find("first-header Cold/Stat")!=std::string::npos,
  "Actual retained UISkin constructors bypassed Cold/Stat");
 Phase retained_text_phase("retainedUI.text_inputs");
 const auto texts=parent->inspect_native_ui_skin_retained_text_inputs();
 need(texts.size()==ui_frame.skin_contexts*10,"Actual original UISkin lexical getter count differs");
 retained_text_phase.end();
 bool ui_replay=false;try{parent->advance_installation_retained_ui_prefix();}catch(const std::exception&){ui_replay=true;}
 need(ui_replay,"Actual retained UI constructor replayed");
 retained_ui_phase.end();
  if(!initial_metadata_case.empty()){
   Phase metadata_phase(metadata_requested_phase==1?"InitialMetadata.actual_source_PRE_negative":"InitialMetadata.actual_source_POST_negative");
   need(ui::detail::bnw_native_initial_descriptor_retained_metadata_phase_for_test()==0&&
    actual.packages>0&&actual.descriptor_direct_reads==0&&texts.size()==ui_frame.skin_contexts*10,
    "Metadata negative lacks genuine pre-DX9 Category2/retained UI/text predecessors");
   const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> metadata_parent_weak=parent;
   const auto* actual_metadata_parent=parent.get();metadata_armed=true;
   exact_refusal([&]{(void)actual_metadata_parent->current();},metadata_reentry?
    "Installation creator process/thread/source retired":"Pre-DX9 lost last outside startup Parent owner");
   need(!metadata_armed&&metadata_callbacks==1&&metadata_seen_phase==metadata_requested_phase&&
    ui::detail::bnw_native_initial_descriptor_retained_metadata_phase_for_test()==0,
    "Metadata negative missed the actual requested guarded phase or failed to unwind its observer");
   if(metadata_reentry){
    need(parent.get()==actual_metadata_parent&&!metadata_parent_weak.expired()&&metadata_inner_reason=="Installation client source reentry",
     "Caught metadata Input reentry lost its real Parent or exact inner refusal");
    const auto before_current=revision_callbacks;
    exact_refusal([&]{(void)parent->current();},"Installation input retired");
    exact_refusal([&]{installation->require_current();},"Installation input retired");
    need(revision_callbacks==before_current,"Poisoned metadata reentry invoked another provider callback");
   }else{
    need(!parent&&metadata_parent_weak.expired(),"Metadata callback did not retire the sole outside Parent after actual scope unwind");
    const auto before_control=revision_callbacks;installation->require_current();
    need(revision_callbacks==before_control+2,"Metadata Parent loss did not leave the independent genuine Input live");
   }
   need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Metadata negative changed the actual HWND/revision source");
   metadata_phase.end();
   std::cout<<"SUBSET initial_metadata_case="<<initial_metadata_case<<" actual_packages="<<actual.packages
    <<" actual_text_inputs="<<texts.size()<<" actual_source_phase="<<(metadata_requested_phase==1?"PRE":"POST")
    <<" callbacks="<<metadata_callbacks<<" outer=refused Parent_after="<<(metadata_reentry?"refused":"expired")
    <<" Input_after="<<(metadata_reentry?"retired_by_Input_reentry":"live")<<" full_game=0 can_continue=0\n";
   return 0;
  }
 Phase cold_stat_phase("ColdStat");
 mandatory([&]{parent->advance_installation_cold_stat_prefix();},"Mandatory installation-aware initial PrefixRegistry/Arena/cache/first archive");
 const auto files_frame=parent->current();
 need(files_frame.descriptor_direct_reads==actual.descriptor_direct_reads&&
  !files_frame.primary_dx9_registration_complete&&!files_frame.native_skin_loading_complete&&
  files_frame.archive_registration_requests==0&&files_frame.archives_registered==0&&
  files_frame.default_indexed_names==0&&files_frame.loose_files_registered==0&&
  files_frame.next_resource_dependency.find("PrefixRegistry/Arena/cache/first archive")!=std::string::npos,
  "Actual Cold/Stat construction admitted archive/Boot/Skin out of order");
 bool files_replay=false;try{parent->advance_installation_cold_stat_prefix();}catch(const std::exception&){files_replay=true;}
 need(files_replay,"Actual installed Cold/Stat constructors replayed");
 cold_stat_phase.end();
 Phase namespace_phase("namespace");
 mandatory([&]{parent->advance_installation_prefix_arena_cache();},"Mandatory installation-aware SAME CurrentDefaultSource assignment");
 const auto namespace_frame=parent->current();
 need(namespace_frame.archive_registration_requests==0&&namespace_frame.archives_registered==0&&
  namespace_frame.default_indexed_names==0&&!namespace_frame.primary_dx9_registration_complete&&
  !namespace_frame.native_skin_loading_complete&&namespace_frame.descriptor_direct_reads==actual.descriptor_direct_reads&&
  namespace_frame.next_resource_dependency.find("SAME CurrentDefaultSource assignment")!=std::string::npos,
  "Actual initial Prefix/Arena/policy/cache constructors bypassed mandatory source assignment/archive");
 bool namespace_replay=false;try{parent->advance_installation_prefix_arena_cache();}catch(const std::exception&){namespace_replay=true;}
 need(namespace_replay,"Actual initial namespace/cache constructors replayed");
 namespace_phase.end();
 Phase first_header_phase("firstheader");
 mandatory([&]{parent->advance_installation_current_first_archive_header();},"Mandatory actual installed Prefix transaction brand");
 const auto header_frame=parent->current();
 need(header_frame.archive_registration_requests==0&&header_frame.archives_registered==0&&
  header_frame.default_indexed_names==0&&!header_frame.primary_dx9_registration_complete&&
  !header_frame.native_skin_loading_complete&&header_frame.descriptor_direct_reads==actual.descriptor_direct_reads&&
  header_frame.next_resource_dependency.find("Prefix transaction brand")!=std::string::npos,
  "Actual CurrentSource/request0/MISS/stat/Header escaped mandatory Prefix transaction or admitted entries");
 bool header_replay=false;try{parent->advance_installation_current_first_archive_header();}catch(const std::exception&){header_replay=true;}
 need(header_replay,"Actual source assignment/request/stat/Header replayed");
 first_header_phase.end();
 if(firstarchive_current_reentry){
  Phase firstarchive_reentry_phase("firstarchive.caught_parent_current");
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation),"Actual borrower leaked before firstarchive drive");
  const auto* actual_parent=parent.get();inject_retained_current=true;
  exact_refusal([&]{parent->advance_installation_first_archive_publication();},"Installation creator process/thread/source retired");
  inject_retained_current=false;
  need(retained_current_callbacks==1&&retained_current_reason=="Retained installation Parent.current reentry",
   "Actual firstarchive drive never entered the caught-current retained callback");
  need(revision_callbacks==retained_current_ordinal+1,"Caught firstarchive current did not return through the real two-sample provider");
  need(parent.get()==actual_parent&&IsWindow(window.hwnd)!=FALSE,"Caught-firstarchive fixture lost actual Parent/window");
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation),"Firstarchive exception cleanup left actual borrower registered");
  const auto before_sticky=revision_callbacks;
  exact_refusal([&]{installation->require_current();},"Installation input retired");
  need(revision_callbacks==before_sticky,"Firstarchive sticky retirement invoked the actual provider again");
  firstarchive_reentry_phase.end();
  std::cout<<"SUBSET firstarchive_current_reentry=1 injected_callbacks="<<retained_current_callbacks
   <<" caught_current=exact outer_refusal=exact sticky_input=refused borrower_cleanup=clear"
   <<" admission_receipt=unissued completed32=unissued Common=unissued full_game=0 can_continue=0\n";
  return 0;
 }
 Phase first_archive_phase("firstarchive");
 mandatory([&]{parent->advance_installation_first_archive_publication();},"Mandatory actual installed primary32 journal/capture bindings");
 const auto archive_frame=parent->current();const auto admission=parent->inspect_installation_first_archive_admission();
 need(admission.admitted_entry_count>0&&admission.admitted_entry_count<=admission.declared_entry_count&&
  archive_frame.archive_registration_requests==0&&archive_frame.archives_registered==0&&
  archive_frame.default_indexed_names==0&&!archive_frame.primary_dx9_registration_complete&&
  !archive_frame.native_skin_loading_complete&&archive_frame.descriptor_direct_reads==actual.descriptor_direct_reads&&
  archive_frame.next_resource_dependency.find("primary32 journal/capture bindings")!=std::string::npos,
  "Actual prefix/metadata/mode0 admission/cache publication forged whole primary32 or declared count as returns");
 bool archive_replay=false;try{parent->advance_installation_first_archive_publication();}catch(const std::exception&){archive_replay=true;}
 need(archive_replay,"Actual shared prefix/admission/cache publication replayed");
 first_archive_phase.end();
 std::cout<<"installation_prefix phase=firstarchive revision_callbacks="<<revision_callbacks<<'\n';
 std::cout<<"actual_first_archive_declared="<<admission.declared_entry_count
  <<" actual_first_archive_admitted="<<admission.admitted_entry_count<<'\n';
 if(retained_current_reentry||acceptance_current_reentry||completed32_current_reentry){
  Phase retained_reentry_phase(completed32_current_reentry?"primary32.completed32.caught_parent_current":
   (acceptance_current_reentry?"primary32.acceptance.caught_parent_current":"primary32.caught_parent_current"));
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation),"Actual borrower leaked before Primary32 drive");
  if(acceptance_current_reentry)need(!bnw_native_primary32_new_step_acceptance_active_for_test(*parent),
   "New-step acceptance observer active before actual Primary32 drive");
  if(completed32_current_reentry)need(!bnw_native_primary32_completed_pass_active_for_test(*parent),
   "Completed32 observer active before actual Primary32 drive");
  const auto* actual_parent=parent.get();inject_retained_current=true;
  exact_refusal([&]{parent->advance_installation_primary32_archives();},"Installation creator process/thread/source retired");
  inject_retained_current=false;
  need(retained_current_callbacks==1&&retained_current_reason=="Retained installation Parent.current reentry",
   "Actual Primary32 drive never entered the caught-current retained callback");
  // The real Win32 provider samples revision before and after GetClientRect.
  need(revision_callbacks==retained_current_ordinal+1,"Caught current did not return through the real unchanged two-sample client provider");
  need(parent.get()==actual_parent&&IsWindow(window.hwnd)!=FALSE,"Caught-current fixture lost actual Parent/window");
  need(!bnw_native_retained_primary32_borrower_active_for_test(*installation),"Exception cleanup left the actual borrower registered");
  if(acceptance_current_reentry)need(!bnw_native_primary32_new_step_acceptance_active_for_test(*parent),
   "Exception cleanup left the actual new-step acceptance tag active");
  if(completed32_current_reentry)need(!bnw_native_primary32_completed_pass_active_for_test(*parent),
   "Exception cleanup left the actual completed32 pass tag active");
  const auto before_sticky=revision_callbacks;
  exact_refusal([&]{installation->require_current();},"Installation input retired");
  need(revision_callbacks==before_sticky,"Sticky retirement invoked the actual provider again");
  retained_reentry_phase.end();
  std::cout<<"SUBSET "<<(completed32_current_reentry?"completed32_current_reentry=1":
   (acceptance_current_reentry?"acceptance_current_reentry=1":"retained_current_reentry=1"))
   <<" injected_callbacks="<<retained_current_callbacks
   <<" caught_current=exact outer_refusal=exact sticky_input=refused borrower_cleanup=clear"
   <<(acceptance_current_reentry?" acceptance_cleanup=clear":"")
   <<(completed32_current_reentry?" completed_pass_cleanup=clear completed32_return=unissued":" completed32=unissued")
   <<" Common=unissued full_game=0 can_continue=0\n";
  return 0;
 }
 Phase whole32_phase("whole32");
 mandatory([&]{parent->advance_installation_primary32_archives();},"Mandatory actual installed primary32 record snapshot/Default projection/publication");
 const auto primary_frame=parent->current();const auto primary=parent->inspect_installation_primary32_archives();
 need(primary.positive_primary32_complete&&primary.completed_archives==32&&
  primary.executed_admitted_total>=admission.admitted_entry_count&&
  primary_frame.archive_registration_requests==0&&primary_frame.archives_registered==0&&
  primary_frame.default_indexed_names==0&&!primary_frame.primary_dx9_registration_complete&&
  !primary_frame.native_skin_loading_complete&&primary_frame.descriptor_direct_reads==actual.descriptor_direct_reads&&
  primary_frame.next_resource_dependency.find("record snapshot/Default projection/publication")!=std::string::npos,
  "Actual original-order whole32 journal invented record publication/Boot readiness or substituted declared counts");
 if(primary32_timed){
  whole32_phase.end();
  std::cout<<"SUBSET primary32_timed=1 actual_first_archive_declared="<<admission.declared_entry_count
   <<" actual_first_archive_admitted="<<admission.admitted_entry_count<<" actual_primary32_completed="<<primary.completed_archives
   <<" actual_primary32_executed_admitted="<<primary.executed_admitted_total
   <<" snapshot=unissued Common=unissued full_game=0 can_continue=0\n";
  return 0;
 }
 bool primary_replay=false;try{parent->advance_installation_primary32_archives();}catch(const std::exception&){primary_replay=true;}
 need(primary_replay,"Actual whole32 closed journal/remaining constructor program replayed");
 std::cout<<"actual_primary32_completed="<<primary.completed_archives
  <<" actual_primary32_executed_admitted="<<primary.executed_admitted_total<<'\n';
 mandatory([&]{(void)parent->early_input_lease();},"Mandatory actual installed primary32 record snapshot/Default projection/publication"); // Cannot fabricate imported save/header affinity.
 mandatory([&]{(void)parent->resources();},"Mandatory actual installed primary32 record snapshot/Default projection/publication");
 const auto before_registration_callbacks=revision_callbacks;
 exact_refusal([&]{parent->register_native_dx9_primary_prefix();},
  "Mandatory actual installed primary32 record snapshot/Default projection/publication before DX9/Boot");
 need(revision_callbacks>before_registration_callbacks,"Installed registration rejection skipped genuine live source callbacks");
 const auto registration_refused_frame=parent->current();
 need(registration_refused_frame.archive_registration_requests==primary_frame.archive_registration_requests&&
  registration_refused_frame.archives_registered==primary_frame.archives_registered&&
  registration_refused_frame.default_indexed_names==primary_frame.default_indexed_names&&
  registration_refused_frame.resource_registration_order==primary_frame.resource_registration_order&&
  registration_refused_frame.loose_registration_requests==primary_frame.loose_registration_requests&&
  registration_refused_frame.loose_files_registered==primary_frame.loose_files_registered&&
  registration_refused_frame.loose_file_registration_requests==primary_frame.loose_file_registration_requests&&
  registration_refused_frame.descriptor_index_misses==primary_frame.descriptor_index_misses&&
  registration_refused_frame.descriptor_direct_reads==primary_frame.descriptor_direct_reads&&
  registration_refused_frame.next_resource_dependency==primary_frame.next_resource_dependency&&
  !registration_refused_frame.primary_dx9_registration_complete&&!registration_refused_frame.native_skin_loading_complete,
  "Refused installed registration changed actual returned32 Default/archive/source effects");
 bool boot=false;try{(void)ui::BnwNativeResourceBootSettingsOwner::create(parent);}catch(const std::exception& e){
  boot=std::string_view(e.what()).find("Native boot settings require initial DX9 resource prefix")!=std::string_view::npos;
 }need(boot,"Incomplete real installation Parent issued Boot");
 const auto boot_refused_frame=parent->current();
 need(boot_refused_frame.descriptor_direct_reads==actual.descriptor_direct_reads&&
  !boot_refused_frame.primary_dx9_registration_complete,"Refused supplier replay changed actual Parent effects");
 whole32_phase.end();
 run_parent_loss_fixture(original_install,canonical_assets,hwnd,revision);
 Phase record_snapshot_phase("recordsnapshot");
 const auto actual_records=parent->advance_installation_record_snapshot_default_projection();
 need(actual_records==primary.executed_admitted_total,"Actual captured record nodes differ from whole32 entry returns");
 const auto projection_frame=parent->current();
 need(projection_frame.default_indexed_names==0&&!projection_frame.primary_dx9_registration_complete&&
  !projection_frame.native_skin_loading_complete&&projection_frame.next_resource_dependency.find("Default publication")!=std::string::npos,
  "Privately prepared actual Default projection invented publication/Boot");
 bool projection_replay=false;try{(void)parent->advance_installation_record_snapshot_default_projection();}catch(const std::exception&){projection_replay=true;}
 need(projection_replay,"Actual record/projection capture replayed");
 mandatory([&]{(void)parent->early_input_lease();},"Mandatory actual installed Default publication");
 mandatory([&]{(void)parent->resources();},"Mandatory actual installed Default publication");
 record_snapshot_phase.end();
 std::cout<<"actual_captured_records="<<actual_records<<'\n';
 Phase default_phase("Default");
 mandatory([&]{parent->advance_installation_default_publication();},"Mandatory actual installed low-resolution source CVarD80DE0");
 const auto published=parent->current();
 need(published.primary_dx9_registration_complete&&published.archives_registered==32&&
  published.default_indexed_names==actual_records&&published.resource_registration_order.size()==32&&
  !published.native_skin_loading_complete&&published.next_resource_dependency.find("CVarD80DE0")!=std::string::npos,
  "Actual same32 Default publication differs from real records or skipped low-resolution/Common/Boot");
 bool publish_replay=false;try{parent->advance_installation_default_publication();}catch(const std::exception&){publish_replay=true;}
 need(publish_replay,"Actual Default publication replayed");
 mandatory([&]{(void)parent->early_input_lease();},"Mandatory actual installed low-resolution source CVarD80DE0");
 mandatory([&]{(void)parent->resources();},"Mandatory actual installed low-resolution source CVarD80DE0");
 default_phase.end();
 std::cout<<"actual_published_Default_records="<<published.default_indexed_names<<'\n';
 Phase boot_phase("Boot");
 const auto installed_boot=parent->advance_installation_boot_settings();
 need(installed_boot.normal_launch_source_closed&&!installed_boot.leader_viewer&&installed_boot.effective_texture_quality.has_value(),
  "Actual installed Boot lacks original negative Viewer/INI-or-adapter texture source");
 const auto boot_frame=parent->current();
 need(boot_frame.primary_dx9_registration_complete&&!boot_frame.low_resolution_source_branch_complete&&
  !boot_frame.native_skin_loading_complete&&boot_frame.next_resource_dependency.find("texture-quality source branch")!=std::string::npos,
  "Actual installed Boot skipped original texture/Common/Skin order");
 bool boot_replay=false;try{(void)parent->advance_installation_boot_settings();}catch(const std::exception&){boot_replay=true;}
 need(boot_replay,"Actual installed Boot factory replayed");
 mandatory([&]{(void)parent->resources();},"Mandatory actual installed texture-quality source branch");
 boot_phase.end();
 std::cout<<"actual_Boot_TextureQuality="<<*installed_boot.effective_texture_quality<<'\n';
 Phase texture_phase("TextureQuality");
 if(*installed_boot.effective_texture_quality==0){
  need(!(installed_default_common||installed_default_common_reentry),"Installed Default/Common mode requires actual HIGH; low22 is an unimplemented source boundary");
  mandatory([&]{parent->advance_installation_high_texture_quality_branch();},"Mandatory actual installed low22 archive constructor/cache source");
  need(!parent->current().low_resolution_source_branch_complete,"Real zero quality skipped low22 mandatory side effects");
  texture_phase.end();
 }else{
  mandatory([&]{parent->advance_installation_high_texture_quality_branch();},"Mandatory actual installed Common13 constructor/cache source");
  const auto texture=parent->current();
  need(texture.low_resolution_source_branch_complete&&texture.high_texture_quality_source_branch&&
   !texture.low_resource_mount_present&&texture.archives_registered==32&&texture.default_indexed_names==actual_records&&
   !texture.native_skin_loading_complete&&texture.next_resource_dependency.find("Common13 constructor/cache source")!=std::string::npos,
   "Actual nonzero texture source skipped or invented original low-resource/Common/Skin effects");
  bool texture_replay=false;try{parent->advance_installation_high_texture_quality_branch();}catch(const std::exception&){texture_replay=true;}
  need(texture_replay,"Actual high texture branch replayed");
  mandatory([&]{(void)parent->resources();},"Mandatory actual installed Common13 constructor/cache source");
  texture_phase.end();
  Phase common_phase("Common");
  const auto common_declared=parent->advance_installation_first_common_archive_header();
  const auto common_frame=parent->current();
  need(common_frame.archives_registered==32&&common_frame.default_indexed_names==actual_records&&
   common_frame.resource_registration_order.size()==32&&!common_frame.common_high_quality_registration_complete&&!common_frame.native_skin_loading_complete&&
   common_frame.next_resource_dependency.find(common_declared?"first Common Prefix":"remaining Common constructor/cache source after actual first absence")!=std::string::npos,
   "Actual first Common request/MISS/probe/Stat/Header invented entries/whole13/Default publication");
  bool common_replay=false;try{(void)parent->advance_installation_first_common_archive_header();}catch(const std::exception&){common_replay=true;}
  need(common_replay,"Actual first Common request/constructor/Header replayed");
  mandatory([&]{(void)parent->resources();},common_declared?"Mandatory actual installed first Common Prefix":"Mandatory actual installed remaining Common constructor/cache source after actual first absence");
  if(common_declared){
   mandatory([&]{parent->advance_installation_first_common_archive_metadata();},"Mandatory actual installed first Common admission source transition/cache");
   const auto metadata_frame=parent->current();
   need(metadata_frame.archives_registered==32&&metadata_frame.default_indexed_names==actual_records&&
    !metadata_frame.common_high_quality_registration_complete&&!metadata_frame.native_skin_loading_complete,
    "Actual Common Prefix/metadata invented admission/cache/whole13/Skin readiness");
   bool metadata_replay=false;try{parent->advance_installation_first_common_archive_metadata();}catch(const std::exception&){metadata_replay=true;}
   need(metadata_replay,"Actual first Common Prefix transaction/metadata replayed");
   mandatory([&]{(void)parent->resources();},"Mandatory actual installed first Common admission source transition/cache");
   const auto common_admitted=parent->advance_installation_first_common_archive_admission();
   need(common_admitted<=*common_declared,"Actual Common0 returns exceeded original declaration");
   const auto admitted_frame=parent->current();
   need(admitted_frame.archives_registered==32&&admitted_frame.default_indexed_names==actual_records&&
    !admitted_frame.common_high_quality_registration_complete&&!admitted_frame.native_skin_loading_complete&&
    admitted_frame.next_resource_dependency.find("remaining Common12")!=std::string::npos,
    "Actual retained Common0 receipt invented whole13 or published partial Default");
   bool admission_replay=false;try{(void)parent->advance_installation_first_common_archive_admission();}catch(const std::exception&){admission_replay=true;}
   need(admission_replay,"Actual Common0 entry/cache source replayed");
   mandatory([&]{(void)parent->resources();},"Mandatory actual installed remaining Common12");
   std::cout<<"actual_Common0_Header_declared="<<*common_declared<<" actual_Common0_admitted="<<common_admitted<<'\n';
  }
  else std::cout<<"actual_Common0_original_probe=absent\n";
  const auto common13_admitted=parent->advance_installation_remaining_common_archives();
  const auto whole_common=parent->current();
  need(whole_common.archives_registered==32&&whole_common.default_indexed_names==actual_records&&
   !whole_common.common_high_quality_registration_complete&&!whole_common.native_skin_loading_complete&&
   whole_common.next_resource_dependency.find("DefaultLoose original-order")!=std::string::npos,
   "Real whole Common13 source published partial Default or bypassed mandatory loose/Skin");
  bool common13_replay=false;try{(void)parent->advance_installation_remaining_common_archives();}catch(const std::exception&){common13_replay=true;}
  need(common13_replay,"Real remaining Common source replayed");
  mandatory([&]{(void)parent->resources();},"Mandatory actual installed DefaultLoose original-order");
  common_phase.end();
  std::cout<<"actual_Common13_admitted="<<common13_admitted<<'\n';
   if(installed_default_common||installed_default_common_reentry){
    // An ordinary call before the first loose attempt must refuse without
    // poisoning this same genuine returned Common13 source.
    exact_refusal([&]{parent->publish_installation_default_common_source();},
     "Installed final publication has no actual DefaultLoose issuer");
    if(installed_default_common_reentry){
     Phase reentry_phase("DefaultLoose.publisher_reentry");
     inject_publisher_reentry=true;bool outer_refused=false;std::string outer_reason;
     try{parent->advance_installation_default_loose_source();}catch(const std::exception& e){outer_refused=true;outer_reason=e.what();}
     need(publisher_reentry_callbacks==1&&!inject_publisher_reentry,"Actual first pre-issuer Input callback did not run exactly once");
     if(publisher_reentry_reason!="Installed final publication reentered before actual DefaultLoose issuer construction")
      throw std::runtime_error(std::string("Unexpected caught pre-issuer publisher refusal; actual=")+publisher_reentry_reason);
     if(!outer_refused||outer_reason.find("Mandatory")!=std::string::npos)
      throw std::runtime_error(std::string("Caught pre-issuer publisher reentry did not invalidate actual outer attempt; actual=")+outer_reason);
     exact_refusal([&]{(void)parent->current();},"Installation Parent prefix foreign/source domain/operation changed");
     reentry_phase.end();
     std::cout<<"SUBSET installed_default_common_reentry=1 actual_Common13_admitted="<<common13_admitted
      <<" caught_inner=refused outer=refused Current=refused full_game=0 can_continue=0\n";
     return 0;
    }
    Phase loose_phase("DefaultLoose");
    parent->advance_installation_default_loose_source();
    const auto loose_frame=parent->current();
    need(loose_frame.archives_registered==32&&loose_frame.default_indexed_names==actual_records&&
     loose_frame.resource_registration_order.size()==32&&!loose_frame.common_high_quality_registration_complete&&
     !loose_frame.native_skin_loading_complete&&loose_frame.next_resource_dependency.find("final Default/Common publication")!=std::string::npos,
     "Real three DefaultLoose returns bypassed final publication or changed unpublished Base Default");
    mandatory([&]{(void)parent->resources();},"Mandatory actual installed final Default/Common publication before Warm/category/Skin");
    loose_phase.end();
    Phase publication_phase("DefaultCommon.publication");
    parent->publish_installation_default_common_source();
    const auto common_return=parent->current();
    need(common_return.common_high_quality_registration_complete&&common_return.high_texture_quality_source_branch&&
     !common_return.low_resource_mount_present&&common_return.archives_registered>=published.archives_registered&&
     common_return.archives_registered<=published.archives_registered+13&&common_return.default_indexed_names>=actual_records&&
     common_return.loose_registration_requests==3&&common_return.resource_registration_order.size()==48&&
     common_return.resource_registration_order[45]=="Resource\\Overrides"&&common_return.resource_registration_order[46]=="Assets"&&
     common_return.resource_registration_order[47]=="Assets\\Sounds\\Speech\\Shared"&&!common_return.native_skin_loading_complete,
     "Actual final Default/Common return differs from real Common13/three loose order or forged Skin readiness");
    publication_phase.end();
    Phase receiver_phase("DefaultCommon.installed_receiver_checkpoint");
    std::size_t paks_callbacks=0,paks_outputs=0;int paks_raw_count=0;
    exact_refusal([&]{ui::detail::BnwNativeDefaultCommonProduction::with_installed_common_receiver_checkpoint(
     *parent,[&](int raw_count,const std::vector<std::string>& outputs){
      ++paks_callbacks;paks_raw_count=raw_count;paks_outputs=outputs.size();
     });},"Civis configured activation lacks SAME immutable early lease");
    need(paks_callbacks==1,"Actual pre-DX9 returned roster did not issue exactly one first Paks checkpoint");
    receiver_phase.end();
    if(!installed_startup_case.empty()){
     Phase startup_phase("Installed.StartupAdmission.return");
     parent->advance_installation_startup_admission();
     const auto startup_return=parent->current();
     need(startup_return.common_high_quality_registration_complete&&startup_return.primary_dx9_registration_complete&&
      !startup_return.native_skin_loading_complete&&startup_return.archives_registered==common_return.archives_registered&&
      startup_return.default_indexed_names==common_return.default_indexed_names&&
      startup_return.resource_registration_order==common_return.resource_registration_order&&
      startup_return.next_resource_dependency=="Mandatory actual656440 per-package category0 then1 constructors/effects before52D590; installed admission alone is not category/Skin readiness",
      "Genuine installed startup admission did not normally return before category/Skin");
     startup_phase.end();
     if(installed_startup_case!="prefix"&&installed_startup_case!="categories"){
      Phase negative_phase(installed_startup_case=="reentry"?"Installed.InitialCategory.caught_entry_reentry":"Installed.InitialCategory.outside_parent_loss");
      const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> initial_parent_weak=parent;
      const auto* actual_parent=parent.get();
      inject_initial_reentry=installed_startup_case=="reentry";drop_initial_parent=installed_startup_case=="parent-loss";
      exact_refusal([&]{actual_parent->advance_installation_initial_category_prefix();},installed_startup_case=="reentry"?
       "Native source Package lost/reentered outside actual active fence":"Pre-DX9 lost last outside startup Parent owner");
      inject_initial_reentry=false;drop_initial_parent=false;
      // Real Win32 provider completed both unchanged revision samples.
      need(revision_callbacks==initial_callback_ordinal+1,"Installed negative did not return through the actual two-sample Input callback");
      if(installed_startup_case=="reentry"){
       need(initial_reentry_callbacks==1&&initial_loss_callbacks==0&&parent.get()==actual_parent&&
        initial_reentry_reason=="Installed initial category prefix replay/reentry","Caught actual installed category entry did not stay isolated");
       const auto before_current=revision_callbacks;
       exact_refusal([&]{(void)parent->current();},"Installation Parent prefix foreign/source domain/operation changed");
       need(revision_callbacks==before_current,"Poisoned installed Parent Current invoked another source callback");
      }else need(initial_loss_callbacks==1&&initial_reentry_callbacks==0&&!parent&&initial_parent_weak.expired(),
       "Actual installed category callback did not retire the last outside Parent through unwind");
      need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Installed negative changed the actual HWND/client source");
      // Independent live Input remains valid; only Parent/Warm was revoked.
      const auto before_input_control=revision_callbacks;installation->require_current();
      need(revision_callbacks==before_input_control+2,"Installed negative replaced the genuine live Input control");
      negative_phase.end();
      std::cout<<"SUBSET installed_startup_case="<<installed_startup_case
       <<" Source5=normal_return Source6a_entry=refused callbacks="<<(initial_reentry_callbacks+initial_loss_callbacks)
       <<" Parent_after="<<(installed_startup_case=="reentry"?"poisoned":"expired")
       <<" Input_control=live category_complete=0 Skin=unissued full_game=0 can_continue=0\n";
      return 0;
     }
     Phase initial_phase("Installed.InitialCategory.prefix_return");
     parent->advance_installation_initial_category_prefix();
     const auto initial_return=parent->current();
     need(initial_return.common_high_quality_registration_complete&&initial_return.primary_dx9_registration_complete&&
      !initial_return.native_skin_loading_complete&&initial_return.archives_registered==common_return.archives_registered&&
      initial_return.default_indexed_names==common_return.default_indexed_names&&
      initial_return.resource_registration_order==common_return.resource_registration_order,
      "Actual installed category prefix changed returned Default/Common or forged Skin");
     need(initial_return.next_resource_dependency=="Mandatory SAME pending6F84F0 Paks archive operations then5F6920 remaining effects before next per-package category",
      "Installed initial prefix returned no pending actual constructor; zero-only cursor is insufficient constructor coverage");
     initial_phase.end();
     if(installed_startup_case=="categories"){
      Phase categories_phase("Installed.InitialCategory.full656440_return");
      parent->drive_installation_initial_categories();
      const auto categories_return=parent->current();
      need(categories_return.common_high_quality_registration_complete&&categories_return.primary_dx9_registration_complete&&
       !categories_return.native_skin_loading_complete&&categories_return.next_resource_dependency==
       "Actual initial656440 per-package0/1 effects returned; mandatory52D590 Color/Skin and subsequent completed startup suppliers",
       "Genuine installed category loop did not normally return at the actual52D590 boundary");
      categories_phase.end();
      std::cout<<"SUBSET installed_startup_case=categories Source5=normal_return Source6a_prefix=normal_return"
       <<" initial656440=normal_return next=52D590 ColorSkin=pending Skin=unissued full_game=0 can_continue=0\n";
      return 0;
     }
     std::cout<<"SUBSET installed_startup_case=prefix Source5=normal_return Source6a_prefix=normal_return"
      <<" actual_constructor=pending Paks_effects=pending category_complete=0 Skin=unissued full_game=0 can_continue=0\n";
     return 0;
    }
    std::cout<<"SUBSET installed_default_common=1 actual_Default_records="<<common_return.default_indexed_names
     <<" actual_loose_callers="<<common_return.loose_registration_requests<<" firstPaks_raw_count="<<paks_raw_count
     <<" firstPaks_outputs="<<paks_outputs
     <<" saved_policy=refused_installed_domain Skin=unissued full_game=0 can_continue=0\n";
    return 0;
   }
 }
 bool foreign=false;std::thread other([&]{try{(void)parent->current();}catch(const std::exception&){foreign=true;}});other.join();
 need(foreign,"Foreign thread consumed actual installation Parent prefix");
 ++revision;bool stale=false;try{(void)parent->current();}catch(const std::exception&){stale=true;}
 need(stale,"Real pre-handoff provider invalidation was ignored");
 std::cout<<"Genuine zero-save Parent actual original-order whole32 archive constructors/entry returns/cache journal prefix"
  <<" next=installation-DefaultLoose-or-low22 Boot=actual-installed-prefix Skin=unissued epoch=unissued full_game=0 can_continue=0\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
