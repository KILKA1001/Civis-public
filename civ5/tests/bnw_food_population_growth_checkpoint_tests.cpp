#include "civ5/runtime/bnw_food_population_growth_checkpoint.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/bnw_city_current_food_difference.hpp"
#include "civ5/runtime/bnw_city_current_growth_threshold.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <thread>
#include <type_traits>
namespace {
using namespace civ5;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refused(F&& f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected,"Food47 unsupported journal accepted");}
const auto& named(const auto& rows,std::string_view n){for(const auto& r:rows)if(r.name&&n==r.name)return r;throw std::runtime_error("Food47 natural native field missing");}
std::int64_t scalar(const auto& c,std::string_view n){return named(c.core.fields,n).values.at(0);}
saves::CivisSaveSection& part(saves::CivisSaveDocument& d,std::uint32_t id){saves::CivisSaveSection* result=nullptr;for(auto& s:d.sections)if(s.id==id){need(!result,"Food47 natural duplicate section");result=&s;}need(result,"Food47 natural section absent");return *result;}
const auto& raw_city(const runtime::LiveSession& s,std::int64_t actor,std::int64_t full){for(const auto& c:s.state().world.all_players->players.at(static_cast<std::size_t>(actor)).cities.cities)if(scalar(c,"m_iID")==full)return c;throw std::runtime_error("Food47 natural full city absent");}
std::shared_ptr<ui::SelectedBnwPreviewHost> host(const std::filesystem::path& path,const std::filesystem::path& assets){
 auto boot=ui::bootstrap_selected_bnw_preview(path,assets,0,true);if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 auto h=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(boot.preview.host));need(bool(h),"Food47 ordinary genuine Host absent");return h;
}
// Pure read-only expected schema from genuine source/serialized SQLite
// images. This never opens a Lua connection or supplies gameplay authority.
auto saved_schema(const std::vector<std::uint8_t>& image){
 auto db=content::RulesDatabase::from_saved_image(image);
 std::vector<std::vector<content::DatabaseValue>> rows;
 db->query({"SELECT type,name,tbl_name,sql FROM sqlite_master ORDER BY type,name,tbl_name,sql",{}},[&](auto row){rows.emplace_back(row.begin(),row.end());});
 db->require_saved_checkpoint_boundary();return rows;
}
void require_bundle(const runtime::BnwPopulationTotalsCheckpointBundle&a,const runtime::BnwPopulationTotalsCheckpointBundle&b){
 const std::array<const saves::CivisSaveSection*,6> x{&a.totals,&a.players,&a.cities.core,&a.cities.production,&a.cities.management,&a.cities.tail},y{&b.totals,&b.players,&b.cities.core,&b.cities.production,&b.cities.management,&b.cities.tail};
 for(std::size_t i=0;i<x.size();++i)need(x[i]->id==y[i]->id&&x[i]->flags==y[i]->flags&&x[i]->bytes==y[i]->bytes,"Food47 ordinary reload changed exact native companion");
}
void natural(const std::filesystem::path& original,const std::filesystem::path& assets,const std::filesystem::path& output){
 need(!std::filesystem::exists(output),"Use fresh private Food47 output directory");std::filesystem::create_directories(output);
 const auto original_sha=runtime::identify_file(original).sha256;auto live=host(original,assets);auto& session=live->session();
 need(session.source().source.container==saves::SaveContainerKind::civ5,"Food47 requires original BNW growth fixture");
 // A pristine comparison is numeric data only. It cannot relax either
 // ordinary changed-only46 producer or restoration of unchanged totals.
 const auto baseline=runtime::make_bnw_population_totals_baseline_checkpoint_bundle(session.source().snapshot,live->identity());
 refused([&]{(void)runtime::make_bnw_population_totals_checkpoint_bundle(session.source().snapshot,session.source().snapshot,live->identity());});
 refused([&]{(void)runtime::restore_bnw_population_totals_checkpoint_bundle(baseline,session.source().snapshot,live->identity());});
 const auto db=std::shared_ptr<const content::RulesDatabase>(live,&live->database());const auto lease=std::shared_ptr<const runtime::LiveSession>(live,&session);
 const auto rules=std::make_shared<const runtime::BnwCityFoodRateRules>(runtime::BnwCityFoodRateRules::load(db,live->identity().rules_sha256));
 std::int64_t actor=-1,id=-1,old_population=0;
 for(const auto& player:session.state().world.all_players->players){for(const auto& c:player.cities.cities){
  if(scalar(c,"m_iRazingTurns")>0||named(named(c.management.blocks,"citizens").fields,"m_bForceAvoidGrowth").value!=0)continue;
  const auto full=scalar(c,"m_iID");const auto diff=runtime::inspect_bnw_city_current_food_difference(lease,rules,player.saved_player_id,full).value_times100();if(diff<0)continue;
  const auto threshold=runtime::inspect_bnw_city_current_growth_threshold(live,player.saved_player_id,full).value();
  if((scalar(c,"m_iFood")+diff)/100>=threshold){actor=player.saved_player_id;id=full;old_population=scalar(c,"m_iPopulation");break;}
 }if(actor>=0)break;}
 need(actor>=0,"No genuinely eligible growth city; no synthetic population input");
 const auto before_map=session.state().random.map.state(),before_game=session.state().random.gameplay.state();
 const auto food=session.begin_bnw_city_food_growth_prefix(live,actor,id);need(food->progress().phase==runtime::BnwCityFoodGrowthPhase::population_growth,"Food47 actual growth branch absent");
 const auto child=food->continue_population();need(child->progress().phase==runtime::BnwCityPopulationPhase::plot_yield_child_required,"Food47 actual prePlot child absent");
 const auto capture=food->capture_population_checkpoint_before_plot();auto document=runtime::BnwFoodPopulationGrowthCheckpoint::document(*capture,original_sha);const auto journal=part(document,47);
 const auto saved_database_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::saved_database);
 const auto& original_saved=session.source().snapshot.saved_database;
 need(original_saved.quick_check_passed&&!original_saved.non_sqlite_image_skipped&&!original_saved.image.empty(),"Food47 actual original saved database absent/unverified");
 const auto original_schema=saved_schema(original_saved.image);
 need(part(document,saved_database_id).flags==0&&saved_schema(part(document,saved_database_id).bytes)==original_schema,"Food47 serialized another database domain instead of actual saved data");
 need(scalar(raw_city(session,actor,id),"m_iPopulation")==old_population+1,"Actual native growth population not +1");
 const auto revision=session.state().ui_source_revision;const auto expected=capture->components();
 // Structural negatives use the same writer/parser, and no invalid input may
 // leave a newly published destination. The original input is never written.
 unsigned n=0;const auto bad_write=[&](auto mutate){auto bad=document;mutate(bad);const auto path=output/("rejected-"+std::to_string(++n)+".CivisSave");refused([&]{saves::write_civis_save(path,bad);});need(!std::filesystem::exists(path),"Malformed Food47 was published");};
 bad_write([](auto& d){d.sections.push_back(part(d,47));});
 bad_write([](auto& d){part(d,47).flags=1;});
 bad_write([](auto& d){part(d,47).bytes.clear();});
 bad_write([](auto& d){part(d,47).bytes.pop_back();});
 bad_write([](auto& d){part(d,47).bytes.at(8)=2;});
 bad_write([](auto& d){part(d,47).bytes.at(744)=255;});
 bad_write([](auto& d){part(d,47).bytes.at(744+44)=255;});
 bad_write([](auto& d){part(d,47).bytes.at(744+64)=0;});
 bad_write([](auto& d){auto& b=part(d,46).bytes;b.back()^=1;});
 bad_write([](auto& d){std::erase_if(d.sections,[](const auto&s){return s.id==46;});});
 const auto reject_boot=[&](auto mutate){auto bad=document;mutate(bad);const auto path=output/("semantic-reject-"+std::to_string(++n)+".CivisSave");saves::write_civis_save(path,bad);auto b=ui::bootstrap_selected_bnw_preview(path,assets,0,true);need(bool(b.preview.blocker)&&!b.preview.host,"Food47 forged semantic input published a command-capable Host");};
 reject_boot([](auto& d){part(d,47).bytes.at(744+12)^=1;}); // native before operand
 reject_boot([](auto& d){auto& b=part(d,47).bytes;b.at(76)=b.at(76)=='0'?'1':'0';}); // genuine rules identity
 reject_boot([](auto& d){std::erase_if(d.sections,[](const auto&s){return s.id==47;});}); // numeric46 alone is not phase authority
 const auto first=output/"actual-growth-first.CivisSave";saves::write_civis_save(first,document);
 auto one=host(first,assets);
 need(one->session().source().snapshot.saved_database.image==part(document,saved_database_id).bytes&&one->session().source().snapshot.saved_database.quick_check_passed,"Food47 first ordinary reload did not consume actual saved image");
 auto restored=one->session().bnw_city_food_growth_pending(actor,id);need(restored&&restored->progress().phase==runtime::BnwCityFoodGrowthPhase::population_growth,"Ordinary Food47 bootstrap did not restore actual parent");
 const auto restored_child=restored->population_pending();need(restored_child&&restored_child->progress().phase==runtime::BnwCityPopulationPhase::plot_yield_child_required,"Ordinary Food47 bootstrap lost exact next child");
 restored->require_current();need(!one->session().has_bnw_food_growth_import_quarantine()&&!one->session().readiness().can_continue(),"Food47 restored join either quarantined or minted full readiness");
 need(one->session().state().ui_source_revision==revision&&one->session().state().random.map.state()==before_map&&one->session().state().random.gameplay.state()==before_game&&scalar(raw_city(one->session(),actor,id),"m_iPopulation")==old_population+1,"Food47 reload replayed writes/RNG or changed ordinal");
 refused([&]{(void)one->session().begin_bnw_city_food_growth_prefix(one,actor,id);});
 const auto restored_capture=restored->capture_population_checkpoint_before_plot();require_bundle(restored_capture->components(),expected);
 need(runtime::BnwFoodPopulationGrowthCheckpoint::make(*restored_capture).bytes==journal.bytes,"Food47 restored earned history changed canonical journal");
 auto second_document=runtime::BnwFoodPopulationGrowthCheckpoint::document(*restored_capture,original_sha);const auto second=output/"actual-growth-second.CivisSave";saves::write_civis_save(second,second_document);
 need(saved_schema(part(second_document,saved_database_id).bytes)==original_schema,"Food47 second write changed saved database domain");
 auto two=host(second,assets);
 need(two->session().source().snapshot.saved_database.image==part(second_document,saved_database_id).bytes&&two->session().source().snapshot.saved_database.quick_check_passed,"Food47 second ordinary reload did not consume actual saved image");
 const auto twice=two->session().bnw_city_food_growth_pending(actor,id);need(twice&&twice->progress().ui_revision==revision,"Food47 second ordinary restart lost once history");
 const auto twice_child=twice->population_pending();twice->require_current();const auto twice_capture=twice->capture_population_checkpoint_before_plot();require_bundle(twice_capture->components(),expected);
 need(runtime::BnwFoodPopulationGrowthCheckpoint::make(*twice_capture).bytes==journal.bytes&&two->session().state().random.map.state()==before_map&&two->session().state().random.gameplay.state()==before_game,"Food47 second restart changed source/RNG/native history");
 bool foreign=false;std::thread worker([&]{try{twice_capture->require_current();}catch(const runtime::BnwCityFoodGrowthUnavailable&){foreign=true;}});worker.join();need(foreign,"Foreign-thread restored capture accepted");
 std::cout<<"PASS FOOD47_ACTUAL_GROWTH_TWO_ORDINARY_RESTARTS; partial once owner only\n";
 // Invoke the existing genuine next consumer. Any missing Plot supplier is
 // still a mandatory stop, never an earned full-growth/turn return.
 try{twice_child->continue_plot();need(twice_child->progress().phase==runtime::BnwCityPopulationPhase::plot_call||twice_child->progress().phase==runtime::BnwCityPopulationPhase::update_religion_required,"Food47 consumer skipped actual Plot frontier");std::cout<<"FOOD47_ACTUAL_NEXT_PLOT_PREFIX_REACHED; later required effects remain\n";}
 catch(const runtime::BnwNormalPlotYieldUnavailable& e){need(twice_child->progress().phase==runtime::BnwCityPopulationPhase::failed,"Food47 missing Plot supplier did not quarantine call");std::cout<<"FOOD47_NEXT_PLOT_MANDATORY_STOP: "<<e.what()<<'\n';}
 refused([&]{twice_capture->require_current();});refused([&]{(void)runtime::BnwFoodPopulationGrowthCheckpoint::make(*twice_capture);});
 need(scalar(raw_city(two->session(),actor,id),"m_iPopulation")==old_population+1&&two->session().state().random.map.state()==before_map&&two->session().state().random.gameplay.state()==before_game,"Next Plot replayed population/RNG");
 need(runtime::identify_file(original).sha256==original_sha,"Food47 wrote original Civ5 save");
}
}
static_assert(!std::is_default_constructible_v<civ5::runtime::BnwFoodPopulationCheckpointCapture>);
int main(int argc,char** argv){try{need(argc==4,"Use original BNW save, canonical civ5-assets, fresh private output directory");natural(argv[1],argv[2],argv[3]);return 0;}catch(const std::exception&e){std::cerr<<"FAIL Food47: "<<e.what()<<'\n';return 1;}}
