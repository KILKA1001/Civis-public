#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/ui/bnw_civilopedia_right_tree_owner.hpp"
#include "civ5/ui/bnw_civilopedia_label_metrics.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char **argv){
 check(argc==4,"Requires actual selected save, local rules root and explicit shared asset root");
 const auto assets=std::filesystem::path(argv[3]),root=std::filesystem::path(argv[2]);
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],root,0);
 check(static_cast<bool>(selected.preview),"Selected BNW bootstrap failed");auto &live=*selected.preview.host;
 auto catalog=content::discover_packages({root/"Assets/DLC"});check(catalog.diagnostics.empty(),"Selected catalog unavailable");
 runtime::BnwSelectedStockLuaBootstrap bootstrap(assets,catalog,selected.profile,live.identity(),live.session().state(),live.database());
 check(bootstrap.progress().selected_sources_verified&&!bootstrap.progress().first_prefix_executed,"Selected source preparation failed");
 const auto first=bootstrap.advance();if(!first.first_prefix_executed)throw std::runtime_error(first.blocker);
 const std::vector<std::string> includes{"Assets/UI/InstanceManager.lua","Assets/UI/IconSupport.lua","Assets/Gameplay/Lua/FLuaVector.lua"};
 check(first.prefix_executions==1&&first.manager_bindings==33&&first.invalidation_listeners==2&&first.includes_executed==includes&&first.pending_api=="UIManager.GetScreenSizeVal"&&!first.loading_complete&&!first.gameplay_callbacks_ready,"Actual selected prefix/frontier differs");
 for(unsigned i=0;i<2;++i)check(bootstrap.advance().prefix_executions==1,"Absent client replayed prefix");
 check(!bootstrap.attach_client_extent({}),"Missing client callback accepted");
 check(bootstrap.attach_client_extent({[]{return std::optional<runtime::BnwStockLuaClientExtent>{};}}),"Unavailable provider structure rejected");
 check(bootstrap.advance().native_screen_queries==0,"Unavailable provider executed source screen getter");
 struct Window{HWND handle=nullptr;~Window(){if(handle)DestroyWindow(handle);}}window;
 window.handle=CreateWindowExW(0,L"STATIC",L"",WS_OVERLAPPEDWINDOW,0,0,840,620,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 check(window.handle&&!IsWindowVisible(window.handle),"Fresh owned hidden test client unavailable");
 RECT rect{};check(GetClientRect(window.handle,&rect)!=0,"Actual HWND extent unavailable");
 std::uint64_t revision=7;
 auto native=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window.handle),"actual-owned-test-client",[&]{return revision;});
 std::size_t reads=0;auto provider=runtime::BnwStockLuaClientExtentProvider{[&]{++reads;return native.current();}};
 check(bootstrap.attach_client_extent(provider),"Actual client binding failed");
 const auto screen=bootstrap.advance();
 check(screen.pending_api=="Controls.ScrollBar.GetSize"&&screen.native_screen_queries==1&&screen.screen_width==rect.right-rect.left&&screen.screen_height==rect.bottom-rect.top&&screen.client_extent_revision==7,"Source screen getter did not consume current-process HWND");
 check(!bootstrap.attach_client_extent(provider),"Source screen locals rebound");
 ++revision;check(bootstrap.advance().pending_api=="Controls.ScrollBar.GetSize"&&bootstrap.progress().native_scrollbar_queries==0,"Stale client created control");
 --revision;const auto bar=bootstrap.advance();if(bar.pending_api!="Controls.ScrollPanel.GetSize")throw std::runtime_error(bar.blocker);
 const auto *frame=bootstrap.scrollbar_frame();
 check(frame&&frame->nodes.size()==11&&frame->client.revision==7&&frame->control_revision==0&&frame->control("ScrollBar")->rectangle.width==18&&frame->control("ScrollBar")->rectangle.height==800&&bar.native_scrollbar_queries==1&&bar.scrollbar_width==18&&bar.scrollbar_height==800,"Created bounded getter/render bar differs");
 ++revision;for(unsigned i=0;i<2;++i)check(bootstrap.advance().native_panel_queries==0&&bootstrap.scrollbar_frame()->control_revision==0,"Stale client consumed source panel getter or mutated layout");
 --revision;const auto panel=bootstrap.advance();if(panel.pending_api!="Controls.ScrollBar.SetSize")throw std::runtime_error(panel.blocker);
 check(panel.native_panel_queries==1&&panel.panel_width==1002&&panel.panel_height==980&&panel.native_scrollbar_resizes==0&&panel.native_sibling_mutations==0&&panel.native_left_panel_queries==0&&panel.initial_content_creations==0&&panel.sibling_operations.empty()&&panel.blocker.find("BEFORE setter")!=std::string::npos,"First mandatory native slider path skipped");
 frame=bootstrap.scrollbar_frame();
 check(frame->control_revision==0&&frame->control("ScrollBar")->rectangle.height==800&&frame->control("ScrollPanel")->rectangle.height==980&&!frame->initial_content,"Unsupported setter mutated or published later content");
 const ui::BnwRightTreeFrame *retained_right=nullptr;
 for(unsigned i=0;i<3;++i){const auto held_reads=reads;const auto held=bootstrap.advance();check(held.pending_api=="Controls.ScrollBar.SetSize"&&held.prefix_executions==1&&held.includes_executed==includes&&held.native_screen_queries==1&&held.native_scrollbar_queries==1&&held.native_panel_queries==1&&held.native_scrollbar_resizes==0&&held.native_sibling_mutations==0&&reads>held_reads&&!held.loading_complete&&!held.gameplay_callbacks_ready,"Unsupported native setter resumed/replayed source or opened epoch");
  const auto *right=bootstrap.right_tree_frame();check(right&&right->label_metrics&&right->label_metrics->snapshot().staged_metrics.size()==19&&right->label_metrics->snapshot().missing_font_keys.empty()&&!right->label_metrics->snapshot().native_initialization_complete&&right->controls.size()==288&&right->completed_literal_labels==2&&!right->literal_parent_bounds_propagation_complete&&!right->initial_panel_calculation_complete&&!right->native_slider_setter_complete&&held.staged_right_controls==288&&held.staged_right_literal_metrics==2,"Initial dependencies not retained in SAME stock bootstrap");
  if(retained_right)check(right==retained_right,"Native right-tree was recreated at unchanged frontier");retained_right=right;
 }
 ++revision;const auto stale_right=bootstrap.advance();check(bootstrap.right_tree_frame()==retained_right&&stale_right.native_scrollbar_resizes==0&&stale_right.blocker.find("Actual client changed")!=std::string::npos,"Stale native dependency mutated retained owner or resumed Lua");--revision;
 // Independent real selected VM: source panel getter executes, but late current
 // HWND revision rejects publication. Retain uncertain VM/native scratch; never retry that prefix.
 runtime::BnwSelectedStockLuaBootstrap rollback(assets,catalog,selected.profile,live.identity(),live.session().state(),live.database());
 check(rollback.advance().first_prefix_executed,"Actual rollback prefix failed");
 std::size_t late_reads=0,flip=0;auto late=runtime::BnwStockLuaClientExtentProvider{[&]{++late_reads;if(flip&&late_reads==flip)++revision;return native.current();}};
 check(rollback.attach_client_extent(late),"Actual rollback provider failed");
 check(rollback.advance().pending_api=="Controls.ScrollBar.GetSize"&&rollback.advance().pending_api=="Controls.ScrollPanel.GetSize","Rollback read setup failed");
 const auto original=*rollback.scrollbar_frame();flip=late_reads+3;
 const auto failed=rollback.advance();
 std::cerr<<"Late panel publication witness: reads="<<late_reads<<" expected="<<flip
          <<" pending="<<failed.pending_api<<" panel_queries="<<failed.native_panel_queries
          <<" resizes="<<failed.native_scrollbar_resizes<<" loading="<<failed.loading_complete
          <<" gameplay="<<failed.gameplay_callbacks_ready<<" blocker="<<failed.blocker<<'\n';
 check(late_reads==flip&&failed.pending_api=="Controls.ScrollPanel.GetSize"&&failed.native_panel_queries==0&&failed.native_scrollbar_resizes==0&&failed.blocker.find("Stock panel getter transaction failed; VM and native scratch retained as uncertain: ")==0&&failed.blocker.find("Actual client changed before panel getter publication")!=std::string::npos&&!failed.loading_complete&&!failed.gameplay_callbacks_ready,"Late client revision published source read receipt");
 check(rollback.scrollbar_frame()->control_revision==0&&rollback.scrollbar_frame()->nodes.size()==original.nodes.size(),"Late getter failure changed owner");
 for(std::size_t i=0;i<original.nodes.size();++i){const auto&a=original.nodes[i].rectangle;const auto&b=rollback.scrollbar_frame()->nodes[i].rectangle;check(a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height,"Late getter changed retained rectangles");}
 for(unsigned i=0;i<2;++i)check(rollback.advance().prefix_executions==1&&late_reads==flip&&rollback.progress().native_panel_queries==0&&rollback.progress().native_scrollbar_resizes==0&&rollback.progress().pending_api==failed.pending_api&&rollback.progress().blocker==failed.blocker,"Uncertain source VM replayed or changed failure receipt");
 std::uint64_t unstable=0;auto changed=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window.handle),"unstable",[&]{return unstable++;});check(!changed.current(),"Unstable client revision fabricated frame");
 auto unknown=live.session().state();unknown.world.pregame->archive.flags.erase("load_wb_scenario");
 runtime::BnwSelectedStockLuaBootstrap missing(assets,catalog,selected.profile,live.identity(),unknown,live.database());check(!missing.advance().selected_sources_verified,"Unknown scenario flag inferred");
 auto scenario=live.session().state();scenario.world.pregame->archive.flags["load_wb_scenario"]=true;
 runtime::BnwSelectedStockLuaBootstrap custom(assets,catalog,selected.profile,live.identity(),scenario,live.database());check(!custom.advance().selected_sources_verified,"Scenario accepted normal context loading");
 check(!live.session().readiness().can_continue(),"Partial stock VM opened gameplay readiness");
 std::cout<<"PASS: actual Gandhi stock prefix/includes/33 managers/2 listeners once; actual HWND screen/bar/panel getters once; strict suspend BEFORE first native Slider.SetSize; no setter/content receipts; late stale query retains uncertainty without publication or replay; complete epoch false\n";
}
