#include "civ5/ui/glyphgen_style.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
namespace {
using namespace civ5;
int checks=0;
void check(bool value,const char* reason){++checks;if(!value)throw std::runtime_error(reason);}
template<class F>void rejects(F f,const char* reason){bool refused=false;try{f();}catch(const std::exception&){refused=true;}check(refused,reason);}
}
int main(int argc,char**argv)try {
 if(argc!=2)throw std::runtime_error("Expected original game root");
 const auto root=std::filesystem::path(argv[1]);
 auto loose=std::make_shared<assets::LooseResourceIndex>(root);
 for(const int size:{14,16,18,20,24})loose->add_file(root/("Assets/UI/Fonts/Tw Cent MT/TwCenMT"+std::to_string(size)+".ggxml"));
 const auto opened=assets::FpkSource::open(root/"Resource/DX9/UITextures.fpk","Resource/DX9/UITextures.fpk");
 check(std::holds_alternative<std::shared_ptr<assets::FpkSource>>(opened),"actual FPK source unavailable");
 const auto fpk=std::get<std::shared_ptr<assets::FpkSource>>(opened);
 assets::ResourceMountTable resources(assets::resource_lookup(loose));resources.mount({1,0,0,assets::resource_lookup(fpk),true,true,{},{}});
 std::size_t metadata_count=0,alpha_count=0;
 for(const int size:{14,16,18,20,24}) {
  const auto name="TwCenMT"+std::to_string(size);
  const auto original=ui::read_glyphgen(root/("Assets/UI/Fonts/Tw Cent MT/"+name+".ggxml"));
  check(original.styles.size()==6,"installed source style count changed");
  for(std::size_t n=0;n<original.styles.size();++n) {
   const auto &declaration=original.styles[n];
   const auto metadata=ui::read_glyphgen_style_metadata(resources,name,declaration.name);++metadata_count;
   check(metadata.source_style_index==n&&metadata.layers.size()==declaration.layers.size(),"per-font ordered source style index/count changed");
   check(metadata.font_source.sha256==runtime::identify_file(root/("Assets/UI/Fonts/Tw Cent MT/"+name+".ggxml")).sha256,"metadata source hash missing or changed");
   for(std::size_t k=0;k<metadata.layers.size();++k) {
    const auto &layer=metadata.layers[k];check(layer.name==declaration.layers[k],"literal ordered layer name rewritten");
    check(layer.texture_index<original.textures.size()&&original.textures[layer.texture_index].name()==layer.name&&original.textures[layer.texture_index].source()==layer.texture_source,"texture declaration source/index mismatch");
   }
   if(declaration.name.empty()) {
    check(metadata.source_style_index==0&&metadata.layers[0].name.empty(),"unnamed source style0 not retained");
    const auto h=fpk->find(assets::AssetKey(metadata.layers[0].texture_source));
    check(std::holds_alternative<assets::AssetHandle>(h),"actual unnamed style packed texture missing");
    const auto data=assets::DdsData::parse(std::get<assets::AssetHandle>(h).read_bytes());
    std::cout<<name<<" unnamed packed bits="<<data.bits_per_pixel()<<" format="<<static_cast<int>(data.format())<<"\n";
    check(data.bits_per_pixel()==8&&data.format()==assets::DdsFormat::alpha8,"actual unnamed packed alpha profile changed");
   }
   const auto loaded=ui::load_glyphgen_style(resources,name,declaration.name);++alpha_count;
   check(loaded.layers.size()==declaration.layers.size(),"actual loaded alpha layer count changed");
   for(std::size_t k=0;k<loaded.layers.size();++k) {
    const auto &layer=loaded.layers[k];check(layer.name==declaration.layers[k]&&layer.color_index==k,"source layer order/color slot changed");
    check(layer.atlas.width==original.atlas_width()&&layer.atlas.height==original.atlas_height(),"actual alpha reconstruction virtual dimensions differ");
    check(layer.source.mount_id==std::optional<std::uint64_t>{1}&&layer.index_source.mount_id==std::optional<std::uint64_t>{1},"actual paired FPK source identities differ");
    check(layer.source.candidate.provenance.archive_relative_path=="Resource/DX9/UITextures.fpk"&&layer.source.sha256.size()==64&&layer.index_source.sha256.size()==64,"actual retained texture source pins missing");
   }
   if(declaration.name=="SoftShadow") {
    check(loaded.layers.size()==3&&loaded.layers[2].name==(size==18?name+" Soft":name+"Soft"),"third literal source layer missing or rewritten");
    check(loaded.layers[2].color_index==2,"third layer color slot was collapsed into shadow slot1");
   }
   if(name=="TwCenMT18"&&declaration.name=="Shadow")check(runtime::identify_bytes(loaded.layers[1].atlas.alpha).sha256=="a7b0ca60ff867e23ea87d9f68e567da6ebb3e643bc6bd622ae8536d43779abe7","legacy Shadow reconstructed pixels changed");
   if(name=="TwCenMT20"&&declaration.name=="Stroke")check(runtime::identify_bytes(loaded.layers[1].atlas.alpha).sha256=="ad11b4aaf99ecaa7d9aadb25dc1c6e783ddf633e089ac22e3136af43f59f96fc","legacy Stroke reconstructed pixels changed");
   std::cout<<name<<" style="<<declaration.name<<" index="<<n<<" layers="<<loaded.layers.size()<<"\n";
  }
 }
 check(metadata_count==30&&alpha_count==30,"bounded actual source coverage incomplete");
 rejects([&]{(void)ui::read_glyphgen_style_metadata(resources,"TwCenMT20","Unknown");},"missing style guessed");
 rejects([&]{(void)ui::read_glyphgen_style_metadata(resources,"TwCenMT20","FontStyleSoftShadow");},"Style ID alias fabricated");
 rejects([&]{(void)ui::read_glyphgen_style_metadata(resources,"TwCenMT20","shadow");},"style case folded");
 rejects([&]{(void)ui::read_glyphgen_style_metadata(resources,"TwCenMT12","Shadow");},"undeclared profile admitted");
 // The actual source callback mutates borrowed arguments before lookup returns.
 std::string font="TwCenMT20",style="SoftShadow";bool mutated=false;
 auto lookup=assets::resource_lookup(loose);
 assets::ResourceMountTable aliased([&](const assets::AssetKey&key){if(!mutated){mutated=true;font.assign(4096,'x');style.assign(4096,'y');}return lookup(key);});
 aliased.mount({1,0,0,assets::resource_lookup(fpk),true,true,{},{}});
 const auto pinned=ui::read_glyphgen_style_metadata(aliased,font,style);
 check(mutated&&pinned.font.styles[pinned.source_style_index].name=="SoftShadow"&&pinned.layers[0].name=="TwCenMT20Base","borrowed metadata inputs retargeted after source callback");
 font="TwCenMT20";style="SoftShadow";mutated=false;
 const auto pinned_alpha=ui::load_glyphgen_style(aliased,font,style);
 check(mutated&&pinned_alpha.layers.size()==3&&pinned_alpha.layers[2].name=="TwCenMT20Soft","alpha loader reread borrowed input after metadata callback");
 assets::ResourceMountTable no_xml(assets::resource_lookup(fpk));
 rejects([&]{(void)ui::read_glyphgen_style_metadata(no_xml,"TwCenMT20","Shadow");},"missing metadata guessed");
 assets::ResourceMountTable no_index(assets::resource_lookup(loose));
 no_index.mount({1,0,0,[&](const assets::AssetKey&key){if(key.normalized_name().ends_with("-index.dds"))return assets::ResourceMountHit{std::monostate{},true};return assets::resource_lookup(fpk)(key);},true,true,{},{}});
 rejects([&]{(void)ui::load_glyphgen_style(no_index,"TwCenMT20","Shadow");},"missing paired index guessed");
 assets::ResourceMountTable ambiguous(assets::resource_lookup(loose));
 ambiguous.mount({1,0,0,[&](const assets::AssetKey&key){if(key.normalized_name().ends_with(".ggxml"))return assets::ResourceMountHit{std::monostate{},true};return assets::ResourceMountHit{assets::AmbiguousAsset{key,fpk->provenance(),{}},true};},true,true,{},{}});
 rejects([&]{(void)ui::load_glyphgen_style(ambiguous,"TwCenMT20","Shadow");},"ambiguous packed source guessed");
 assets::ResourceMountTable split_pair(assets::resource_lookup(loose));
 split_pair.mount({1,0,0,[&](const assets::AssetKey&key){if(key.normalized_name().ends_with("-index.dds"))return assets::ResourceMountHit{std::monostate{},true};return assets::resource_lookup(fpk)(key);},true,true,{},{}});
 split_pair.mount({2,0,0,[&](const assets::AssetKey&key){if(!key.normalized_name().ends_with("-index.dds"))return assets::ResourceMountHit{std::monostate{},true};return assets::resource_lookup(fpk)(key);},true,true,{},{}});
 rejects([&]{(void)ui::load_glyphgen_style(split_pair,"TwCenMT20","Shadow");},"mixed image/index mount identities were published");
 std::cout<<"PASS "<<checks<<" checks; actual metadata30 alpha30; source registry/nativeLabel/render authority unavailable\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}
