#include "bnw_warrior_gr2_metadata_cache.hpp"
#include <fstream>
#include <iostream>
using namespace civ5::graphics;
namespace fs=std::filesystem;
static void check(bool b) {if(!b)throw std::runtime_error("Warrior metadata runtime cache check failed");}
static void write(const fs::path &p,std::string_view bytes) {std::ofstream f(p,std::ios::binary);f.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));check(bool(f));}
int main(int argc,char **argv) {try {
    check(argc==3);const fs::path cache=argv[1],out=argv[2];fs::create_directories(out);
    const auto source=load_bnw_warrior_gr2_metadata_cache(cache);
    check(source.metadata_sha256==bnw_warrior_arttool_metadata_sha256);
    check(source.native_model_scale(.14f)==.14f && source.native_model_scale(.28f)==.28f);
    unsigned rejected=0;const auto reject=[&](auto f){try{f();}catch(const std::exception &){++rejected;}};
    const auto actual=cache/"warrior-arttool-metadata.json";
    write(out/"malformed.json","{broken");reject([&]{load_bnw_warrior_gr2_metadata_cache(cache,out/"malformed.json");});
    reject([&]{load_bnw_warrior_gr2_metadata_cache(cache,out/"missing.json");});
    auto changed=warrior_read_bounded(actual,20'000);const auto at=changed.find("39.37008");check(at!=std::string::npos);
    changed.replace(at,8,"0.000000");write(out/"unknown-units.json",changed);
    reject([&]{load_bnw_warrior_gr2_metadata_cache(cache,out/"unknown-units.json");});
    const auto empty=out/"missing-source-cache";fs::create_directories(empty);
    reject([&]{load_bnw_warrior_gr2_metadata_cache(empty,actual);});
    const auto bad=out/"mismatched-vertex-cache";fs::create_directories(bad);
    const auto source_gr2=cache/"warrior_v2.gr2";
    if(!fs::exists(bad/"warrior_v2.gr2"))fs::create_hard_link(source_gr2,bad/"warrior_v2.gr2");
    write(bad/"warrior_v2.mesh.json","{\"sourceGr2Sha256\": \"6101693ea454cbdfe1a026903fd30ca6489596a462994eddc21465f8b3740272\", \"rawVertexBytesSha256\": \"0000000000000000000000000000000000000000000000000000000000000000\"}");
    reject([&]{load_bnw_warrior_gr2_metadata_cache(bad,actual);});
    const auto broken=out/"tampered-gr2-cache";fs::create_directories(broken);write(broken/"warrior_v2.gr2","broken");
    reject([&]{load_bnw_warrior_gr2_metadata_cache(broken,actual);});
    check(rejected==6);
    std::cout<<"PASS actual3 metadata runtime cache and6 malformed metadata/source fixtures; native scalar applied by matrix owner only\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
