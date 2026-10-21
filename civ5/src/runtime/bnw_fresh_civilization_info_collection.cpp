#include "civ5/runtime/bnw_fresh_civilization_info_collection.hpp"
#include "civ5/runtime/bnw_fresh_pregame_speed.hpp"
#include "civ5/content/rules_database.hpp"
#include <array>
#include <atomic>
#include <climits>
#include <map>
#include <optional>
#include <stdexcept>
#include <thread>
#include <functional>
namespace civ5::runtime {
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::int32_t i32(std::int64_t value){need(value>=INT32_MIN&&value<=INT32_MAX,"Ordered Info native signed32 source unavailable");return static_cast<std::int32_t>(value);}
// Exact PrefetchGameData order (SDK CvDllDatabaseUtility.cpp). Optional native
// Notifications pointer needs its actual producer; it cannot be omitted here.
constexpr const char* queue[]={"Colors","PlayerColors","InterfaceModes","AICityStrategies","AIEconomicStrategies",
 "AIGrandStrategies","AIMilitaryStrategies","CitySpecializations","TacticalMoves","UnitAIInfos","UnitCombatInfos",
 "BuildingClasses","Buildings","EmphasizeInfos","Eras","GameOptions","GameSpeeds","HandicapInfos","HurryInfos",
 "MultiplayerOptions","PlayerOptions","Policies","PolicyBranchTypes","Processes","Projects","SmallAwards",
 "Specialists","Victories","Votes","VoteSources","Domains","Leaders","Routes","Notifications","Technologies",
 "Features","Improvements","ResourceClasses","Resources","Terrains","Yields","Automates","Builds","Commands",
 "Controls","EntityEvents","Missions","MultiUnitFormations","SpecialUnits","UnitClasses","UnitPromotions","Units",
 "Civilizations","MinorCivilizations","Traits","Religions","Beliefs","LeagueSpecialSessions","LeagueNames",
 "LeagueProjects","LeagueProjectRewards","Resolutions"};
struct Info {
 std::int32_t id=-1;std::map<std::string,std::string,std::less<>> strings;
 std::map<std::string,std::int32_t,std::less<>> integers;
 std::map<std::string,bool,std::less<>> booleans;
 std::map<std::string,float,std::less<>> floats;
 std::array<std::uint32_t,11> package{};
 std::map<std::string,std::vector<std::int32_t>,std::less<>> integer_arrays;
 std::map<std::string,std::vector<bool>,std::less<>> boolean_arrays;
 std::vector<std::string> city_names,spy_names;
 std::vector<std::array<std::optional<std::int32_t>,2>> turns;
};
// Original ExtractGUID/GetGUIDSegment keeps partial stores on malformed input.
void extract_guid(std::string_view text,std::array<std::uint32_t,11>& out){
 if(const auto nul=text.find('\0');nul!=text.npos)text=text.substr(0,nul);
 std::size_t at=0;constexpr unsigned digits[]={8,4,4,2,2,2,2,2,2,2,2};
 for(std::size_t part=0;part<out.size();++part){
  while(at<text.size()&&(text[at]=='{'||text[at]=='-'||text[at]==' '))++at;
  out[part]=0;if(at==text.size())return;
  unsigned remaining=digits[part];do{
   const auto c=text[at];unsigned v=0;
   if(c>='0'&&c<='9')v=static_cast<unsigned>(c-'0');else if(c>='a'&&c<='f')v=static_cast<unsigned>(c-'a'+10);
   else if(c>='A'&&c<='F')v=static_cast<unsigned>(c-'A'+10);else return;
   out[part]=out[part]*16+v;++at;
  }while(at<text.size()&&--remaining);
 }
}
}
struct BnwFreshCivilizationInfoCollection::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::weak_ptr<BnwFreshPregameSpeed> parent;
 const content::RulesDatabase* db=nullptr;const content::InfoTypeCatalog* types=nullptr;
 std::map<std::string,std::vector<std::unique_ptr<Info>>,std::less<>> collections;
 std::vector<std::string> order;std::size_t cursor=0;bool attempted=false;
 std::map<std::string,content::GlobalDefineValue,std::less<>> global_defines;
 mutable bool busy=false;mutable std::atomic<bool> retired{false};
 std::function<std::string(std::string_view)> localized;
 std::shared_ptr<BnwFreshPregameSpeed> current()const{
  need(thread==std::this_thread::get_id()&&!retired.load(),"Ordered Info source foreign thread/retired");
  auto p=parent.lock();need(bool(p),"Ordered Info original Pregame retired");p->require_current();
  need(!retired.load(),"Ordered Info retired during parent validation");return p;
 }
 void text(Info& info,const content::RulesRowView& row,const char* key,const char* field=nullptr){
  info.strings[field?field:key]=row.text(key).value_or("");order.emplace_back(std::string("CacheResults ")+(field?field:key));
 }
 void base(Info& info,const content::RulesRowView& row){
  info.id=i32(row.integer("ID"));order.emplace_back("CvBaseInfo.m_iID");
  for(const auto* field:{"Type","Text","Civilopedia","Strategy","Help","DisabledHelp"})text(info,row,field);
  if(!info.strings["Text"].empty()){info.strings["LocalizedText"]=localized(info.strings["Text"]);current();order.emplace_back("CvBaseInfo Text Locale");}
  if(const auto description=row.text("Description")){
   info.strings["DescriptionKey"]=*description;info.strings["Description"]=localized(*description);current();order.emplace_back("CvBaseInfo Description Locale");
   if(info.strings["Text"].empty()){info.strings["Text"]=*description;info.strings["LocalizedText"]=info.strings["Description"];order.emplace_back("CvBaseInfo Description fallback Text");}
  }
 }
 std::int32_t type(const std::optional<std::string>& text_value){
  if(!text_value)return -1;const auto* found=types->find(*text_value);return found?i32(found->id):-1;
 }
 template<class F>void rows(std::string sql,std::vector<content::DatabaseValue> values,F&& f){
  current();const auto result=db->query_rows({std::move(sql),std::move(values)},[&](const auto& row){current();f(row);current();},65536);
  current();need(result.all_succeeded(),"Ordered Info mandatory linked query/Step failed");
 }
 std::size_t max_rows(const char* table){
  std::size_t out=0;bool stepped=false;rows(std::string("SELECT max(rowid) from ")+table,{},[&](const auto& row){
   const auto max=i32(row.integer_at(0));need(max>=-1&&max<65536,"Info native MaxRows outside bounded allocation");out=static_cast<std::size_t>(max+1);stepped=true;
  });need(stepped,"Native MaxRows aggregate did not step");order.emplace_back(std::string("MaxRows ")+table);return out;
 }
 std::size_t index(const content::RulesRowView& row,int column,std::size_t span){const auto id=i32(row.integer_at(column));need(id>=0&&static_cast<std::size_t>(id)<span,"Native Info linked array index outside actual allocation");return static_cast<std::size_t>(id);}
 void existence(Info& info,const char* field,const char* type_table,const char* data,const char* type_column,const std::string& civ){
  auto& out=info.boolean_arrays[field];out.assign(max_rows(type_table),false);order.emplace_back(std::string("InitializeArray false ")+field);
  rows(std::string("select ")+type_table+".ID from "+data+" inner join "+type_table+" on "+type_column+" = "+type_table+".Type where CivilizationType = ?",
   {civ},[&](const auto& row){out[index(row,0,out.size())]=true;});order.emplace_back(std::string("PopulateArrayByExistence ")+field);
 }
 void civilization(Info& info,const content::RulesRowView& row){
  info.integers["DefaultPlayerColor"]=-1;info.integers["ArtStyleType"]=-1;info.integers["NumLeaders"]=0;
  info.booleans["Playable"]=false;info.booleans["AIPlayable"]=false;info.booleans["Coastal"]=false;info.booleans["PlaceFirst"]=false;
  order.emplace_back("CvCivilizationInfo constructor/base/NULL arrays");base(info,row);
  info.booleans["Playable"]=row.boolean("Playable");info.booleans["AIPlayable"]=row.boolean("AIPlayable");
  text(info,row,"ShortDescription","ShortDescriptionKey");info.strings["ShortDescription"]=localized(info.strings["ShortDescriptionKey"]);current();
  if(const auto package=row.text("PackageID"))extract_guid(*package,info.package);
  order.emplace_back("CvCivilizationBaseInfo CacheResults return");
  const auto units=max_rows("UnitClasses"),buildings=max_rows("BuildingClasses");
  text(info,row,"Adjective","AdjectiveKey");info.strings["Adjective"]=localized(info.strings["AdjectiveKey"]);current();
  info.integers["DefaultPlayerColor"]=type(row.text("DefaultPlayerColor"));text(info,row,"ArtDefineTag");
  info.integers["ArtStyleType"]=type(row.text("ArtStyleType"));
  for(const auto* field:{"ArtStylePrefix","ArtStyleSuffix","DawnOfManAudio","SoundtrackTag"})text(info,row,field);
  const auto& civ=info.strings.at("Type");
  rows("select StartAlongOcean from Civilization_Start_Along_Ocean where CivilizationType = ?",{civ},[&](const auto& r){info.booleans["Coastal"]=r.boolean("StartAlongOcean");});
  rows("select PlaceFirst from Civilization_Start_Place_First_Along_Ocean where CivilizationType = ?",{civ},[&](const auto& r){info.booleans["PlaceFirst"]=r.boolean("PlaceFirst");});
  info.city_names.clear();rows("select CityName from Civilization_CityNames where CivilizationType = ?",{civ},[&](const auto& r){info.city_names.push_back(r.text("CityName").value_or(""));});
  auto& b=info.integer_arrays["Buildings"];b.assign(max_rows("BuildingClasses"),-1);
  rows("select BuildingClasses.ID, Buildings.ID as BuildingID from BuildingClasses inner join Buildings on Buildings.Type = DefaultBuilding",{},[&](const auto& r){b[index(r,0,b.size())]=i32(r.integer_at(1));});
  auto& bo=info.boolean_arrays["BuildingOverridden"];bo.reserve(buildings);bo.clear();bo.resize(buildings,false);
  rows("select BuildingClasses.ID, coalesce(Buildings.ID, -1) from Civilization_BuildingClassOverrides inner join BuildingClasses on BuildingClassType = BuildingClasses.Type left outer join Buildings on BuildingType = Buildings.Type where CivilizationType = ?",{civ},[&](const auto& r){const auto at=index(r,0,b.size());need(at<bo.size(),"Building override extent mismatch");b[at]=i32(r.integer_at(1));bo[at]=true;});
  auto& u=info.integer_arrays["Units"];u.assign(max_rows("UnitClasses"),-1);
  rows("select UnitClasses.ID, Units.ID as UnitID from UnitClasses inner join Units on Units.Type = DefaultUnit",{},[&](const auto& r){u[index(r,0,u.size())]=i32(r.integer_at(1));});
  auto& uo=info.boolean_arrays["UnitOverridden"];uo.reserve(units);uo.clear();uo.resize(units,false);
  rows("select UnitClasses.ID, coalesce(Units.ID, -1) from Civilization_UnitClassOverrides inner join UnitClasses on UnitClassType = UnitClasses.Type left outer join Units on UnitType = Units.Type where CivilizationType = ?",{civ},[&](const auto& r){const auto at=index(r,0,u.size());need(at<uo.size(),"Unit override extent mismatch");u[at]=i32(r.integer_at(1));uo[at]=true;});
  auto& count=info.integer_arrays["FreeUnitCount"];count.assign(units,-1);auto& ai=info.integer_arrays["FreeUnitAI"];ai.assign(units,-1);
  rows("select UnitClasses.ID as UnitClassID, UnitAIInfos.ID as UnitAITypeID, Count from Civilization_FreeUnits inner join UnitClasses on UnitClassType = UnitClasses.Type inner join UnitAIInfos on UnitAIType = UnitAIInfos.Type where CivilizationType = ?",{civ},[&](const auto& r){const auto at=index(r,0,count.size());count[at]=i32(r.integer_at(2));ai[at]=i32(r.integer_at(1));});
  existence(info,"FreeBuildingClasses","BuildingClasses","Civilization_FreeBuildingClasses","BuildingClassType",civ);
  existence(info,"FreeTechs","Technologies","Civilization_FreeTechs","TechType",civ);
  existence(info,"DisabledTechs","Technologies","Civilization_DisableTechs","TechType",civ);
  existence(info,"Leaders","Leaders","Civilization_Leaders","LeaderheadType",civ);
  existence(info,"Religions","Religions","Civilization_Religions","ReligionType",civ);
  info.spy_names.clear();rows("select SpyName from Civilization_SpyNames where CivilizationType = ?",{civ},[&](const auto& r){info.spy_names.push_back(r.text("SpyName").value_or(""));});
  order.emplace_back("CvCivilizationInfo full CacheResults return");
 }
 void speed(Info& info,const content::RulesRowView& row){
  info.integers["NumTurnIncrements"]=0;
  base(info,row);
  for(const auto* field:{"DealDuration","GrowthPercent","TrainPercent","ConstructPercent","CreatePercent","ResearchPercent",
   "GoldPercent","GoldGiftMod","BuildPercent","ImprovementPercent","GreatPeoplePercent","CulturePercent","FaithPercent","BarbPercent",
   "FeatureProductionPercent","UnitDiscoverPercent","UnitHurryPercent","UnitTradePercent","GoldenAgePercent","HurryPercent","InflationOffset",
   "InflationPercent","ReligiousPressureAdjacentCity","VictoryDelayPercent","MinorCivElectionFreqMod","OpinionDurationPercent","SpyRatePercent",
   "PeaceDealDuration","RelationshipDuration","LeaguePercent"}){info.integers[field]=i32(row.integer(field));order.emplace_back(std::string("GameSpeed ")+field);}
  const auto& name=info.strings.at("Type");need(name.find('\'')==name.npos,"Native formatted GameSpeed query literal outside bounded proven subset");
  std::size_t span=0;rows("select count(*) from GameSpeed_Turns where GameSpeedType = '"+name+"'",{},[&](const auto& r){const auto n=i32(r.integer_at(0));need(n>=0&&n<=65536,"Native turn count outside budget");info.integers["NumTurnIncrements"]=n;span=static_cast<std::size_t>(n);});
  info.turns.clear();info.turns.resize(span);std::size_t at=0;
  rows("select * from GameSpeed_Turns where GameSpeedType = '"+name+"'",{},[&](const auto& r){need(at<info.turns.size(),"Native turn rows exceed allocated span");info.turns[at++]={i32(r.integer("MonthIncrement")),i32(r.integer("TurnsPerIncrement"))};});
  need(at==span,"Native turn CacheResults leaves mandatory unknown allocated slots");order.emplace_back("CvGameSpeedInfo full CacheResults return");
 }
 void colors(Info& info,const content::RulesRowView& row){
  base(info,row);for(const auto* field:{"Red","Green","Blue","Alpha"})info.floats[field]=static_cast<float>(row.real(field));
  order.emplace_back("CvColorInfo actual RGBA CacheResults return");
 }
 void populate(const char* table){
  auto& collection=collections[table];collection.clear();order.emplace_back(std::string("PrefetchCollection delete/clear ")+table);
  const auto result=db->prefetch_info(table,[&](std::uint64_t slot,std::int64_t,const auto& row){
   current();need(slot<65536,"Native Info slot span exceeds bounded allocation");
   while(collection.size()<slot){collection.push_back({});order.emplace_back(std::string("NULL slot ")+table);}
   auto node=std::make_unique<Info>();order.emplace_back(std::string("constructor ")+table);
   // Original CacheResults completes BEFORE append. Failed stores survive in
   // pending_node, not as a manufactured initialized/null collection slot.
   pending_node=std::move(node);
   const auto name=std::string_view(table);
   if(name=="Colors")colors(*pending_node,row);else if(name=="Civilizations")civilization(*pending_node,row);
   else if(name=="GameSpeeds")speed(*pending_node,row);else throw std::runtime_error("Actual ordered Info CacheResults predecessor unavailable");
   collection.push_back(std::move(pending_node));order.emplace_back(std::string("append actual Info ")+table);current();return true;
  },content::InfoCollectionBudget{65536});current();need(result.all_succeeded(),"Native PrefetchCollection SelectWhere/Step failed");
  need(collection.size()==result.slots,"Actual owned Info collection span differs from executed append order");
 }
 std::unique_ptr<Info> pending_node;
};
BnwFreshCivilizationInfoCollection::BnwFreshCivilizationInfoCollection(std::shared_ptr<Impl> i):impl_(std::move(i)){}
BnwFreshCivilizationInfoCollection::~BnwFreshCivilizationInfoCollection(){impl_->retired.store(true);}
std::shared_ptr<BnwFreshCivilizationInfoCollection> BnwFreshCivilizationInfoCollection::from_same_pregame(
 const std::shared_ptr<BnwFreshPregameSpeed>& parent,const content::RulesDatabase& db,const content::InfoTypeCatalog& types){
 need(bool(parent),"Ordered Info SAME Pregame missing");parent->require_current();
 auto i=std::make_shared<Impl>();i->parent=parent;i->db=&db;i->types=&types;
 const std::weak_ptr<BnwFreshPregameSpeed> same=parent;
 i->localized=[same](std::string_view key){const auto retained=same.lock();need(bool(retained),"Ordered Info Locale original Pregame retired");
  return retained->localized_for_ordered_info(key);};
 auto out=std::shared_ptr<BnwFreshCivilizationInfoCollection>(new BnwFreshCivilizationInfoCollection(i));i->current();return out;
}
void BnwFreshCivilizationInfoCollection::advance_ordered_prefix(){
 const auto i=impl_;const auto parent=i->current();(void)parent;
 need(!i->busy&&!i->attempted,"Ordered Globals Info prefix cannot replay/reenter");i->attempted=true;i->busy=true;
 struct End{Impl& i;~End(){i.busy=false;}}end{*i};
 // First genuine queue node. PlayerColors additionally requires exact native
 // retained Results Bind/Step lifetime qualification, never guessed reset.
 try{
  // Actual CvDllDatabaseUtility::LoadGlobalDefines = GC.cacheGlobals. Reuse
  // established BNW typed source specifications, storing in native order.
  for(const auto& spec:content::global_define_specs(content::GlobalDefineProfile::bnw)){
   i->current();auto value=i->db->global_define(spec.name,spec.kind);i->current();
   need(value.error.empty(),"Mandatory full native cacheGlobals typed getter failed");
   if(spec.kind==content::GlobalDefineKind::integer){auto& number=std::get<std::int64_t>(value.value);(void)i32(number);if(spec.zero_is_none&&number==0)number=-1;}
   i->global_defines.emplace(std::string(spec.name),std::move(value.value));i->order.emplace_back(std::string("GC.cacheGlobals ")+std::string(spec.name));
  }
  i->order.emplace_back("actual LoadGlobalDefines return");
  i->populate(queue[0]);++i->cursor;i->order.emplace_back("mandatory PlayerColors actual retained ColorsLookup Results lifetime");
 }
 catch(...){i->retired.store(true);throw;}
 throw std::runtime_error("Ordered Globals requires next actual PlayerColors retained ColorsLookup source; Civilization/GameSpeed/global completion not issued");
}
BnwFreshOrderedInfoObservation BnwFreshCivilizationInfoCollection::observe()const{
 const auto i=impl_;const auto parent=i->current();(void)parent;need(!i->busy,"Ordered Info observation during active CacheResults");
 BnwFreshOrderedInfoObservation out;for(std::size_t n=0;n<i->cursor;++n)out.completed_tables.emplace_back(queue[n]);
 out.source_order=i->order;out.mandatory_next=i->cursor<std::size(queue)?queue[i->cursor]:"PrefetchGameData Flavors/CvTypes/actions/count-cache tail";
 const auto slots=[&](const char* table){const auto it=i->collections.find(table);return it==i->collections.end()?0ULL:static_cast<std::uint64_t>(it->second.size());};
 out.color_slots=slots("Colors");out.player_color_slots=slots("PlayerColors");out.civilization_slots=slots("Civilizations");i->current();return out;
}
}
