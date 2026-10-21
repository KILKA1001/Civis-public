#include "civ5/graphics/gl_bnw_system_font_atlas.hpp"
#include "civ5/graphics/detail/bnw_renderer_context_pin.hpp"
#include "civ5/graphics/detail/bnw_owned_gl_retirement_node.hpp"
#include <windows.h>
#include <map>
#include <thread>
#include <atomic>
#include <vector>
#include <optional>
#include "civ5/graphics/detail/bnw_label_system_font_atlas_metadata.hpp"
#include "civ5/graphics/detail/bnw_owned_font_paint_effect_fence.hpp"
#include "civ5/ui/detail/bnw_label_paint_text_loan.hpp"
#include "civ5/ui/detail/bnw_label_paint_prepared_flush_loan.hpp"
#include "civ5/ui/detail/bnw_label_paint_positioned_data.hpp"
#include <exception>
#include "civ5/ui/detail/bnw_label_system_font_borrower.hpp"
#include "civ5/ui/detail/bnw_label_system_font_prepared_run.hpp"
#include "civ5/ui/detail/bnw_system_font_native_run_data.hpp"
namespace civ5::graphics {
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
// Preserve ALL fields which may reinterpret packed producer/readback bytes,
// including PBO bindings. This is a local scoped transfer, not a renderer reset.
class Transfer {
 const GlApi&gl_;bool pack_;GLint buffer_=0;
 std::array<GLenum,12>fields_;std::array<GLint,12>old_{};
 public:
 Transfer(const GlApi&gl,bool pack):gl_(gl),pack_(pack),fields_(pack?std::array<GLenum,12>{GL_PACK_ALIGNMENT,GL_PACK_ROW_LENGTH,GL_PACK_IMAGE_HEIGHT,GL_PACK_SKIP_PIXELS,GL_PACK_SKIP_ROWS,GL_PACK_SKIP_IMAGES,GL_PACK_SWAP_BYTES,GL_PACK_LSB_FIRST,GL_PACK_COMPRESSED_BLOCK_WIDTH,GL_PACK_COMPRESSED_BLOCK_HEIGHT,GL_PACK_COMPRESSED_BLOCK_DEPTH,GL_PACK_COMPRESSED_BLOCK_SIZE}:std::array<GLenum,12>{GL_UNPACK_ALIGNMENT,GL_UNPACK_ROW_LENGTH,GL_UNPACK_IMAGE_HEIGHT,GL_UNPACK_SKIP_PIXELS,GL_UNPACK_SKIP_ROWS,GL_UNPACK_SKIP_IMAGES,GL_UNPACK_SWAP_BYTES,GL_UNPACK_LSB_FIRST,GL_UNPACK_COMPRESSED_BLOCK_WIDTH,GL_UNPACK_COMPRESSED_BLOCK_HEIGHT,GL_UNPACK_COMPRESSED_BLOCK_DEPTH,GL_UNPACK_COMPRESSED_BLOCK_SIZE}){
  gl_.GetIntegerv(pack?GL_PIXEL_PACK_BUFFER_BINDING:GL_PIXEL_UNPACK_BUFFER_BINDING,&buffer_);for(std::size_t i=0;i<fields_.size();++i)gl_.GetIntegerv(fields_[i],&old_[i]);gl_.BindBuffer(pack?GL_PIXEL_PACK_BUFFER:GL_PIXEL_UNPACK_BUFFER,0);for(std::size_t i=0;i<fields_.size();++i)gl_.PixelStorei(fields_[i],i==0?1:0);
 }
 ~Transfer(){for(std::size_t i=0;i<fields_.size();++i)gl_.PixelStorei(fields_[i],old_[i]);gl_.BindBuffer(pack_?GL_PIXEL_PACK_BUFFER:GL_PIXEL_UNPACK_BUFFER,static_cast<GLuint>(buffer_));}
};
} // namespace
struct GlBnwSystemFontAtlas::Pair {
 std::shared_ptr<GlContext>context; // Legacy public route only.
 std::shared_ptr<const detail::BnwRendererContextPin>context_pin; // Same Pin object as Impl/receiver.
 std::array<GLuint,2>id{}; // Legacy names only.
 // Declared last: enqueue retains the SAME Pin before other member releases.
 detail::BnwOwnedGlRetirementNode::Owner retirement;
 std::array<GLuint,2>&textures_without_callbacks()noexcept{
  if(retirement)return retirement->texture_pair_storage_without_callbacks().textures;
  return id;
 }
 ~Pair(){
  if(retirement)return; // Owner deleter enqueues; no facade/proc/native access.
  if(context&&(id[0]||id[1]))try{context->api().DeleteTextures(2,id.data());}catch(...){}
 }
};
std::shared_ptr<GlBnwSystemFontAtlas::Pair>GlBnwSystemFontAtlas::allocate(GlContext&c,int width,int height,
 std::shared_ptr<GlContext>legacy,std::shared_ptr<const detail::BnwRendererContextPin>pin){
 // Value snapshots above survive allocators even if a caller facade retires.
 GlContextNativeIdentity identity{};
 if(pin){need(!legacy,"Owned Pair cannot retain a public Context clone");
  pin->first_without_callbacks();
  need(&pin->context_reference_without_callbacks()==&c,"Owned Pair Context is not SAME counted Pin");
  identity=c.native_identity();
 }
 const auto owned_first=[&]{if(pin){
  pin->first_without_callbacks();
  need(identity.owner_thread&&identity.window&&identity.dc&&identity.context&&
   reinterpret_cast<std::uintptr_t>(wglGetCurrentContext())==identity.context&&
   reinterpret_cast<std::uintptr_t>(wglGetCurrentDC())==identity.dc&&
   reinterpret_cast<std::uintptr_t>(WindowFromDC(reinterpret_cast<HDC>(identity.dc)))==identity.window,
   "Owned Pair allocation rejects foreign current HWND/DC/WGL without switching");
 }};
 owned_first();auto p=std::make_shared<Pair>();owned_first();
 p->context=std::move(legacy);p->context_pin=pin;
 const auto&gl=c.api();owned_first();
 if(pin){p->retirement=detail::BnwOwnedGlRetirementNode::texture_pair_owned(pin,gl);owned_first();}
 // GL writes generated names directly into the already-owned Node fields.
 // No post-callback copy from unowned local IDs can lose physical custody.
 auto&ids=p->textures_without_callbacks();
 GLint maximum=0;gl.GetIntegerv(GL_MAX_TEXTURE_SIZE,&maximum);owned_first();
 need(width>0&&height>0&&width<=maximum&&height<=maximum,"Native L8 font atlas exceeds actual GL limits");
 gl.CreateTextures(GL_TEXTURE_2D,2,ids.data());owned_first();
 need(ids[0]&&ids[1],"Actual native L8 font atlas pair allocation failed");
 const std::array<GLint,4>swizzle{GL_RED,GL_RED,GL_RED,GL_ONE};
 for(const auto id:ids){
  gl.TextureStorage2D(id,1,GL_R8,width,height);owned_first();
  gl.TextureParameteri(id,GL_TEXTURE_BASE_LEVEL,0);owned_first();
  gl.TextureParameteri(id,GL_TEXTURE_MAX_LEVEL,0);owned_first();
  gl.TextureParameteriv(id,GL_TEXTURE_SWIZZLE_RGBA,swizzle.data());owned_first();
 }
 c.check("Allocate native font L8 pair");owned_first();return p;
}
void GlBnwSystemFontAtlas::require_owned_pair_retirement_snapshot_complete(
 std::shared_ptr<const detail::BnwRendererContextPin>pin){
 need(bool(pin),"Owned Pair producer retirement gate requires SAME Context Pin");
 pin->first_without_callbacks();
 const auto status=detail::BnwOwnedGlRetirementNode::drain_current_creator_owned();
 pin->first_without_callbacks();
 need(!status.incomplete&&!status.cleanup_failure&&!status.pending_creator_nodes&&
  !detail::BnwOwnedGlRetirementNode::sticky_cleanup_failure_without_callbacks(),
  "Owned Pair retirement snapshot incomplete/failed; producer receipt refused");
 pin->first_without_callbacks();
}
struct GlBnwSystemFontAtlas::Impl {
 std::shared_ptr<GlContext>context; // Legacy public create only.
 std::shared_ptr<const detail::BnwRendererContextPin>context_pin;
 std::shared_ptr<ui::BnwSystemFontCacheOwner>cache;std::shared_ptr<Pair>pair;
 GlBnwSystemFontAtlasFrame frame;std::map<std::size_t,std::uint64_t>uploaded;
 std::thread::id thread=std::this_thread::get_id();std::uintptr_t hwnd=0;
 bool owned_flush_active=false,owned_glyph_active=false;mutable bool owned_flush_failed=false;
 void reject_owned_observation_without_callbacks()const{
  if(context_pin){context_first();if(owned_flush_active||owned_glyph_active||owned_flush_failed){
   owned_flush_failed=true;throw std::runtime_error("Owned font Pair recursive observation or failed owner");
  }}
 }
 GlContext&actual_context()const{
  if(context_pin)return context_pin->context_reference_without_callbacks();
  need(bool(context),"Actual legacy font GL context missing");return *context;
 }
 void context_first()const{if(context_pin)context_pin->first_without_callbacks();}
 void validate()const{
  reject_owned_observation_without_callbacks();
  need(std::this_thread::get_id()==thread,"Font atlas used on foreign GL thread");context_first();actual_context().check("Native font pair epoch");need(reinterpret_cast<std::uintptr_t>(actual_context().native_window_handle())==hwnd&&cache->cpu_->native_window_handle()==hwnd,"Native font atlas must use SAME actual CPU client HWND");const auto&f=cache->cpu_->current();need(f.atlas_width==frame.width&&f.atlas_height==frame.height,"Stale native font pair geometry");context_first();
 }
 void uploaded_quote(const ui::BnwSystemFontSlotQuote&q)const{validate();const auto it=uploaded.find(q.slot_index);need(cache->current(q)&&q.slot.raster_uploaded&&it!=uploaded.end()&&it->second==q.slot.generation,"Glyph does not have SAME accepted atlas pair receipt");}
};
GlBnwSystemFontAtlas::GlBnwSystemFontAtlas(std::unique_ptr<Impl>i):impl_(std::move(i)){}
GlBnwSystemFontAtlas::~GlBnwSystemFontAtlas()=default;
std::shared_ptr<GlBnwSystemFontAtlas>GlBnwSystemFontAtlas::create(std::shared_ptr<GlContext>context,std::shared_ptr<ui::BnwSystemFontCacheOwner>cache){
 need(context&&cache,"Actual font GL context/cache missing");auto i=std::make_unique<Impl>();i->context=std::move(context);i->cache=std::move(cache);i->hwnd=reinterpret_cast<std::uintptr_t>(i->context->native_window_handle());const auto&f=i->cache->cpu_->current();i->frame.width=f.atlas_width;i->frame.height=f.atlas_height;i->validate();i->pair=allocate(i->actual_context(),i->frame.width,i->frame.height,i->context,{});i->frame.green_texture=i->pair->textures_without_callbacks()[0];i->frame.shadow_texture=i->pair->textures_without_callbacks()[1];i->frame.pair_storage_available=true;i->validate();return std::shared_ptr<GlBnwSystemFontAtlas>(new GlBnwSystemFontAtlas(std::move(i)));
}
std::shared_ptr<GlBnwSystemFontAtlas>GlBnwSystemFontAtlas::create_from_owned_renderer_context(
 std::shared_ptr<const detail::BnwRendererContextPin>pin,std::shared_ptr<ui::BnwSystemFontCacheOwner>cache){
 need(pin&&cache,"Actual counted renderer Context/cache missing");pin->first_without_callbacks();
 auto i=std::make_unique<Impl>();i->context_pin=pin;i->cache=std::move(cache);i->context_first();
 i->hwnd=reinterpret_cast<std::uintptr_t>(i->actual_context().native_window_handle());
 {const auto&f=i->cache->cpu_->current();i->frame.width=f.atlas_width;i->frame.height=f.atlas_height;}
 i->validate();i->pair=allocate(i->actual_context(),i->frame.width,i->frame.height,{},pin);
 i->frame.green_texture=i->pair->textures_without_callbacks()[0];i->frame.shadow_texture=i->pair->textures_without_callbacks()[1];
 i->frame.pair_storage_available=true;i->validate();
 auto result=std::shared_ptr<GlBnwSystemFontAtlas>(new GlBnwSystemFontAtlas(std::move(i)));
 pin->first_without_callbacks();return result;
}
GlBnwSystemFontAtlasIdentitySources GlBnwSystemFontAtlas::sources_identity_no_current()const{
 auto&i=*impl_;need(std::this_thread::get_id()==i.thread,"Font atlas identity used on foreign thread");
 i.context_first();auto*context=&i.actual_context();
 GlBnwSystemFontAtlasIdentitySources result{context,i.cache,i.hwnd,i.frame};
 i.context_first();return result;
}
void GlBnwSystemFontAtlas::require_same_owned_context_pin_no_current(
 const std::shared_ptr<const detail::BnwRendererContextPin>&pin)const{
 auto&i=*impl_;need(std::this_thread::get_id()==i.thread,"Font atlas owned token used on foreign thread");
 need(pin&&i.context_pin&&i.pair&&i.pair->context_pin&&!i.context&&!i.pair->context,
  "FontShader requires actual owned Atlas counted token");
 pin->first_without_callbacks();i.context_first();
 const auto same=[&](const auto&other){return pin.get()==other.get()&&
  !pin.owner_before(other)&&!other.owner_before(pin);};
 need(same(i.context_pin)&&same(i.pair->context_pin)&&
  &pin->context_reference_without_callbacks()==&i.actual_context(),
  "FontShader Atlas counted token/control changed");
 i.context_first();pin->first_without_callbacks();
}
std::shared_ptr<const ui::BnwSystemFontCacheOwner>GlBnwSystemFontAtlas::source_cache_owner()const noexcept{return impl_->cache;}
GlBnwSystemFontAtlasSources GlBnwSystemFontAtlas::sources_no_current()const{
 impl_->reject_owned_observation_without_callbacks();
 auto&i=*impl_;need(std::this_thread::get_id()==i.thread,"Font atlas identity snapshot used on foreign thread");
 need(!i.context_pin,"Private renderer Context cannot escape through public strong sources snapshot");
 return {i.context,i.cache,i.hwnd,i.frame};
}
bool GlBnwSystemFontAtlas::has_uploaded_quote_no_current(const ui::BnwSystemFontSlotQuote&q)const{
 impl_->reject_owned_observation_without_callbacks();
 auto&i=*impl_;need(std::this_thread::get_id()==i.thread,"Font atlas upload quote inspected on foreign thread");
 const auto it=i.uploaded.find(q.slot_index);
 return q.slot.raster_uploaded&&it!=i.uploaded.end()&&it->second==q.slot.generation&&i.cache->ring_.current(q);
}
GlBnwSystemFontAtlasFrame GlBnwSystemFontAtlas::current()const{impl_->validate();return impl_->frame;}
ui::BnwSystemFontSlotQuote GlBnwSystemFontAtlas::upload(std::uint32_t cp,const ui::BnwSystemFontSlotQuote&q){
 impl_->reject_owned_observation_without_callbacks();
 need(!impl_->context_pin,"Owned native physical slot requires sealed Paint whole-flush route");
 auto&i=*impl_;i.validate();const auto stage=i.cache->stage_raster(cp,q);const auto receipt=i.uploaded.find(q.slot_index);if(q.slot.raster_uploaded&&receipt!=i.uploaded.end()&&receipt->second==q.slot.generation){i.validate();return q;}
 // Retain accepted pair on every refusal. Fresh storage copies the current
 // pair, preserving actual prior bytes (including untouched storage).
 auto next=allocate(i.actual_context(),i.frame.width,i.frame.height,i.context,i.context_pin);auto next_uploaded=i.uploaded;next_uploaded[q.slot_index]=q.slot.generation;const auto&gl=i.actual_context().api();const auto copy=reinterpret_cast<PFNGLCOPYIMAGESUBDATAPROC>(i.actual_context().proc_address("glCopyImageSubData"));need(copy,"Actual GL pair copy entrypoint missing");for(std::size_t k=0;k<2;++k)copy(i.pair->textures_without_callbacks()[k],GL_TEXTURE_2D,0,0,0,0,next->textures_without_callbacks()[k],GL_TEXTURE_2D,0,0,0,0,i.frame.width,i.frame.height,1);i.actual_context().check("Preserve accepted font atlas pair");
 {Transfer transfer(gl,false);const auto width=stage.right-stage.left,height=stage.bottom-stage.top;need(width>=0&&height>0&&stage.left>=0&&stage.top>=0&&stage.right<=i.frame.width&&stage.bottom<=i.frame.height&&stage.cpu.green_brightness.size()==static_cast<std::size_t>(width)*height&&stage.cpu.shadow_brightness.size()==stage.cpu.green_brightness.size(),"Native font pair producer subregion invalid");gl.TextureSubImage2D(next->textures_without_callbacks()[0],0,stage.left,stage.top,width,height,GL_RED,GL_UNSIGNED_BYTE,stage.cpu.green_brightness.data());i.actual_context().check("Upload native green font atlas subregion");gl.TextureSubImage2D(next->textures_without_callbacks()[1],0,stage.left,stage.top,width,height,GL_RED,GL_UNSIGNED_BYTE,stage.cpu.shadow_brightness.data());i.actual_context().check("Upload native shadow font atlas subregion");}
 i.actual_context().check("Restore font atlas transfer state");i.validate();need(i.cache->current(q),"Stale native font slot before pair publication");need(i.frame.revision!=UINT64_MAX,"Native font pair revision exhausted");i.pair.swap(next);i.uploaded.swap(next_uploaded);i.frame.green_texture=i.pair->textures_without_callbacks()[0];i.frame.shadow_texture=i.pair->textures_without_callbacks()[1];++i.frame.revision;return i.cache->publish_uploaded(q);
}
assets::Bytes GlBnwSystemFontAtlas::read_uploaded(const ui::BnwSystemFontSlotQuote&q,bool shadow)const{
 impl_->reject_owned_observation_without_callbacks();
 need(!impl_->context_pin,"Owned native physical slot requires sealed Paint whole-flush route");
 auto&i=*impl_;i.uploaded_quote(q);const auto&f=i.cache->cpu_->current();const int left=f.cell_width*static_cast<int>(q.slot_index%f.columns),top=f.cell_height*static_cast<int>(q.slot_index/f.columns);assets::Bytes bytes(static_cast<std::size_t>(q.slot.advance)*f.cell_height);if(!bytes.empty()){const auto&gl=i.actual_context().api();Transfer transfer(gl,true);gl.GetTextureSubImage(i.pair->textures_without_callbacks()[shadow?1:0],0,left,top,0,q.slot.advance,f.cell_height,1,GL_RED,GL_UNSIGNED_BYTE,static_cast<GLsizei>(bytes.size()),bytes.data());i.actual_context().check("Read accepted native font atlas subregion");}i.uploaded_quote(q);return bytes;
}
void GlBnwSystemFontAtlas::bind_uploaded(const ui::BnwSystemFontSlotQuote&q,GLuint green_unit,GLuint shadow_unit)const{impl_->reject_owned_observation_without_callbacks();need(!impl_->context_pin,"Owned native Pair binding requires sealed Paint draw route");auto&i=*impl_;i.uploaded_quote(q);const auto&gl=i.actual_context().api();GLint maximum=0;gl.GetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,&maximum);need(green_unit!=shadow_unit&&green_unit<static_cast<GLuint>(maximum)&&shadow_unit<static_cast<GLuint>(maximum),"Native font pair texture units unavailable");gl.BindTextureUnit(green_unit,i.pair->textures_without_callbacks()[0]);gl.BindTextureUnit(shadow_unit,i.pair->textures_without_callbacks()[1]);i.actual_context().check("Bind accepted native font pair; shader owner still required");i.uploaded_quote(q);}
}
namespace civ5::graphics {
struct GlBnwSystemFontAtlas::OwnedFlushState final : detail::BnwOwnedFontPaintEffectFence {
 using Paint=ui::detail::BnwLabelPaintTextLoan;
 using Metadata=detail::BnwLabelSystemFontAtlasMetadata;
 const Paint::Guard paint;
 const Metadata::Guard metadata;
 const std::weak_ptr<GlBnwSystemFontAtlas> atlas;
 const std::weak_ptr<const detail::BnwRendererContextPin> pin;
 const std::weak_ptr<ui::BnwSystemFontCacheOwner> cache;
 const std::thread::id creator=std::this_thread::get_id();
 std::shared_ptr<const ui::detail::BnwLabelSystemFontPreparedRun> prepared;
 Pair* accepted=nullptr;
 GlBnwSystemFontAtlasFrame expected;
 std::vector<ui::BnwSystemFontSlotQuote> physical_slots;
 std::optional<ui::BnwSystemFontSlotQuote> processing;
 std::size_t slot_count=0;
 bool producing=true,sealed=false,drain_closed=false;
 OwnedFlushState(Paint::Guard p,Metadata::Guard m,const std::shared_ptr<GlBnwSystemFontAtlas>& a,
  const std::shared_ptr<const detail::BnwRendererContextPin>& token,
  const std::shared_ptr<ui::BnwSystemFontCacheOwner>& c,Pair* pair,GlBnwSystemFontAtlasFrame frame)
  :BnwOwnedFontPaintEffectFence(detail::BnwOwnedFontPaintEffectFence::IssuerKey{}),
   paint(std::move(p)),metadata(std::move(m)),atlas(a),pin(token),cache(c),accepted(pair),expected(frame){}
 OwnedFlushState(const OwnedFlushState& other)
  :BnwOwnedFontPaintEffectFence(detail::BnwOwnedFontPaintEffectFence::IssuerKey{}),
   paint(other.paint),metadata(other.metadata),atlas(other.atlas),pin(other.pin),cache(other.cache),
   creator(other.creator),prepared(other.prepared),accepted(other.accepted),expected(other.expected){}
 void require_current_without_callbacks()const override {
  need(creator==std::this_thread::get_id(),"Owned font Paint fence foreign creator");
  paint.require_current_without_callbacks();metadata.require_current_without_callbacks();
  if(prepared)prepared->require_current_without_callbacks();
  {
   const auto a=atlas.lock();const auto token=pin.lock();const auto c=cache.lock();
   need(a&&token&&c,"Owned font Paint physical storage expired");
   metadata.require_actual_atlas_without_callbacks(*a);
   auto& i=*a->impl_;
   need(i.context_pin&&i.cache&&!i.context&&i.pair&&i.pair->retirement,
    "Owned font Paint requires genuine renderer Pair custody");
   const auto same=[](const auto& x,const auto& y){return x.get()==y.get()&&!x.owner_before(y)&&!y.owner_before(x);};
   need(same(token,i.context_pin)&&same(c,i.cache),"Owned font Paint SAME counted Pin/cache changed");
   token->first_without_callbacks();
   if(i.owned_glyph_active){i.owned_flush_failed=true;
    throw std::runtime_error("Owned font receipt/flush observed during active native glyph");}
   need(!i.owned_flush_failed&&i.owned_flush_active==producing,
    "Owned font Paint owner recursive observation or failed transaction");
   need(i.pair.get()==accepted&&i.frame.revision==expected.revision&&
    i.frame.width==expected.width&&i.frame.height==expected.height&&
    i.frame.green_texture==expected.green_texture&&i.frame.shadow_texture==expected.shadow_texture&&
    i.frame.pair_storage_available==expected.pair_storage_available,
    "Owned font Paint accepted Pair/revision changed");
   // Only the current physical slot is checked at each effect bottom.
   // The complete immutable snapshot is rescanned once after LAST FULL.
   if(producing&&processing){
    c->require_native_slot_current_without_callbacks(*processing);
    const auto actual=c->native_slot_quote_without_callbacks(processing->slot_index);
    need(actual.slot.raster_uploaded==processing->slot.raster_uploaded,
     "Owned font currently processed physical pending flag changed");
   }
   const auto native=token->context_reference_without_callbacks().native_identity();
   need(native.owner_thread&&native.window==i.hwnd&&native.dc&&native.context&&
    reinterpret_cast<std::uintptr_t>(wglGetCurrentContext())==native.context&&
    reinterpret_cast<std::uintptr_t>(wglGetCurrentDC())==native.dc&&
    reinterpret_cast<std::uintptr_t>(WindowFromDC(reinterpret_cast<HDC>(native.dc)))==native.window,
    "Owned font Paint rejects foreign current HWND/DC/WGL without switching");
   need(!detail::BnwOwnedGlRetirementNode::sticky_cleanup_failure_without_callbacks(),
    "Owned font Paint physical cleanup has sticky failure");
  } // Pure-cut Atlas/cache/Pin locks end BEFORE Source observers.
  paint.require_current_without_callbacks();metadata.require_current_without_callbacks();
  if(prepared)prepared->require_current_without_callbacks();
 }
 void require_same_paint_without_callbacks(const Paint& facade)const {
  require_current_without_callbacks();
  const auto other=facade.capture_guard_without_callbacks();
  require_same_paint_guard_without_callbacks(other);
 }
 void require_same_paint_guard_without_callbacks(const Paint::Guard& other)const override {
  require_current_without_callbacks();
  // Whole-flush receipt requires the originating typed Paint owner.
  paint.require_same_paint_guard_without_callbacks(other);
  require_current_without_callbacks();
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const {
  require_current_without_callbacks();paint.require_source_owned(operation);require_current_without_callbacks();
  metadata.require_source_owned(operation);require_current_without_callbacks();
  if(prepared){prepared->require_source_owned(operation);require_current_without_callbacks();}
  paint.require_source_owned(operation);require_current_without_callbacks();
 }
 void check_native(const GlApi& gl)const {
  require_current_without_callbacks();const auto reset=gl.GetGraphicsResetStatus();require_current_without_callbacks();
  need(reset==GL_NO_ERROR,"Owned font Paint native graphics reset");
  const auto error=gl.GetError();require_current_without_callbacks();
  need(error==GL_NO_ERROR,"Owned font Paint native GL error");
 }
 void require_receipt_without_callbacks()const {
  need(sealed&&!producing&&drain_closed&&prepared&&physical_slots.size()==slot_count,
   "Owned font preparation/whole-flush completion is not sealed");
  require_current_without_callbacks();
 }
};
} // namespace civ5::graphics

namespace civ5::ui::detail {
struct BnwLabelPaintPreparedFlushLoan::Impl final {
 const std::shared_ptr<::civ5::graphics::GlBnwSystemFontAtlas::OwnedFlushState> state;
 std::atomic<bool> alive{true};
 explicit Impl(std::shared_ptr<::civ5::graphics::GlBnwSystemFontAtlas::OwnedFlushState> s):state(std::move(s)){}
 void first()const {
  if(!alive.load(std::memory_order_acquire))throw std::runtime_error("Prepared flush lexical receipt retired");
  state->require_receipt_without_callbacks();
  if(!alive.load(std::memory_order_acquire))throw std::runtime_error("Prepared flush lexical receipt retired at POST");
 }
};
BnwLabelPaintPreparedFlushLoan::BnwLabelPaintPreparedFlushLoan(IssuerKey,std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwLabelPaintPreparedFlushLoan::~BnwLabelPaintPreparedFlushLoan(){impl_->alive.store(false,std::memory_order_release);}
BnwLabelPaintPreparedFlushLoan::Guard::Guard(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwLabelPaintPreparedFlushLoan::Guard BnwLabelPaintPreparedFlushLoan::capture_guard_without_callbacks()const {
 const auto saved=impl_;saved->first();return Guard(saved);
}
void BnwLabelPaintPreparedFlushLoan::Guard::require_current_without_callbacks()const {const auto saved=impl_;saved->first();}
void BnwLabelPaintPreparedFlushLoan::Guard::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const {
 const auto saved=impl_;saved->first();saved->state->full(operation);saved->first();
}
void BnwLabelPaintPreparedFlushLoan::Guard::require_same_paint_without_callbacks(const BnwLabelPaintTextLoan& facade)const {
 const auto saved=impl_;saved->first();saved->state->require_same_paint_without_callbacks(facade);saved->first();
}
void BnwLabelPaintPreparedFlushLoan::Guard::require_same_paint_guard_without_callbacks(const ::civ5::graphics::GlBnwSystemFontAtlas::PaintGuard& input)const {
 const auto saved=impl_;saved->first();saved->state->require_same_paint_guard_without_callbacks(input);saved->first();
}
::civ5::graphics::GlBnwSystemFontAtlasFrame
BnwLabelPaintPreparedFlushLoan::Guard::accepted_pair_without_callbacks(
 const ::civ5::graphics::GlBnwSystemFontAtlas& input)const {
 // Capture the reference identity and physical custody before existing FIRST.
 // No FULL, slot/journal/generation lookup or additional flush participates.
 const auto* const input_identity=&input;
 const auto saved=impl_;const auto state=saved->state;const auto canonical=state->atlas;
 saved->first();
 ::civ5::graphics::GlBnwSystemFontAtlasFrame result;
 {
  const auto owner=canonical.lock();
  if(!owner||owner.get()!=input_identity)
   throw std::runtime_error("Prepared flush accepted Pair belongs to another actual Atlas");
  // Canonical shared control belongs to state, checked through its real
  // metadata by FIRST. A const input reference conveys no shared control.
  saved->first();
  result=state->expected;
  saved->first();
 } // Temporary Atlas owner lock ends before independent final FIRST.
 saved->first();return result;
}
} // namespace civ5::ui::detail

namespace civ5::graphics {
std::unique_ptr<ui::detail::BnwLabelPaintPreparedFlushLoan>GlBnwSystemFontAtlas::prepare_and_flush_native_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const ui::detail::BnwLabelSystemFontBorrower& borrower,const PaintGuard& input){
 // Every original facade and physical snapshot is captured before any effect.
 const auto paint=input;paint.require_current_without_callbacks();
 paint.require_system_font_borrower_without_callbacks(borrower);
 const auto descriptor=borrower.atlas_metadata_without_callbacks();
 const auto metadata=descriptor->capture_guard_without_callbacks();
 const auto self=metadata.retain_atlas_for_physical_cleanup_without_callbacks();
 need(self.get()==this,"Actual Paint borrower names another physical Atlas");
 metadata.require_actual_atlas_without_callbacks(*self);
 auto* const storage=self->impl_.get();
 const auto pin=storage->context_pin;const auto cache=storage->cache;
 need(pin&&cache&&storage->pair&&storage->pair->retirement,"Actual Paint requires owned renderer Atlas");
 pin->first_without_callbacks();
 if(storage->owned_flush_active||storage->owned_glyph_active||storage->owned_flush_failed){storage->owned_flush_failed=true;
  throw std::runtime_error("Actual whole-flush recursive observation or failed owner");}
 storage->owned_flush_active=true;
 struct Active final {Impl& i;~Active(){i.owned_flush_active=false;}} active{*storage};
 OwnedFlushState initial(paint,metadata,self,pin,cache,storage->pair.get(),storage->frame);
 initial.require_current_without_callbacks();
 try {
  // Genuine caller performs its native dot-prefix BEFORE this midphase entry.
  // This is the LAST original Borrower facade read, before its first callback.
  // Paint is a captured Guard VALUE; the original Paint facade is never read.
  initial.prepared=borrower.prepare_label_text_run_owned(operation,input,initial);
  initial.require_current_without_callbacks();initial.full(operation);initial.require_current_without_callbacks();
  auto state=std::make_shared<OwnedFlushState>(initial);initial.require_current_without_callbacks();
  state->full(operation);state->require_current_without_callbacks();
  using Receipt=ui::detail::BnwLabelPaintPreparedFlushLoan;
  auto receipt_data=std::make_shared<Receipt::Impl>(state);state->require_current_without_callbacks();
  auto result=std::unique_ptr<Receipt>(new Receipt(Receipt::IssuerKey{},receipt_data));
  state->require_current_without_callbacks();state->full(operation);state->require_current_without_callbacks();
  {
   state->slot_count=cache->native_slot_count_without_callbacks();state->require_current_without_callbacks();
   state->physical_slots.reserve(state->slot_count);state->require_current_without_callbacks();
   std::vector<ui::BnwSystemFontSlotQuote> pending;
   pending.reserve(state->slot_count);state->require_current_without_callbacks();
   for(std::size_t n=0;n<state->slot_count;++n){
    state->require_current_without_callbacks();const auto q=cache->native_slot_quote_without_callbacks(n);
    state->physical_slots.push_back(q);
    if(!q.slot.raster_uploaded)pending.push_back(q);
    state->require_current_without_callbacks();
   }
   state->full(operation);state->require_current_without_callbacks();
   if(!pending.empty()){
    need(storage->frame.revision!=UINT64_MAX,"Owned whole-flush Pair revision exhausted");
    std::map<std::size_t,std::uint64_t> journal;
    // Each map insertion has exactly one allocation followed by bottom FIRST.
    for(const auto& entry:storage->uploaded){state->require_current_without_callbacks();
     journal.emplace(entry.first,entry.second);state->require_current_without_callbacks();}
    for(const auto& q:pending){state->require_current_without_callbacks();
     journal.insert_or_assign(q.slot_index,q.slot.generation);state->require_current_without_callbacks();}
    auto next=std::make_shared<Pair>();state->require_current_without_callbacks();next->context_pin=pin;
    const auto& context=pin->context_reference_without_callbacks();state->require_current_without_callbacks();
    const auto& gl=context.api();state->require_current_without_callbacks();
    next->retirement=detail::BnwOwnedGlRetirementNode::texture_pair_owned(pin,gl);
    state->require_current_without_callbacks();
    auto& names=next->textures_without_callbacks();
    GLint maximum=0;state->check_native(gl);state->require_current_without_callbacks();gl.GetIntegerv(GL_MAX_TEXTURE_SIZE,&maximum);
    state->require_current_without_callbacks();state->check_native(gl);
    need(state->expected.width>0&&state->expected.height>0&&state->expected.width<=maximum&&state->expected.height<=maximum,
     "Owned whole-flush actual Pair geometry exceeds native limits");
    state->require_current_without_callbacks();gl.CreateTextures(GL_TEXTURE_2D,2,names.data());
    state->require_current_without_callbacks();state->check_native(gl);need(names[0]&&names[1],"Owned whole-flush actual Pair creation failed");
    const std::array<GLint,4> swizzle{GL_RED,GL_RED,GL_RED,GL_ONE};
    for(const auto name:names){
     state->require_current_without_callbacks();gl.TextureStorage2D(name,1,GL_R8,state->expected.width,state->expected.height);state->require_current_without_callbacks();state->check_native(gl);
     gl.TextureParameteri(name,GL_TEXTURE_BASE_LEVEL,0);state->require_current_without_callbacks();state->check_native(gl);
     gl.TextureParameteri(name,GL_TEXTURE_MAX_LEVEL,0);state->require_current_without_callbacks();state->check_native(gl);
     gl.TextureParameteriv(name,GL_TEXTURE_SWIZZLE_RGBA,swizzle.data());state->require_current_without_callbacks();state->check_native(gl);
    }
    state->check_native(gl);state->full(operation);state->require_current_without_callbacks();
    state->check_native(gl);auto raw_copy=wglGetProcAddress("glCopyImageSubData");state->require_current_without_callbacks();state->check_native(gl);
    const auto bits=reinterpret_cast<std::uintptr_t>(raw_copy);
    if(!bits||bits==1||bits==2||bits==3||bits==~std::uintptr_t{0}){
     const auto module=GetModuleHandleW(L"opengl32.dll");state->require_current_without_callbacks();
     raw_copy=GetProcAddress(module,"glCopyImageSubData");state->require_current_without_callbacks();state->check_native(gl);
    }
    const auto copy=reinterpret_cast<PFNGLCOPYIMAGESUBDATAPROC>(raw_copy);
    need(copy,"Owned whole-flush actual Pair copy dispatch missing");
    for(std::size_t plane=0;plane<2;++plane){
     state->check_native(gl);state->require_current_without_callbacks();
     copy(storage->pair->textures_without_callbacks()[plane],GL_TEXTURE_2D,0,0,0,0,
      names[plane],GL_TEXTURE_2D,0,0,0,0,state->expected.width,state->expected.height,1);
     state->require_current_without_callbacks();state->check_native(gl);
    }
    state->full(operation);state->require_current_without_callbacks();
    // Preallocated Node owns captured unpack state and physical restore custody.
    // The lexical destructor ONLY enqueues; no raw native/Context access remains.
    struct Unpack final {
     using Node=detail::BnwOwnedGlRetirementNode;
     const GlApi& gl;const OwnedFlushState& state;
     Node::Owner retirement;
     const std::array<GLenum,12>& fields=Node::pixel_unpack_fields_;
     Unpack(const GlApi& g,const OwnedFlushState& s,std::shared_ptr<const detail::BnwRendererContextPin> token)
      :gl(g),state(s),retirement(Node::PixelUnpackChannel::pixel_unpack_restore_owned(std::move(token),g)){
      state.require_current_without_callbacks();
     }
     void begin(){
      auto& saved=Node::PixelUnpackChannel::storage(*retirement);
      state.check_native(gl);state.require_current_without_callbacks();
      gl.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&saved.buffer);
      state.require_current_without_callbacks();state.check_native(gl);saved.buffer_captured=true;
      for(std::size_t n=0;n<fields.size();++n){
       state.require_current_without_callbacks();gl.GetIntegerv(fields[n],&saved.saved[n]);
       state.require_current_without_callbacks();state.check_native(gl);saved.captured=n+1;
      }
      // Flags are armed BEFORE effects, and remain armed through any failed POST.
      state.require_current_without_callbacks();saved.buffer_changed=true;gl.BindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
      state.require_current_without_callbacks();state.check_native(gl);
      for(std::size_t n=0;n<fields.size();++n){
       state.require_current_without_callbacks();saved.changed=n+1;gl.PixelStorei(fields[n],n?0:1);
       state.require_current_without_callbacks();state.check_native(gl);
      }
     }
     void restore(){
      state.require_current_without_callbacks();
      const auto status=Node::PixelUnpackChannel::restore(*retirement);
      state.require_current_without_callbacks();
      need(status.complete&&!status.cleanup_failure,
       "Owned whole-flush physical unpack restore pending/uncertain/failed");
      state.check_native(gl);state.require_current_without_callbacks();
     }
     // Owner deleter hands off to creator queue without allocation or effects.
    };
    for(const auto& q:pending){
     state->processing=q;state->require_current_without_callbacks();
     {
      const auto stage=cache->stage_native_slot_raster_owned(q,*state);state->require_current_without_callbacks();
      state->full(operation);state->require_current_without_callbacks();
      const auto width=stage.right-stage.left,height=stage.bottom-stage.top;
      need(width>=0&&height>0&&stage.left>=0&&stage.top>=0&&stage.right<=state->expected.width&&stage.bottom<=state->expected.height&&
       stage.cpu.green_brightness.size()==static_cast<std::size_t>(width)*height&&
       stage.cpu.shadow_brightness.size()==stage.cpu.green_brightness.size(),"Owned whole-flush actual native raster bounds/bytes disagree");
      state->require_current_without_callbacks();
      Unpack transfer(gl,*state,pin);transfer.begin();
      const std::uint8_t dummy=0xff; // Real width 0 remains width 0, BOTH effects still issue.
      const auto green=stage.cpu.green_brightness.empty()?&dummy:stage.cpu.green_brightness.data();
      const auto shadow=stage.cpu.shadow_brightness.empty()?&dummy:stage.cpu.shadow_brightness.data();
      state->require_current_without_callbacks();gl.TextureSubImage2D(names[0],0,stage.left,stage.top,width,height,GL_RED,GL_UNSIGNED_BYTE,green);
      state->require_current_without_callbacks();state->check_native(gl);
      state->require_current_without_callbacks();gl.TextureSubImage2D(names[1],0,stage.left,stage.top,width,height,GL_RED,GL_UNSIGNED_BYTE,shadow);
      state->require_current_without_callbacks();state->check_native(gl);
      transfer.restore();state->check_native(gl);
     } // Raster bytes and transfer target cleanup BEFORE independent FULL POST.
     state->require_current_without_callbacks();state->full(operation);state->require_current_without_callbacks();
     state->processing.reset();
    }
    state->full(operation);state->require_current_without_callbacks();
    need(state->physical_slots.size()==state->slot_count&&cache->native_slot_count_without_callbacks()==state->slot_count,
     "Owned whole-flush complete physical slot count changed");
    // Complete generation/glyph/A+B/nine-DWORD and pending-bit rescan is pure.
    // No requested-map or stale quote raster gate participates in this commit.
    for(const auto& q:state->physical_slots){cache->require_native_slot_current_without_callbacks(q);
     const auto actual=cache->native_slot_quote_without_callbacks(q.slot_index);
     need(actual.slot.raster_uploaded==q.slot.raster_uploaded,"Owned whole-flush pending set changed before commit");}
    state->require_current_without_callbacks();
    // Callback-free, noalloc publication: live Pair, journal, flags and revision once.
    storage->pair.swap(next);storage->uploaded.swap(journal);
    storage->frame.green_texture=names[0];storage->frame.shadow_texture=names[1];++storage->frame.revision;
    for(auto& q:state->physical_slots)if(!q.slot.raster_uploaded)q=cache->publish_uploaded(q);
    state->accepted=storage->pair.get();state->expected=storage->frame;
    next.reset(); // Last SAME Pair owner enqueues; historical consumers retain the original Node.
   }else{
    state->full(operation);state->require_current_without_callbacks();
    need(state->physical_slots.size()==state->slot_count&&cache->native_slot_count_without_callbacks()==state->slot_count,
     "Owned whole-flush no-pending complete slot count changed");
    for(const auto& q:state->physical_slots){
     cache->require_native_slot_current_without_callbacks(q);
     const auto actual=cache->native_slot_quote_without_callbacks(q.slot_index);
     need(actual.slot.raster_uploaded==q.slot.raster_uploaded,
      "Owned whole-flush no-pending set changed after last FULL");
    }
    state->require_current_without_callbacks();
   } // Journal/old Pair scratch cleanup precedes the drain and independent POST.
  } // Pending vector cleanup before acceptance, including the no-pending route.
  state->require_current_without_callbacks();
  require_owned_pair_retirement_snapshot_complete(pin);
  state->require_current_without_callbacks();state->full(operation);state->require_current_without_callbacks();
  // Historical completed EARLY flush only. Later genuine GetGlyph may add pending
  // slots without another automatic flush; receipt grants no raster/membership claim.
  state->drain_closed=true;storage->owned_flush_active=false;state->producing=false;state->sealed=true;
  receipt_data->first();
  return result;
 }catch(...){storage->owned_flush_failed=true;throw;}
}
} // namespace civ5::graphics
namespace civ5::graphics {
// Genuine actual170 GetGlyph has its own lexical exclusion and BOTH-input
// bottom fence. It neither prepares a run nor manufactures a flush receipt.
struct GlBnwSystemFontAtlas::OwnedGlyphState final : detail::BnwOwnedFontPaintEffectFence {
 using Paint=ui::detail::BnwLabelPaintTextLoan;
 using Metadata=detail::BnwLabelSystemFontAtlasMetadata;
 const Paint::Guard paint;
 const Metadata::Guard metadata;
 const std::weak_ptr<GlBnwSystemFontAtlas> atlas;
 const std::weak_ptr<const detail::BnwRendererContextPin> pin;
 const std::weak_ptr<ui::BnwSystemFontCacheOwner> cache;
 const std::thread::id creator=std::this_thread::get_id();
 Pair* const accepted;
 const GlBnwSystemFontAtlasFrame expected;
 OwnedGlyphState(Paint::Guard p,Metadata::Guard m,const std::shared_ptr<GlBnwSystemFontAtlas>& a,
  const std::shared_ptr<const detail::BnwRendererContextPin>& token,
  const std::shared_ptr<ui::BnwSystemFontCacheOwner>& c,Pair* pair,GlBnwSystemFontAtlasFrame frame)
  :BnwOwnedFontPaintEffectFence(detail::BnwOwnedFontPaintEffectFence::IssuerKey{}),
   paint(std::move(p)),metadata(std::move(m)),atlas(a),pin(token),cache(c),accepted(pair),expected(frame){}
 void require_current_without_callbacks()const override {
  need(creator==std::this_thread::get_id(),"Owned native glyph fence foreign creator");
  paint.require_current_without_callbacks();metadata.require_current_without_callbacks();
  {
   const auto a=atlas.lock();const auto token=pin.lock();const auto c=cache.lock();
   need(a&&token&&c,"Owned native glyph physical storage expired");
   metadata.require_actual_atlas_without_callbacks(*a);
   auto& i=*a->impl_;
   need(i.context_pin&&i.cache&&!i.context&&i.pair&&i.pair->retirement,
    "Owned native glyph requires actual renderer Pair custody");
   const auto same=[](const auto& x,const auto& y){return x.get()==y.get()&&!x.owner_before(y)&&!y.owner_before(x);};
   need(same(token,i.context_pin)&&same(c,i.cache),"Owned native glyph SAME counted Pin/cache changed");
   token->first_without_callbacks();
   need(!i.owned_flush_failed&&!i.owned_flush_active&&i.owned_glyph_active,
    "Owned native glyph owner recursive observation or failure");
   need(i.pair.get()==accepted&&i.frame.revision==expected.revision&&
    i.frame.width==expected.width&&i.frame.height==expected.height&&
    i.frame.green_texture==expected.green_texture&&i.frame.shadow_texture==expected.shadow_texture&&
    i.frame.pair_storage_available==expected.pair_storage_available,
    "Owned native glyph actual Pair/revision changed");
   const auto native=token->context_reference_without_callbacks().native_identity();
   need(native.owner_thread&&native.window==i.hwnd&&native.dc&&native.context&&
    reinterpret_cast<std::uintptr_t>(wglGetCurrentContext())==native.context&&
    reinterpret_cast<std::uintptr_t>(wglGetCurrentDC())==native.dc&&
    reinterpret_cast<std::uintptr_t>(WindowFromDC(reinterpret_cast<HDC>(native.dc)))==native.window,
    "Owned native glyph rejects foreign current HWND/DC/WGL without switching");
   need(!detail::BnwOwnedGlRetirementNode::sticky_cleanup_failure_without_callbacks(),
    "Owned native glyph physical cleanup has sticky failure");
  } // No GPU Entry/Atlas/cache/Pin weak-lock custody across Source callbacks.
  paint.require_current_without_callbacks();metadata.require_current_without_callbacks();
 }
 void require_same_paint_guard_without_callbacks(const Paint::Guard& other)const override {
  require_current_without_callbacks();
  // Native glyph fence requires the originating typed Paint owner.
  paint.require_same_paint_guard_without_callbacks(other);
  require_current_without_callbacks();
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const {
  require_current_without_callbacks();paint.require_source_owned(operation);require_current_without_callbacks();
  metadata.require_source_owned(operation);require_current_without_callbacks();
  paint.require_source_owned(operation);require_current_without_callbacks();
 }
};
std::array<std::uint32_t,9>GlBnwSystemFontAtlas::glyph_bits_native_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const ui::detail::BnwLabelSystemFontBorrower& borrower,const PaintGuard& input,std::uint32_t codepoint){
 const auto paint=input;paint.require_current_without_callbacks();
 paint.require_system_font_borrower_without_callbacks(borrower);
 const auto descriptor=borrower.atlas_metadata_without_callbacks();
 const auto metadata=descriptor->capture_guard_without_callbacks();
 const auto self=metadata.retain_atlas_for_physical_cleanup_without_callbacks();
 need(self.get()==this,"Actual native glyph Borrower names another physical Atlas");
 metadata.require_actual_atlas_without_callbacks(*self);
 auto* const storage=self->impl_.get();
 const auto pin=storage->context_pin;const auto cache=storage->cache;
 need(pin&&cache&&storage->pair&&storage->pair->retirement,"Actual native glyph requires owned renderer Atlas");
 pin->first_without_callbacks();
 if(storage->owned_glyph_active||storage->owned_flush_active||storage->owned_flush_failed){
  storage->owned_flush_failed=true;throw std::runtime_error("Actual native glyph recursive observation or failed owner");
 }
 storage->owned_glyph_active=true;
 struct Active final {Impl& i;~Active(){i.owned_glyph_active=false;}} active{*storage};
 OwnedGlyphState state(paint,metadata,self,pin,cache,storage->pair.get(),storage->frame);
 state.require_current_without_callbacks();
 try{
  // LAST original Borrower facade read before its first Source callback.
  // Input is our captured Guard VALUE, never the original Paint facade.
  const auto data=borrower.glyph_data_owned(operation,paint,codepoint,state);
  state.require_current_without_callbacks();state.full(operation);state.require_current_without_callbacks();
  cache->require_native_glyph_current_without_callbacks(data);state.require_current_without_callbacks();
  return data.words;
 }catch(...){storage->owned_flush_failed=true;throw;}
}
} // namespace civ5::graphics

namespace civ5::graphics {
struct GlBnwSystemFontAtlas::AcceptedFontPlanes::Storage final {
 using Data=::civ5::ui::detail::BnwLabelPaintPositionedData;
 using Paint=::civ5::ui::detail::BnwLabelPaintTextLoan;
 using Early=::civ5::ui::detail::BnwLabelPaintPreparedFlushLoan;
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> alive{true};
 const Data::Guard data;
 const Paint::Guard paint;
 const Early::Guard early;
 const std::shared_ptr<GlBnwSystemFontAtlas> atlas;
 const std::shared_ptr<const detail::BnwRendererContextPin> pin;
 const std::shared_ptr<Pair> retained_pair;
 Pair* const pair_identity;
 const detail::BnwOwnedGlRetirementNode* const node_identity;
 const GlBnwSystemFontAtlasFrame expected;
 const std::array<GLuint,2> names;
 Storage(Data::Guard d,Paint::Guard p,Early::Guard e,std::shared_ptr<GlBnwSystemFontAtlas> a,
  std::shared_ptr<const detail::BnwRendererContextPin> token,std::shared_ptr<Pair> pair,
  const detail::BnwOwnedGlRetirementNode* node,GlBnwSystemFontAtlasFrame frame,std::array<GLuint,2> textures)
  :data(std::move(d)),paint(std::move(p)),early(std::move(e)),atlas(std::move(a)),
   pin(std::move(token)),retained_pair(std::move(pair)),pair_identity(retained_pair.get()),node_identity(node),expected(frame),names(textures){first();}
 static bool same_frame(const GlBnwSystemFontAtlasFrame& a,const GlBnwSystemFontAtlasFrame& b)noexcept{
  return a.green_texture==b.green_texture&&a.shadow_texture==b.shadow_texture&&
   a.width==b.width&&a.height==b.height&&a.revision==b.revision&&
   a.pair_storage_available==b.pair_storage_available&&a.shader_draw_available==b.shader_draw_available&&
   a.native_label_initialized==b.native_label_initialized;
 }
 void first()const{
  need(creator==std::this_thread::get_id()&&alive.load(),"Accepted font plane provider retired/foreign");
  data.require_current_without_callbacks();paint.require_current_without_callbacks();
  data.require_same_paint_guard_without_callbacks(paint);early.require_same_paint_guard_without_callbacks(paint);
  need(atlas&&pin,"Accepted font plane physical custody absent");
  const auto receipt_pair=early.accepted_pair_without_callbacks(*atlas);
  need(same_frame(receipt_pair,expected),"Accepted font plane historical receipt Pair changed");
  pin->first_without_callbacks();
  const auto& actual=*atlas->impl_;
  need(actual.context_pin&&actual.pair&&!actual.context&&actual.pair->context_pin&&
   !actual.pair->context&&actual.pair.get()==pair_identity&&actual.pair->retirement&&
   actual.pair->retirement.get()==node_identity,"Accepted font plane Pair/Node custody changed");
  const auto same=[&](const auto& other){return pin.get()==other.get()&&
   !pin.owner_before(other)&&!other.owner_before(pin);};
  need(same(actual.context_pin)&&same(actual.pair->context_pin),"Accepted font plane SAME Pin/control changed");
  need(retained_pair&&actual.pair.get()==retained_pair.get()&&
   !actual.pair.owner_before(retained_pair)&&!retained_pair.owner_before(actual.pair),
   "Accepted font plane SAME original Pair shared control changed");
  need(same_frame(actual.frame,expected)&&actual.pair->textures_without_callbacks()==names&&
   names[0]==expected.green_texture&&names[1]==expected.shadow_texture&&names[0]&&names[1],
   "Accepted font plane physical names/frame changed");
  // Existing receipt FIRST proves canonical Atlas control/current Pair.
  // No later glyph/current-upload/raster/journal gate participates.
  pin->first_without_callbacks();early.require_current_without_callbacks();
  paint.require_current_without_callbacks();data.require_current_without_callbacks();
  need(alive.load(),"Accepted font plane provider retired at POST");
 }
};
GlBnwSystemFontAtlas::AcceptedFontPlanes::AcceptedFontPlanes(std::shared_ptr<const Storage> actual)
 :storage_(std::move(actual)){storage_->first();}
GlBnwSystemFontAtlas::AcceptedFontPlanes::~AcceptedFontPlanes(){
 if(storage_->creator!=std::this_thread::get_id())std::terminate();
 storage_->alive.store(false);
}
void GlBnwSystemFontAtlas::AcceptedFontPlanes::require_current_without_callbacks()const{
 const auto saved=storage_;saved->first();
}
void GlBnwSystemFontAtlas::AcceptedFontPlanes::require_actual_atlas_without_callbacks(const GlBnwSystemFontAtlas& input)const{
 const auto* const identity=&input;const auto saved=storage_;saved->first();
 need(saved->atlas.get()==identity,"Accepted font plane provider another Atlas");saved->first();
}
GlBnwSystemFontAtlasFrame GlBnwSystemFontAtlas::AcceptedFontPlanes::accepted_pair_without_callbacks()const{
 const auto saved=storage_;saved->first();const auto result=saved->expected;saved->first();return result;
}
GlBnwSystemFontAtlas::AcceptedFontPlanes::SelectedStages
GlBnwSystemFontAtlas::AcceptedFontPlanes::selected_stages_without_callbacks(unsigned pass)const{
 const auto saved=storage_;const auto actual_pass=pass;saved->first();need(actual_pass<3,"Accepted font plane pass out of range");
 const auto chosen=saved->names[actual_pass==2?1:0];const SelectedStages result{{chosen,chosen},9u};saved->first();return result;
}
std::unique_ptr<GlBnwSystemFontAtlas::AcceptedFontPlanes>
GlBnwSystemFontAtlas::capture_accepted_font_planes_owned(const ::civ5::ui::detail::BnwLabelPaintPositionedData& input){
 std::unique_ptr<AcceptedFontPlanes> result;
 {
  // ONLY original DATA facade read precedes every allocator effect.
  const auto data=input.capture_guard_without_callbacks();data.require_current_without_callbacks();
  const auto paint=data.captured_paint_guard_without_callbacks();
  const auto early=data.captured_early_flush_guard_without_callbacks();
  const auto receipt=early.impl_;const auto receipt_state=receipt->state;const auto canonical=receipt_state->atlas;
  const auto first=[&]{data.require_current_without_callbacks();paint.require_current_without_callbacks();
   data.require_same_paint_guard_without_callbacks(paint);early.require_same_paint_guard_without_callbacks(paint);
   data.require_current_without_callbacks();};
  first();const auto atlas=canonical.lock();need(bool(atlas),"Accepted font plane canonical Atlas retired");first();
  const auto expected=early.accepted_pair_without_callbacks(*atlas);first();
  const auto pin=atlas->impl_->context_pin;const auto pair=atlas->impl_->pair;
  need(pin&&pair&&pair->retirement,"Accepted font plane requires actual owned Pair/Node");first();
  const auto* const node=pair->retirement.get();const auto names=pair->textures_without_callbacks();first();
  first();std::unique_ptr<AcceptedFontPlanes::Storage> provisional(new AcceptedFontPlanes::Storage(
   data,paint,early,atlas,pin,pair,node,expected,names));first();provisional->first();
  first();std::shared_ptr<const AcceptedFontPlanes::Storage> frozen(std::move(provisional));first();frozen->first();
  frozen->first();result=std::unique_ptr<AcceptedFontPlanes>(new AcceptedFontPlanes(frozen));frozen->first();first();
 } // Captured factory locals cleanup before final independent FIRST.
 result->require_current_without_callbacks();return result;
}
// Supplier-owned historical physical custody. No active item lease is renewed.
struct GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard::Impl final {
 using Data=::civ5::ui::detail::BnwLabelPaintPositionedData;
 using Node=detail::BnwOwnedGlRetirementNode;
 const std::thread::id creator=std::this_thread::get_id();
 const Data::Guard data;
 const std::shared_ptr<GlBnwSystemFontAtlas> atlas;
 const std::shared_ptr<const detail::BnwRendererContextPin> pin;
 const std::shared_ptr<Pair> retained_pair;
 Pair* const pair_identity;
 const Node* const node_identity;
 const GlBnwSystemFontAtlasFrame expected;
 const std::array<GLuint,2> names;
 Impl(Data::Guard d,std::shared_ptr<GlBnwSystemFontAtlas> a,
  std::shared_ptr<const detail::BnwRendererContextPin> token,std::shared_ptr<Pair> pair,
  const Node* node,GlBnwSystemFontAtlasFrame frame,std::array<GLuint,2> textures)
  :data(std::move(d)),atlas(std::move(a)),pin(std::move(token)),retained_pair(std::move(pair)),
   pair_identity(retained_pair.get()),node_identity(node),expected(frame),names(textures){first();}
 template<class A,class B> static bool same(const A& a,const B& b)noexcept{
  return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
 }
 void first()const{
  // Creator refusal precedes every creator-only Pair/Node field read.
  need(creator==std::this_thread::get_id(),"Historical font planes foreign creator");
  need(data.impl_&&atlas&&pin&&retained_pair,"Historical font planes original custody absent");
  pin->first_without_callbacks();
  need(retained_pair.get()==pair_identity&&!retained_pair->context&&retained_pair->context_pin&&
   same(pin,retained_pair->context_pin)&&retained_pair->retirement&&
   retained_pair->retirement.get()==node_identity,"Historical font planes SAME Pair/Node/Pin custody changed");
  const auto& node=*retained_pair->retirement;
  need(node.creator_==creator&&node.kind_==Node::Kind::texture_pair&&node.pin_&&same(pin,node.pin_)&&
   !node.queue_custody_&&!node.quarantined_&&!node.deletion_in_flight_&&!node.next_&&
   node.failure_==Node::Failure::none,"Historical font planes original Node queued/quarantined/deleting/failed");
  need(node.pair_.textures==names&&names[0]&&names[1]&&names[0]==expected.green_texture&&
   names[1]==expected.shadow_texture&&expected.width>0&&expected.height>0&&expected.pair_storage_available,
   "Historical font planes retained original names/geometry changed");
  // Do not inspect Atlas current Pair/frame/revision, DATA/Paint/Early FIRST,
  // source-cache/raster journals, or WGL. Root owns current consumer health.
  pin->first_without_callbacks();
 }
};
GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard::HistoricalGuard(
 std::shared_ptr<const Impl> actual,const ConstructionKey&):impl_(std::move(actual)){
 need(bool(impl_),"Historical font planes missing immutable supplier control");impl_->first();
}
GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard::~HistoricalGuard()=default;
void GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard::require_physical_current_without_callbacks()const{
 const auto saved=impl_;need(bool(saved),"Historical font planes empty control");saved->first();
}
void GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard::require_actual_atlas_without_callbacks(
 const GlBnwSystemFontAtlas& input)const{
 const auto*const actual=&input;const auto saved=impl_;saved->first();
 need(saved->atlas.get()==actual,"Historical font planes another actual Atlas");saved->first();
}
void GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard::require_actual_positioned_data_without_callbacks(
 const ::civ5::ui::detail::BnwLabelPaintPositionedData::Guard& input)const{
 const auto actual=input.impl_;const auto saved=impl_;saved->first();
 need(actual&&Impl::same(actual,saved->data.impl_),"Historical font planes another original DATA/control");
 saved->first();
}
void GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard::require_same_accepted_planes_without_callbacks(
 const AcceptedFontPlanes& input)const{
 // Snapshot the actual original plane storage before any check. Its facade
 // lifetime remains separate: a retained capsule cannot resurrect alive=false.
 const auto planes=input.storage_;const auto saved=impl_;saved->first();
 need(planes&&planes->creator==saved->creator&&planes->alive.load(),
  "Historical font planes original AcceptedPlanes retired/foreign");
 need(Impl::same(planes->data.impl_,saved->data.impl_)&&Impl::same(planes->atlas,saved->atlas)&&
  Impl::same(planes->pin,saved->pin)&&Impl::same(planes->retained_pair,saved->retained_pair)&&
  planes->pair_identity==saved->pair_identity&&planes->node_identity==saved->node_identity&&
  planes->names==saved->names&&Storage::same_frame(planes->expected,saved->expected),
  "Historical font planes original AcceptedPlanes DATA/Atlas/Pair/Node/frame association differs");
 saved->first();need(planes->alive.load(),"Historical font planes original AcceptedPlanes retired at POST");
}
GlBnwSystemFontAtlas::AcceptedFontPlanes::SelectedStages
GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard::selected_stages_without_callbacks(unsigned pass)const{
 const auto saved=impl_;const auto actual_pass=pass;saved->first();
 need(actual_pass<3,"Historical font plane pass out of range");
 const auto chosen=saved->names[actual_pass==2?1:0];const SelectedStages result{{chosen,chosen},9u};
 saved->first();return result;
}
GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard
GlBnwSystemFontAtlas::capture_historical_font_planes_owned(
 const HistoricalPlanesIssuerKey&,const runtime::BnwWorldViewSourceOperationLoan& operation,
 const ::civ5::ui::detail::BnwLabelPaintPositionedData& input,
 const std::shared_ptr<GlBnwSystemFontAtlas>& actual_atlas){
 using Historical=AcceptedFontPlanes::HistoricalGuard;using Node=detail::BnwOwnedGlRetirementNode;
 // LAST original facade/borrowed shared operand reads precede FULL callbacks
 // and every allocator effect. All later inputs are captured owning VALUES.
 const auto atlas=actual_atlas;const auto data=input.capture_guard_without_callbacks();
 data.require_current_without_callbacks();
 const auto paint=data.captured_paint_guard_without_callbacks();
 const auto early=data.captured_early_flush_guard_without_callbacks();
 const auto active_first=[&]{
  data.require_current_without_callbacks();paint.require_current_without_callbacks();
  data.require_same_paint_guard_without_callbacks(paint);early.require_same_paint_guard_without_callbacks(paint);
  data.require_current_without_callbacks();
 };
 const auto active_full=[&]{active_first();data.require_source_owned(operation);active_first();
  paint.require_source_owned(operation);active_first();early.require_source_owned(operation);active_first();};
 active_first();need(bool(atlas),"Historical font plane capture missing actual Atlas");
 const auto receipt=early.impl_;const auto state=receipt->state;const auto canonical=state->atlas.lock();
 active_first();need(canonical&&Historical::Impl::same(canonical,atlas),
  "Historical font plane capture another canonical Atlas/control");
 const auto expected=early.accepted_pair_without_callbacks(*atlas);active_first();
 const auto pin=atlas->impl_->context_pin;const auto pair=atlas->impl_->pair;
 active_first();need(pin&&pair&&pair->retirement,"Historical font plane capture missing genuine owned Pair");
 const auto*const node=pair->retirement.get();
 // Read actual Node names only after original active custody and physical kind
 // have been admitted. No mutable texture getter or native query participates.
 need(node->creator_==std::this_thread::get_id()&&node->kind_==Node::Kind::texture_pair&&
  node->pin_&&Historical::Impl::same(pin,node->pin_)&&!node->queue_custody_&&!node->quarantined_&&
  !node->deletion_in_flight_&&!node->next_&&node->failure_==Node::Failure::none,
  "Historical font plane capture actual Node custody invalid");
 const auto names=node->pair_.textures;
 const auto first=[&]{
  active_first();pin->first_without_callbacks();
  const auto& current=*atlas->impl_;
  need(current.thread==std::this_thread::get_id()&&!current.context&&current.context_pin&&
   Historical::Impl::same(current.context_pin,pin)&&Historical::Impl::same(current.pair,pair)&&
   !pair->context&&pair->context_pin&&Historical::Impl::same(pair->context_pin,pin)&&
   pair->retirement.get()==node&&AcceptedFontPlanes::Storage::same_frame(current.frame,expected),
   "Historical font plane ACTIVE original Pair/control/frame changed");
  need(node->creator_==std::this_thread::get_id()&&node->kind_==Node::Kind::texture_pair&&
   node->pin_&&Historical::Impl::same(pin,node->pin_)&&!node->queue_custody_&&!node->quarantined_&&
   !node->deletion_in_flight_&&!node->next_&&node->failure_==Node::Failure::none&&node->pair_.textures==names&&
   names[0]&&names[1]&&names[0]==expected.green_texture&&names[1]==expected.shadow_texture,
   "Historical font plane ACTIVE physical Node/names changed");
  const auto repeated=early.accepted_pair_without_callbacks(*atlas);
  need(AcceptedFontPlanes::Storage::same_frame(repeated,expected),
   "Historical font plane ACTIVE accepted commit frame changed");active_first();
 };
 const auto full=[&]{first();active_full();first();};
 first();full();first();
 std::unique_ptr<Historical::Impl> provisional(new Historical::Impl(data,atlas,pin,pair,node,expected,names));
 first();full();first();provisional->first();
 first();full();first();std::shared_ptr<const Historical::Impl> frozen(std::move(provisional));
 first();full();first();frozen->first();
 const Historical::ConstructionKey key;Historical result(frozen,key);
 first();full();first();result.require_physical_current_without_callbacks();
 return result;
}
} // namespace civ5::graphics
