#include "civ5/runtime/bnw_plot_terrain_checkpoint_pair.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/saved_terrain.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <type_traits>
using namespace civ5;
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool refused=false;try{f();}catch(const std::exception&){refused=true;}
    check(refused,"Invalid plot/terrain component pair accepted");}
auto& named(auto& rows,std::string_view name){for(auto& row:rows)if(row.name&&name==row.name)return row;
    throw std::runtime_error("Selected pair source field absent");}
struct Pick{std::size_t player_slot=0,city_slot=0,plot=0;std::int64_t owner=-1,id=-1;};
Pick choose(const runtime::LiveSessionState& state){
    for(std::size_t p=0;p<state.world.all_players->players.size();++p){
        const auto& player=state.world.all_players->players[p];if(player.cities.cities.empty())continue;
        const auto& city=player.cities.cities.front();const auto x=named(city.core.fields,"m_iX").values.front();
        const auto y=named(city.core.fields,"m_iY").values.front();
        return {p,0,static_cast<std::size_t>(state.world.map_header.grid.index(
            static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y))),
            player.saved_player_id,named(city.core.fields,"m_iID").values.front()};
    }throw std::runtime_error("Selected pair city unavailable");
}
auto& city(auto& world,const Pick& p){return world.all_players->players.at(p.player_slot).cities.cities.at(p.city_slot);}
auto& raw_terrain(auto& world,const Pick& p){return named(city(world,p).core.fields,"m_aiBaseYieldRateFromTerrain").values;}
void run(const std::filesystem::path& save,const std::filesystem::path& assets){
    const auto original=runtime::identify_file(save).sha256;
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,assets,0);
    if(bootstrap.preview.blocker)throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected pair host absent");const auto& host=*bootstrap.preview.host;
    const auto rules=runtime::BnwPlotTerrainRulesSnapshot::from_rules(host.database());
    const auto features=content::saved_terrain_features(host.database()).resolve;
    const auto make_pair=[&](const runtime::LiveSession& s,const runtime::SessionCheckpointIdentity& identity){
        return runtime::make_bnw_plot_terrain_checkpoint_pair(s,identity,rules);};
    const auto restore_pair=[&](const runtime::BnwPlotTerrainCheckpointPair& p,const runtime::LiveSession& s,
                               const runtime::SessionCheckpointIdentity& identity){
        return runtime::restore_bnw_plot_terrain_checkpoint_pair(p,s,identity,rules);};
    auto pick=choose(host.session().state());auto fixture=host.session().state();
    // Transparent finite NO_TERRAIN/working-city source fixture on the actual
    // decoded roster. Both immutable source snapshot and owned baseline carry
    // these inputs. It is not an original-engine oracle or whole-save import.
    auto& plot=fixture.world.map_plots->plots.at(pick.plot);
    named(plot.fields,"terrain_type").value=-1;
    named(plot.fields,"potential_city_work").value=1;
    named(plot.fields,"working_city_owner").value=pick.owner;
    // Include a tagged native city ID and its legal index-only working handle.
    const auto original_id=pick.id;const auto tagged_id=pick.id|8192;
    if(tagged_id!=original_id){
        auto& owned=fixture.cities.players.at(pick.owner).cities;
        auto node=owned.extract(original_id);check(!node.empty(),"Selected owned city node missing");
        node.key()=tagged_id;node.mapped().id=tagged_id;owned.insert(std::move(node));
        named(city(fixture.world,pick).core.fields,"m_iID").values.front()=tagged_id;
        named(plot.fields,"plot_city_id").value=tagged_id;
        pick.id=tagged_id;
    }
    named(plot.fields,"working_city_id").value=pick.id&8191;
    auto& cache=named(plot.arrays,"yields").values;cache={3,5,0,0,0,0};
    auto& flags=named(named(city(fixture.world,pick).management.blocks,"citizens").arrays,"m_pabWorkingPlot").values;
    check(flags.size()==37,"Selected citizen source shape differs");flags.at(0)=1;
    raw_terrain(fixture.world,pick).at(1)=0;
    fixture.cities.players.at(pick.owner).cities.at(pick.id).yields.erase(1);
    const auto original_projection=*fixture.world.map_projection;
    fixture.world.map_projection=saves::project_saved_map(fixture.world.map_header,*fixture.world.map_plots,features);
    for(std::size_t i=0;i<original_projection.cells.size();++i)
        fixture.world.map_projection->cells[i].is_city=original_projection.cells[i].is_city;
    fixture.world.map_terrain=saves::project_saved_map_terrain(*fixture.world.map_projection);
    auto source=host.session().source();source.snapshot=fixture.world;
    const auto baseline_factory=[&]{return runtime::LiveSession::from_restored_bnw(source,fixture);};
    auto baseline=baseline_factory();auto current=baseline_factory();
    const auto pending=current.begin_bnw_plot_early_zero_yield_refresh(static_cast<std::int64_t>(pick.plot));
    const auto prefix=pending->progress();check(prefix.plot_writes==1&&prefix.terrain_writes==1&&
        prefix.phase==runtime::BnwPlotYieldRefreshPhase::city_screen_predicate&&!prefix.effect_started,
        "Pair fixture did not perform actual raw/owned prefix stores before UI");
    const auto pair=make_pair(current,host.identity());
    auto old_rules=rules;const auto retained_rules=std::move(old_rules);
    refuses([&]{(void)runtime::make_bnw_plot_terrain_checkpoint_pair(current,host.identity(),old_rules);});
    check(runtime::make_bnw_plot_terrain_checkpoint_pair(current,host.identity(),retained_rules).map.bytes==pair.map.bytes,
          "Owned rules snapshot lost source feature data on move");
    check(pair.map.id==17&&pair.city_core.id==25&&pair.map.bytes.at(8)==6&&pair.city_core.bytes.at(8)==1,
          "Pair duplicated format or omitted terrain mutation");
    const auto restored=restore_pair(pair,baseline,host.identity());
    check(raw_terrain(restored.world,pick)==raw_terrain(current.state().world,pick)&&
          named(restored.world.map_plots->plots.at(pick.plot).arrays,"yields").values==
          named(current.state().world.map_plots->plots.at(pick.plot).arrays,"yields").values,
          "Real component pair lost exact raw plot/terrain facts");
    for(std::int64_t y=0;y<6;++y){const auto& owned=restored.cities.players.at(pick.owner).cities.at(pick.id);
        const auto found=owned.yields.find(y);check((found==owned.yields.end()?0:found->second.terrain)==
            raw_terrain(restored.world,pick).at(static_cast<std::size_t>(y)),"Restored owned/raw terrain mirror differs");}
    check(!restored.cities.players.at(pick.owner).cities.at(pick.id).yields.contains(1),
          "Unchanged zero terrain created a fabricated sparse node");
    const auto before=raw_terrain(baseline.state().world,pick);
    auto bad=pair;bad.city_core.bytes.pop_back();
    refuses([&]{(void)restore_pair(bad,baseline,host.identity());});
    bad=pair;bad.map.bytes.pop_back();
    refuses([&]{(void)restore_pair(bad,baseline,host.identity());});
    auto unsupported=current.state().world;
    ++named(city(unsupported,pick).core.fields,"m_iFood").values.front();
    bad=pair;bad.city_core=runtime::make_bnw_city_core_checkpoint(unsupported,source.snapshot,host.identity());
    refuses([&]{(void)restore_pair(bad,baseline,host.identity());});
    auto other=baseline_factory();
    auto& altered=other.mutable_state();++altered.cities.players.at(pick.owner).cities.at(pick.id).yields.at(0).terrain;
    refuses([&]{(void)restore_pair(pair,other,host.identity());});
    other=baseline_factory();auto& stale=other.mutable_state();++raw_terrain(stale.world,pick).at(0);
    refuses([&]{(void)restore_pair(pair,other,host.identity());});
    auto foreign=host.identity();foreign.rules_sha256[0]=foreign.rules_sha256[0]=='a'?'b':'a';
    refuses([&]{(void)restore_pair(pair,baseline,foreign);});
    refuses([&]{(void)make_pair(baseline,host.identity());});
    auto fabricated=fixture;
    fabricated.world=restored.world;fabricated.cities=restored.cities;
    auto no_owner=runtime::LiveSession::from_restored_bnw(source,std::move(fabricated));
    refuses([&]{(void)make_pair(no_owner,host.identity());});
    bool foreign_capture=false,foreign_restore=false;
    const auto thread_identity=host.identity();
    std::thread worker([&]{
        try{(void)make_pair(current,thread_identity);}catch(const std::exception& e){
            foreign_capture=std::string_view(e.what()).find("before live borrow")!=std::string_view::npos;}
        try{(void)restore_pair(pair,baseline,thread_identity);}catch(const std::exception& e){
            foreign_restore=std::string_view(e.what()).find("before live borrow")!=std::string_view::npos;}
    });worker.join();check(foreign_capture&&foreign_restore,
        "Pair accessed foreign live allocation before private source-thread refusal");
    const auto output=std::filesystem::current_path()/("bnw-plot-terrain-components-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave");
    struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code e;std::filesystem::remove(path,e);}}cleanup{output};
    auto document=saves::make_civis_import_document(source.source,original);
    document.sections.push_back(pair.map);document.sections.push_back(pair.city_core);
    saves::write_civis_save(output,document);const auto reopened=saves::read_civis_save(output);
    runtime::BnwPlotTerrainCheckpointPair read_pair;
    for(const auto& section:reopened.sections){if(section.id==17)read_pair.map=section;else if(section.id==25)read_pair.city_core=section;}
    check(read_pair.map.bytes==pair.map.bytes&&read_pair.city_core.bytes==pair.city_core.bytes,
          "Atomic container read/write lost real paired component bytes");
    const auto from_file=restore_pair(read_pair,baseline,host.identity());
    check(raw_terrain(from_file.world,pick)==raw_terrain(current.state().world,pick)&&
          raw_terrain(baseline.state().world,pick)==before&&!current.readiness().can_continue()&&
          pending->progress().phase==prefix.phase&&runtime::identify_file(save).sha256==original,
          "Pair roundtrip changed baseline, acknowledged UI, or opened continuation");
    (void)current.mutable_state();refuses([&]{(void)make_pair(current,host.identity());});
    auto moved=std::move(current);
    refuses([&]{(void)make_pair(current,host.identity());});
    refuses([&]{(void)restore_pair(pair,current,host.identity());});
    (void)moved;
    std::cout<<"PASS actual issued plot-cache/raw+owned terrain component bytes and atomic detached restore; private UI history not serialized\n";
}
}
static_assert(!std::is_default_constructible_v<runtime::BnwPlotTerrainRulesSnapshot>);
static_assert(!std::is_constructible_v<runtime::BnwPlotTerrainRulesSnapshot,saves::SavedTerrainFeatureResolver>);
static_assert(!std::is_invocable_v<decltype(&runtime::make_bnw_plot_terrain_checkpoint_pair),
    const runtime::LiveSession&,const runtime::SessionCheckpointIdentity&,const saves::SavedTerrainFeatureResolver&>);
int main(int argc,char** argv){try{check(argc==3,"Supply selected BNW save and canonical game-assets root");
    run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL BNW plot/terrain pair: "<<e.what()<<'\n';return 1;}}
