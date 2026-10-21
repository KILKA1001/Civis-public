#include "civ5/graphics/bnw_unit_fx_resource_chain.hpp"
#include <algorithm>
namespace civ5::graphics {
namespace {
void need(bool v,const char* why){if(!v)throw std::invalid_argument(why);}
bool same(const assets::Provenance&a,const assets::Provenance&b){return a.kind==b.kind&&a.source_id==b.source_id&&a.source_relative_path==b.source_relative_path&&a.archive_relative_path==b.archive_relative_path;}
BnwUnitFxSelectedResource read(const assets::ResourceMountTable& r,std::string request){assets::AssetKey key(request);need(key.safe_relative_path(),"FX resource source unsafe path");auto hit=r.resolve(key);const auto* h=std::get_if<assets::AssetHandle>(&hit.asset);need(h,"FX source resource missing/ambiguous");auto bytes=h->read_bytes(64*1024*1024);return {std::move(request),runtime::identify_bytes(bytes).sha256,hit.mount_id,h->candidate().provenance,*h};}
void verify(const assets::ResourceMountTable&r,const BnwUnitFxSelectedResource& old){auto now=read(r,old.request);need(now.sha256==old.sha256&&now.mount==old.mount&&same(now.provenance,old.provenance)&&now.handle.candidate().payload_size==old.handle.candidate().payload_size&&now.handle.candidate().normalized_name==old.handle.candidate().normalized_name&&now.handle.candidate().original_name==old.handle.candidate().original_name,"FX SAME selected resource identity changed");}
}
struct BnwUnitFxResourceChain::Impl {
 std::shared_ptr<const BnwUnitArtScaleCaller> caller;
 BnwUnitCompiledMemberScaleQuote quote;
 const assets::ResourceMountTable* resources;
 BnwUnitFxSelectedResource definition;
 std::vector<BnwUnitFxFileDeclaration> declarations;
 std::vector<BnwUnitFxSelectedResource> dependencies;
 Impl(std::shared_ptr<const BnwUnitArtScaleCaller> c,BnwUnitCompiledMemberScaleQuote q,const assets::ResourceMountTable&r,BnwUnitFxSelectedResource d):caller(std::move(c)),quote(std::move(q)),resources(&r),definition(std::move(d)){}
};
BnwUnitFxResourceChain::BnwUnitFxResourceChain(std::shared_ptr<const Impl> i):impl_(std::move(i)){}
std::shared_ptr<const BnwUnitFxResourceChain> BnwUnitFxResourceChain::create(std::shared_ptr<const BnwUnitArtScaleCaller> caller,BnwUnitCompiledMemberScaleQuote quote){need(bool(caller),"FX retained scale caller absent");const auto& resources=caller->resources_for_quote(quote);need(bnw_unit_fx_native_extension(quote.model())==".fxsxml","FX source compiled SQL Model is not FXSXML");auto source=read(resources,quote.model());auto bytes=source.handle.read_bytes(64*1024*1024);need(runtime::identify_bytes(bytes).sha256==source.sha256,"FX source changed during XML read");auto i=std::make_shared<Impl>(std::move(caller),std::move(quote),resources,std::move(source));i->declarations=read_bnw_unit_fx_declarations(content::read_xml_metadata(std::string(bytes.begin(),bytes.end())));
 // Source NULL load filter is fixed by6EF0DC/9723E0 args, +10/+14=0.
 // Comparator972D00 groups textures before GR2/DGE; within each class it
 // provides no ordering. No native sort equivalence-order receipt is claimed.
 for(auto kind:{BnwUnitFxFileClass::texture,BnwUnitFxFileClass::granny})for(const auto& d:i->declarations)if(d.extension_class==kind)i->dependencies.push_back(read(resources,d.file));
 auto out=std::shared_ptr<const BnwUnitFxResourceChain>(new BnwUnitFxResourceChain(std::move(i)));out->validate_current();return out;}
void BnwUnitFxResourceChain::validate_current()const{need(bool(impl_),"FX moved resource owner");const auto& r=impl_->caller->resources_for_quote(impl_->quote);need(&r==impl_->resources,"FX SAME resource table replaced");verify(r,impl_->definition);for(const auto& s:impl_->dependencies)verify(r,s);}
const BnwUnitFxSelectedResource& BnwUnitFxResourceChain::definition()const{validate_current();return impl_->definition;}
const std::vector<BnwUnitFxFileDeclaration>& BnwUnitFxResourceChain::declarations()const{validate_current();return impl_->declarations;}
const std::vector<BnwUnitFxSelectedResource>& BnwUnitFxResourceChain::selected_loader_dependencies()const{validate_current();return impl_->dependencies;}
const BnwUnitCompiledMemberScaleQuote& BnwUnitFxResourceChain::compiled_scale_quote()const{validate_current();return impl_->quote;}
const BnwUnitFxSelectedResource& BnwUnitFxResourceChain::warrior_geometry_source()const{validate_current();const auto& type=impl_->quote.xml_declaration().type;std::string file;if(type=="ART_DEF_UNIT_MEMBER_WARRIOR")file="warrior.gr2";else if(type=="ART_DEF_UNIT_MEMBER_WARRIOR_V2")file="warrior_v2.gr2";else if(type=="ART_DEF_UNIT_MEMBER_WARRIOR_V4")file="warrior_v4.gr2";else throw std::invalid_argument("FX projection not selected Warrior");const BnwUnitFxSelectedResource* result=nullptr;unsigned matching=0;for(const auto& d:impl_->declarations)if(!d.has_event_attribute&&bnw_unit_fx_native_extension(d.file)==".gr2"){need(assets::resource_basename(d.file)==file,"FX selected Warrior geometry declaration differs");++matching;for(const auto& s:impl_->dependencies)if(s.request==d.file)result=&s;}need(matching==1&&result,"FX unique selected Warrior geometry unavailable");return *result;}
const BnwUnitFxSelectedResource& BnwUnitFxResourceChain::warrior_idle_animation_source()const{(void)warrior_geometry_source();const BnwUnitFxSelectedResource* result=nullptr;unsigned matching=0;for(const auto& d:impl_->declarations)if(std::find(d.event_codes.begin(),d.event_codes.end(),1000)!=d.event_codes.end()){need(bnw_unit_fx_native_extension(d.file)==".gr2"&&assets::resource_basename(d.file)=="warrior_idlea.gr2","FX selected Warrior IdleA mapping differs");++matching;for(const auto& s:impl_->dependencies)if(s.request==d.file)result=&s;}need(matching==1&&result,"FX unique source IdleA event1000 unavailable");return *result;}
} // namespace civ5::graphics