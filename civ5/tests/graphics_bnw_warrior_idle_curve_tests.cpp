#include "civ5/graphics/bnw_warrior_idle_curve.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
using namespace civ5::graphics;
struct Reader {
    std::vector<std::uint8_t> bytes;std::size_t at=0;
    std::uint32_t word(){const auto value=warrior_curve_word(bytes,at);at+=4;return value;}
    std::vector<std::uint8_t> take(std::size_t n){warrior_curve_require(n<=bytes.size()-at,"Idle fixture truncated");std::vector<std::uint8_t> out(bytes.begin()+std::ptrdiff_t(at),bytes.begin()+std::ptrdiff_t(at+n));at+=n;return out;}
    std::string name(){const auto n=word();warrior_curve_require(n>0&&n<=96,"Idle fixture name extent");const auto b=take(n);return std::string(b.begin(),b.end());}
    std::vector<std::uint32_t> words(unsigned n){std::vector<std::uint32_t> out;for(unsigned i=0;i<n;++i)out.push_back(word());return out;}
};
struct Expected {float clock;std::vector<std::uint32_t> raw,normalized;};
static void equal(std::span<const float> actual,std::span<const std::uint32_t> expected,const char* why){warrior_curve_require(actual.size()==expected.size(),why);for(std::size_t i=0;i<actual.size();++i)warrior_curve_require(std::bit_cast<std::uint32_t>(actual[i])==expected[i],why);}
template<class F>void refuse(F f,unsigned& count){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}warrior_curve_require(rejected,"Unsupported IdleA source accepted");++count;}
int main(int argc,char**argv){try {
    warrior_curve_require(argc==2,"Usage: idle_curve SOURCE_NATIVE_CURVE_FIXTURE");std::ifstream file(argv[1],std::ios::binary);warrior_curve_require(bool(file),"Actual Idle curve fixture missing");Reader r{{std::istreambuf_iterator<char>(file),{}}};const auto magic=r.take(8);warrior_curve_require(std::string(magic.begin(),magic.end())=="BNWCVF01"&&r.word()==23&&r.word()==69,"Actual Idle curve fixture schema/cardinality");
    // Source identity bytes, also pinned independently by the replay audit.
    const auto identities=r.take(96);constexpr std::string_view expected="7a6bde9dbb15f0916a920119774c1e803c92677660119c69c5d0bd32bee0cd88f063955e71728fc76fdc097b9759e291f203c895d32f5f6eeaf5e4f78f73a95975f342eff9db2a866837d7aad7486cabdc10e2ad11866ceb38c5c050e3ec04f7";
    constexpr char digits[]="0123456789abcdef";std::string hex;for(auto x:identities){hex+=digits[x>>4];hex+=digits[x&15];}warrior_curve_require(hex==expected,"Foreign Idle/GSD/SDK fixture identity");
    std::array<std::string,23> skeleton;std::array<std::string_view,23> names;for(auto& name:skeleton)name=r.name();for(std::size_t i=0;i<23;++i)names[i]=skeleton[i];const auto palette=r.words(20);for(auto bone:palette)warrior_curve_require(bone<23,"Actual reused mesh palette extent");
    std::vector<BnwWarriorIdleTrack> tracks;std::vector<std::array<std::vector<Expected>,3>> samples;std::vector<BnwWarriorIdleCurveData> sources;unsigned components=0,queries=0;
    for(unsigned t=0;t<23;++t) {
        BnwWarriorIdleTrack track;track.name=r.name();std::array<std::vector<Expected>,3> expected_samples;
        for(unsigned channel=0;channel<3;++channel) {
            BnwWarriorIdleCurveData source{r.word(),r.word(),r.word(),r.word(),{}, {}};const auto header=r.word(),payload=r.word();warrior_curve_require(header<=36&&payload<16384,"Actual source header/payload extent");source.header=r.take(header);source.payload=r.take(payload);sources.push_back(source);
            auto decoded=decode_bnw_warrior_idle_curve(source);const auto knots=r.words(r.word()),controls=r.words(r.word());equal(decoded.knots,knots,"Native CPU knot decode mismatch");equal(decoded.controls,controls,"Native CPU control decode mismatch");components+=unsigned(knots.size()+controls.size());
            warrior_curve_require(r.word()==16,"Independent SDK/native source sample count");
            for(unsigned sample=0;sample<16;++sample) {
                Expected e{std::bit_cast<float>(r.word()),r.words(source.dimension),r.words(source.dimension)};const auto raw=sample_bnw_warrior_idle_curve(decoded,e.clock,std::bit_cast<float>(0x40c00001u),false,false,false);equal(raw,e.raw,"Native CPU raw degree2 sample mismatch");const auto normalized=sample_bnw_warrior_idle_curve(decoded,e.clock,std::bit_cast<float>(0x40c00001u),channel==1,false,false);equal(normalized,e.normalized,"Native CPU normalization sample mismatch");queries+=2;expected_samples[channel].push_back(e);
            }
            track.curves[channel]=std::move(decoded);
        }
        tracks.push_back(std::move(track));samples.push_back(std::move(expected_samples));
    }
    warrior_curve_require(r.at==r.bytes.size(),"Idle fixture trailing data");unsigned local_transforms=0,palette_checks=0;
    for(unsigned sample=0;sample<16;++sample) {
        const float clock=samples[0][0][sample].clock;const BnwWarriorPoseAnimTimeInput controlled{clock,0,1,std::bit_cast<float>(0x40c00001u),1,0,1};
        const auto pose=sample_bnw_warrior_idle_local_pose(tracks,names,controlled);
        for(unsigned bone=0;bone<23;++bone) {
            const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto& tr){return tr.name==names[bone];});const auto index=std::size_t(std::distance(tracks.begin(),track));std::size_t at=0;std::uint32_t flags=0;
            for(unsigned channel=0;channel<3;++channel) {
                const auto& values=samples[index][channel][sample].normalized;
                equal(std::span(pose[bone].position_orientation_scale).subspan(at,values.size()),values,"Native local transform/skeleton binding mismatch");at+=values.size();if(track->curves[channel].format!=2)flags|=1u<<channel;
            }
            warrior_curve_require(pose[bone].flags==flags,"Source local transform identity flags differ");++local_transforms;
        }
        for(auto bone:palette){warrior_curve_require(pose[bone].position_orientation_scale[7]==1&&pose[bone].position_orientation_scale[11]==1&&pose[bone].position_orientation_scale[15]==1,"Reused actual palette scale-shear identity changed");++palette_checks;}
    }
    unsigned negative=0;const auto animated=std::find_if(sources.begin(),sources.end(),[](const auto& c){return c.format==9;});warrior_curve_require(animated!=sources.end(),"Actual quaternion9 test source missing");const auto decoded=decode_bnw_warrior_idle_curve(*animated);const float duration=std::bit_cast<float>(0x40c00001u);
    for(auto t:{-.25f,duration+1,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})refuse([&]{sample_bnw_warrior_idle_curve(decoded,t,duration,true,false,false);},negative);
    refuse([&]{sample_bnw_warrior_idle_curve(decoded,1,6,true,false,false);},negative);
    for(unsigned policy=1;policy<4;++policy)refuse([&]{sample_bnw_warrior_idle_curve(decoded,1,duration,true,(policy&1)!=0,(policy&2)!=0);},negative);
    auto bad=*animated;bad.format=255;refuse([&]{decode_bnw_warrior_idle_curve(bad);},negative);bad=*animated;bad.degree=1;refuse([&]{decode_bnw_warrior_idle_curve(bad);},negative);bad=*animated;bad.dimension=3;refuse([&]{decode_bnw_warrior_idle_curve(bad);},negative);bad=*animated;bad.header.pop_back();refuse([&]{decode_bnw_warrior_idle_curve(bad);},negative);bad=*animated;bad.payload.pop_back();refuse([&]{decode_bnw_warrior_idle_curve(bad);},negative);bad=*animated;bad.header[12]=1;refuse([&]{decode_bnw_warrior_idle_curve(bad);},negative);bad=*animated;bad.header[8]++;refuse([&]{decode_bnw_warrior_idle_curve(bad);},negative);bad=*animated;bad.payload[0]=255;refuse([&]{decode_bnw_warrior_idle_curve(bad);},negative);
    BnwWarriorPoseAnimTimeInput input{1,0,1,duration,0,0,1};auto wrong_names=names;wrong_names[0]=wrong_names[1];refuse([&]{sample_bnw_warrior_idle_local_pose(tracks,wrong_names,input);},negative);wrong_names=names;wrong_names[0]="foreign-bone";refuse([&]{sample_bnw_warrior_idle_local_pose(tracks,wrong_names,input);},negative);auto wrong_tracks=tracks;wrong_tracks[0].name=wrong_tracks[1].name;refuse([&]{sample_bnw_warrior_idle_local_pose(wrong_tracks,names,input);},negative);refuse([&]{sample_bnw_warrior_idle_local_pose(tracks,std::span(names).first(22),input);},negative);input.clamped_loop=0;refuse([&]{sample_bnw_warrior_idle_local_pose(tracks,names,input);},negative);
    std::cout<<"PASS "<<components<<" exact decoded components, "<<queries<<" source sample comparisons, "<<local_transforms<<" local23 transforms/"<<palette_checks<<" reused palette references, "<<negative<<" refusals; looping/FSM/advance/BoneMatrices remain blocked\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
