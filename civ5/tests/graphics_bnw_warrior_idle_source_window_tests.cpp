#include "civ5/graphics/bnw_warrior_idle_source_window.hpp"
#include <fstream>
#include <iostream>
#include <limits>
using namespace civ5::graphics;
struct Reader {
    std::vector<std::uint8_t> bytes;std::size_t at=0;
    std::uint32_t word(){const auto value=warrior_curve_word(bytes,at);at+=4;return value;}
    std::vector<std::uint8_t> take(std::size_t n){warrior_curve_require(at<=bytes.size()&&n<=bytes.size()-at,"Source window fixture truncated");std::vector<std::uint8_t> out(bytes.begin()+std::ptrdiff_t(at),bytes.begin()+std::ptrdiff_t(at+n));at+=n;return out;}
    float value(){return std::bit_cast<float>(word());}
};
static void equal(std::span<const float> values,const std::vector<std::uint32_t>& words,const char* error){warrior_curve_require(values.size()==words.size(),error);for(std::size_t i=0;i<words.size();++i)warrior_curve_require(std::bit_cast<std::uint32_t>(values[i])==words[i],error);}
template<class F>void refuse(F fn,unsigned& count){bool rejected=false;try{fn();}catch(const std::invalid_argument&){rejected=true;}warrior_curve_require(rejected,"Missing source window guard");++count;}
int main(int argc,char**argv){try {
    warrior_curve_require(argc==2,"Usage: source_window NATIVE_WINDOW_FIXTURE");std::ifstream file(argv[1],std::ios::binary);warrior_curve_require(bool(file),"Source window fixture missing");Reader r{{std::istreambuf_iterator<char>(file),{}}};const auto magic=r.take(8);warrior_curve_require(std::string(magic.begin(),magic.end())=="BNWWIN01"&&r.word()==13,"Source window fixture identity/count");
    const auto identity=r.take(64);constexpr std::string_view expected="7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686b0330e1ebf35834d2997f6c702efcf109af34e33f935337acd60300d07ad4409";constexpr char digits[]="0123456789abcdef";std::string hex;for(auto b:identity){hex+=digits[b>>4];hex+=digits[b&15];}warrior_curve_require(hex==expected,"Foreign source EXE/cache fixture");
    unsigned queries=0,clamped=0,policies[4]{},copied[2]{};BnwWarriorIdleSourceWindow accepted{};
    for(unsigned curve=0;curve<13;++curve) {
        const auto fmt=r.word(),dim=r.word(),n=r.word(),hlen=r.word(),plen=r.word(),samples=r.word();warrior_curve_require(hlen<=36&&plen<16384&&samples==64,"Source window curve fixture extent");BnwWarriorIdleCurveData source{fmt,2,dim,n,r.take(hlen),r.take(plen)};const auto decoded=decode_bnw_warrior_idle_curve(source);
        for(unsigned sample=0;sample<samples;++sample) {
            const float clock=r.value();const auto policy=r.word(),index=r.word(),flag=r.word(),ko=r.word(),co=r.word();warrior_curve_require(policy<4&&flag<=1,"Source window controlled policy/flag");
            BnwWarriorIdleSourceWindow window{fmt,dim,n,index,flag,clock,std::bit_cast<float>(0x40c00001u),(policy&1)!=0,(policy&2)!=0,ko,co,{}, {}};for(auto& value:window.knots)value=r.value();for(unsigned j=0;j<dim*4;++j)window.controls.push_back(r.value());std::vector<std::uint32_t> raw,normal;for(unsigned j=0;j<dim;++j)raw.push_back(r.word());for(unsigned j=0;j<dim;++j)normal.push_back(r.word());
            equal(sample_bnw_warrior_source_idle_window(window,false),raw,"Source9B2410 raw interpolation mismatch");equal(sample_bnw_warrior_source_idle_window(window,true),normal,"Source9B2410 gated normalization mismatch");queries+=2;++policies[policy];++copied[flag];
            if(policy==0){equal(sample_bnw_warrior_idle_curve(decoded,clock,window.duration,false,false,false),raw,"Reconstructed clamped raw versus actual source window mismatch");equal(sample_bnw_warrior_idle_curve(decoded,clock,window.duration,true,false,false),normal,"Reconstructed clamped normalized versus actual source window mismatch");clamped+=2;}accepted=window;
        }
    }
    warrior_curve_require(r.at==r.bytes.size()&&copied[0]==292&&copied[1]==540&&std::ranges::all_of(policies,[](auto count){return count==208;}),"Source window policy coverage/trailing data");
    unsigned negative=0;auto check=[&](BnwWarriorIdleSourceWindow w){refuse([&]{sample_bnw_warrior_source_idle_window(w,true);},negative);};auto w=accepted;w.format=255;check(w);w=accepted;w.dimension=9;check(w);w=accepted;w.knot_count=2;check(w);w=accepted;w.source_index=w.knot_count;check(w);w=accepted;w.copied_flag=2;check(w);w=accepted;w.duration=6;check(w);w=accepted;w.local_clock=-1;check(w);w=accepted;w.local_clock=w.duration+1;check(w);w=accepted;w.local_clock=std::numeric_limits<float>::infinity();check(w);w=accepted;w.out_knots_byte_offset=0;check(w);w=accepted;w.out_controls_byte_offset=0;check(w);w=accepted;w.controls.pop_back();check(w);w=accepted;w.controls[0]=std::numeric_limits<float>::quiet_NaN();check(w);w=accepted;w.knots[0]=std::numeric_limits<float>::infinity();check(w);w=accepted;w.knots[2]=w.knots[1];check(w);
    std::cout<<"PASS "<<queries<<" source-window samples, "<<clamped<<" clamped comparisons, 4 controlled policies/AL0+AL1, "<<negative<<" refusals; producer/controller/BoneMatrices remain separate\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
