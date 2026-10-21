#include "civ5/graphics/bnw_warrior_team_color_rules.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
using namespace civ5::graphics;
namespace content=civ5::content;namespace runtime=civ5::runtime;
static void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>static void rejects(F fn){bool bad=false;try{fn();}catch(const std::invalid_argument&){bad=true;}require(bad,"Missing rejection");}
static std::string read(const std::filesystem::path&p){std::ifstream in(p);if(!in)throw std::runtime_error("Missing actual installed XML");return {std::istreambuf_iterator<char>(in),{}};}
int main(int argc,char**argv){try{
 if(argc!=2)throw std::invalid_argument("Expected canonical AssetRoot");
 const std::filesystem::path root=std::filesystem::weakly_canonical(argv[1]);
 require(root==std::filesystem::weakly_canonical("C:/CrazyProject/civ5-assets"),"Foreign AssetRoot");
 auto players=read(root/"Assets/Gameplay/XML/Interface/CIV5PlayerColors.xml");
 auto colors=read(root/"Assets/Gameplay/XML/Interface/CIV5Colors.xml");
 auto row=players.find("<Type>PLAYERCOLOR_INDIA</Type>");require(row!=std::string::npos,"Missing India player color fixture");
 require(players.substr(row,players.find("</Row>",row)-row).find("<SecondaryColor>COLOR_PLAYER_INDIA_BACKGROUND</SecondaryColor>")!=std::string::npos,"India secondary source mismatch");
 row=colors.find("<Type>COLOR_PLAYER_INDIA_BACKGROUND</Type>");require(row!=std::string::npos,"Missing India secondary fixture");
 const auto text=colors.substr(row,colors.find("</Row>",row)-row);
 const auto channel=[&](const char*name){auto begin=text.find(std::string("<")+name+">");require(begin!=std::string::npos,"Missing actual color channel");begin+=std::string(name).size()+2;return std::stof(text.substr(begin));};
 BnwWarriorTeamColorSource s{0,16385,42,"PLAYERCOLOR_INDIA","COLOR_PLAYER_INDIA_BACKGROUND",{channel("Red"),channel("Green"),channel("Blue"),channel("Alpha")}};
 auto out=project_bnw_warrior_team_color(s,0,16385,42);require(out.packed_rgba==0x128706ff,"Actual India truncation/order mismatch");
 require(out.team_color_s[0]==18.f*std::bit_cast<float>(std::uint32_t{0x3b808081})&&out.team_color_s[1]==135.f*std::bit_cast<float>(std::uint32_t{0x3b808081})&&out.team_color_s[2]==6.f*std::bit_cast<float>(std::uint32_t{0x3b808081})&&out.team_color_s[3]==1.f,"Native unpack mismatch");
 require(pack_bnw_warrior_secondary_rgba({.5f,.5f,.5f,.5f})==0x7f7f7f7f,"Native uses truncation, not rounding");
 for(unsigned i=0;i<256;++i){auto v=unpack_bnw_warrior_team_color_s((i<<24)|(i<<16)|(i<<8)|i);require(pack_bnw_warrior_secondary_rgba(v)==((i<<24)|(i<<16)|(i<<8)|i),"Packed native channel roundtrip");}
 rejects([&]{project_bnw_warrior_team_color(s,1,16385,42);});rejects([&]{project_bnw_warrior_team_color(s,0,7,42);});rejects([&]{project_bnw_warrior_team_color(s,0,16385,43);});
 auto bad=s;bad.secondary_color_type.clear();rejects([&]{project_bnw_warrior_team_color(bad,0,16385,42);});bad=s;bad.selected_player_color_type.clear();rejects([&]{project_bnw_warrior_team_color(bad,0,16385,42);});
 bad=s;bad.secondary_rgba[0]=std::numeric_limits<float>::quiet_NaN();rejects([&]{project_bnw_warrior_team_color(bad,0,16385,42);});bad.secondary_rgba[0]=-1.f;rejects([&]{project_bnw_warrior_team_color(bad,0,16385,42);});bad.secondary_rgba[0]=1.01f;rejects([&]{project_bnw_warrior_team_color(bad,0,16385,42);});
 require(evaluate_bnw_warrior_code_alpha({10,10,{.2f,.8f,.5f}})==.5f,"Native water equality gate");
 require(evaluate_bnw_warrior_code_alpha({9,10,{.2f,.8f,.5f}})==.5f*std::bit_cast<float>(std::uint32_t{0x3ea8f5c3}),"Native below-water alpha");
 rejects([]{evaluate_bnw_warrior_code_alpha({std::numeric_limits<float>::quiet_NaN(),10,{1,1,1}});});rejects([]{evaluate_bnw_warrior_code_alpha({10,10,{1,1,2}});});
 content::RulesDatabase rules;
 rules.apply_sql("CREATE TABLE PlayerColors(ID INTEGER,Type TEXT,SecondaryColor TEXT);CREATE TABLE Colors(Type TEXT,Red REAL,Green REAL,Blue REAL,Alpha REAL)");
 std::vector<content::DatabaseStatement> inserts={
   {"INSERT INTO PlayerColors VALUES(?, ?, ?)",{std::int64_t{41},s.selected_player_color_type,s.secondary_color_type}},
   {"INSERT INTO Colors VALUES(?, ?, ?, ?, ?)",{s.secondary_color_type,double(s.secondary_rgba[0]),double(s.secondary_rgba[1]),double(s.secondary_rgba[2]),double(s.secondary_rgba[3])}}};
 rules.apply_statements(inserts);
 runtime::PregameRoster roster;roster.slots[0].player_color=41;roster.slots[0].player_color_key=s.selected_player_color_type;
 require(project_bnw_warrior_team_color_rules(roster,rules,0,16385,42).packed_rgba==out.packed_rgba,"Actual source-bound selected rules query");
 rules.apply_sql("INSERT INTO PlayerColors VALUES(99,'PLAYERCOLOR_CHANGED','COLOR_CHANGED');INSERT INTO Colors VALUES('COLOR_CHANGED',0.5,0.5,0.5,1)");
 roster.slots[0].player_color=99;roster.slots[0].player_color_key="PLAYERCOLOR_CHANGED";
 require(project_bnw_warrior_team_color_rules(roster,rules,0,16385,43).packed_rgba==0x7f7f7fff,"Live roster color must replace civilization default");
 roster.slots[0].player_color=41;rejects([&]{project_bnw_warrior_team_color_rules(roster,rules,0,16385,43);});
 roster.slots[0].player_color_key=s.selected_player_color_type;
 rejects([&]{project_bnw_warrior_team_color_rules(roster,rules,1,16385,43);});
 rules.apply_sql("DELETE FROM Colors WHERE Type='COLOR_PLAYER_INDIA_BACKGROUND'");rejects([&]{project_bnw_warrior_team_color_rules(roster,rules,0,16385,43);});
 rules.apply_sql("INSERT INTO Colors VALUES('COLOR_PLAYER_INDIA_BACKGROUND',NULL,0,0,1)");rejects([&]{project_bnw_warrior_team_color_rules(roster,rules,0,16385,43);});
 std::cout<<"PASS actual India secondary packed128706ff,256 channels,14 negatives,live roster rules; CODE alpha; samplers/native draw not accepted\n";return 0;
 }catch(const std::exception&e){std::cerr<<"BLOCKED: "<<e.what()<<'\n';return 1;}}
