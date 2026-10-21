#include "civ5/terrain/scene_direction.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
using namespace civ5::terrain;
namespace {
unsigned checks=0;
void check(bool value,const char *message) {++checks;if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F f) {bool caught=false;try{f();}catch(const std::invalid_argument &){caught=true;}check(caught,"Invalid scene direction accepted");}
struct Reader {
    std::ifstream in;
    explicit Reader(const char *path):in(path,std::ios::binary){
        char magic[8]{};check(bool(in.read(magic,8)) && std::string_view(magic,8)=="C5SDF002","Invalid direction oracle");
    }
    std::uint32_t word(){std::uint32_t v=0;for(unsigned i=0;i<4;++i){const auto c=in.get();check(c!=EOF,"Truncated oracle");v|=std::uint32_t(c)<<(i*8);}return v;}
    float real(){return std::bit_cast<float>(word());}
    Point2 point(){const auto x=real(),y=real();return {x,y};}
    X87Precision precision(){const auto pc=word();check(pc==24||pc==53,"Invalid oracle precision");return pc==24?X87Precision::single:X87Precision::double_precision;}
};
void oracle(const char *path) {
    Reader r(path);const auto angles=r.word(),noise=r.word(),directions=r.word();
    check(angles==2 && noise<=100000 && directions<=100000,"Excessive oracle cases");
    for(unsigned i=0;i<angles;++i){const auto pc=r.precision();const auto expected=r.word();
        check(std::bit_cast<std::uint32_t>(initial_scene_direction_angle(pc))==expected,"Original initial angle differs");}
    for(unsigned i=0;i<noise;++i){
        const auto pc=r.precision();const auto p=r.point();const auto low=r.word(),high=r.word();
        const auto expected=std::uint64_t(high)<<32|low;
        const auto actual=std::bit_cast<std::uint64_t>(scene_direction_noise(p,pc));
        if(actual!=expected)throw std::runtime_error("Original noise differs at case "+std::to_string(i)+" actual="+std::to_string(actual)+" expected="+std::to_string(expected));
    }
    for(unsigned i=0;i<directions;++i){
        const auto pc=r.precision();const auto p=r.point();const auto angle=r.real();const auto offset=r.point();const auto expected=r.point();
        const auto actual=SceneDirectionField{angle,offset}.direction(p,pc);
        if(std::bit_cast<std::uint32_t>(actual.x)!=std::bit_cast<std::uint32_t>(expected.x)||
           std::bit_cast<std::uint32_t>(actual.y)!=std::bit_cast<std::uint32_t>(expected.y))
            throw std::runtime_error("Original direction differs at case "+std::to_string(i));
    }
    check(r.in.get()==EOF,"Oracle trailing data");
    std::cout<<"Original initial angles="<<angles<<"; noise="<<noise<<"; direction="<<directions<<"; exact bytes="<<(noise+directions)*8+angles*4<<'\n';
}
}
int main(int argc,char **argv) {
    try {
        check(argc<=2,"Usage: civ5_scene_direction_tests [oracle.bin]");
        const auto pc=X87Precision::double_precision;
        check(scene_direction_noise({0,0},pc)==0,"Noise origin");
        check(SceneDirectionField{0,{0,0}}.direction({0,0},pc)==Point2(64,0),"Direction origin");
        rejects([&]{scene_direction_noise({-1,-1},pc);});
        rejects([&]{scene_direction_noise({65537,0},pc);});
        rejects([&]{scene_direction_noise({std::numeric_limits<float>::quiet_NaN(),0},pc);});
        rejects([&]{scene_direction_noise({0,0},static_cast<X87Precision>(3));});
        rejects([&]{SceneDirectionField{std::numeric_limits<float>::infinity(),{0,0}}.direction({0,0},pc);});
        rejects([&]{SceneDirectionField{0,{0,0}}.direction({-8192,-8192},pc);});
        if(argc==2)oracle(argv[1]);
        std::cout<<"PASS scene direction checks="<<checks<<'\n';return 0;
    }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
