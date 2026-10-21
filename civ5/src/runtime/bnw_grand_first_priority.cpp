#include "civ5/runtime/bnw_grand_first_priority.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <climits>
#include <stdexcept>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool v,const char *why){if(!v)throw std::runtime_error(why);}
std::int64_t i32(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"Grand priority native int overflow/width");return v;}
auto &named(const auto &rows,std::string_view name){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R *p=nullptr;for(const auto &r:rows)if(r.name&&name==r.name){need(!p,"Grand priority duplicate native field");p=&r;}if(!p)throw std::runtime_error("Grand priority missing native field: "+std::string(name));return *p;}
std::int64_t core(const LiveSessionState &s,std::int64_t p,std::string_view name,bool boolean=false){need(p>=0&&p<64,"Grand priority native player ID invalid");const auto &raw=s.world.all_players->players.at(p);need(raw.saved_player_id==p&&raw.index==static_cast<std::uint64_t>(p)&&raw.core.layout==bnw,"Grand priority native player Read ID/layout differs");const auto &f=named(raw.core.fields,name);need(f.values.size()==1&&f.encoding==(boolean?saves::PlayerCoreEncoding::boolean:saves::PlayerCoreEncoding::i32),"Grand priority native player field encoding unknown");const auto value=i32(f.values[0]);if(boolean)need(value==0||value==1,"Grand priority native bool invalid");return value;}
const auto &team(const LiveSessionState &s,std::int64_t t){need(t>=0&&t<64,"Grand priority native team ID invalid");const auto &raw=s.world.teams->teams.at(t);need(raw.index==static_cast<std::uint64_t>(t)&&named(raw.fields,"id").value==t,"Grand priority native team Read ID differs");return raw;}
std::int64_t player_team(const LiveSessionState &s,std::int64_t p){const auto &mapping=s.world.pregame->archive.integer_arrays.at("team_types");need(mapping.size()==64&&s.setup.players.contains(p)&&mapping.at(p)==s.setup.players.at(p).team,"Grand priority current player-team mirror stale");(void)team(s,mapping.at(p));return mapping.at(p);}
std::int64_t def(const BnwHappinessSuffixRules &r,std::string_view name){const auto *v=r.defines().find(name);need(v&&v->lookup.found&&v->lookup.sqlite_code==0&&!v->retained_previous&&std::holds_alternative<std::int64_t>(v->lookup.value),"Grand priority current native define unavailable");return i32(std::get<std::int64_t>(v->lookup.value));}
bool minor(const LiveSessionState &s,std::int64_t p){const auto it=s.world.pregame->archive.flag_arrays.find("minor_nation_civs");need(it!=s.world.pregame->archive.flag_arrays.end()&&it->second.size()==64,"Grand priority native minor64 inventory unknown");return it->second.at(p);}
bool team_minor(const LiveSessionState &s,std::int64_t t){for(std::int64_t p=22;p<64;++p)if(core(s,p,"m_bAlive",true)&&player_team(s,p)==t){if(minor(s,p))return true;return false;}return false;}
const auto &met(const LiveSessionState &s,std::int64_t t){const auto &m=named(team(s,t).arrays,"has_met");need(m.columns==1&&m.values.size()==64,"Grand priority actual hasMet64 unknown");for(auto v:m.values)need(v==0||v==1,"Grand priority native hasMet bool invalid");return m.values;}
bool victory(const LiveSessionState &s,std::int64_t id){const auto &a=s.world.pregame->archive;const auto n=a.integers.find("num_victory_infos");const auto v=a.flag_arrays.find("victories");need(n!=a.integers.end()&&n->second>=0&&n->second<=INT32_MAX&&v!=a.flag_arrays.end()&&v->second.size()==static_cast<std::size_t>(n->second),"Grand priority actual CvPreGame victories inventory unknown");need(id>=0,"Grand priority native victory index invalid");return id<n->second?v->second.at(id):false;}
bool option(const LiveSessionState &s,const content::RulesDatabase &db,std::string_view name){
 const auto first=[&](const auto &rows)->std::optional<std::int64_t>{for(const auto &row:rows)if(std::string_view(row.name).substr(0,row.name.find('\0'))==name)return i32(row.value);return {};};
 const auto raw=first(s.world.pregame->game_options),owned=first(s.setup.game_options);
 need(!raw||owned.has_value(),"Grand Culture actual option missing owned mirror");
 std::optional<std::int64_t> fallback;
 if(!raw){const auto query=db.query_rows({"SELECT \"Default\" FROM GameOptions WHERE Type=? LIMIT 1",{std::string(name)}},[&](const auto &row){fallback=i32(row.integer_at(0));},1);need(query.all_succeeded(),"Grand Culture actual option default lookup unavailable");}
 const auto value=raw.value_or(fallback.value_or(0));need(!owned||*owned==value,"Grand Culture actual source/rules option mirror stale");return value==1;
}
std::int64_t culture_prefix(const LiveSessionState &s,const BnwHappinessSuffixRules &r,
 const std::vector<std::int64_t> &personality,std::int64_t p,BnwGrandFirstPriorityQuote &q,
 const BnwGrandWorldAverageRules *cities,const content::RulesDatabase &db,const SessionCheckpointIdentity &identity){
 const auto *cultural=r.types().find("VICTORY_CULTURAL");
 if(!cultural||cultural->table!="Victories"||!victory(s,cultural->id)){
  q.culture_victory_disabled=true;return -100;
 }
 const auto *culture=r.types().find("FLAVOR_CULTURE");
 need(culture&&culture->table=="Flavors"&&culture->id>=0&&culture->id<static_cast<std::int64_t>(personality.size()),"Grand Culture actual FLAVOR_CULTURE/native personality binding unavailable");
 const auto era=i32(named(team(s,player_team(s,p)).fields,"era").value);
 bool known=false;for(const auto &[key,v]:r.types().entries()){(void)key;if(v.table=="Eras"&&v.id==era)known=true;}
 need(known,"Grand Culture actual current team era Info unavailable");
 q.culture_initial=i32(i32(i32(10-era)*personality.at(culture->id))*200)/100;
 q.culture_prefix_quoted=true;
 if(option(s,db,"GAMEOPTION_NO_POLICIES")||core(s,p,"m_iAnarchyNumTurns")>0){
  q.total_culture_option_anarchy_checked=true;q.own_culture=0;
  throw std::runtime_error("Grand Culture GetTotalJONSCulturePerTurn native early-return0; next actual own GetTourism/foreign culture/influence source owner required before JonRand");
 }
 q.total_culture_option_anarchy_checked=true;
 need(cities&&cities->identity()==identity.rules_sha256,"Grand Culture GetTotalJONSCulturePerTurn requires actual SAME native city culture rules owner");
 const auto city=quote_bnw_jons_culture_from_cities(s,*cities,r,identity,p);
 if(!city.calculated)throw std::runtime_error("Grand Culture GetTotalJONSCulturePerTurn cities: "+city.blocker);
 q.culture_cities=city.total;q.culture_cities_visited=city.cities.size();
 throw std::runtime_error("Grand Culture GetTotalJONSCulturePerTurn source cities complete; next actual excess-happiness/traits/free/minors/religion founder Lua/bonus/golden-age aggregate on SAME retained epoch before tourism/foreign culture/influence and JonRand");
}
std::int64_t conquest(const LiveSessionState &s,const BnwHappinessSuffixRules &r,std::int64_t p,BnwGrandFirstPriorityQuote &q,const BnwGrandMilitaryAverageJournal *average,const std::function<const BnwGrandMilitaryAverageJournal &()> *provider){
 const auto *domination=r.types().find("VICTORY_DOMINATION");
 if(!domination||domination->table!="Victories"||!victory(s,domination->id)){
  bool any=false;for(const auto &[key,v]:r.types().entries()){(void)key;if(v.table=="Victories"&&victory(s,v.id)){any=true;break;}}
  if(any){q.conquest_victory_disabled=true;return -100;}
 }
 const auto &raw=s.world.all_players->players.at(p);const auto &bias=named(raw.diplomacy.fields,"m_paiPersonalityMajorCivApproachBiases");need(raw.diplomacy.layout==bnw&&bias.encoding==saves::DiplomacyEncoding::i8&&bias.rows==1&&bias.columns==7&&bias.values.size()==7,"Grand priority actual diplomacy personality bias shape unknown");for(auto v:bias.values)need(v>=INT8_MIN&&v<=INT8_MAX,"Grand priority native i8 personality bias invalid");
 const auto &bold=named(raw.diplomacy.fields,"m_iBoldness");need(bold.encoding==saves::DiplomacyEncoding::i32&&bold.rows==1&&bold.columns==1&&bold.values.size()==1,"Grand priority actual Boldness shape unknown");
 const auto own_team=player_team(s,p);const auto era=i32(named(team(s,own_team).fields,"era").value);bool era_known=false;for(const auto &[key,v]:r.types().entries()){(void)key;if(v.table=="Eras"&&v.id==era)era_known=true;}need(era_known,"Grand priority actual current team era Info unavailable");
 // Native enum order WAR0 HOSTILE1 DECEPTIVE2 GUARDED3 AFRAID4 FRIENDLY5 NEUTRAL6.
 const auto approach=i32(std::max({bias.values[0],bias.values[1],bias.values[2]})-bias.values[5]);
 auto priority=i32(i32(i32(bold.values[0])+approach)*i32(12-era));const auto &has_met=met(s,own_team);
 if(q.elapsed>=def(r,"AI_GS_CONQUEST_NOBODY_MET_FIRST_TURN")){q.nobody_met_query=true;bool has_major=false;for(std::int64_t t=0;t<63;++t)if(own_team!=t&&!team_minor(s,t)&&has_met.at(t)){has_major=true;break;}if(!has_major)priority=i32(priority+def(r,"AI_GRAND_STRATEGY_CONQUEST_NOBODY_MET_WEIGHT"));}
 if(q.elapsed>=def(r,"AI_GS_CONQUEST_MILITARY_STRENGTH_FIRST_TURN")){q.military_ratio_query=true;if(provider)average=&(*provider)();need(average&&average->player()==p&&average->elapsed()==q.elapsed&&average->input_revision()==bnw_grand_military_inputs_revision(s),"Grand priority requires actual SAME current world military average/GetMilitaryMight calculator owner");if(average->average()>0){need(core(s,p,"m_iTurnMightRecomputed")>=q.elapsed,"Grand priority own GetMilitaryMight requires actual calculateMilitaryMight AND calculateEconomicMight");auto ratio=i32(i32(core(s,p,"m_iMilitaryMight")-average->average())*def(r,"AI_GRAND_STRATEGY_CONQUEST_POWER_RATIO_MULTIPLIER"))/average->average();if(ratio>0)ratio/=2;priority=i32(priority+ratio);}}
 const auto &at_war=named(team(s,own_team).arrays,"at_war");need(at_war.columns==1&&at_war.values.size()==64,"Grand priority actual atWar64 unknown");for(auto v:at_war.values)need(v==0||v==1,"Grand priority native war bool invalid");std::int64_t wars=0;for(std::int64_t t=0;t<63;++t){++q.war_teams_visited;const auto alive=i32(named(team(s,t).fields,"alive_count").value);need(alive>=0,"Grand priority native team alive counter invalid");if(alive>0&&at_war.values[t]){need(t!=own_team,"Grand priority native self-war invalid; source assert");++wars;}}if(wars>0)priority=i32(priority+def(r,"AI_GRAND_STRATEGY_CONQUEST_AT_WAR_WEIGHT"));
 if(core(s,p,"m_bCramped",true)){
  std::int64_t players_met=1;for(std::int64_t other=0;other<22;++other)if(core(s,other,"m_bAlive",true)&&other!=p&&has_met.at(player_team(s,other)))++players_met;
  need(s.world.map_projection&&s.world.map_plots&&s.world.map_plots->layout==bnw,"Grand priority cramped actual map/plot owner unavailable");const auto &map=*s.world.map_projection;need(map.cells.size()==map.grid.size()&&s.world.map_plots->plots.size()==map.cells.size(),"Grand priority cramped current plot inventory differs");std::int64_t mine=0,all=0;
  for(std::size_t i=0;i<map.cells.size();++i){const auto &c=map.cells[i];const auto &plot=s.world.map_plots->plots[i];const auto owner=named(plot.fields,"owner").value;const auto xy=map.grid.coordinates(i);need(c.index==i&&c.x==xy[0]&&c.y==xy[1]&&plot.index==i&&plot.x==xy[0]&&plot.y==xy[1]&&c.owner&&*c.owner==owner&&owner>=-1&&owner<64,"Grand priority cramped raw/live plot owner or geometry stale");++q.cramped_plots_visited;if(owner==-1)continue;if(owner==p){++mine;++all;}else if(!minor(s,owner)&&has_met.at(player_team(s,owner)))++all;}
  all/=players_met;if(mine>0&&all/mine>0)priority=i32(priority+def(r,"AI_GRAND_STRATEGY_CONQUEST_CRAMPED_WEIGHT"));
 }
 const auto nukes=core(s,p,"m_iNumNukeUnits");need(nukes>=0,"Grand priority actual native nuke counter invalid");if(nukes==0)for(std::int64_t other=0;other<22;++other){++q.nuke_major_slots_visited;if(core(s,other,"m_bAlive",true)&&other!=p&&has_met.at(player_team(s,other))){const auto foreign=core(s,other,"m_iNumNukeUnits");need(foreign>=0,"Grand priority foreign native nuke counter invalid");if(foreign>0){priority=i32(priority-50);break;}}}
 return priority;
}
}
BnwGrandPriorityRules BnwGrandPriorityRules::load(const content::RulesDatabase &db){
 BnwGrandPriorityRules r;r.identity_=identify_bytes(db.gameplay_fingerprint_data()).sha256;std::set<std::int64_t> flavors;
 auto report=db.query_rows({"SELECT ID FROM Flavors",{}},[&](const auto &row){need(flavors.insert(row.integer_at(0)).second,"Grand priority duplicate native Flavor ID");},1024);need(report.all_succeeded()&&!flavors.empty()&&*flavors.begin()==0&&*flavors.rbegin()==static_cast<std::int64_t>(flavors.size()-1),"Grand priority contiguous actual Flavor Info inventory unavailable");r.flavors_=flavors.size();
 std::map<std::int64_t,std::string> strategies;report=db.query_rows({"SELECT ID,Type FROM AIGrandStrategies",{}},[&](const auto &row){need(strategies.emplace(row.integer_at(0),row.text("Type").value()).second,"Grand priority duplicate native strategy ID");},16);need(report.all_succeeded()&&strategies.size()==4&&strategies.begin()->first==0&&strategies.rbegin()->first==3,"Grand priority actual native4 strategy Info order unavailable");
 for(const auto &[id,type]:strategies){Strategy entry{id,type,std::vector<std::int64_t>(r.flavors_,0)};report=db.query_rows({"SELECT Flavors.ID,Flavor FROM AIGrandStrategy_Flavors INNER JOIN Flavors ON FlavorType=Flavors.Type WHERE AIGrandStrategyType=?",{type}},[&](const auto &row){const auto f=row.integer_at(0);need(f>=0&&f<static_cast<std::int64_t>(r.flavors_),"Grand priority native flavor matrix index invalid");entry.flavors[f]=i32(row.integer_at(1));},65536);need(report.all_succeeded(),"Grand priority actual native SetFlavors SQL matrix unavailable");r.strategies_.push_back(std::move(entry));}
 return r;
}
static BnwGrandFirstPriorityQuote quote_slot(const LiveSessionState &s,const BnwGrandPriorityRules &rules,const BnwHappinessSuffixRules &native,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t player,const BnwGrandMilitaryAverageJournal *average,bool conquest_only,const BnwGrandWorldAverageRules *cities,std::optional<std::int64_t> base_only_slot={},const std::function<const BnwGrandMilitaryAverageJournal &()> *provider=nullptr){
 BnwGrandFirstPriorityQuote q;q.player=player;
 try{
  need(id.payload_sha256.size()==64&&id.rules_sha256==rules.identity()&&id.rules_sha256==native.identity()&&identify_bytes(db.gameplay_fingerprint_data()).sha256==id.rules_sha256,"Grand priority actual source/rules identity differs");
  need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64&&s.world.pregame&&s.world.pregame->layout==bnw&&player>=0&&player<22,"Grand priority complete actual BNW world unavailable");
  need(core(s,player,"m_eID")==player&&core(s,player,"m_bAlive",true)&&s.setup.players.at(player).alive,"Grand priority current own identity/alive mirror stale");
  need(s.world.game_state.layout==bnw&&std::holds_alternative<saves::Expansion2PrefixObservation>(s.world.game_state.prefix)&&s.maintenance.profile==content::TechnologyProfile::bnw,"Grand priority native elapsed clock unavailable");const auto &clock=std::get<saves::Expansion2PrefixObservation>(s.world.game_state.prefix);need(clock.scalars.size()>1&&clock.scalars[1]>=0&&clock.scalars[1]==s.maintenance.elapsed_turns,"Grand priority native elapsed clock mirror stale");q.elapsed=i32(clock.scalars[1]);
  need(!rules.strategies().empty()&&rules.strategies()[0].id==0&&rules.strategies()[0].flavors.size()==rules.flavor_slots(),"Grand priority first source strategy/flavor slots unknown");const BnwGrandPriorityRules::Strategy *entry=&rules.strategies()[0];
  if(conquest_only){entry=nullptr;for(const auto &v:rules.strategies())if(v.type=="AIGRANDSTRATEGY_CONQUEST"){need(!entry,"Grand duplicate Conquest native Info");entry=&v;}need(entry,"Grand actual Conquest native Info unavailable");}
  if(base_only_slot){need(*base_only_slot>=0&&*base_only_slot<static_cast<std::int64_t>(rules.strategies().size()),"Grand GetBase requested native Info slot unavailable");entry=&rules.strategies().at(*base_only_slot);need(entry->id==*base_only_slot&&entry->flavors.size()==rules.flavor_slots(),"Grand GetBase native Info/flavor shape differs");}
  const auto &first=*entry;q.strategy=first.id;q.strategy_type=first.type;const auto &raw=s.world.all_players->players.at(player);
  const auto &personality=checked_bnw_personality_flavors(raw,raw,db);need(personality.size()==rules.flavor_slots(),"Grand priority actual FlavorManager/native rules dimensions stale");std::int64_t base=0;for(std::size_t flavor=0;flavor<rules.flavor_slots();++flavor){++q.flavor_slots_visited;const auto weight=first.flavors[flavor];if(weight!=0){++q.nonzero_flavors;base=i32(base+i32(weight*personality[flavor]));}}
  q.base=base;q.base_quoted=true;
  if(base_only_slot){q.blocker="Actual native slot GetBase only; category/source predecessors and RNG/write/Grand completion remain ungranted";return q;}
  if(first.type=="AIGRANDSTRATEGY_CONQUEST")q.category=conquest(s,native,player,q,average,provider);
  else if(first.type=="AIGRANDSTRATEGY_CULTURE")q.category=culture_prefix(s,native,personality,player,q,cities,db,id);
  else throw std::runtime_error("Grand actual source strategy id="+std::to_string(first.id)+" type="+first.type+" requires native category owner before JonRand");
  q.before_random=i32(*q.base+*q.category);q.category_quoted=true;
  q.blocker="Actual source slot base/category read queries complete; next mandatory SAME source slot JonRand/current strategy boost/SetPriority; earlier/later slots and Grand decisions remain uncompleted";
 }catch(const std::exception &e){q.category_quoted=false;q.category.reset();q.before_random.reset();q.blocker=e.what();}return q;
}
BnwGrandFirstPriorityQuote quote_bnw_grand_first_priority(const LiveSessionState &s,
 const BnwGrandPriorityRules &r,const BnwHappinessSuffixRules &native,
 const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t p,
 const BnwGrandMilitaryAverageJournal *a,const BnwGrandWorldAverageRules *cities){return quote_slot(s,r,native,db,id,p,a,false,cities);}
BnwGrandFirstPriorityQuote quote_bnw_grand_base_priority(const LiveSessionState &s,
 const BnwGrandPriorityRules &r,const BnwHappinessSuffixRules &native,
 const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t p,
 std::int64_t strategy){return quote_slot(s,r,native,db,id,p,nullptr,false,nullptr,strategy);}
BnwGrandFirstPriorityQuote quote_bnw_grand_conquest_priority(const LiveSessionState &s,
 const BnwGrandPriorityRules &r,const BnwHappinessSuffixRules &native,
 const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t p,
 const BnwGrandMilitaryAverageJournal *a){return quote_slot(s,r,native,db,id,p,a,true,nullptr);}
BnwGrandFirstPriorityQuote quote_bnw_grand_conquest_priority_at_source(const LiveSessionState &s,
 const BnwGrandPriorityRules &r,const BnwHappinessSuffixRules &native,
 const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t p,
 const std::function<const BnwGrandMilitaryAverageJournal &()> &provider){return quote_slot(s,r,native,db,id,p,nullptr,true,nullptr,{},&provider);}

}
