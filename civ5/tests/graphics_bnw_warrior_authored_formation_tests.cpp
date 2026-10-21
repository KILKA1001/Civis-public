#include "civ5/graphics/bnw_warrior_authored_formation.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
using namespace civ5::graphics;
void check(bool b,const char *s) {if(!b)throw std::runtime_error(s);}
bool near(float a,float b){return std::fabs(a-b)<0.00001F;}
int main(){
  {const auto f=bnw_warrior_authored_formation(0U,1.0F);
   const std::array<BnwWarriorFormationPoint,12> expected{{
     {4.531182765960693F,-15.315600395202637F},
     {-5.043120861053467F,-15.650912284851074F},
     {20.170841217041016F,-13.324707984924316F},
     {-20.940250396728516F,-12.024169921875F},
     {7.272101402282715F,-0.8624305725097656F},
     {-6.019694805145264F,1.058152198791504F},
     {19.68128204345703F,1.024301528930664F},
     {-20.167606353759766F,-0.06293487548828125F},
     {5.096536159515381F,13.942904472351074F},
     {-6.201017379760742F,13.206244468688965F},
     {20.940250396728516F,13.950797080993652F},
     {-18.01662826538086F,15.650912284851074F}
   }};
   for(std::size_t i=0;i<12;++i)check(near(f.members[i].x,expected[i].x)&&near(f.members[i].y,expected[i].y),"scalar fixture differs");
   check(f.final_graphics_rng_state==3845303128U&&f.random_draws==24,"graphic RNG differs");}
  {const auto f=bnw_warrior_authored_formation(7U,1.0F);
   const std::array<BnwWarriorFormationPoint,12> expected{{
     {3.765946388244629F,-12.244205474853516F},
     {-6.1921162605285645F,-14.983231544494629F},
     {19.827293395996094F,-11.204431533813477F},
     {-20.323959350585938F,-12.949382781982422F},
     {7.602109909057617F,-1.1387548446655273F},
     {-8.932107925415039F,-0.6267976760864258F},
     {20.43169403076172F,0.43191051483154297F},
     {-19.393516540527344F,-0.3035154342651367F},
     {5.8460516929626465F,12.105372428894043F},
     {-8.234254837036133F,13.6345853805542F},
     {16.925872802734375F,12.885014533996582F},
     {-20.43169403076172F,14.983231544494629F}
   }};
   for(std::size_t i=0;i<12;++i)check(near(f.members[i].x,expected[i].x)&&near(f.members[i].y,expected[i].y),"scalar fixture differs");
   check(f.final_graphics_rng_state==2768296319U&&f.random_draws==24,"graphic RNG differs");}
  const auto f=bnw_warrior_authored_formation(7,1);
  const auto again=bnw_warrior_authored_formation(7,1);
  for(std::size_t i=0;i<12;++i)check(f.members[i].x==again.members[i].x&&f.members[i].y==again.members[i].y,"repeat differs");
  const auto wide=bnw_warrior_authored_formation(7,2);
  for(std::size_t i=0;i<12;++i)check(near(wide.members[i].x,2*f.members[i].x),"scene spacing ignored");
  BnwWarriorWorldTransformInput input;input.center_x=300;input.center_y=-100;input.model_scale=.14F;
  for(std::size_t i=0;i<12;++i)input.member_height[i]=static_cast<float>(i)+4;
  const auto zero=bnw_warrior_render_transforms(f,input);
  for(std::size_t i=0;i<12;++i){check(near(zero[i][12],300+f.members[i].x)&&near(zero[i][13],-100+f.members[i].y),"zero rotation");check(zero[i][14]==input.member_height[i]&&zero[i][15]==1,"height/affine");check(zero[i][0]==.14F&&zero[i][5]==.14F&&zero[i][10]==.14F,"explicit scale");}
  input.heading_radians=1.5707963267948966F;
  const auto quarter=bnw_warrior_render_transforms(f,input);
  for(std::size_t i=0;i<12;++i)check(std::fabs(quarter[i][12]-(300-f.members[i].y))<0.0001F&&std::fabs(quarter[i][13]-(-100+f.members[i].x))<0.0001F,"Z rotation handedness");
  check(bnw_warrior_initial_heading(1)==1.5707963705062866F,"saved facing mapping");
  for(int direction=0;direction<6;++direction)check(std::isfinite(bnw_warrior_initial_heading(direction)),"valid facing");
  int rejects=0;
  auto rejects_call=[&](auto fn){try {fn();}catch(const std::invalid_argument &){++rejects;}};
  rejects_call([]{bnw_warrior_authored_formation(0xffffffffU,1);});
  rejects_call([]{bnw_warrior_authored_formation(7,0);});
  rejects_call([]{bnw_warrior_authored_formation(7,-1);});
  rejects_call([]{bnw_warrior_authored_formation(7,std::numeric_limits<float>::quiet_NaN());});
  rejects_call([]{bnw_warrior_authored_formation(7,101);});
  auto bad=input;bad.model_scale=0;rejects_call([&]{bnw_warrior_render_transforms(f,bad);});
  bad=input;bad.member_height[11]=std::numeric_limits<float>::infinity();rejects_call([&]{bnw_warrior_render_transforms(f,bad);});
  bad=input;bad.heading_radians=std::numeric_limits<float>::quiet_NaN();rejects_call([&]{bnw_warrior_render_transforms(f,bad);});
  auto badf=f;badf.members[0].x=std::numeric_limits<float>::infinity();rejects_call([&]{bnw_warrior_render_transforms(badf,input);});
  rejects_call([]{bnw_warrior_initial_heading(-1);});
  rejects_call([]{bnw_warrior_initial_heading(6);});
  check(rejects==11,"malformed input accepted");
  std::cout<<"Warrior authored formation: two 12-member fixtures, RNG, repeat, spacing, heading/height/scale and 11 negative cases PASS\n";
}
