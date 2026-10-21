// Actual post-Prefix callback negatives. The previously qualified sparse
// installation fixture and every genuine production predecessor are reused.
namespace firstarchive_post_prefix_fixture {
void run(const char* original_install,const char* canonical_assets,std::string_view which){
 need(which=="reentry"||which=="parent-loss","Expected firstarchive post-Prefix reentry|parent-loss");
 const bool reentry=which=="reentry";Phase total("firstarchive.post_prefix.callback_refusal");
 held_metadata_fixture::SparseInstallation fixture(original_install,false);
 Window window;need(window.hwnd&&!IsWindowVisible(window.hwnd),"Firstarchive post-Prefix fixture needs the genuine hidden HWND");
 std::uint64_t revision=1;std::size_t callbacks=0,injections=0,injection_ordinal=0,prior_borrower_samples=0,releases=0;
 bool armed=false;std::string inner_reason;
 std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner> parent;
 const ui::BnwNativePackageDiscoveryOwner* callback_parent=nullptr;
 const ui::BnwNativeInstallationInputOwner* callback_input=nullptr;
 auto installation=ui::BnwNativeInstallationInputOwner::acquire(fixture.root,canonical_assets,reinterpret_cast<std::uintptr_t>(window.hwnd),
  runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window.hwnd),"firstarchive-actual-post-prefix",[&]{
   ++callbacks;
   if(armed&&callback_input&&callback_parent&&bnw_native_retained_primary32_borrower_active_for_test(*callback_input)){
    if(!bnw_native_firstarchive_post_prefix_active_for_test(*callback_parent)){++prior_borrower_samples;}
    else{
     need(prior_borrower_samples>0&&ui::bnw_native_firstarchive_post_prefix_observer_active_for_test(),
      "Post-Prefix callback incorrectly selected the earlier actual producer borrower");
     armed=false;++injections;injection_ordinal=callbacks;
     if(reentry){
      try{(void)callback_parent->current();}catch(const std::exception& e){inner_reason=e.what();}
      need(inner_reason=="Retained installation Parent.current reentry","Actual firstarchive post-Prefix callback caught the wrong current refusal");
     }else{
      need(parent&&parent.get()==callback_parent,"Post-Prefix owner loss lost the sole canonical outside slot");
      ++releases;parent.reset();callback_parent=nullptr;
     }
    }
   }
   return revision;
  }));
 callback_input=installation.get();parent=ui::BnwNativePackageDiscoveryOwner::create(installation);callback_parent=parent.get();
 exact_refusal([&]{parent->advance_installation_category2_first_prefix();},"Mandatory installation-aware retained UISkin occurrences/Cold/Stat before Boot/Skin");
 exact_refusal([&]{parent->advance_installation_retained_ui_prefix();},"Mandatory actual installation first-header Cold/Stat constructors before Boot/Skin");
 exact_refusal([&]{parent->advance_installation_cold_stat_prefix();},"Mandatory installation-aware initial PrefixRegistry/Arena/cache/first archive constructor before Boot/Skin");
 exact_refusal([&]{parent->advance_installation_prefix_arena_cache();},"Mandatory installation-aware SAME CurrentDefaultSource assignment before first archive request/constructor/Header/Boot");
 exact_refusal([&]{parent->advance_installation_current_first_archive_header();},"Mandatory actual installed Prefix transaction brand before shared prefix attempt/store/entry metadata/Boot");
 need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
  !ui::bnw_native_firstarchive_post_prefix_observer_active_for_test(),"Firstarchive post-Prefix fixture armed with an already active scope");
 const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> witness=parent;
 const auto before_arm=callbacks;armed=true;
 exact_refusal([&]{parent->advance_installation_first_archive_publication();},
  reentry?"Installation creator process/thread/source retired":"Retained firstarchive post-Prefix lost actual source/fence/outside Parent");
 armed=false;
 need(injections==1&&prior_borrower_samples>0&&injection_ordinal>before_arm,
  "Actual firstarchive post-Prefix did not invoke one isolated callback after the original producer scope");
 need(callbacks==injection_ordinal+1,"Post-Prefix refusal did not return through both actual unchanged revision samples");
 // These checks query only the live Input and a no-Parent TLS observation.
 need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&
  !ui::bnw_native_firstarchive_post_prefix_observer_active_for_test(),"Post-Prefix unwind left a borrower/TLS observer active");
 need(IsWindow(window.hwnd)!=FALSE&&revision==1,"Post-Prefix negative changed the actual HWND/client source");
 if(reentry){
  need(parent&&parent.get()==callback_parent&&!witness.expired()&&releases==0&&inner_reason=="Retained installation Parent.current reentry",
   "Caught post-Prefix reentry changed the actual outside Parent");
 }else{
  need(!parent&&callback_parent==nullptr&&witness.expired()&&releases==1&&inner_reason.empty(),
   "Post-Prefix callback did not release exactly the last real outside Parent through unwind");
 }
 const auto before_sticky=callbacks;exact_refusal([&]{installation->require_current();},"Installation input retired");
 need(callbacks==before_sticky,"Sticky post-Prefix Input retirement invoked another callback");
 fixture.require_real_sources_unchanged();total.end();
 std::cout<<"SUBSET firstarchive_post_prefix_case="<<which<<" actual_post_prefix=observed injections="<<injections
  <<" prior_producer_samples="<<prior_borrower_samples<<" refusal_sample_delta="<<(callbacks-injection_ordinal)
  <<" caught_current="<<(reentry?"exact":"unissued")<<" outside_Parent="<<(reentry?"live":"expired")
  <<" outer_refusal=exact Input_after=retired sticky_callbacks="<<(callbacks-before_sticky)
  <<" borrower_cleanup=clear post_prefix_observer_cleanup=clear actual_HWND=live"
  <<" firstarchive_method_frontier=refused whole32=unissued Default=unissued full_game=0 can_continue=0\n";
}
}
