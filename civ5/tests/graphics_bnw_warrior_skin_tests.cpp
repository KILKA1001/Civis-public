#include "../validation/bnw_warrior_skin_diagnostic.hpp"
#include <iostream>

namespace {
void require(bool ok,const char *why) { if(!ok) throw std::runtime_error(why); }
double position_delta(const std::vector<float> &a,const std::vector<float> &b) {
    double worst=0;
    for(std::size_t v=0;v<a.size()/8;++v) {
        double d=0;
        for(std::size_t j=0;j<3;++j) { const double x=a[v*8+j]-b[v*8+j]; d+=x*x; }
        worst=std::max(worst,std::sqrt(d));
    }
    return worst;
}
}
int main(int argc,char **argv) {
    try {
        if(argc!=2) throw std::runtime_error("Usage: graphics_bnw_warrior_skin_tests LOCAL_SDK_CACHE");
        const std::filesystem::path cache=argv[1];
        const std::array<std::size_t,3> counts{829,863,796},triangles{654,674,638};
        double bind_error=0,bind_delta=0,idle_delta=0;
        for(std::size_t member=0;member<3;++member) {
            const auto &spec=civ5::graphics::bnw_gandhi_warrior_art[member];
            const auto mesh=civ5::graphics::load_bnw_warrior_source_mesh(cache,spec,counts[member],triangles[member]);
            const auto skin=civ5::graphics::load_bnw_warrior_skin_diagnostic(cache,spec,counts[member]);
            require(skin.palette==std::vector<int>{3,11,14,22,12,13,21,4,5,15,16,17,18,7,8,9,10,19,20,1},"Authored bone palette differs");
            bind_error=std::max(bind_error,skin.bind_matrix_error());
            bind_delta=std::max(bind_delta,position_delta(mesh.vertex_pnuv,skin.pose(mesh)));
            const auto zero=skin.pose(mesh,0);
            for(int sample=0;sample<4;++sample) {
                const auto posed=skin.pose(mesh,sample);
                idle_delta=std::max(idle_delta,position_delta(zero,posed));
                for(std::size_t v=0;v<counts[member];++v) {
                    require(posed[v*8+6]==mesh.vertex_pnuv[v*8+6] && posed[v*8+7]==mesh.vertex_pnuv[v*8+7],"Skinning changed UV");
                    double n=0; for(std::size_t j=0;j<3;++j) {
                        require(std::isfinite(posed[v*8+j]) && std::abs(posed[v*8+j])<250,"Idle vertex outside source world extent");
                        const double x=posed[v*8+3+j]; n+=x*x;
                    }
                    require(std::abs(n-1)<1e-5,"Posed normal is not unit");
                }
            }
            bool rejected=false; try { (void)skin.pose(mesh,4); } catch(const std::exception &) { rejected=true; }
            require(rejected,"Unexported SDK time was accepted");
        }
        require(bind_error<0.001 && bind_delta<0.001,"Bind pose did not recover source vertices");
        require(idle_delta>0.1 && idle_delta<20,"Idle SDK samples are constant or implausible");
        const auto tamper=cache/"skin-test-tamper"; std::filesystem::create_directories(tamper);
        const auto file=cache/"warrior_v2.skin-flat.json";
        auto bytes=civ5::graphics::warrior_read_bounded(file,1'000'000); bytes[0]='[';
        { std::ofstream out(tamper/file.filename(),std::ios::binary); out<<bytes; require(bool(out),"Tamper fixture write failed"); }
        bool rejected=false;
        try { (void)civ5::graphics::load_bnw_warrior_skin_diagnostic(tamper,civ5::graphics::bnw_gandhi_warrior_art[0],829); }
        catch(const std::exception &) { rejected=true; }
        require(rejected,"Tampered skin cache was accepted");
        std::cout<<"{\"status\":\"PASS\",\"bindMatrixError\":"<<bind_error
                 <<",\"bindVertexDelta\":"<<bind_delta<<",\"idleVertexDelta\":"<<idle_delta
                 <<",\"sdkSampleTimes\":[0,1,3,5],\"modelDrawable\":false}\n";
        return 0;
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
