#pragma once
#include "bnw_warrior_geometry_cache.hpp"
#include <array>
#include <algorithm>

namespace civ5::graphics {
using WarriorMatrix=std::array<double,16>; // Row-major mathematical matrices, column vectors.
inline WarriorMatrix warrior_identity() {
    WarriorMatrix m{}; for(int i=0;i<4;++i) m[i*4+i]=1; return m;
}
inline WarriorMatrix warrior_multiply(const WarriorMatrix &a,const WarriorMatrix &b) {
    WarriorMatrix m{};
    for(int i=0;i<4;++i) for(int j=0;j<4;++j) for(int k=0;k<4;++k)
        m[i*4+j]+=a[i*4+k]*b[k*4+j];
    return m;
}
inline WarriorMatrix warrior_local(const double *p) {
    double x=p[3],y=p[4],z=p[5],w=p[6];
    const double q=x*x+y*y+z*z+w*w;
    if(!std::isfinite(q) || std::abs(q-1)>0.001)
        throw std::runtime_error("Warrior source quaternion invalid");
    const double inv=1/std::sqrt(q); x*=inv; y*=inv; z*=inv; w*=inv;
    WarriorMatrix r{1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),0,
        2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),0,
        2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y),0,0,0,0,1};
    auto s=warrior_identity();
    for(int i=0;i<3;++i) for(int j=0;j<3;++j) s[i*4+j]=p[7+i*3+j];
    auto m=warrior_multiply(r,s);
    for(int i=0;i<3;++i) m[i*4+3]=p[i];
    return m;
}
struct BnwWarriorSkinDiagnostic {
    std::vector<int> parents,palette,weights,indices;
    std::vector<double> local_bind,inverse_bind,sample_local;
    std::array<double,4> sample_times{0,1,3,5};
    std::string flat_sha256;

    std::array<WarriorMatrix,23> matrices(int sample=-1) const {
        if(sample<-1 || sample>3) throw std::runtime_error("Warrior diagnostic sample outside SDK clocks");
        std::array<WarriorMatrix,23> world{},skin{};
        const auto &local=sample<0?local_bind:sample_local;
        const std::size_t offset=sample<0?0:std::size_t(sample)*23*16;
        for(std::size_t i=0;i<23;++i) {
            const auto m=warrior_local(local.data()+offset+i*16);
            world[i]=parents[i]<0?m:warrior_multiply(world[std::size_t(parents[i])],m);
            WarriorMatrix inverse{};
            for(int r=0;r<4;++r) for(int c=0;c<4;++c)
                inverse[r*4+c]=inverse_bind[i*16+c*4+r];
            skin[i]=warrior_multiply(world[i],inverse);
        }
        return skin;
    }
    double bind_matrix_error() const {
        double error=0; const auto skin=matrices();
        for(const auto &m:skin) for(int i=0;i<16;++i)
            error=std::max(error,std::abs(m[i]-double(i/4==i%4)));
        return error;
    }
    std::vector<float> pose(const BnwWarriorSourceMesh &mesh,int sample=-1) const {
        if(mesh.vertex_pnuv.size()/8*4!=weights.size())
            throw std::runtime_error("Warrior skin/geometry cardinality differs");
        const auto skin=matrices(sample);
        auto out=mesh.vertex_pnuv;
        for(std::size_t v=0;v<out.size()/8;++v) {
            std::array<double,3> point{},normal{};
            for(std::size_t j=0;j<4;++j) {
                const auto weight=double(weights[v*4+j])/255;
                const auto &m=skin[std::size_t(palette[std::size_t(indices[v*4+j])])];
                for(std::size_t r=0;r<3;++r) {
                    double p=m[r*4+3],n=0;
                    for(std::size_t c=0;c<3;++c) {
                        p+=m[r*4+c]*mesh.vertex_pnuv[v*8+c];
                        n+=m[r*4+c]*mesh.vertex_pnuv[v*8+3+c];
                    }
                    point[r]+=weight*p; normal[r]+=weight*n;
                }
            }
            double length=0; for(auto n:normal) length+=n*n;
            if(length<1e-12 || !std::isfinite(length)) throw std::runtime_error("Warrior posed normal invalid");
            length=std::sqrt(length);
            for(std::size_t r=0;r<3;++r) {
                out[v*8+r]=float(point[r]); out[v*8+3+r]=float(normal[r]/length);
                if(!std::isfinite(out[v*8+r])) throw std::runtime_error("Warrior posed vertex nonfinite");
            }
        }
        return out;
    }
};
inline const char *warrior_expected_skin_sha(std::string_view name) {
    if(name=="warrior_v2.gr2") return "398c9f09ee11ae5734f34f5c34e5a76e8a69336789bd7ff19ee2155c6bb3b427";
    if(name=="warrior.gr2") return "237bc50a38f7237abcc2718ef5253799cb2fed8067b2504cad7ada1856a2a9d1";
    if(name=="warrior_v4.gr2") return "e548d274bff97b30652e9e0a5bb40e7082b5e4788f8be7f4e58ab2c614b1c756";
    throw std::runtime_error("Unknown Warrior skin source");
}
inline BnwWarriorSkinDiagnostic load_bnw_warrior_skin_diagnostic(
    const std::filesystem::path &cache,const BnwWarriorArtMember &spec,std::size_t vertices) {
    const auto file=cache/(std::filesystem::path(spec.gr2_name).stem().string()+".skin-flat.json");
    const auto sha=runtime::identify_file(file).sha256;
    require_bnw_warrior_asset_sha(sha,warrior_expected_skin_sha(spec.gr2_name));
    constexpr auto idle="7a6bde9dbb15f0916a920119774c1e803c92677660119c69c5d0bd32bee0cd88";
    constexpr auto gsd="f063955e71728fc76fdc097b9759e291f203c895d32f5f6eeaf5e4f78f73a959";
    require_bnw_warrior_asset_sha(runtime::identify_file(cache/"warrior_idlea.gr2").sha256,idle);
    require_bnw_warrior_asset_sha(runtime::identify_file(cache/"unitmodels_stringdatabase.gsd").sha256,gsd);
    require_bnw_warrior_asset_sha(runtime::identify_file(cache/spec.gr2_name).sha256,spec.gr2_sha256);
    const auto json=warrior_read_bounded(file,1'000'000);
    if(warrior_json_string(json,"format")!="CivisWarriorSkinFlatV1") throw std::runtime_error("Warrior skin format differs");
    require_bnw_warrior_asset_sha(warrior_json_string(json,"sourceGr2Sha256"),spec.gr2_sha256);
    require_bnw_warrior_asset_sha(warrior_json_string(json,"sourceIdleGr2Sha256"),idle);
    require_bnw_warrior_asset_sha(warrior_json_string(json,"sourceGsdSha256"),gsd);
    BnwWarriorSkinDiagnostic d; d.flat_sha256=sha;
    d.parents=warrior_json_numbers<int>(warrior_json_array(json,"parents"),23);
    d.palette=warrior_json_numbers<int>(warrior_json_array(json,"palette"),20);
    d.weights=warrior_json_numbers<int>(warrior_json_array(json,"weightsU8"),vertices*4);
    d.indices=warrior_json_numbers<int>(warrior_json_array(json,"indicesU8"),vertices*4);
    d.local_bind=warrior_json_numbers<double>(warrior_json_array(json,"localBind"),23*16);
    d.inverse_bind=warrior_json_numbers<double>(warrior_json_array(json,"inverseBind"),23*16);
    d.sample_local=warrior_json_numbers<double>(warrior_json_array(json,"sampleLocal"),4*23*16);
    if(warrior_json_numbers<double>(warrior_json_array(json,"sampleTimes"),4)!=std::vector<double>{0,1,3,5})
        throw std::runtime_error("Warrior SDK sample clocks differ");
    for(std::size_t i=0;i<23;++i) if(d.parents[i]<-1 || d.parents[i]>=int(i))
        throw std::runtime_error("Warrior skeleton topology invalid");
    for(auto i:d.palette) if(i<0 || i>=23) throw std::runtime_error("Warrior palette bounds invalid");
    for(auto i:d.indices) if(i<0 || i>=20) throw std::runtime_error("Warrior packed bone index invalid");
    for(std::size_t v=0;v<vertices;++v) {
        int sum=0; for(std::size_t j=0;j<4;++j) {
            const auto w=d.weights[v*4+j]; if(w<0 || w>255) throw std::runtime_error("Warrior packed weight invalid"); sum+=w;
        }
        if(sum!=255) throw std::runtime_error("Warrior packed weights not normalized");
    }
    if(d.bind_matrix_error()>0.001) throw std::runtime_error("Warrior bind/inverse convention mismatch");
    return d;
}
} // namespace civ5::graphics
