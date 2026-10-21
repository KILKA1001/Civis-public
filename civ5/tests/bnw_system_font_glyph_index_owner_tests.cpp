#include "civ5/ui/bnw_client_text_policy.hpp"
#include "civ5/ui/bnw_text_locale_adapter.hpp"
#include "civ5/ui/bnw_system_font_cpu_owner.hpp"
#include "civ5/ui/bnw_system_font_cache_owner.hpp"
#include <algorithm>
#include <thread>
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
static_assert(!std::is_copy_constructible_v<ui::BnwSystemFontCpuOwner>);
static_assert(!std::is_copy_assignable_v<ui::BnwSystemFontCpuOwner>);
static_assert(!std::is_move_constructible_v<ui::BnwSystemFontCpuOwner>);
static_assert(!std::is_move_assignable_v<ui::BnwSystemFontCpuOwner>);
void check(bool v,const char*s){if(!v)throw std::runtime_error(s);}
template<class F>void rejects(F f){bool blocked=false;try{f();}catch(const std::exception&){blocked=true;}check(blocked,"Missing/stale/native font producer accepted");}
int main(int argc,char**argv){
 check(argc==4,"Actual Gandhi/rules/shared assets required");const std::filesystem::path root(argv[3]);auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0,true);if(!selected.preview){
  std::cerr<<"Selected BNW bootstrap failed";
  if(selected.preview.blocker)std::cerr<<" stage="<<static_cast<int>(selected.preview.blocker->stage)<<" reason="<<selected.preview.blocker->reason;
  else std::cerr<<" without host or blocker";
  std::cerr<<std::endl;return 1;
 }auto&live=*selected.preview.host;
 struct Window{HWND handle=nullptr;~Window(){if(handle)DestroyWindow(handle);}}window;window.handle=CreateWindowExW(0,L"STATIC",L"",WS_OVERLAPPEDWINDOW,0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);check(window.handle&&!IsWindowVisible(window.handle),"Own current-process client unavailable");std::uint64_t client_revision=4;std::size_t client_queries=0,poison_client_at=0;std::function<void()> observer;auto client=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window.handle),"gandhi-text-policy",[&]{++client_queries;if(observer){auto callback=std::move(observer);observer={};callback();}if(poison_client_at&&client_queries==poison_client_at)++client_revision;return client_revision;});
 auto loose=std::make_shared<assets::LooseResourceIndex>(root);loose->add_file("Assets/UI/Styles.xml");for(const auto font:{"TwCenMT16","TwCenMT18","TwCenMT20"})loose->add_file(std::string("Assets/UI/Fonts/Tw Cent MT/")+font+".ggxml");for(const auto prefix:{"Assets/Gameplay/XML/NewText/EN_US","Assets/Gameplay/XML/NewText/RU_RU","Assets/DLC/Expansion/Gameplay/XML/Text/en_US","Assets/DLC/Expansion/Gameplay/XML/Text/ru_RU","Assets/DLC/Expansion2/Gameplay/XML/Text/en_US","Assets/DLC/Expansion2/Gameplay/XML/Text/ru_RU"})loose->scan({root/prefix,{},true});assets::ResourceMountTable resources(assets::resource_lookup(loose),assets::resource_lookup_all(loose),assets::resource_keys(loose));const assets::AssetEnvironment environment(root);std::uint64_t mount=1;for(const auto key:{"Resource/DX9/UITextures.fpk","Resource/DX9/Expansion1UITextures.fpk","Resource/DX9/Expansion2UITextures.fpk"}){auto opened=environment.open_fpk_archive(key);auto*archive=std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);check(archive,"Shared source FPK missing");resources.mount({mount++,0,0,assets::resource_lookup(*archive),true,true,{},{}});}
 auto shell=ui::BnwCivilopediaScrollBarOwner::create(root,resources,*client.current(),runtime::identify_file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml").sha256);auto tree=ui::BnwCivilopediaRightTreeOwner::create(root,resources,shell->render_snapshot());const auto metrics=tree->snapshot().label_metrics;check(metrics&&metrics->snapshot().staged_metrics.size()==19,"Selected metric producer missing");const auto*a=tree->control(*client.current(),0,"ArticleID");const auto index=static_cast<std::size_t>(a-tree->snapshot().controls.data());
 const auto sources=ui::inspect_bnw_civilopedia_layout_sources(root);check(sources.verified,"Actual locale source unavailable");ui::CivilopediaLayoutOwnerKey identity;identity.frame_number=9;identity.owner_token="actual-gandhi-retained-locale";identity.player_id=live.active_player_id();identity.game_turn=live.session().state().setup.game_turn;identity.payload_sha256=live.identity().payload_sha256;identity.rules_sha256=live.identity().rules_sha256;identity.styles_sha256=sources.styles_sha256;identity.civilopedia_sha256=sources.civilopedia_sha256;identity.xml_sha256=sources.xml_sha256;

 ui::BnwLabelLocaleSelection selection{"ru_RU","explicit-current-glyph-test",7};
 auto policy=ui::BnwClientTextPolicyOwner::create(live.database(),client,[&]()->std::optional<ui::BnwLabelLocaleSelection>{return selection;},ui::BnwClientTextPolicy::selected_language,metrics);
 auto make_cpu=[&]{return ui::BnwSystemFontCpuOwner::create(reinterpret_cast<std::uintptr_t>(window.handle),policy,resources,policy->declared_metric(index));};
 auto cpu=make_cpu();auto cache=ui::BnwSystemFontCacheOwner::create(cpu);std::size_t checks=0;
 auto same_raster=[&](const auto&a,const auto&b){check(a.glyph_index==b.glyph_index&&a.width==b.width&&a.advance==b.advance&&a.height==b.height&&a.green_brightness==b.green_brightness&&a.shadow_brightness==b.shadow_brightness,"Same glyph-index raster changed actual GDI bytes");++checks;};
 for(const std::uint32_t cp:std::vector<std::uint32_t>{'W','A','T',' ',0x416,0x414}){
  const auto source=cpu->measure(cp);const auto direct=cpu->measure_glyph_index(source.glyph_index);
  check(direct.glyph_index==source.glyph_index&&direct.advance==source.advance&&direct.codepoint==0&&direct.gcp_flags==0&&!direct.pointers_preserved,"Direct glyph admission remapped Unicode or invented GCP receipt");++checks;
  const auto q=cache->measure_glyph_index(source.glyph_index);const auto legacy=cache->measure(cp);
  check(legacy.cache_hit&&legacy.slot==q.slot&&cache->current(q)&&q.requested_glyph_index==source.glyph_index,"Direct glyph entry failed SAME ring reuse");++checks;
  same_raster(cpu->raster_glyph_index(source.glyph_index),cpu->measure_and_raster(cp));
  const auto stage=cache->stage_raster_glyph_index(q);check(stage.cpu.glyph_index==source.glyph_index&&!stage.atlas_pair_uploaded&&!stage.quote.slot.raster_uploaded,"Direct raster stage invented GPU upload");++checks;
 }
 const auto w=cpu->measure('W');check(w.glyph_index!='W',"Actual W glyph fixture failed to distinguish glyph ID from Unicode");++checks;
 const auto expected=cpu->measure_plain_run("WA");std::string borrowed="WA";observer=[&]{borrowed.assign(20000,'X');borrowed.shrink_to_fit();};
 const auto pinned=cpu->measure_plain_run(borrowed);check(pinned.utf8=="WA"&&pinned.glyphs==expected.glyphs&&pinned.dx==expected.dx&&borrowed.size()==20000,"Callback retargeted/freed borrowed run text");++checks;
 check(!pinned.original_class_inputs_verified&&!pinned.native_label_initialized&&pinned.explicit_zero_class_input,"Civis plain-run diagnostic falsely proved original CLASSIN");++checks;
 bool reentry_refused=false;observer=[&]{try{cpu->measure_plain_run("T");}catch(const std::exception&){reentry_refused=true;}};
 check(cpu->measure_plain_run("WA").utf8=="WA"&&reentry_refused,"CPU callback reentry was accepted or poisoned outer operation");++checks;
 auto solo=make_cpu();std::weak_ptr<ui::BnwSystemFontCpuOwner> weak=solo;auto* facade=solo.get();observer=[&]{solo.reset();};
 const auto retained=facade->measure_plain_run("WA");check(weak.expired()&&retained.utf8=="WA"&&retained.glyphs==expected.glyphs,"CPU last-release lost retained SAME Impl operation");++checks;
 auto released_cache=ui::BnwSystemFontCacheOwner::create(cpu);std::weak_ptr<ui::BnwSystemFontCacheOwner> weak_cache=released_cache;auto* cache_facade=released_cache.get();
 observer=[&]{released_cache.reset();};const auto retained_cache=cache_facade->measure_plain_run("WA");check(weak_cache.expired()&&retained_cache.gcp.utf8=="WA"&&retained_cache.final_lookup_slots.size()==2,"Cache last-release lost owned ring operation");++checks;
 reentry_refused=false;observer=[&]{try{cache->measure_glyph_index(w.glyph_index);}catch(const std::exception&){reentry_refused=true;}};
 const auto q=cache->measure_glyph_index(w.glyph_index);check(reentry_refused&&cache->current(q),"Cache callback reentry mutated shared ring");++checks;
 auto borrowed_q=q;observer=[&]{borrowed_q.slot.u1=9;borrowed_q.requested_glyph_index=0xffff;};
 const auto stage=cache->stage_raster_glyph_index(borrowed_q);check(stage.quote.slot==q.slot&&stage.quote.requested_glyph_index==q.requested_glyph_index&&cache->current(q),"Callback retargeted borrowed raster quote");++checks;
 bool foreign_refused=false;std::thread other([&]{try{cache->measure_glyph_index(w.glyph_index);}catch(const std::exception&){foreign_refused=true;}});other.join();check(foreign_refused&&cache->current(q),"Foreign thread modified cache operation guard/ring");++checks;
 ++client_revision;rejects([&]{cpu->measure_glyph_index(w.glyph_index);});rejects([&]{cache->measure_glyph_index(w.glyph_index);});--client_revision;check(cache->current(q),"Stale operation published partial glyph state");++checks;
 observer=[&]{++client_revision;};rejects([&]{cpu->measure_plain_run("WA");});--client_revision;check(cache->current(q),"Callback stale failure changed ring");++checks;

 auto borrowed_owner=make_cpu();std::weak_ptr<ui::BnwSystemFontCpuOwner> weak_borrowed=borrowed_owner;auto* borrowed_facade=borrowed_owner.get();
 observer=[&]{borrowed_owner.reset();};rejects([&]{(void)borrowed_facade->current();});check(weak_borrowed.expired(),"Retired CPU facade returned dangling borrowed frame");++checks;
 auto handle_owner=make_cpu();std::weak_ptr<ui::BnwSystemFontCpuOwner> weak_handle=handle_owner;auto* handle_facade=handle_owner.get();
 observer=[&]{handle_owner.reset();};rejects([&]{(void)handle_facade->native_window_handle();});check(weak_handle.expired(),"Retired CPU facade published current HWND identity");++checks;
 auto capacity_owner=ui::BnwSystemFontCacheOwner::create(cpu);const auto expected_capacity=capacity_owner->capacity();std::weak_ptr<ui::BnwSystemFontCacheOwner> weak_capacity=capacity_owner;auto* capacity_facade=capacity_owner.get();
 observer=[&]{capacity_owner.reset();};const auto retained_capacity=capacity_facade->capacity();check(weak_capacity.expired()&&retained_capacity==expected_capacity,"capacity() accessed freed cache facade/ring");++checks;
 reentry_refused=false;observer=[&]{try{(void)cpu->current();}catch(const std::exception&){reentry_refused=true;}};
 check(cpu->current().cpu_raster_available&&reentry_refused,"CPU current() source query recursed");++checks;
 reentry_refused=false;observer=[&]{try{(void)cpu->measure_glyph_index(w.glyph_index);}catch(const std::exception&){reentry_refused=true;}};
 check(cpu->native_window_handle()==reinterpret_cast<std::uintptr_t>(window.handle)&&reentry_refused,"CPU current-source callback entered a mutation");++checks;
 reentry_refused=false;observer=[&]{try{(void)cache->capacity();}catch(const std::exception&){reentry_refused=true;}};
 check(cache->current(q)&&reentry_refused,"Cache current() query admitted recursive capacity()");++checks;
 reentry_refused=false;observer=[&]{try{(void)cache->current(q);}catch(const std::exception&){reentry_refused=true;}};
 check(cache->capacity()==expected_capacity&&reentry_refused,"Cache capacity() query admitted recursive current()");++checks;
 reentry_refused=false;observer=[&]{try{(void)cache->measure_glyph_index(w.glyph_index);}catch(const std::exception&){reentry_refused=true;}};
 check(cache->current(q)&&reentry_refused,"Cache source query admitted ring mutation");++checks;
 observer=[] {throw std::runtime_error("Explicit failing source-query observer");};rejects([&]{(void)cpu->current();});check(cpu->current().cpu_raster_available,"CPU query guard remained stuck after callback error");++checks;
 observer=[] {throw std::runtime_error("Explicit failing cache-query observer");};rejects([&]{(void)cache->capacity();});check(cache->capacity()==expected_capacity&&cache->current(q),"Cache query guard remained stuck after callback error");++checks;
 foreign_refused=false;observer=[&]{std::thread other([&]{try{(void)cache->measure_glyph_index(w.glyph_index);}catch(const std::exception&){foreign_refused=true;}});other.join();};
 check(cache->current(q)&&foreign_refused,"Foreign thread observed cache query flags before owning-thread check");++checks;
 check(!policy->snapshot().native_label_initialization_complete&&!cpu->current().gpu_pair_ready&&!live.session().readiness().can_continue(),"Diagnostic glyph join granted Label/game readiness");++checks;
 std::cout<<"PASS "<<checks<<" real SAME RU HWND/HFONT/GDI glyph-index/cache checks; plain-run zero-class diagnostic only; original CLASSIN/Label/whole readiness false\n";
}
