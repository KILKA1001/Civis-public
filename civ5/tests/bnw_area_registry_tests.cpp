#include "civ5/runtime/bnw_area_registry.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <climits>
#include <iostream>
using namespace civ5;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const std::exception&){denied=true;}need(denied,"Area registry accepted ambiguous native source");}
void run(){
 // Pure borrowed lookup fixtures, never private gameplay import owners.
 saves::SavedSessionSnapshot s{};need(!runtime::resolve_bnw_area_registry(s,-1),"INVALID_INDEX read missing FLTA");
 s.map_regions.emplace();s.map_regions->layout=saves::GameStateLayout::expansion2_version1;
 saves::MapRegionContainer a{};a.name="areas";a.wire_count=1;a.legacy_metadata={2,1,-1,1,16384};a.next_free={-1,-1};
 saves::MapRegionRecord r{};r.fields={{"id",8193}};r.start_offset=10;r.end_offset=20;a.records={r};s.map_regions->containers={a};
 const auto* current=&s.map_regions->containers[0].records[0];
 need(runtime::resolve_bnw_area_registry(s,1)==current&&runtime::resolve_bnw_area_registry(s,8193)==current,"Native index/full area handles differ");
 need(!runtime::resolve_bnw_area_registry(s,16385)&&!runtime::resolve_bnw_area_registry(s,0),"Native stale tagged/empty area resolved");
 auto& area=s.map_regions->containers[0];area.records[0].fields[0].value=INT64_MAX;area.legacy_metadata={};
 refuses([&]{(void)runtime::resolve_bnw_area_registry(s,1);}); // metadata first
 area=a;area.next_free.pop_back();refuses([&]{(void)runtime::resolve_bnw_area_registry(s,1);});
 area=a;area.records.push_back(r);area.wire_count=2;area.legacy_metadata[3]=0;refuses([&]{(void)runtime::resolve_bnw_area_registry(s,1);});
 area=a;area.legacy_metadata[0]=8193;refuses([&]{(void)runtime::resolve_bnw_area_registry(s,1);});
 area=a;area.legacy_metadata[3]=-1;refuses([&]{(void)runtime::resolve_bnw_area_registry(s,1);});
 area=a;area.records[0].fields.push_back({"id",8193});refuses([&]{(void)runtime::resolve_bnw_area_registry(s,1);});
 std::cout<<"PASS pure native area FLTA null/index/tagged/capacity/count/unique lookup; no freshness capability\n";
}
}
int main(){try{run();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
