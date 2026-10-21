#include "civ5/runtime/bnw_food_population_growth_checkpoint.hpp"
#include "civ5/runtime/bnw_area_registry.hpp"
#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include "civ5/runtime/simulation_random_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <bit>
#include <climits>
#include <map>
#include <set>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','F','G','R','1'};
constexpr std::array<std::uint32_t,8> part_ids{46,24,25,26,27,28,16,13};
void need(bool b,const char* m){if(!b)throw BnwCityFoodGrowthUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Food journal native integer overflow");return static_cast<std::int32_t>(n);}
void hash_shape(std::string_view s){need(s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');}),"Food journal malformed hash");}
auto& named(auto& rows,std::string_view n){
 using R=std::remove_reference_t<decltype(rows)>;using T=typename R::value_type;
 using P=std::conditional_t<std::is_const_v<R>,const T,T>;P* out=nullptr;
 for(auto& f:rows)if(f.name&&std::string_view(f.name)==n){need(!out,"Food journal duplicate field");out=&f;}
 need(out,"Food journal missing native field");return *out;
}
auto& city(auto& w,std::int32_t owner,std::int32_t id){
 auto& rows=w.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities;
 using R=std::remove_reference_t<decltype(rows)>;using T=typename R::value_type;
 using P=std::conditional_t<std::is_const_v<R>,const T,T>;P* out=nullptr;
 for(auto& c:rows)if(named(c.core.fields,"m_iID").values.at(0)==id){need(!out,"Food journal duplicate full city ID");out=&c;}
 need(out&&named(out->core.fields,"m_eOwner").values.at(0)==owner,"Food journal actor/city absent");return *out;
}
auto& scalar(auto& c,std::string_view n){auto& f=named(c.core.fields,n);need(f.values.size()==1,"Food journal non-scalar field");return f.values.front();}
bool same(const saves::CivisSaveSection&a,const saves::CivisSaveSection&b){return a.id==b.id&&a.flags==b.flags&&a.bytes==b.bytes;}
std::array<const saves::CivisSaveSection*,6> parts(const BnwPopulationTotalsCheckpointBundle& b){return {&b.totals,&b.players,&b.cities.core,&b.cities.production,&b.cities.management,&b.cities.tail};}
struct Writer {
 std::vector<std::uint8_t> bytes;
 void u32(std::uint32_t n){for(unsigned i=0;i<4;++i)bytes.push_back(static_cast<std::uint8_t>(n>>(i*8)));}
 void i32(std::int32_t n){u32(std::bit_cast<std::uint32_t>(n));}
 void u64(std::uint64_t n){for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(n>>(i*8)));}
 void hash(std::string_view s){hash_shape(s);bytes.insert(bytes.end(),s.begin(),s.end());}
 void rng(SimulationRandomState n){u64(n.seed);u64(n.calls);u64(n.resets);}
};
struct Reader {
 std::span<const std::uint8_t> bytes;std::size_t at=8;
 std::uint32_t u32(){need(at<=bytes.size()&&bytes.size()-at>=4,"Food journal truncated integer");std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(bytes[at++])<<(i*8);return n;}
 std::int32_t i32(){return std::bit_cast<std::int32_t>(u32());}
 std::uint64_t u64(){need(at<=bytes.size()&&bytes.size()-at>=8,"Food journal truncated counter");std::uint64_t n=0;for(unsigned i=0;i<8;++i)n|=std::uint64_t(bytes[at++])<<(i*8);return n;}
 std::string hash(){need(at<=bytes.size()&&bytes.size()-at>=64,"Food journal truncated hash");std::string n(reinterpret_cast<const char*>(bytes.data()+at),64);at+=64;hash_shape(n);return n;}
 SimulationRandomState rng(){SimulationRandomState s{u64(),u64(),u64()};need(s.seed<=UINT32_MAX,"Food journal non-BNW RNG seed");return s;}
 bool boolean(){const auto n=u32();need(n<=1,"Food journal noncanonical boolean");return n!=0;}
};
void write_event(Writer& o,const BnwFoodPopulationStoreRecord& r){
 o.u32(static_cast<std::uint32_t>(r.kind));o.i32(r.actor);o.i32(r.full_object);o.i32(r.before);o.i32(r.after);o.i32(r.projection_before);
 o.u64(r.revision_before);o.u64(r.revision_after);o.u32(static_cast<std::uint32_t>(r.food_phase));o.u32(static_cast<std::uint32_t>(r.population_phase));
 o.u32(r.population?1u:0u);o.u32(r.conditional?1u:0u);o.u32(r.stored?1u:0u);o.u32(r.projection?1u:0u);o.u32(r.returned?1u:0u);
}
}
saves::CivisSaveSection BnwFoodPopulationGrowthCheckpoint::encode(const Record&r){
 Writer o;o.bytes.insert(o.bytes.end(),magic.begin(),magic.end());o.u32(1);o.hash(r.identity.payload_sha256);o.hash(r.identity.rules_sha256);
 o.i32(r.actor);o.i32(r.city);o.i32(r.difference);o.i32(r.threshold);o.u64(r.first_revision);o.u64(r.revision);o.u64(static_cast<std::uint64_t>(r.turn));o.rng(r.map_rng);o.rng(r.game_rng);
 for(const auto& h:r.companions)o.hash(h);need(r.stores.size()<=8208,"Food journal excessive native records");o.u32(static_cast<std::uint32_t>(r.stores.size()));for(const auto& e:r.stores)write_event(o,e);
 return {47,0,std::move(o.bytes)};
}
BnwFoodPopulationGrowthCheckpoint::Record BnwFoodPopulationGrowthCheckpoint::decode(const saves::CivisSaveSection&s){
 need(s.id==47&&s.flags==0&&s.bytes.size()>=744&&s.bytes.size()<=744+8208ULL*68&&std::equal(magic.begin(),magic.end(),s.bytes.begin()),"Food journal unsupported section/flags/extent");
 Reader in{s.bytes};need(in.u32()==1,"Food journal unsupported version");Record r;r.identity={in.hash(),in.hash()};r.actor=in.i32();r.city=in.i32();r.difference=in.i32();r.threshold=in.i32();r.first_revision=in.u64();r.revision=in.u64();const auto turn=in.u64();need(turn<=INT32_MAX,"Food journal unsupported game turn");r.turn=static_cast<std::int64_t>(turn);r.map_rng=in.rng();r.game_rng=in.rng();
 need(r.actor>=0&&r.actor<64&&r.city>=0&&r.difference>=0&&r.threshold>0&&r.revision>=r.first_revision,"Food journal unsupported actor/growth prefix");
 for(auto& h:r.companions)h=in.hash();const auto count=in.u32();need(count>=10&&count<=8208&&s.bytes.size()-in.at==static_cast<std::size_t>(count)*68,"Food journal record count/trailing data");r.stores.reserve(count);
 std::uint64_t revision=r.first_revision;
 for(std::uint32_t n=0;n<count;++n){BnwFoodPopulationStoreRecord e;const auto kind=in.u32();need(kind<=static_cast<std::uint32_t>(BnwFoodPopulationStoreKind::game_population),"Food journal unknown semantic assignment");e.kind=static_cast<BnwFoodPopulationStoreKind>(kind);e.actor=in.i32();e.full_object=in.i32();e.before=in.i32();e.after=in.i32();e.projection_before=in.i32();e.revision_before=in.u64();e.revision_after=in.u64();const auto food=in.u32(),population=in.u32();need(food<=static_cast<std::uint32_t>(BnwCityFoodGrowthPhase::population_growth)&&population<=static_cast<std::uint32_t>(BnwCityPopulationPhase::game_stored),"Food journal unknown/unsupported phase");e.food_phase=static_cast<BnwCityFoodGrowthPhase>(food);e.population_phase=static_cast<BnwCityPopulationPhase>(population);e.population=in.boolean();e.conditional=in.boolean();e.stored=in.boolean();e.projection=in.boolean();e.returned=in.boolean();
  need(e.returned&&e.actor==r.actor&&e.full_object>=0&&e.revision_before==revision&&(!e.stored||revision<UINT64_MAX)&&e.revision_after==revision+(e.stored?1u:0u),"Food journal incomplete/out-of-order return");revision=e.revision_after;r.stores.push_back(e);
 }
 need(revision==r.revision&&in.at==s.bytes.size(),"Food journal final ordinal/extent differs");need(encode(r).bytes==s.bytes,"Food journal noncanonical representation");return r;
}
void validate_bnw_food_growth_journal_shape(const saves::CivisSaveSection&s){(void)BnwFoodPopulationGrowthCheckpoint::decode(s);}
void validate_bnw_food_growth_journal_binding(const saves::CivisSaveSection&s,const SessionCheckpointIdentity&i){const auto r=BnwFoodPopulationGrowthCheckpoint::decode(s);need(r.identity.payload_sha256==i.payload_sha256&&r.identity.rules_sha256==i.rules_sha256,"Food journal original party/rules mismatch");}
void validate_bnw_food_growth_journal_companions(const saves::CivisSaveDocument&d){
 const saves::CivisSaveSection* journal=nullptr;std::array<const saves::CivisSaveSection*,8> all{};
 for(const auto&s:d.sections){if(s.id==47){need(!journal,"Food journal duplicate47");journal=&s;}for(std::size_t n=0;n<8;++n)if(s.id==part_ids[n]){need(!all[n],"Food journal duplicate companion");all[n]=&s;}}
 need(journal,"Food journal absent");const auto r=BnwFoodPopulationGrowthCheckpoint::decode(*journal);for(std::size_t n=0;n<8;++n)need(all[n]&&all[n]->flags==0&&identify_bytes(all[n]->bytes).sha256==r.companions[n],"Food journal missing/foreign companion");validate_bnw_population_totals_checkpoint_companions(d);
}
void BnwFoodPopulationGrowthCheckpoint::validate_growth(const Record&r,const saves::SavedSessionSnapshot& original,const BnwPopulationTotalsCheckpointBundle& bundle,std::int32_t genuine_diff,std::int32_t genuine_threshold){
 need(r.difference==genuine_diff&&r.threshold==genuine_threshold,"Food journal differs from genuine original numeric reads");
 auto staged=original;auto& c=city(staged,r.actor,r.city);need(scalar(c,"m_iRazingTurns")==0&&named(named(c.management.blocks,"citizens").fields,"m_bForceAvoidGrowth").value==0,"Food journal unsupported razing/forced growth");
 std::size_t at=0;std::uint64_t revision=r.first_revision;
 const auto consume=[&](BnwFoodPopulationStoreRecord expected,std::int64_t& target){
  expected.actor=r.actor;expected.before=native(target);expected.revision_before=revision;expected.stored=!expected.conditional||expected.before!=expected.after;
  need(!expected.stored||revision<UINT64_MAX,"Food journal replay counter overflow");expected.revision_after=revision+(expected.stored?1u:0u);expected.returned=true;
  need(at<r.stores.size()&&r.stores[at]==expected,"Food journal forged native assignment/order/before/projection");++at;revision=expected.revision_after;target=expected.after;
 };
 const auto food=[&](BnwFoodPopulationStoreKind kind,std::string_view field,std::int32_t value,bool conditional,BnwCityFoodGrowthPhase phase){BnwFoodPopulationStoreRecord e;e.kind=kind;e.full_object=r.city;e.after=value;e.conditional=conditional;e.food_phase=phase;consume(e,scalar(c,field));};
 food(BnwFoodPopulationStoreKind::food,"m_iFood",detail::bnw_growth_add_food(native(scalar(c,"m_iFood")),genuine_diff),true,BnwCityFoodGrowthPhase::food_stored);
 food(BnwFoodPopulationStoreKind::food_kept,"m_iFoodKept",detail::bnw_growth_add_kept(native(scalar(c,"m_iFoodKept")),genuine_diff),false,BnwCityFoodGrowthPhase::kept_stored);
 food(BnwFoodPopulationStoreKind::food_kept,"m_iFoodKept",detail::bnw_growth_clamp_kept(native(scalar(c,"m_iFoodKept")),genuine_threshold,native(scalar(c,"m_iMaxFoodKeptPercent"))),false,BnwCityFoodGrowthPhase::kept_clamped);
 need(scalar(c,"m_iFood")/100>=genuine_threshold,"Food journal original growth branch not taken");const auto minus=native(std::int64_t(genuine_threshold)-scalar(c,"m_iFoodKept"));const auto kept=std::max(0,minus);
 food(BnwFoodPopulationStoreKind::food,"m_iFood",detail::bnw_growth_food_change(native(scalar(c,"m_iFood")),-kept),true,BnwCityFoodGrowthPhase::population_growth);
 const auto population=detail::bnw_population_add(native(scalar(c,"m_iPopulation")),1);
 const auto pop=[&](BnwFoodPopulationStoreKind kind,std::int32_t id,std::int64_t& target,std::int32_t value,BnwCityPopulationPhase phase,bool projection=false){BnwFoodPopulationStoreRecord e;e.kind=kind;e.full_object=id;e.after=value;e.population=true;e.population_phase=phase;e.projection=projection;if(projection)e.projection_before=native(target);consume(e,target);};
 pop(BnwFoodPopulationStoreKind::city_population,r.city,scalar(c,"m_iPopulation"),population,BnwCityPopulationPhase::population_stored,true);
 std::map<std::int32_t,std::int32_t> slots;for(const auto& row:staged.all_players->players.at(static_cast<std::size_t>(r.actor)).cities.cities){const auto id=native(scalar(row,"m_iID"));need(slots.emplace(id&8191,id).second,"Food journal original FLTA city slot alias");}
 for(const auto&[slot,id]:slots){(void)slot;auto& rank=named(named(city(staged,r.actor,id).production.blocks,"before_strategy").fields,"m_bPopulationRankValid").value;need(rank==0||rank==1,"Food journal native rank flag malformed");pop(BnwFoodPopulationStoreKind::population_rank,id,rank,0,BnwCityPopulationPhase::ranks_invalidated);}
 if(population>scalar(c,"m_iHighestPopulation"))pop(BnwFoodPopulationStoreKind::highest_population,r.city,scalar(c,"m_iHighestPopulation"),population,BnwCityPopulationPhase::highest_population_stored);
 const auto x=scalar(c,"m_iX"),y=scalar(c,"m_iY");need(x>=0&&y>=0&&std::uint64_t(x)<staged.map_header.grid.width&&std::uint64_t(y)<staged.map_header.grid.height&&staged.map_plots,"Food journal original city plot unavailable");const auto index=staged.map_header.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));const auto& plot=staged.map_plots->plots.at(static_cast<std::size_t>(index));need(plot.index==index,"Food journal physical map index alias");const auto area_id=native(named(plot.fields,"area").value);const auto* resolved=resolve_bnw_area_registry(staged,area_id);need(resolved,"Food journal original area unavailable");auto& areas=named(staged.map_regions->containers,"areas");saves::MapRegionRecord* area=nullptr;for(auto& a:areas.records)if(&a==resolved)area=&a;need(area,"Food journal original area object alias");const auto full_area=native(named(area->fields,"id").value);
 auto& area_pop=named(area->fields,"population").value;pop(BnwFoodPopulationStoreKind::area_population,full_area,area_pop,detail::bnw_population_add(native(area_pop),1),BnwCityPopulationPhase::area_stored);
 auto& array=named(area->arrays,"population_per_player");need(array.columns==1&&array.values.size()==80,"Food journal original area owner roster invalid");auto& area_owner=array.values.at(static_cast<std::size_t>(r.actor));pop(BnwFoodPopulationStoreKind::area_player_population,full_area,area_owner,detail::bnw_population_add(native(area_owner),1),BnwCityPopulationPhase::area_stored);
 auto& player_total=scalar(staged.all_players->players.at(static_cast<std::size_t>(r.actor)),"m_iTotalPopulation");pop(BnwFoodPopulationStoreKind::player_population,r.actor,player_total,detail::bnw_population_add(native(player_total),1),BnwCityPopulationPhase::player_stored,true);
 need(staged.pregame&&staged.pregame->archive.integer_arrays.at("team_types").size()==64&&staged.teams&&staged.teams->teams.size()==64,"Food journal original team roster invalid");const auto team=native(staged.pregame->archive.integer_arrays.at("team_types").at(static_cast<std::size_t>(r.actor)));need(team>=0&&team<64,"Food journal original actor team invalid");auto& team_row=staged.teams->teams.at(static_cast<std::size_t>(team));need(team_row.index==static_cast<std::uint64_t>(team)&&named(team_row.fields,"id").value==team,"Food journal original team object alias");auto& team_total=named(team_row.fields,"population").value;pop(BnwFoodPopulationStoreKind::team_population,team,team_total,detail::bnw_population_add(native(team_total),1),BnwCityPopulationPhase::team_stored);
 auto& game_total=std::get<saves::Expansion2PrefixObservation>(staged.game_state.prefix).scalars.at(10);pop(BnwFoodPopulationStoreKind::game_population,10,game_total,detail::bnw_population_add(native(game_total),1),BnwCityPopulationPhase::game_stored);
 need(at==r.stores.size()&&revision==r.revision,"Food journal contains unreached/omitted native history");const auto expected=make_bnw_population_totals_checkpoint_bundle(staged,original,r.identity);const auto got=parts(bundle),want=parts(expected);
 for(std::size_t n=0;n<6;++n)need(same(*got[n],*want[n])&&identify_bytes(got[n]->bytes).sha256==r.companions[n],"Food journal current fields/full roster differ from exact native history");
 // Existing decoder independently validates roster/schema/area/team/Game deltas.
 (void)restore_bnw_population_totals_checkpoint_bundle(bundle,original,r.identity);
}
saves::CivisSaveSection BnwFoodPopulationGrowthCheckpoint::make(const BnwFoodPopulationCheckpointCapture& capture){
 capture.require_current();const auto parent=capture.parent_;const auto host=capture.host_;need(parent&&host,"Food journal genuine capture owner absent");
 Record r;r.identity=capture.identity_;r.actor=capture.food_.owner;r.city=capture.food_.city;r.difference=capture.food_.difference_times100;r.threshold=parent->growth_checkpoint_threshold_for(capture);r.revision=capture.revision_;r.turn=capture.turn_;r.map_rng=capture.map_rng_;r.game_rng=capture.game_rng_;r.stores=capture.stores_;need(!r.stores.empty(),"Food journal empty history");r.first_revision=r.stores.front().revision_before;
 const auto p=parts(capture.components_);for(std::size_t n=0;n<6;++n)r.companions[n]=identify_bytes(p[n]->bytes).sha256;
 const auto& source=host->session().source();const auto& state=host->session().state();const auto original_random=restore_session_random(source.snapshot.progression);
 need(r.first_revision==1&&source.snapshot.pregame&&r.turn==source.snapshot.pregame->archive.integers.at("game_turn")&&
  state.maintenance.elapsed_turns==std::get<saves::Expansion2PrefixObservation>(source.snapshot.game_state.prefix).scalars.at(1)&&
  r.map_rng==original_random.map.state()&&r.game_rng==original_random.gameplay.state()&&!state.game_clock.sent_auto_moves,
  "Food journal cannot persist unsupported prior clock/RNG/once changes");
 r.companions[6]=identify_bytes(capture.clock_.bytes).sha256;const auto random=make_simulation_random_checkpoint(state.random,r.identity);r.companions[7]=identify_bytes(random.bytes).sha256;
 validate_growth(r,host->session().source().snapshot,capture.components_,r.difference,r.threshold);auto section=encode(r);(void)decode(section);capture.require_current();return section;
}
saves::CivisSaveDocument BnwFoodPopulationGrowthCheckpoint::document(const BnwFoodPopulationCheckpointCapture& capture,std::string_view source_sha){
 auto journal=make(capture);const auto host=capture.host_;need(source_sha==host->original_source_sha256_,"Food journal caller source hash is not the actual factory-measured original file");
 // The immutable gameplay rules DB is not the native saved-data connection.
 // Retain the actual imported Host owner; a claimed VM must use its genuine
 // SourceOwner save operation, whose history this Food-only join cannot issue.
 capture.require_current();const auto saved_database=host->saved_database_;
 need(saved_database&&!host->saved_database_claimed_.load(),"Food journal requires actual unclaimed imported saved database owner");
 saved_database->require_saved_checkpoint_boundary();capture.require_current();
 need(host->saved_database_==saved_database&&!host->saved_database_claimed_.load(),"Food journal saved database owner changed before serialization");
 const auto image=saved_database->saved_image();
 saved_database->require_saved_checkpoint_boundary();capture.require_current();
 need(host->saved_database_==saved_database&&!host->saved_database_claimed_.load(),"Food journal saved database owner changed after serialization");
 auto result=saves::make_civis_import_document(host->session().source().source,source_sha,std::span<const std::uint8_t>(image));
 const auto upsert=[&](saves::CivisSaveSection s){auto found=result.sections.end();for(auto i=result.sections.begin();i!=result.sections.end();++i)if(i->id==s.id){need(found==result.sections.end(),"Food journal duplicated existing companion");found=i;}if(found==result.sections.end())result.sections.push_back(std::move(s));else{need(same(*found,s),"Food journal cannot overwrite mismatched persisted history");}};
 for(const auto* s:parts(capture.components_))upsert(*s);upsert(capture.clock_);upsert(make_simulation_random_checkpoint(host->session().state().random,capture.identity_));upsert(std::move(journal));validate_bnw_food_growth_journal_companions(result);capture.require_current();return result;
}
}
