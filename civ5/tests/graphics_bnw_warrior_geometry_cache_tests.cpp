#include "../validation/bnw_warrior_geometry_cache.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {
void require(bool yes,const char *message) {
    if(!yes) throw std::runtime_error(message);
}
template<class F> void rejects(F &&run,const char *message) {
    bool rejected=false;
    try { run(); }
    catch(const std::exception &) { rejected=true; }
    require(rejected,message);
}
}

int main(int argc,char **argv) {
    if(argc!=3) throw std::runtime_error(
        "Usage: graphics_bnw_warrior_geometry_cache_tests CACHE_DIR TEMP_DIR");
    const std::filesystem::path cache=argv[1],scratch=argv[2];
    constexpr std::array<std::size_t,3> vertices{829,863,796};
    constexpr std::array<std::size_t,3> triangles{654,674,638};
    std::size_t actual_vertices=0,actual_triangles=0;
    for(std::size_t i=0;i<3;++i) {
        const auto mesh=civ5::graphics::load_bnw_warrior_source_mesh(
            cache,civ5::graphics::bnw_gandhi_warrior_art[i],vertices[i],triangles[i]);
        actual_vertices+=mesh.vertex_pnuv.size()/8;
        actual_triangles+=mesh.indices.size()/3;
    }
    require(actual_vertices==2488 && actual_triangles==1966,
            "SDK Warrior source geometry shape differs");
    auto wrong_source=civ5::graphics::bnw_gandhi_warrior_art[1];
    wrong_source.gr2_sha256="incorrect-source-sha";
    rejects([&]{ (void)civ5::graphics::load_bnw_warrior_source_mesh(
                cache,wrong_source,863,674); },"Wrong source GR2 SHA accepted");
    rejects([&]{ (void)civ5::graphics::load_bnw_warrior_source_mesh(
                cache,civ5::graphics::bnw_gandhi_warrior_art[1],864,674); },
            "Wrong geometry vertex count accepted");
    std::filesystem::create_directories(scratch);
    for(const auto *name:{"warrior.gr2","warrior.g3dj","warrior.mesh.json"})
        std::filesystem::copy_file(cache/name,scratch/name,
                                   std::filesystem::copy_options::overwrite_existing);
    {
        std::fstream modified(scratch/"warrior.g3dj",
                              std::ios::in|std::ios::out|std::ios::binary);
        require(static_cast<bool>(modified),"Cannot open local geometry cache negative copy");
        modified.put('X');
    }
    rejects([&]{ (void)civ5::graphics::load_bnw_warrior_source_mesh(
                scratch,civ5::graphics::bnw_gandhi_warrior_art[1],863,674); },
            "Tampered local SDK geometry cache accepted");
    for(const auto *name:{"warrior.gr2","warrior.g3dj","warrior.mesh.json"})
        std::filesystem::remove(scratch/name);
    std::filesystem::remove(scratch);
}
