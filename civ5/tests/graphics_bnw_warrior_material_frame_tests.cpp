#include "civ5/graphics/bnw_warrior_owned_material_uniforms.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <limits>
#include <string>
using namespace civ5::graphics;
static void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
static std::uint32_t read(std::istream& in,unsigned n=4){std::uint32_t v=0;for(unsigned i=0;i<n;++i){const int c=in.get();check(c!=EOF,"Truncated native alpha fixture");v|=std::uint32_t(c)<<(i*8);}return v;}
template<class F>static void rejects(F&& f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Invalid material owner input accepted");}
int main(int argc,char** argv){try{
 check(argc==2,"Expected native-alpha.bin");std::ifstream in(argv[1],std::ios::binary);check(bool(in),"Missing native alpha oracle");
 char magic[8];in.read(magic,8);check(std::string(magic,8)=="BNWALPH1","Native alpha fixture schema");
 auto cases=read(in),water=read(in);check(cases==74&&water==0x4121de22u,"Unexpected source oracle cardinality/WaterHeight");
 std::unique_ptr<BnwWarriorMaterialFrameOwner> owner;unsigned count=0;constexpr int unit=16385;constexpr std::uint64_t revision=73;
 std::array<std::uint32_t,32> packet_flags;
 for(unsigned row=0;row<cases;++row){
  std::array<std::uint32_t,8> op;for(auto& v:op)v=read(in);BnwWarriorAlphaEffects result;
  if(op[0]==0){count=op[1];owner=std::make_unique<BnwWarriorMaterialFrameOwner>(unit,revision,count);packet_flags.fill(0xffff);}
  else if(op[0]==1)result=owner->schedule(op[1],std::bit_cast<float>(op[2]),std::bit_cast<float>(op[3]),std::bit_cast<float>(op[4]),std::bit_cast<float>(op[5]),std::uint16_t(op[6]),std::uint8_t(op[7]),unit,revision);
  else if(op[0]==2)result=owner->tick(std::bit_cast<float>(op[1]),unit,revision);
  else if(op[0]==3){const std::array<std::uint8_t,3> states{std::uint8_t(op[2]),std::uint8_t(op[3]),std::uint8_t(op[4])};result=owner->visibility_frame(states,std::bit_cast<float>(op[1]),unit,revision);}
  else throw std::runtime_error("Unknown native fixture operation");
  const auto flags=read(in),packets=read(in),flush=read(in),visibility=read(in),refresh=read(in);
  check(refresh==(result.refresh_packet_time?std::bit_cast<std::uint32_t>(*result.refresh_packet_time):0xffffffffu),"Native packet refresh time differs");
  if(result.refresh_packet_time)for(unsigned i=0;i<count;++i)if(owner->packet_transition_window(i,*result.refresh_packet_time,unit,revision))packet_flags[i]=2;
  check(flags==owner->flags()&&packets==result.packet_updates.size()&&bool(flush)==result.flush_visibility&&visibility==result.visibility_updates.size(),"Native alpha effects differ");
  for(unsigned i=0;i<32;++i){auto expected=read(in);if(i<packets)check(expected==result.packet_updates[i],"Native packet update differs");}
  for(unsigned i=0;i<32;++i){auto expected=read(in);if(i<visibility)check(expected==result.visibility_updates[i],"Native visibility update differs");}
  for(unsigned i=0;i<32;++i){
   std::array<std::uint32_t,3> alpha;for(auto& v:alpha)v=read(in);
   const auto start=read(in),duration=read(in),kind=read(in,2),pending=read(in,1),current=read(in,1),vis=read(in,1);
   if(i<count){const auto& s=owner->slot(i,unit,revision);
    for(unsigned a=0;a<3;++a)check(alpha[a]==std::bit_cast<std::uint32_t>(s.alpha[a]),"Native alpha SSE float bits differ");
    check(start==std::bit_cast<std::uint32_t>(s.start)&&duration==std::bit_cast<std::uint32_t>(s.duration)&&kind==s.kind&&pending==s.pending&&current==s.current&&vis==s.visibility,"Native transition state differs");
   }
  }
  for(unsigned i=0;i<32;++i)check(read(in,2)==packet_flags[i],"Native packet alpha family transition differs");
  check(std::bit_cast<std::uint32_t>(owner->water_height(unit,revision))==water,"Native WaterHeight frame differs");
 }
 check(in.peek()==EOF,"Unexpected native fixture trailing data");
 // Native visibility event scalar branches use the retained fresh fade duration.
 BnwWarriorMaterialFrameOwner event(unit,revision,3);
 auto hide=event.visibility(0,0,2,unit,revision);check(hide.flush_visibility&&event.slot(0,unit,revision).current==0,"Visibility hide event");
 event.tick(2.25f,unit,revision);check(event.slot(0,unit,revision).alpha[2]==.5f,"Hide midpoint");
 auto above=bnw_warrior_code_alpha_from_owned_frame(event,unit,revision,0,12.f);
 auto below=bnw_warrior_code_alpha_from_owned_frame(event,unit,revision,0,5.f);
 check(evaluate_bnw_warrior_code_alpha(above)==.5f&&evaluate_bnw_warrior_code_alpha(below)==.5f*std::bit_cast<float>(0x3ea8f5c3u),"Owned frame CODE PS water/alpha consumer");
 auto show=event.visibility(0,3,2.3f,unit,revision);check(show.flush_visibility&&event.slot(0,unit,revision).visibility==3,"Visibility show interrupt");
 event.tick(2.8f,unit,revision);check(event.slot(0,unit,revision).kind==0&&event.slot(0,unit,revision).alpha[2]==1.f,"Visibility show completion");
 rejects([&]{event.water_height(unit+1,revision);});rejects([&]{event.tick(0,unit,revision+1);});
 rejects([&]{event.slot(3,unit,revision);});rejects([&]{event.tick(std::numeric_limits<float>::quiet_NaN(),unit,revision);});
 rejects([&]{event.schedule(0,1,0,0,std::numeric_limits<float>::infinity(),1,0,unit,revision);});
 rejects([&]{event.schedule(0,1,0,0,.5f,3,0,unit,revision);});
 rejects([&]{BnwWarriorMaterialFrameOwner x(unit,revision,33);});rejects([&]{BnwWarriorMaterialFrameOwner x(unit,0,3);});
 rejects([&]{bnw_warrior_code_alpha_from_owned_frame(event,unit+1,revision,0,12.f);});
 rejects([&]{bnw_warrior_code_alpha_from_owned_frame(event,unit,revision,0,std::numeric_limits<float>::infinity());});
 rejects([&]{event.visibility_frame(std::array<std::uint8_t,1>{0},0,unit,revision);});
 std::cout<<"PASS WaterHeight source bits; "<<cases<<" native alpha differential states/effects; visibility branches; owned CODE PS consumer; 11 invalid owner/input controls\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
