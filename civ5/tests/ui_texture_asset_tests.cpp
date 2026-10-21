#include "civ5/ui/ui_texture_asset.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace {
using namespace civ5;
void check(bool value,const char *why) {if(!value)throw std::runtime_error(why);}
template<class F> void rejects(F action) {try {action();} catch(const std::exception &) {return;}throw std::runtime_error("Required UI asset rejection missing");}
void put(assets::Bytes &b,std::size_t offset,std::uint32_t value) {for(int i=0;i<4;++i)b.at(offset+i)=static_cast<std::uint8_t>(value>>(8*i));}
assets::Bytes dds(std::uint32_t width,std::uint32_t height,std::uint32_t bpp,std::string_view tag) {
    assets::Bytes b(128+width*height*bpp/8,0);std::copy_n("DDS ",4,b.begin());put(b,4,124);put(b,8,0x1007);put(b,12,height);put(b,16,width);
    std::copy_n("FTXTInterface",13,b.begin()+32);put(b,76,32);put(b,80,bpp==32?0x41:0x20000);put(b,88,bpp);
    if(bpp==32) {put(b,92,0x00ff0000);put(b,96,0x0000ff00);put(b,100,0xff);put(b,104,0xff000000);}else put(b,92,255);
    std::copy(tag.begin(),tag.end(),b.begin()+116);return b;
}
void write(const std::filesystem::path &p,const assets::Bytes &bytes) {
    std::filesystem::create_directories(p.parent_path());std::ofstream out(p,std::ios::binary);out.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());
    if(!out)throw std::runtime_error("Synthetic DDS write failed");
}
void fixtures(const std::filesystem::path &root) {
    auto atlas=dds(4,2,32,"BC002"),index=dds(2,1,8,"BC002");index[128]=1;index[129]=0;
    for(std::uint32_t y=0;y<2;++y)for(std::uint32_t x=0;x<4;++x) {
        const auto offset=128+(y*4+x)*4;const std::uint8_t v=x<2?10:100;
        atlas[offset]=v;atlas[offset+1]=v+1;atlas[offset+2]=v+2;atlas[offset+3]=v+3;
    }
    write(root/"Pair/Test.dds",atlas);write(root/"Pair/Test-index.dds",index);
    auto loose=std::make_shared<assets::LooseResourceIndex>(root);loose->add_file(root/"Pair/Test.dds");loose->add_file(root/"Pair/Test-index.dds");
    assets::ResourceMountTable resources(assets::resource_lookup(loose));
    const auto loaded=ui::load_ui_texture_asset(resources,"Assets/UI/Art/Test.dds");
    check(loaded.expanded&&loaded.expanded->width==4&&loaded.expanded->height==2,"BC002 logical layout was not detected without indexed prefix");
    const auto &rgba=loaded.expanded->rgba;
    check(rgba[0]==102&&rgba[1]==101&&rgba[2]==100&&rgba[3]==103&&rgba[8]==12&&rgba[11]==13,"Block placement, BGRA channels or original alpha changed");
    check(loaded.index&&loaded.index->sha256==runtime::identify_bytes(index).sha256&&loaded.source.sha256==runtime::identify_bytes(atlas).sha256,"UI pair source digests missing");
    check(loaded.source.candidate.original_name=="Pair/Test.dds"&&!loaded.source.mount_id,"Selected UI provenance changed");
    // Authored-key compatibility lookup must neither prefer an ambiguous
    // basename nor bypass an ambiguous authored key via a valid basename.
    const auto candidate=std::get<assets::AssetHandle>(loose->find(assets::AssetKey("Pair/Test.dds"))).candidate();
    assets::ResourceMountTable exact_first([&](const assets::AssetKey &key) {
        if(key.original_name()=="Pair/Test.dds" || key.original_name()=="Pair/Test-index.dds")
            return assets::ResourceMountHit{loose->find(key),true};
        return assets::ResourceMountHit{assets::AmbiguousAsset{key,candidate.provenance,{candidate,candidate}},true};
    });
    check(ui::load_ui_texture_asset(exact_first,"Pair/Test.dds").expanded->rgba==rgba,
          "Authored mounted key was replaced by an ambiguous basename lookup");
    std::size_t fallback_queries=0;
    assets::ResourceMountTable exact_ambiguous([&](const assets::AssetKey &key) {
        if(key.original_name()=="Pair/Test.dds")
            return assets::ResourceMountHit{assets::AmbiguousAsset{key,candidate.provenance,{candidate,candidate}},true};
        ++fallback_queries;
        return assets::ResourceMountHit{loose->find(key),true};
    });
    rejects([&]{ui::load_ui_texture_asset(exact_ambiguous,"Pair/Test.dds");});
    check(fallback_queries==0,"Ambiguous authored key bypassed via basename fallback");
    check(ui::load_ui_texture_asset(resources,"indexed:Test.dds").expanded->rgba==rgba,"Legacy indexed input changed logical pixels");
    auto ordinary=dds(2,2,32,"COLOR");write(root/"Plain.dds",ordinary);loose->add_file(root/"Plain.dds");
    const auto plain=ui::load_ui_texture_asset(resources,"Plain.dds");
    check(!plain.expanded&&!plain.index&&plain.dds.original_bytes()==ordinary,"Ordinary COLOR DDS was expanded or rewritten");
    rejects([&]{ui::load_ui_texture_asset(resources,"indexed:Plain.dds");});
    auto bad=index;std::copy_n("BC004",5,bad.begin()+116);write(root/"Pair/Test-index.dds",bad);
    rejects([&]{ui::load_ui_texture_asset(resources,"Test.dds");});write(root/"Pair/Test-index.dds",index);
    bad=index;bad[128]=99;write(root/"Pair/Test-index.dds",bad);
    rejects([&]{ui::load_ui_texture_asset(resources,"Test.dds");});write(root/"Pair/Test-index.dds",index);
    assets::ResourceMountTable mixed(assets::resource_lookup(loose));
    mixed.mount({42,0,0,[&](const assets::AssetKey &key) {
        return assets::ResourceMountHit{assets::resource_basename(key.original_name())=="test-index.dds"?loose->find(key):assets::AssetLookup{},true};
    },true,true,{},{}});
    rejects([&]{ui::load_ui_texture_asset(mixed,"Test.dds");});
    assets::ResourceMountTable ambiguous([&](const assets::AssetKey &key){return assets::ResourceMountHit{assets::AmbiguousAsset{key,candidate.provenance,{candidate,candidate}},true};});
    rejects([&]{ui::load_ui_texture_asset(ambiguous,"Art/Test.dds");});
    bool mutated=false;
    assets::ResourceMountTable changing([&](const assets::AssetKey &key) {
        if(!mutated&&assets::resource_basename(key.original_name())=="test-index.dds") {auto changed=atlas;changed[128]^=1;write(root/"Pair/Test.dds",changed);mutated=true;}
        return assets::ResourceMountHit{loose->find(key),true};
    });
    rejects([&]{ui::load_ui_texture_asset(changing,"Test.dds");});
    check(mutated,"Fixture did not mutate source between paired reads");write(root/"Pair/Test.dds",atlas);
    std::filesystem::remove(root/"Pair/Test-index.dds");
    rejects([&]{ui::load_ui_texture_asset(resources,"Test.dds");});
}
void actual(const std::filesystem::path &root) {
    const auto open=[&](const std::string &path) {
        auto result=assets::FpkSource::open(root/path,path);
        if(auto *source=std::get_if<std::shared_ptr<assets::FpkSource>>(&result))return *source;
        throw std::runtime_error("Actual UI FPK cannot be opened");
    };
    auto base=open("Resource/DX9/UITextures.fpk"),bnw=open("Resource/DX9/Expansion2UITextures.fpk");
    assets::ResourceMountTable resources(assets::resource_lookup(base));resources.mount({2,0,0,assets::resource_lookup(bnw),true,true,{},{}});
    struct Pin {const char *name,*raw,*logical;std::uint32_t size;};
    const Pin pins[]{
        {"NotificationGeneric.dds","ee55c548428fc641ee9237356d966ed9f36e3295cb06ef5f1ad708501608688f","ee23ab61ddcf8dfc7fdc14c65ae26664874220a8f99f933be2fd91532fea69ff",80},
        {"NotificationFrameBase.dds","13ec2d82aa1c7243136841d66ab9fb78221cc2c08fcdfd71accfb6de16022312","4ea506f5a967f669b2d39b3dc38869253c136901097b596020e85ec184824c2f",80},
        {"CityBannerBackground.dds","258693e5e9db0abb1a26e63548bf87c6759af5431eaabc29008e969f570c6ee1","7cfbddfac8b5213ba48c41ad09deb4963b597d1ea27fa73c78354648c6654f1d",32},
        {"CityBannerLeftBackground.dds","43e5a154981c2fd315669a86de0d15dbb7286d0a2967aae93373b6b7856a88ed","7d9ea374d733ea9caecad19af3a854225d2af9268784fec0906cf77f00eb826b",32},
        {"CityBannerRightBackground.dds","e3b8e57732da1cc119e0fb58e9890c487159c2008be8c7920c6af4adaf058dee","af2b373ccb223f2c0d94a4e7afe260257f12fdf9fb54ba1991ac39403da7163e",32}};
    for(const auto &pin:pins) {
        const auto image=ui::load_ui_texture_asset(resources,std::string("Assets/UI/Art/")+pin.name);
        check(image.source.sha256==pin.raw,"Actual selected image differs from captured source identity");
        check(image.expanded&&image.index&&image.expanded->width==pin.size&&image.expanded->height==pin.size,"Actual native UI logical dimensions are wrong");
        check(runtime::identify_bytes(image.expanded->rgba).sha256==pin.logical,"Actual UI expansion differs from independent tile/channel/alpha reference");
        check(image.source.mount_id==image.index->mount_id&&image.source.candidate.provenance.archive_relative_path==image.index->candidate.provenance.archive_relative_path,"Actual image/index split native source identity");
    }
    for(const char *name:{"Grid9DarkBlue8Button.dds","Grid9FrameTurns.dds","MainOpen.dds"})
        check(!ui::load_ui_texture_asset(resources,name).expanded,"Actual COLOR control art was treated as block atlas");
}
}
int main(int argc,char **argv) {try {
    const auto base=argc>=2?std::filesystem::path(argv[1]):std::filesystem::current_path();
    fixtures(base/("synthetic-ui-art-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())));
    if(argc>=3)actual(argv[2]);
    std::cout<<"PASS UI texture asset fixtures"<<(argc>=3?" and actual captured FPK sources":"")<<'\n';return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}
