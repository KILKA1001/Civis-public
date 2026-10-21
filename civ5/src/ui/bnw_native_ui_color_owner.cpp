#include "civ5/ui/bnw_native_ui_color_owner.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include <charconv>
#include <cmath>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui {
namespace {
void need(bool value,const char*message){if(!value)throw std::runtime_error(message);}
bool same(const runtime::BnwStockLuaClientExtent&a,const runtime::BnwStockLuaClientExtent&b){return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;}
std::vector<BnwNativeGlobalColor> read_globals(const content::RulesDatabase&db){
 std::vector<BnwNativeGlobalColor> rows;const auto report=db.query_rows({"SELECT ID,Type,Red,Green,Blue,Alpha,typeof(ID) AS id_storage,typeof(Type) AS type_storage,typeof(Red) AS r_storage,typeof(Green) AS g_storage,typeof(Blue) AS b_storage,typeof(Alpha) AS a_storage FROM Colors ORDER BY ID",{}},[&](const content::RulesRowView&row){
  const auto type=row.text("Type");need(row.text("id_storage")=="integer"&&row.text("type_storage")=="text"&&type&&!type->empty()&&type->find('\0')==std::string::npos,"Native selected Colors key absent/unknown");BnwNativeGlobalColor value;value.id=row.integer("ID");value.type=*type;need(value.id>=0&&value.id<=1048576&&(rows.empty()||rows.back().id<value.id),"Native Colors ID/cache order unknown");value.hash=content::saved_info_type_hash(value.type);
  constexpr std::array<const char*,4>channels{"Red","Green","Blue","Alpha"},storage{"r_storage","g_storage","b_storage","a_storage"};for(std::size_t i=0;i<4;++i){const auto kind=row.text(storage[i]);const auto v=row.real(channels[i]);need(kind&&(*kind=="real"||*kind=="integer")&&std::isfinite(v)&&std::isfinite(static_cast<float>(v)),"Native selected Colors channel absent/nonfinite");value.rgba[i]=static_cast<float>(v);}rows.push_back(std::move(value));
 },1048576);need(report.all_succeeded()&&!rows.empty(),"Native selected global Color registry unavailable");return rows;
}
// Scoped exact numeric branch for the authored enclosing Box declarations.
// Other named misses/default-token variants remain unsupported, not guessed.
BnwNativeColor numeric(std::string_view input){
 BnwNativeColor out;for(std::size_t i=0;i<4;++i){const auto delimiter=input.find(',');const auto token=delimiter==std::string_view::npos?input:input.substr(0,delimiter);std::int64_t v=0;const auto parsed=std::from_chars(token.data(),token.data()+token.size(),v);need(!token.empty()&&parsed.ec==std::errc{}&&parsed.ptr==token.data()+token.size(),"Native Color miss outside supported four-integer literal branch");if(v<0)v=0;if(v>255)v=255;out[i]=static_cast<float>(v)*0.0039215688593685626983642578125f;need((i==3)==(delimiter==std::string_view::npos),"Native Color literal arity unsupported");if(delimiter!=std::string_view::npos)input.remove_prefix(delimiter+1);}return out;
}
}
struct BnwNativeUiColorQuote::Data {
 std::shared_ptr<const std::uint8_t> owner_identity;
 BnwNativeColorRegistryRead registry;
 std::string source;
 BnwNativeColor result;
 BnwNativeColorRoute route=BnwNativeColorRoute::numeric;
};
struct BnwNativeUiColorOwner::Impl {
 std::shared_ptr<const SelectedBnwPreviewHost> host;
 HWND hwnd=nullptr;std::thread::id thread=std::this_thread::get_id();
 runtime::BnwStockLuaClientExtentProvider client;
 runtime::BnwStockLuaClientExtent extent;
 BnwNativeColorRegistryProvider provider;
 std::shared_ptr<const std::uint8_t> identity=std::make_shared<const std::uint8_t>(0);
 std::vector<BnwNativeGlobalColor> rows;
 std::map<std::uint32_t,BnwNativeColor> global;
 void validate()const{
  need(std::this_thread::get_id()==thread,"Native Color owner foreign thread");DWORD process=0;RECT rc{};need(IsWindow(hwnd)&&GetWindowThreadProcessId(hwnd,&process)==GetCurrentThreadId()&&process==GetCurrentProcessId()&&GetClientRect(hwnd,&rc)&&rc.right-rc.left==extent.width&&rc.bottom-rc.top==extent.height,"Native Color requires SAME actual current-process HWND");const auto now=client.current();need(now&&same(*now,extent),"Native Color stale client epoch");need(read_globals(host->database())==rows,"Native selected global Colors source changed");
 }
 BnwNativeColorRegistryRead registry()const{
  need(provider.owner&&provider.current,"Native Color mandatory active context registry producer unavailable");const auto value=provider.current();need(value&&value->identity.get()==provider.owner.get()&&value->revision,"Native Color active context registry unknown/foreign");for(const auto&context:value->contexts)need(static_cast<bool>(context.identity),"Native Color active context identity absent");return *value;
 }
};
BnwNativeUiColorQuote::BnwNativeUiColorQuote(std::shared_ptr<const Data>data):data_(std::move(data)){}
const BnwNativeColor&BnwNativeUiColorQuote::rgba()const{need(static_cast<bool>(data_),"Native Color moved quote unavailable");return data_->result;}
BnwNativeColorRoute BnwNativeUiColorQuote::route()const{need(static_cast<bool>(data_),"Native Color moved quote unavailable");return data_->route;}
BnwNativeUiColorOwner::BnwNativeUiColorOwner(std::unique_ptr<Impl>impl):impl_(std::move(impl)){}
BnwNativeUiColorOwner::~BnwNativeUiColorOwner()=default;
std::shared_ptr<const BnwNativeUiColorOwner>BnwNativeUiColorOwner::create_from_adopted_host(
 std::shared_ptr<const SelectedBnwPreviewHost>host){
 need(host&&host->early_package_,"Native Color requires genuine adopted early host");
 const auto package=host->early_package_;
 host->require_same_early_package(*package);
 const auto lease=package->early_input_lease();
 return create(std::move(host),lease->actual_hwnd(),lease->client_provider(),package->color_provider());
}
std::shared_ptr<const BnwNativeUiColorOwner>BnwNativeUiColorOwner::create(std::shared_ptr<const SelectedBnwPreviewHost>host,std::uintptr_t hwnd,runtime::BnwStockLuaClientExtentProvider client,BnwNativeColorRegistryProvider provider){
 need(host&&hwnd&&client.current,"Native Color actual selected host/client missing");const auto extent=client.current();need(extent.has_value(),"Native Color actual client extent absent");auto i=std::make_unique<Impl>();i->host=std::move(host);i->hwnd=reinterpret_cast<HWND>(hwnd);i->client=std::move(client);i->extent=*extent;i->provider=std::move(provider);i->rows=read_globals(i->host->database());for(const auto&row:i->rows)i->global.insert_or_assign(row.hash,row.rgba);i->validate();return std::shared_ptr<const BnwNativeUiColorOwner>(new BnwNativeUiColorOwner(std::move(i)));
}
const std::vector<BnwNativeGlobalColor>&BnwNativeUiColorOwner::global_colors()const{impl_->validate();return impl_->rows;}
const saves::SaveHeader&BnwNativeUiColorOwner::selected_header()const{impl_->validate();return impl_->host->session().source().source.header;}
const runtime::BnwStockLuaClientExtent&BnwNativeUiColorOwner::client_extent()const{impl_->validate();return impl_->extent;}
BnwNativeUiColorQuote BnwNativeUiColorOwner::quote(std::string_view input)const{
 impl_->validate();need(!input.empty()&&input.find('\0')==std::string_view::npos,"Native Color declaration unavailable");auto data=std::make_shared<BnwNativeUiColorQuote::Data>();data->owner_identity=impl_->identity;data->source=input;const auto token=input.substr(0,input.find_first_of(" .,x*|"));const auto hash=content::saved_info_type_hash(token);need(hash!=0x8f4455c8&&hash!=0x164d0472,"Native Color current-player special provider unavailable");data->registry=impl_->registry();need(!data->registry.unresolved_provider,"Native Color mandatory BaseGame/Common/platform provider boundary unresolved");bool found=false;
 // Native5307B0 walks providers last-to-first;657930 visits slots0 then1.
 for(auto context=data->registry.contexts.rbegin();context!=data->registry.contexts.rend()&&!found;++context)for(std::size_t slot=0;slot<2&&!found;++slot){if(context->maps[slot]&&context->maps[slot]->empty())continue;need(context->enabled[slot].has_value(),"Native Color enabled context state unknown");if(!*context->enabled[slot])continue;need(context->maps[slot].has_value(),"Native Color enabled context map unknown");const auto match=context->maps[slot]->find(hash);if(match!=context->maps[slot]->end()){for(const auto v:match->second)need(std::isfinite(v),"Native context Color nonfinite");data->result=match->second;data->route=BnwNativeColorRoute::context;found=true;}}
 if(!found){need(data->registry.global_map.has_value(),"Native Color mandatory base ColorAtlas/global load-order producer unavailable");const auto global=data->registry.global_map->find(hash);if(global!=data->registry.global_map->end()){data->result=global->second;data->route=BnwNativeColorRoute::global;found=true;}}
 if(!found){data->result=numeric(input);data->route=BnwNativeColorRoute::numeric;}
 impl_->validate();need(impl_->registry()==data->registry,"Native Color provider changed during quote");impl_->validate();return BnwNativeUiColorQuote(std::move(data));
}
void BnwNativeUiColorOwner::validate_quote(const BnwNativeUiColorQuote&quote)const{impl_->validate();need(quote.data_&&quote.data_->owner_identity.get()==impl_->identity.get(),"Native Color quote belongs to another owner");need(impl_->registry()==quote.data_->registry,"Native Color quote provider epoch changed");impl_->validate();}
}
