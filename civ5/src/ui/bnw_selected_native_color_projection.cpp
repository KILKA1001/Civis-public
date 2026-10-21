#include "civ5/ui/bnw_selected_native_color_projection.hpp"
#include <charconv>
#include <set>
#include <thread>
namespace civ5::ui {
namespace {
void need(bool v,const char*why){if(!v)throw std::runtime_error(why);}
BnwNativeColor channels(std::string_view text){
 BnwNativeColor out;for(std::size_t n=0;n<4;++n){const auto comma=text.find(',');const auto token=text.substr(0,comma);std::int64_t v=0;const auto p=std::from_chars(token.data(),token.data()+token.size(),v);need(!token.empty()&&p.ec==std::errc{}&&p.ptr==token.data()+token.size(),"Selected native ColorAtlas numeric form unsupported");need((n==3)==(comma==std::string_view::npos),"Selected native ColorAtlas channel arity unsupported");if(v<0)v=0;if(v>255)v=255;out[n]=static_cast<float>(v)*0.0039215688593685626983642578125f;if(comma!=std::string_view::npos)text.remove_prefix(comma+1);}return out;
}
bool equal(const content::XmlNode&a,const content::XmlNode&b){if(a.name!=b.name||a.text!=b.text||a.attributes!=b.attributes||a.children.size()!=b.children.size())return false;for(std::size_t n=0;n<a.children.size();++n)if(!equal(a.children[n],b.children[n]))return false;return true;}
struct Pin{std::filesystem::path path;runtime::FileIdentity identity;};
}
BnwNativeColorAtlasRead parse_bnw_native_color_atlas(const content::XmlNode&root){
 need(root.name=="ColorAtlas"&&root.attributes.empty(),"Native ColorAtlas root unsupported");BnwNativeColorAtlasRead r;
 for(const auto&node:root.children){if(node.name=="ColorSet"){++r.color_sets;continue;}need(node.name=="Color"&&node.children.empty(),"Native ColorAtlas child outside supported Color/ColorSet projection");const auto name=node.attribute("Name"),value=node.attribute("Color");need(!name.empty()&&!value.empty()&&node.attributes.size()==2,"Native ColorAtlas Color declaration incomplete/extended");const auto hash=content::saved_info_type_hash(name);++r.color_rows;if(!r.colors.contains(hash))r.colors.emplace(hash,channels(value));}
 return r;
}
struct BnwSelectedNativeColorProjection::Impl {
 std::filesystem::path root;std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const BnwNativeUiColorOwner>rules;
 std::shared_ptr<const std::uint8_t>identity=std::make_shared<const std::uint8_t>(0);
 BnwSelectedColorProjectionFrame frame;BnwNativeColorRegistryRead registry;
 std::vector<Pin>pins;
 content::XmlNode load(const std::filesystem::path&path){const auto full=std::filesystem::canonical(path);const auto relative=full.lexically_relative(root);need(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!=std::filesystem::path(".."),"Native Color source escapes explicit shared root");const auto id=runtime::identify_file(full);need(id.bytes<=64ULL*1024*1024,"Native Color source exceeds budget");auto parsed=content::read_xml(full);need(runtime::identify_file(full).sha256==id.sha256,"Native Color source changed during read");pins.push_back({full,id});frame.source_order.push_back(assets::path_utf8(relative));return parsed;}
 void validate()const{need(std::this_thread::get_id()==thread,"Selected Color projection foreign thread");(void)rules->client_extent();for(const auto&p:pins){const auto now=runtime::identify_file(p.path);need(now.sha256==p.identity.sha256&&now.bytes==p.identity.bytes,"Selected native Color source revision changed");}}
};
BnwSelectedNativeColorProjection::BnwSelectedNativeColorProjection(std::shared_ptr<Impl>i):impl_(std::move(i)){}
BnwSelectedNativeColorProjection::~BnwSelectedNativeColorProjection()=default;
std::shared_ptr<const BnwSelectedNativeColorProjection>BnwSelectedNativeColorProjection::create(const std::filesystem::path&root,const content::Catalog&catalog,const runtime::SavedContentProfile&selected,std::shared_ptr<const BnwNativeUiColorOwner>rules){
 need(rules&&selected.can_activate&&selected.all_requirements_located&&selected.enabled_mods.empty(),"Selected native Color projection requires actual located zero-mod BNW profile");auto i=std::make_shared<Impl>();i->root=std::filesystem::canonical(root);i->rules=std::move(rules);need(std::filesystem::is_directory(i->root/"Assets"),"Shared native Color root unavailable");i->registry.identity=i->identity;i->registry.revision=1;
 const auto base=parse_bnw_native_color_atlas(i->load(i->root/"Assets/UI/ColorAtlas.xml"));i->frame.base_color_rows=base.color_rows;i->frame.base_color_sets=base.color_sets;i->registry.global_map=base.colors;
 // Execute this owned initialization in explicit source stages: fresh global
 // UI atlas map, then native Rules cache registration into that SAME map.
 // Native52D590 clears first: do NOT claim reset/load order commutes.
 for(const auto&row:i->rules->global_colors()){const auto found=i->registry.global_map->find(row.hash);need(found==i->registry.global_map->end()||found->second==row.rgba,"Native global Color collision requires actual cache load-order owner");i->registry.global_map->insert_or_assign(row.hash,row.rgba);}i->frame.global_hashes_compatible=true;i->frame.owned_global_ui_then_rules=true;
 std::set<std::string>saved_ids,selected_ids;const auto&header=i->rules->selected_header();need(header.mods.empty(),"Selected native Color source unexpectedly requests mods");for(const auto&d:header.dlcs)need(saved_ids.insert(content::canonical_guid(d.id)).second,"Selected native Color source DLC duplicate");for(const auto n:selected.enabled_dlc){need(n<catalog.packages.size(),"Selected native Color profile index unavailable");need(selected_ids.insert(content::canonical_guid(catalog.packages[n].id)).second,"Selected native Color profile duplicate source");}need(saved_ids==selected_ids,"Native Color selected profile differs from SAME saved session");
 const auto shared=content::discover_packages({i->root/"Assets/DLC"});need(shared.diagnostics.empty(),"Shared selected Skin manifest discovery incomplete");std::set<std::size_t>seen;bool bnw=false;
 for(const auto index:selected.enabled_dlc){need(index<catalog.packages.size()&&seen.insert(index).second,"Selected Skin package identity unavailable");const auto&authoritative=catalog.packages[index];need(authoritative.kind==content::PackageKind::dlc,"Selected Skin source is not DLC");const auto guid=content::canonical_guid(authoritative.id);bnw=bnw||guid=="6da07636-4123-4018-b643-6575b4ec336b";const content::PackageManifest*mirror=nullptr;for(const auto&p:shared.packages)if(p.kind==authoritative.kind&&content::canonical_guid(p.id)==guid&&p.version==authoritative.version){need(!mirror,"Shared selected Skin manifest ambiguous");mirror=&p;}need(mirror,"Shared selected Skin manifest absent");const auto mapped_before=runtime::identify_file(mirror->path);const auto mapped_checked=content::read_manifest(mirror->path);need(content::canonical_guid(mapped_checked.id)==guid&&mapped_checked.version==mirror->version&&equal(mapped_checked.document,mirror->document)&&runtime::identify_file(mirror->path).sha256==mapped_before.sha256,"Shared selected Skin manifest changed during mapping");i->pins.push_back({mirror->path,mapped_before});
  // Do not reread UI selection from modified shared metadata. Bind the source
  // installation descriptor to its real shared payload root, as bootstrap does.
  const auto original=runtime::identify_file(authoritative.path);need(original.bytes<=64ULL*1024*1024,"Selected Skin descriptor exceeds budget");const auto checked=content::read_manifest(authoritative.path);need(content::canonical_guid(checked.id)==guid&&checked.version==authoritative.version&&equal(checked.document,authoritative.document)&&runtime::identify_file(authoritative.path).sha256==original.sha256,"Selected Skin descriptor changed during read");i->pins.push_back({authoritative.path,original});std::size_t occurrence=0;
  for(const auto&node:checked.document.children){if(node.name!="UISkin")continue;need(!node.attribute("name").empty()&&!node.attribute("set").empty()&&!node.attribute("platform").empty(),"Native Skin attribute eligibility unknown");BnwNativeColorContextRead context;context.identity=std::make_shared<const std::uint8_t>(0);
   // Native ctor798CE0 starts enabled flags false. Actual selected load has
   // additional Style/resource prerequisites, so current flags remain unknown.
   for(std::size_t slot=0;slot<2;++slot){content::PackageResourceQuery q;q.selector=std::string(slot==0?"GameplaySkin":"Skin")+"/Color";q.enabled_categories={false,false,true};q.category=content::PackageCategory::ui_skin;q.category_occurrence=occurrence;std::map<std::uint32_t,BnwNativeColor>map;
    for(const auto&d:content::package_resources(checked,q)){const auto path=content::package_file(mirror->path.parent_path(),d.value);const auto atlas=parse_bnw_native_color_atlas(i->load(path));i->frame.skin_color_sets+=atlas.color_sets;for(const auto&pair:atlas.colors)map.try_emplace(pair.first,pair.second);}
    // Nonempty maps would require actual native selection/enable/order stages.
    // The current stock BNW source projection closes only proved empty maps.
    need(map.empty(),"Selected Skin Color map nonempty: actual enabled/source-load order required");context.maps[slot]=std::move(map);++i->frame.slot_maps;
   }i->registry.contexts.push_back(std::move(context));++occurrence;
  }++i->frame.selected_packages;
 }
 need(bnw,"Selected native Color projection missing BNW");i->frame.skin_contexts=i->registry.contexts.size();i->frame.all_skin_color_maps_empty=true;
 // 4691E0/628BE0 also select BaseGame/Common/platform providers. Their
 // construction and potential maps are NOT closed by selected DLC metadata.
 // Preserve one explicit unresolved producer boundary, without inventing its
 // native context count, position, enabled flags or empty Color maps.
 i->registry.unresolved_provider=BnwNativeColorUnresolvedProvider{std::make_shared<const std::uint8_t>(0),"BaseGame/Common/platform Skin construction and dynamic Color writer boundary"};i->validate();return std::shared_ptr<const BnwSelectedNativeColorProjection>(new BnwSelectedNativeColorProjection(std::move(i)));
}
const BnwSelectedColorProjectionFrame&BnwSelectedNativeColorProjection::current()const{impl_->validate();return impl_->frame;}
BnwNativeColorRegistryProvider BnwSelectedNativeColorProjection::provider()const{impl_->validate();auto i=impl_;return {i->identity,[i]()->std::optional<BnwNativeColorRegistryRead>{i->validate();const auto r=i->registry;i->validate();return r;}};}
}
