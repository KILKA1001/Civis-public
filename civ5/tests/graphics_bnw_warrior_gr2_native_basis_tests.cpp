#include "civ5/graphics/bnw_warrior_gr2_native_basis.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <regex>
#include <sstream>
#include <string>
using namespace civ5::graphics;
static void check(bool b) {if(!b)throw std::runtime_error("Warrior ArtTool basis check failed");}
static std::string field(const std::string &row,const std::string &key) {
    std::smatch m;check(std::regex_search(row,m,std::regex("\\\""+key+"\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"")));return m[1];
}
static std::array<float,3> vector(const std::string &row,const std::string &key) {
    std::smatch m;check(std::regex_search(row,m,std::regex("\\\""+key+"\\\"\\s*:\\s*\\[([^\\]]*)\\]")));
    std::string s=m[1];for(auto &c:s)if(c==',')c=' ';std::istringstream stream(s);std::array<float,3> v{};
    for(auto &x:v)check(bool(stream>>x));float extra;check(!(stream>>extra));return v;
}
int main(int argc,char **argv) {try {
    check(argc==2);std::ifstream input(argv[1],std::ios::binary);check(bool(input));
    const std::string json((std::istreambuf_iterator<char>(input)),{});
    check(field(json,"schema")=="CivisWarriorArtToolMetadataV1");
    const std::array<std::string,3> names={"warrior_v2","warrior","warrior_v4"};
    const std::array<std::string,3> source_sha={"6101693ea454cbdfe1a026903fd30ca6489596a462994eddc21465f8b3740272","9d34effba92d5dcc4340951bce65a08d620af2ee688cabf60d0bab8464ac0429","524fe9979ce361e8e486b49dff6fcc8e145dae268a26003aac0adb23bd52728d"};
    std::size_t cursor=0;BnwWarriorGr2ArtToolMetadata actual;
    for(std::size_t i=0;i<3;++i) {
        cursor=json.find("\"source\"",cursor);check(cursor!=std::string::npos);
        const auto end=json.find("\"source\"",cursor+8);const auto row=json.substr(cursor,end==std::string::npos?end:end-cursor);
        check(field(row,"source")==names[i] && field(row,"sourceGr2Sha256")==source_sha[i]);
        check(field(row,"tool")=="3D Studio MAX");std::smatch m;
        check(std::regex_search(row,m,std::regex("\\\"unitsPerMeter\\\"\\s*:\\s*([-+.eE0-9]+)")));
        actual={std::stof(m[1]),vector(row,"origin"),vector(row,"right"),vector(row,"up"),vector(row,"back")};
        const auto basis=compile_bnw_warrior_gr2_native_basis(actual,.14f);
        check(basis.uniform_scale==.14f && basis.linear==std::array<float,9>{.14f,0,0,0,.14f,0,0,0,.14f});
        check(compile_bnw_warrior_gr2_native_basis(actual,1).uniform_scale==1);
        check(compile_bnw_warrior_gr2_native_basis(actual,.28f).uniform_scale==.28f);
        cursor=end;
    }
    unsigned rejected=0;const auto reject=[&](auto f){try{f();}catch(const std::exception &){++rejected;}};
    for(unsigned field=0;field<5;++field) {auto bad=actual;if(field==0)bad.units_per_meter=0;if(field==1)bad.origin[0]=1;
        if(field==2)bad.right[0]=-1;if(field==3)bad.up[1]=1;if(field==4)bad.back[1]=1;
        reject([&]{compile_bnw_warrior_gr2_native_basis(bad,.14f);});}
    for(float scale:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::max()})
        reject([&]{compile_bnw_warrior_gr2_native_basis(actual,scale);});
    check(rejected==10);
    std::cout<<"PASS actual3 GR2 ArtTool bases: native axes identity, Float32 .14/.28 ratio, negative metadata/scales="<<rejected<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

