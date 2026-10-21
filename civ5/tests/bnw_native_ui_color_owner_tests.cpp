#include "civ5/ui/bnw_native_ui_color_owner.hpp"
#include "../src/ui/bnw_native_color_load_kernel.hpp"
#include "civ5/ui/bnw_selected_native_color_projection.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Native Color accepted unknown/foreign source authority");}
// Data-kernel tests below do not issue native registry/instance authority.
void native_color_private_load_kernel_regressions(){
 namespace k=ui::native_color_load_detail;
 constexpr float scale=0.0039215688593685626983642578125f;
 const auto bits=[](float x){return std::bit_cast<std::uint32_t>(x);};
 k::ColorBits poison{0xdeadbeef,0xcafebabe,0x12345678,0x87654321},v=poison;
 k::numeric(" \t\r\n",v);check(v==poison,"Native empty trim overwrote caller raw storage");
 k::numeric("255,128,0,64",v);check(v==k::ColorBits{0x3f800000,0x3f008081,0,0x3e808081},"Native float32 scale/clamp bits differ");
 k::numeric("4",v);check(v==k::ColorBits{bits(4.0f*scale),0,0,0x3f800000},"Native missing components/default alpha differ");
 k::numeric("1,,2",v);check(v==k::ColorBits{bits(scale),0,bits(2.0f*scale),0x3f800000},"Native repeated delimiters were compressed");
 for(const auto text:{"255.0x128*64|","255 0,128x64","255|0*128.64"}){
  k::numeric(text,v);check(v==k::ColorBits{0x3f800000,0,0x3f008081,0x3e808081},"Native literal delimiter-set/token-step differs");
 }
 k::numeric("12suffix,-1,999,255",v);check(v==k::ColorBits{0x3d40c0c2,0,0,0x3f800000},"Native x delimiter/token cutoff differs");
 k::numeric("12tail,-1,999,255",v);check(v==k::ColorBits{bits(12.0f*scale),0,0x3f800000,0x3f800000},"Native prefix integer/full-token or clamp policy changed");
 check(k::integer("0xF")==15&&k::integer("0X12")==0&&k::integer("-0xF")==-15,"Native lowercase hex/sign order differs");
 check(k::integer("4294967295")==-1&&k::integer("4294967296")==0&&k::integer("4294967297")==1&&k::integer("-4294967295")==1,"Native DWORD wrap/NEG was replaced by overflow rejection");
 check(k::integer("  +17suffix")==17&&k::integer("bad")==0,"Native CRTspace/sign/prefix-stop differs");
 const std::string embedded("7\0,255",6);k::Tokens cursor{embedded};
 check(cursor.take().size()==6&&cursor.next==std::string_view::npos,"Native logical length was collapsed at embedded NUL");
 k::numeric(embedded,v);check(v==k::ColorBits{bits(7.0f*scale),0,0,0x3f800000},"Native C token search/integer NUL boundary differs");
 check(k::hash_query("A")==0x2c266174&&k::hash_query("a")==0x174841bc&&k::hash_query("123456789")==0x340bc6d9,"Native name hash case/start/no-finalxor differs");
 const std::string nul_name("A\0a",3);check(k::hash_query(nul_name)==0x2c266174&&k::hash_length(nul_name)!=0x2c266174,"Native query-NUL and load logical-length hashes conflated");
 // Actual current CRT operator domain, including high bytes. Native locale
 // provenance is a separate owner prerequisite; this is not ASCII parity.
 std::string high(1,char(0xe9));high+="255";
 const auto expected=std::isspace(static_cast<unsigned char>(high.front()))?255:0;
 check(k::integer(high)==expected,"Native unsigned-byte classification input contract differs");
 k::ColorSetBytes raw;raw.fill(0xa5);k::default_color_set(raw);
 for(std::size_t i=0;i<12;++i)check(k::read_word(raw,i*4)==0x3f800000,"Native default is not three four-float values1");
 for(std::size_t i=0x30;i<0x3f;++i)check(raw[i]==0,"Native default key/tag bytes differ");
 check(raw[0x3f]==0xa5,"Native untouched byte3f was initialized");
 const auto before_empty=raw;k::component_literal(raw,3,"\t ");check(raw==before_empty,"Native empty component zeroed key/tag before earlyreturn");
 k::component_literal(raw,3,"255,0,0,255");
 check(k::read_word(raw,0x30)==0x3f800000&&k::read_word(raw,0x34)==0&&k::read_word(raw,0x38)==0&&k::read_word(raw,0x3c)==0x3f800000&&raw[0x3f]==0x3f,"Native Color3 value overwrite/order or byte overlap differs");
 k::default_color_set(raw);k::component_named_value(raw,0,{0x3f800000,0,0,0x3f800000},0xaabbccdd);
 check(k::read_word(raw,0x30)==0xaabbccdd&&raw[0x3c]==4,"Native named-hit reference/type4 mutation differs");
 k::component_named_alpha(raw,0,"");check(k::read_word(raw,12)==0&&raw[0x3c]==3,"Native EMPTY named alpha must convert0/type3");
 k::component_named_value(raw,3,{0x3f800000,0,0,0x3f800000},0xaabbccdd);
 check(k::read_word(raw,0x3c)==0x04bbccdd,"Native Color3 key then type4 overlap order differs");
 k::component_named_alpha(raw,3,"255");check(k::read_word(raw,0x3c)==0x03800000,"Native Color3 alpha then type3 overlap order differs");
 std::map<std::uint32_t,k::ColorBits> values;
 check(k::remember_first_hash(values,0x10203040,{1,2,3,4})&&!k::remember_first_hash(values,0x10203040,{9,8,7,6})&&values.at(0x10203040)==k::ColorBits{1,2,3,4},"Native same-hash/collision overwrote retained first payload");
}
// Expected values from retained62EC20/81D6B0 call operands; no filename owner.
void native_color_fallback_value_regressions(){
 namespace k=ui::native_color_load_detail;
 const auto value=[](std::string_view name,std::int32_t length,std::string_view segment){
  return k::fallback_color_nonnull_value({name,length},segment);};
 check(value("C:\\Dir\\pkg.civ5pkg",18,"Color.xml")=="\\Dir\\Color.xml","Native fallback retained drive prefix/canonicalized directory");
 check(value("Dir\\pkg",7,"x/y")=="Dir\\x\\y","Native appended slash replacement/order differs");
 check(k::fallback_nonnull_value({"Dir\\pkg",7},"x/y")=="Dir\\x/y","Caller slash replacement leaked into62EC20 helper");
 check(value("C:/Dir/pkg",10,"x/y")=="\\x\\y","Native backslash search was replaced with generic path normalization");
 check(value("pkg",3,"Color.xml")=="\\Color.xml"&&value("",0,"")=="\\","Native empty-prefix/non-null empty segment route differs");
 check(value("a:b\\pkg",7,"x")=="b\\x"&&value("dir\\f:name",10,"x")=="\\x","Native first-colon/negative substring clamp differs");
 check(value("\\\\server\\share\\pkg",18,"/x")=="\\\\server\\share\\\\x","Native UNC/repeated separators were canonicalized");
 check(value("Dir\\\\pkg",8,"x")=="Dir\\\\x","Native repeated trailing separator spelling lost");
 const std::string filename("A\\pkg\0Tail\\ignored",18),segment("x/y\0ignored",11);
 check(value(filename,18,segment)=="A\\x\\y","Native first-NUL filename/append search crossed embedded NUL");
 check(value("A\\B\\pkg",3,"x")=="A\\x","Native search ignored supplied logical-end boundary");
 check(value("A\\B\\x:y",3,"x")=="\\x","Native restored-end colon search was incorrectly limited to logical search prefix");
 check(value("A\\B\\pkg",-1,"x")=="\\x"&&value("A\\B",std::numeric_limits<std::int32_t>::min(),"x")=="\\x","Native negative logical length incorrectly inspected filename");
 const std::string high={'D','\\',char(0xe9)};
 check(value(high,3,std::string_view(high).substr(2))==high,"Raw native bytes were decoded/case-folded as filesystem UTF8");
 rejects([&]{(void)value("A",2,"x");});
 rejects([&]{(void)value("A",std::numeric_limits<std::int32_t>::max(),"x");});
}
int main(int argc,char**argv){
 native_color_fallback_value_regressions();
 native_color_private_load_kernel_regressions();
 check(argc==4,"Actual Gandhi save and selected rule installation and explicit shared assets required");auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(static_cast<bool>(selected.preview),"Actual selected Gandhi unavailable");auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(selected.preview.host));
 struct Window{HWND handle=nullptr;~Window(){if(handle)DestroyWindow(handle);}}window;window.handle=CreateWindowExW(0,L"STATIC",L"",WS_OVERLAPPEDWINDOW,0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);check(window.handle&&!IsWindowVisible(window.handle),"Actual current-process HWND missing");std::uint64_t client_revision=4;auto client=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window.handle),"actual-native-color",[&]{return client_revision;});
 auto actual=ui::BnwNativeUiColorOwner::create(host,reinterpret_cast<std::uintptr_t>(window.handle),client);const auto&rows=actual->global_colors();check(!rows.empty(),"Actual selected global Colors absent");for(const auto&row:rows)check(row.hash==content::saved_info_type_hash(row.type),"Native global Color hash differs");rejects([&]{actual->quote("0,0,0,0");});rejects([&]{actual->quote("255,255,255,0");});
 const auto atlas_fixture=ui::parse_bnw_native_color_atlas(content::read_xml_metadata("<ColorAtlas><Color Name='A' Color='255,0,0,255'/><Color Name='A' Color='0,255,0,255'/><ColorSet Name='Separate' Color0='0.0.0.0'/></ColorAtlas>"));check(atlas_fixture.color_rows==2&&atlas_fixture.colors.size()==1&&atlas_fixture.color_sets==1&&atlas_fixture.colors.begin()->second==ui::BnwNativeColor{1,0,0,1},"Native atlas duplicate first-wins or separate ColorSet projection differs");rejects([&]{ui::parse_bnw_native_color_atlas(content::read_xml_metadata("<ColorAtlas><Color Name='A'/></ColorAtlas>"));});
 const auto catalog=content::discover_packages({std::filesystem::path(argv[2])/"Assets/DLC"});check(catalog.diagnostics.empty(),"Actual selected package source catalog unavailable");auto wrong_profile=selected.profile;wrong_profile.enabled_dlc.pop_back();rejects([&]{ui::BnwSelectedNativeColorProjection::create(argv[3],catalog,wrong_profile,actual);});auto changed_catalog=catalog;changed_catalog.packages.at(selected.profile.enabled_dlc.front()).document.attributes.emplace_back("UnboundRuntimeEntry","1");rejects([&]{ui::BnwSelectedNativeColorProjection::create(argv[3],changed_catalog,selected.profile,actual);});auto projection=ui::BnwSelectedNativeColorProjection::create(argv[3],catalog,selected.profile,actual);const auto&covered=projection->current();check(covered.selected_packages==selected.profile.enabled_dlc.size()&&covered.skin_contexts==4&&covered.slot_maps==8&&covered.base_color_rows==17&&covered.base_color_sets==15&&covered.skin_color_sets==2&&covered.all_skin_color_maps_empty&&covered.global_hashes_compatible&&covered.owned_global_ui_then_rules&&!covered.potential_registry_complete&&!covered.native_skin_loading_complete&&!covered.stock_loading_complete,"Actual selected Color source/map boundary differs");const auto native_registry=projection->provider().current();check(native_registry&&native_registry->contexts.size()==4&&native_registry->unresolved_provider&&native_registry->unresolved_provider->identity&&!native_registry->unresolved_provider->source.empty()&&native_registry->global_map&&!native_registry->global_map->empty(),"Actual Color maps unavailable");for(const auto&ctx:native_registry->contexts)for(std::size_t n=0;n<2;++n)check(!ctx.enabled[n]&&ctx.maps[n]&&ctx.maps[n]->empty(),"Color projection fabricated enabled flag or map");auto production=ui::BnwNativeUiColorOwner::create(host,reinterpret_cast<std::uintptr_t>(window.handle),client,projection->provider());rejects([&]{production->quote("0,0,0,0");});rejects([&]{production->quote("255,255,255,0");});rejects([&]{production->quote("Beige");});rejects([&]{production->quote(rows.front().type);});check(!projection->current().potential_registry_complete,"Unresolved base boundary opened registry completeness");
 // Registry maps below are explicitly lookup fixtures, NOT a production
 // selected stock context/epoch/empty-registry certificate.
 auto registry_identity=std::make_shared<const int>(1);ui::BnwNativeColorRegistryRead state{registry_identity,1,{}};auto older=std::make_shared<const int>(2),newer=std::make_shared<const int>(3);const auto zero=content::saved_info_type_hash("0");ui::BnwNativeColorContextRead old{older,{{true,false}},{{std::map<std::uint32_t,ui::BnwNativeColor>{{zero,{0,1,0,1}}},std::nullopt}}};ui::BnwNativeColorContextRead recent{newer,{{true,true}},{{std::map<std::uint32_t,ui::BnwNativeColor>{{zero,{1,0,0,1}}},std::map<std::uint32_t,ui::BnwNativeColor>{{zero,{0,0,1,1}}}}}};state.contexts={old,recent};
 std::size_t calls=0,poison_at=0,client_poison_at=0;ui::BnwNativeColorRegistryProvider fixture{registry_identity,[&]()->std::optional<ui::BnwNativeColorRegistryRead>{++calls;if(poison_at&&calls==poison_at)++state.revision;if(client_poison_at&&calls==client_poison_at)++client_revision;return state;}};auto owner=ui::BnwNativeUiColorOwner::create(host,reinterpret_cast<std::uintptr_t>(window.handle),client,fixture);
 auto red=owner->quote("0,0,0,0");check(red.route()==ui::BnwNativeColorRoute::context&&red.rgba()==ui::BnwNativeColor{1,0,0,1},"Native context reverse order/map0-first override differs");owner->validate_quote(red);state.unresolved_provider=ui::BnwNativeColorUnresolvedProvider{std::make_shared<const int>(4),"Unordered unknown base provider fixture"};rejects([&]{owner->quote("0,0,0,0");});rejects([&]{owner->validate_quote(red);});state.unresolved_provider.reset();owner->validate_quote(red);old.enabled[0]=std::nullopt;state.contexts[0]=old;check(owner->quote("0,0,0,0").rgba()==ui::BnwNativeColor{1,0,0,1},"Native earlier hit incorrectly inspected later unknown source");recent.enabled[0]=std::nullopt;state.contexts[1]=recent;rejects([&]{owner->quote("0,0,0,0");});recent.enabled[0]=true;recent.maps[0].reset();state.contexts[1]=recent;rejects([&]{owner->quote("0,0,0,0");});
 state.contexts.clear();++state.revision;rejects([&]{owner->quote("0,0,0,0");});rejects([&]{owner->quote(rows.front().type);});state.global_map.emplace();for(const auto&row:rows)state.global_map->insert_or_assign(row.hash,row.rgba);auto numeric=owner->quote("255,255,255,0");check(numeric.route()==ui::BnwNativeColorRoute::numeric&&numeric.rgba()==ui::BnwNativeColor{1,1,1,0},"Native supported numeric fallback float255 normalization differs");check(owner->quote("-7,999,128,255").rgba()==ui::BnwNativeColor{0,1,128.0f*0.0039215688593685626983642578125f,1},"Native numeric branch clamp/float factor differs");rejects([&]{owner->quote("0,0,0");});
 const auto chosen=std::find_if(rows.begin(),rows.end(),[](const auto&row){return row.hash!=0x8f4455c8&&row.hash!=0x164d0472;});check(chosen!=rows.end(),"Selected non-player-special global color absent");const auto global=owner->quote(chosen->type);check(global.route()==ui::BnwNativeColorRoute::global&&global.rgba()==chosen->rgba,"Explicit Rules-stage global map fixture differs");
 ++state.revision;rejects([&]{owner->validate_quote(numeric);});--state.revision;owner->validate_quote(numeric);auto other=ui::BnwNativeUiColorOwner::create(host,reinterpret_cast<std::uintptr_t>(window.handle),client,fixture);rejects([&]{other->validate_quote(numeric);});calls=0;poison_at=2;rejects([&]{owner->quote("255,255,255,0");});poison_at=0;--state.revision;owner->validate_quote(numeric);
 calls=0;client_poison_at=2;rejects([&]{owner->quote("0,0,0,0");});client_poison_at=0;--client_revision;owner->validate_quote(numeric);auto moved=numeric;auto retained=std::move(moved);rejects([&]{moved.rgba();});rejects([&]{owner->validate_quote(moved);});owner->validate_quote(retained);
 ++client_revision;rejects([&]{actual->global_colors();});--client_revision;bool foreign=false;std::thread thread([&]{try{owner->quote("0,0,0,0");}catch(const std::exception&){foreign=true;}});thread.join();check(foreign&&!host->session().readiness().can_continue(),"Color owner admitted foreign thread or opened gameplay");
 std::cout<<"PASS: actual selected Gandhi/global Rules Colors/current-process HWND, production missing-context blocker; honest ordered/enabled context lookup fixtures, numeric miss-only fallback, stale/foreign/atomic guards; actual selected DLC Color maps inspected, unresolved base provider blocks numeric/global production quotes; native skin/style/full registry/loading still unavailable\n";
}
