#include "civ5/ui/bnw_system_font_cache_owner.hpp"
#include "civ5/graphics/detail/bnw_owned_font_paint_effect_fence.hpp"
#include <limits>
#include <bit>
#include <type_traits>
#include <algorithm>
#include "civ5/ui/detail/bnw_system_font_native_run_data.hpp"
namespace civ5::ui {
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
struct Operation {
 bool& active;
 explicit Operation(bool& value,bool& query,std::thread::id thread):active(value){need(std::this_thread::get_id()==thread,"Native glyph cache foreign thread refused");need(!active&&!query,"Native glyph cache callback reentry refused");active=true;}
 ~Operation(){active=false;}
};
struct Query {
 bool& active;
 explicit Query(bool& value,std::thread::id thread):active(value){
  need(std::this_thread::get_id()==thread,"Native glyph cache source query used on foreign thread");
  need(!active,"Native glyph cache source query callback reentry refused");active=true;
 }
 ~Query(){active=false;}
};
BnwSystemFontCacheGeometry geometry(const BnwSystemFontCpuFrame&f){return {f.cell_width,f.cell_height,f.columns,f.rows,f.atlas_width,f.atlas_height};}
}
BnwSystemFontSlotRing::BnwSystemFontSlotRing(BnwSystemFontCacheGeometry g):geometry_(g){
 need(g.cell_width>0&&g.cell_height>1&&g.columns>0&&g.rows>0&&g.atlas_width>0&&g.atlas_width<=2048&&g.atlas_height>0&&g.atlas_height<=2048&&static_cast<std::int64_t>(g.cell_width)*g.columns==g.atlas_width&&static_cast<std::int64_t>(g.cell_height)*g.rows==g.atlas_height,"Invalid native glyph-slot geometry");mapping_.fill(-1);slots_.resize(static_cast<std::size_t>(g.columns)*g.rows);
}
BnwSystemFontSlotRing::Candidate BnwSystemFontSlotRing::prepare(BnwSystemFontGlyphMeasurement measured)const{
 const auto found=mapping_[measured.glyph_index];if(found!=-1)return {{measured.glyph_index,static_cast<std::size_t>(found),slots_[found],true},0,revision_};
 need(measured.advance>=0&&measured.advance<=geometry_.cell_width&&revision_!=std::numeric_limits<std::uint64_t>::max(),"Native glyph advance/revision unavailable");Candidate c;c.revision=revision_;c.evicted=slots_[next_].glyph_index;c.quote.requested_glyph_index=measured.glyph_index;c.quote.slot_index=next_;auto&s=c.quote.slot;s.glyph_index=measured.glyph_index;s.advance=measured.advance;s.raster_uploaded=false;s.generation=revision_+1;
 // 0x4065E0 retains x87 intermediates across FST (not FSTP), then
 // rounds each stored UV to float32. Quad uses SSE float32 subtraction.
 // The original PE 0x9EB1E0 holds float32 0x3ebf7cee, not guessed 0.5.
 constexpr float bias=0.374000012874603271484375f;const long double inv_w=1.0L/geometry_.atlas_width,inv_h=1.0L/geometry_.atlas_height;
 const auto u=static_cast<long double>(geometry_.cell_width*(next_%geometry_.columns))*inv_w,v=static_cast<long double>(geometry_.cell_height*(next_/geometry_.columns)+1)*inv_h;
 s.u0=static_cast<float>(u);s.v0=static_cast<float>(v);s.u1=static_cast<float>(static_cast<long double>(measured.advance)*inv_w+u);s.v1=static_cast<float>(inv_h*(geometry_.cell_height-1)+v);s.x0=0.0f-bias;s.y0=0.0f-bias;s.x1=static_cast<float>(measured.advance)-bias;s.y1=static_cast<float>(geometry_.cell_height-1)-bias;s.advance_float=static_cast<float>(measured.advance);return c;
}
void BnwSystemFontSlotRing::publish(const Candidate&c)noexcept{
 if(c.quote.cache_hit)return;
 // Source deliberately does not invalidate mapping[0] when evicting an empty
 // or zero-index glyph. Preserve this sentinel branch, including zero alias.
 if(c.evicted!=0)mapping_[c.evicted]=-1;
 slots_[next_]=c.quote.slot;mapping_[c.quote.requested_glyph_index]=static_cast<std::int32_t>(next_);next_=(next_+1)%slots_.size();++revision_;
}
BnwSystemFontSlotQuote BnwSystemFontSlotRing::measure(BnwSystemFontGlyphMeasurement m){const auto c=prepare(m);publish(c);return c.quote;}
bool BnwSystemFontSlotRing::current(const BnwSystemFontSlotQuote&q)const noexcept{return q.slot_index<slots_.size()&&mapping_[q.requested_glyph_index]==static_cast<std::int32_t>(q.slot_index)&&slots_[q.slot_index]==q.slot;}
BnwSystemFontCacheOwner::BnwSystemFontCacheOwner(std::shared_ptr<BnwSystemFontCpuOwner>c,BnwSystemFontCacheGeometry g):cpu_(std::move(c)),ring_(g){}
std::shared_ptr<BnwSystemFontCacheOwner>BnwSystemFontCacheOwner::create(std::shared_ptr<BnwSystemFontCpuOwner>c){need(static_cast<bool>(c),"Native glyph-slot CPU owner missing");const auto g=geometry(c->current());auto owner=std::shared_ptr<BnwSystemFontCacheOwner>(new BnwSystemFontCacheOwner(std::move(c),g));(void)owner->cpu_->current();return owner;}
BnwSystemFontSlotQuote BnwSystemFontCacheOwner::measure(std::uint32_t cp){const auto retained=shared_from_this();Operation operation(operation_active_,query_active_,thread_);(void)cpu_->current();const auto m=cpu_->measure(cp);need(geometry(cpu_->current())==ring_.geometry(),"Native glyph-slot font geometry changed");const auto c=ring_.prepare(m);(void)cpu_->current();ring_.publish(c);return c.quote;}
BnwSystemFontSlotQuote BnwSystemFontCacheOwner::measure_glyph_index(std::uint16_t glyph){
 const auto retained=shared_from_this();Operation operation(operation_active_,query_active_,thread_);(void)cpu_->current();
 const auto m=cpu_->measure_glyph_index(glyph);need(geometry(cpu_->current())==ring_.geometry(),"Native glyph-index geometry changed");
 const auto c=ring_.prepare(m);(void)cpu_->current();ring_.publish(c);return c.quote;
}
BnwSystemFontRunStage BnwSystemFontCacheOwner::measure_plain_run(std::string_view borrowed){
 need(!borrowed.empty()&&borrowed.size()<=4096,"Native cache run outside bounded input budget");
 const std::string pinned_text(borrowed);const auto retained=shared_from_this();
 const std::string_view text(pinned_text);Operation operation(operation_active_,query_active_,thread_);(void)cpu_->current();
 BnwSystemFontRunStage out;out.gcp=cpu_->measure_plain_run(text);need(geometry(cpu_->current())==ring_.geometry(),"Native run font geometry changed");
 // Detached SAME ring: none of the two ensure passes or advance writes become
 // visible until every GDI query, allocation and final epoch check succeeds.
 auto staged=ring_;std::vector<std::size_t>indices;indices.reserve(out.gcp.glyphs.size());
 for(int pass=0;pass<2;++pass)for(const auto&m:out.gcp.single_lookups){const auto q=staged.measure(m);++out.ensure_calls;if(pass==1)indices.push_back(q.slot_index);}
 for(std::size_t n=0;n<indices.size();++n){auto&slot=staged.slots_[indices[n]];need(slot.glyph_index==out.gcp.glyphs[n],"Native run exceeds retained ring scope");slot.advance_float=static_cast<float>(out.gcp.dx[n]);++out.advance_writes;}
 // Later native lookup observes the FINAL glyph-keyed value, so repeated IDs
 // use the last occurrence dx. A+B/raster width, UV and quad remain unchanged.
 out.final_lookup_slots.reserve(indices.size());for(const auto&m:out.gcp.single_lookups){auto q=staged.measure(m);need(staged.current(q)&&q.slot.glyph_index==m.glyph_index,"Native run final lookup mapping changed");out.final_lookup_slots.push_back(q);}
 (void)cpu_->current();ring_.mapping_.swap(staged.mapping_);ring_.slots_.swap(staged.slots_);std::swap(ring_.next_,staged.next_);std::swap(ring_.revision_,staged.revision_);return out;
}
bool BnwSystemFontCacheOwner::current(const BnwSystemFontRunStage&borrowed)const{const auto run=borrowed;const auto retained=shared_from_this();Query query(query_active_,thread_);(void)cpu_->current();if(run.final_lookup_slots.empty()||run.final_lookup_slots.size()!=run.gcp.glyphs.size())return false;for(const auto&q:run.final_lookup_slots)if(!ring_.current(q))return false;return true;}
bool BnwSystemFontCacheOwner::measurement_current(const BnwSystemFontRunStage&borrowed)const{
 const auto run=borrowed;const auto retained=shared_from_this();Query query(query_active_,thread_);(void)cpu_->current();
 if(run.final_lookup_slots.empty()||run.final_lookup_slots.size()!=run.gcp.glyphs.size())return false;
 for(std::size_t n=0;n<run.final_lookup_slots.size();++n){
  const auto&q=run.final_lookup_slots[n];
  if(q.requested_glyph_index!=run.gcp.glyphs[n]||q.slot.glyph_index!=run.gcp.glyphs[n]||
   q.slot_index>=ring_.slots_.size()||ring_.mapping_[q.requested_glyph_index]!=static_cast<std::int32_t>(q.slot_index))return false;
  const auto&actual=ring_.slots_[q.slot_index];
  if(actual.glyph_index!=q.slot.glyph_index||actual.advance!=q.slot.advance||actual.u0!=q.slot.u0||actual.v0!=q.slot.v0||
   actual.u1!=q.slot.u1||actual.v1!=q.slot.v1||actual.x0!=q.slot.x0||actual.y0!=q.slot.y0||
   actual.x1!=q.slot.x1||actual.y1!=q.slot.y1||actual.advance_float!=q.slot.advance_float||actual.generation!=q.slot.generation)return false;
 }
 (void)cpu_->current();return true;
}
bool BnwSystemFontCacheOwner::current(const BnwSystemFontSlotQuote&borrowed)const{const auto q=borrowed;const auto retained=shared_from_this();Query query(query_active_,thread_);(void)cpu_->current();return ring_.current(q);}
std::size_t BnwSystemFontCacheOwner::capacity()const{const auto retained=shared_from_this();Query query(query_active_,thread_);(void)cpu_->current();return ring_.capacity();}
BnwSystemFontRasterStage BnwSystemFontCacheOwner::stage_raster(std::uint32_t cp,const BnwSystemFontSlotQuote&borrowed){const auto q=borrowed;const auto retained=shared_from_this();Operation operation(operation_active_,query_active_,thread_);need(current(q),"Stale native glyph-slot quote");const auto raster=cpu_->measure_and_raster(cp);need(raster.glyph_index==q.requested_glyph_index&&raster.advance==q.slot.advance,"Native glyph-slot measurement/raster disagree");BnwSystemFontRasterStage s;s.quote=q;s.cpu=raster;const auto&g=ring_.geometry();s.left=g.cell_width*static_cast<int>(q.slot_index%g.columns);s.top=g.cell_height*static_cast<int>(q.slot_index/g.columns);s.right=s.left+raster.width;s.bottom=s.top+g.cell_height;s.required_producer="SAME renderer pair of D3DFMT_L8 atlases and both upload receipts (0x5084E0), then draw owner; not uploaded";need(current(q),"Native glyph slot changed during raster staging");return s;}
BnwSystemFontRasterStage BnwSystemFontCacheOwner::stage_raster_glyph_index(const BnwSystemFontSlotQuote&borrowed){const auto q=borrowed;const auto retained=shared_from_this();Operation operation(operation_active_,query_active_,thread_);need(current(q),"Stale native glyph-slot quote");const auto raster=cpu_->raster_glyph_index(q.requested_glyph_index);need(raster.glyph_index==q.requested_glyph_index&&raster.advance==q.slot.advance,"Native glyph-slot measurement/raster disagree");BnwSystemFontRasterStage s;s.quote=q;s.cpu=raster;const auto&g=ring_.geometry();s.left=g.cell_width*static_cast<int>(q.slot_index%g.columns);s.top=g.cell_height*static_cast<int>(q.slot_index/g.columns);s.right=s.left+raster.width;s.bottom=s.top+g.cell_height;s.required_producer="SAME renderer pair of D3DFMT_L8 atlases and both upload receipts (0x5084E0), then draw owner; not uploaded";need(current(q),"Native glyph slot changed during raster staging");return s;}
namespace {
std::array<std::uint32_t,9> native_words(const BnwSystemFontGlyphSlot&s)noexcept{
 return {std::bit_cast<std::uint32_t>(s.u0),std::bit_cast<std::uint32_t>(s.v0),
         std::bit_cast<std::uint32_t>(s.u1),std::bit_cast<std::uint32_t>(s.v1),
         std::bit_cast<std::uint32_t>(s.x0),std::bit_cast<std::uint32_t>(s.y0),
         std::bit_cast<std::uint32_t>(s.x1),std::bit_cast<std::uint32_t>(s.y1),
         std::bit_cast<std::uint32_t>(s.advance_float)};
}
}
BnwSystemFontSlotQuote BnwSystemFontCacheOwner::ensure_native_ring_glyph_owned(BnwSystemFontSlotRing&ring,std::uint16_t glyph){
 // 4065E0 canonical map HIT precedes every A+B/WidthI probe. An authentic
 // zero-index mapping may alias a different actual slot glyph after eviction.
 const auto found=ring.mapping_[glyph];
 if(found!=-1){
  need(found>=0&&static_cast<std::size_t>(found)<ring.slots_.size(),"Native glyph map references unavailable actual slot");
  return {glyph,static_cast<std::size_t>(found),ring.slots_[found],true};
 }
 const auto measured=cpu_->measure_glyph_index(glyph); // SAME CPU, MISS only.
 need(measured.glyph_index==glyph&&geometry(cpu_->current())==ring.geometry(),"Native glyph MISS width/source geometry changed");
 const auto candidate=ring.prepare(measured);(void)cpu_->current();ring.publish(candidate);return candidate.quote;
}
BnwSystemFontSlotQuote BnwSystemFontCacheOwner::ensure_glyph_index_owned(std::uint16_t glyph){
 const auto retained=shared_from_this();Operation operation(operation_active_,query_active_,thread_);
 need(geometry(cpu_->current())==ring_.geometry(),"Native glyph source geometry changed");
 const auto result=ensure_native_ring_glyph_owned(ring_,glyph);
 need(geometry(cpu_->current())==ring_.geometry(),"Native glyph source changed after ensure");
 require_native_slot_current_without_callbacks(result);
 need(ring_.mapping_[result.requested_glyph_index]==static_cast<std::int32_t>(result.slot_index),"Native glyph canonical map changed after ensure");
 return result;
}
detail::BnwSystemFontNativeRunStage BnwSystemFontCacheOwner::prepare_native_gcp_owned(std::u16string_view units){
 const auto retained=shared_from_this();Operation operation(operation_active_,query_active_,thread_);
 need(geometry(cpu_->current())==ring_.geometry(),"Native GCP source geometry changed");
 auto genuine=cpu_->whole_gcp_utf16_owned(units);
 cpu_->require_native_gcp_current_without_callbacks(genuine);
 detail::BnwSystemFontNativeRunStage result{std::move(genuine),0,0};
 if(result.gcp.glyph_count()!=0){
  // Copy transactionally after genuine GCP and Source guards. Native outputs
  // are neither per-character verified nor filtered by lpOrder/glyph value.
  auto staged=ring_;std::vector<std::size_t> second_indices;
  const auto glyphs=result.gcp.glyphs();const auto dx=result.gcp.dx();
  need(glyphs.size()==result.gcp.glyph_count()&&dx.size()==glyphs.size(),"Native GCP actual returned arrays unavailable");
  second_indices.reserve(glyphs.size());
  cpu_->require_native_gcp_current_without_callbacks(result.gcp);
  need(geometry(cpu_->current())==ring_.geometry(),"Native GCP source changed during staging allocation");
  for(int pass=0;pass<2;++pass){
   for(const auto glyph:glyphs){
    cpu_->require_native_gcp_current_without_callbacks(result.gcp);
    const auto actual=ensure_native_ring_glyph_owned(staged,glyph);++result.ensure_calls;
    if(pass==1)second_indices.push_back(actual.slot_index);
   }
  }
  // 6722E0..67239F writes to the saved SECOND pass slot addresses. No extra
  // ensure or requested==actual glyph condition: aliases/evictions are native.
  for(std::size_t n=0;n<second_indices.size();++n){
   need(second_indices[n]<staged.slots_.size(),"Native GCP saved actual slot unavailable");
   staged.slots_[second_indices[n]].advance_float=static_cast<float>(dx[n]);++result.advance_writes;
  }
  cpu_->require_native_gcp_current_without_callbacks(result.gcp);
  need(geometry(cpu_->current())==ring_.geometry(),"Native GCP source changed before publication");
  ring_.mapping_.swap(staged.mapping_);ring_.slots_.swap(staged.slots_);
  std::swap(ring_.next_,staged.next_);std::swap(ring_.revision_,staged.revision_);
 } // Old ring/vector temporaries are cleaned before the final source gate.
 need(geometry(cpu_->current())==ring_.geometry(),"Native GCP source changed after staging cleanup");
 cpu_->require_native_gcp_current_without_callbacks(result.gcp);return result;
}

BnwSystemFontSlotQuote BnwSystemFontCacheOwner::ensure_native_ring_glyph_owned(
 BnwSystemFontSlotRing& ring,std::uint16_t glyph,const graphics::detail::BnwOwnedFontPaintEffectFence& fence){
 fence.require_current_without_callbacks();const auto cpu=cpu_;fence.require_current_without_callbacks();
 const auto physical=cpu->impl_;
 BnwSystemFontCpuOwner::require_native_origin_current_without_callbacks_owned(physical,physical);
 fence.require_current_without_callbacks();const auto found=ring.mapping_[glyph];
 if(found!=-1){
  need(found>=0&&static_cast<std::size_t>(found)<ring.slots_.size(),"Paint native glyph map references unavailable slot");
  const BnwSystemFontSlotQuote result{glyph,static_cast<std::size_t>(found),ring.slots_[found],true};
  fence.require_current_without_callbacks();
  BnwSystemFontCpuOwner::require_native_origin_current_without_callbacks_owned(physical,physical);
  fence.require_current_without_callbacks();return result;
 }
 // Canonical actual map HIT above NEVER probes widths. MISS uses the SAME
 // genuine measured glyph index and actual A+B native GDI branch.
 const auto measured=cpu->measure_glyph_index_owned(glyph,fence);fence.require_current_without_callbacks();
 const auto fields=cpu->native_raster_geometry_owned(fence);fence.require_current_without_callbacks();
 const BnwSystemFontCacheGeometry actual{fields[0],fields[1],fields[2],fields[3],fields[4],fields[5]};
 need(measured.glyph_index==glyph&&actual==ring.geometry(),"Paint native glyph MISS source geometry changed");
 fence.require_current_without_callbacks();const auto candidate=ring.prepare(measured);fence.require_current_without_callbacks();
 (void)cpu->native_raster_geometry_owned(fence);fence.require_current_without_callbacks();
 ring.publish(candidate);fence.require_current_without_callbacks();return candidate.quote;
}
detail::BnwSystemFontNativeRunStage BnwSystemFontCacheOwner::prepare_native_gcp_owned(
 std::u16string_view units,const graphics::detail::BnwOwnedFontPaintEffectFence& fence){
 fence.require_current_without_callbacks();const auto retained=shared_from_this();fence.require_current_without_callbacks();
 const auto cpu=retained->cpu_;fence.require_current_without_callbacks();
 const auto first=[&]{
  fence.require_current_without_callbacks();const auto physical=cpu->impl_;
  BnwSystemFontCpuOwner::require_native_origin_current_without_callbacks_owned(physical,physical);
  need(retained->cpu_.get()==cpu.get()&&!retained->cpu_.owner_before(cpu)&&!cpu.owner_before(retained->cpu_),
   "Paint native GCP SAME CPU allocation changed");fence.require_current_without_callbacks();
 };
 const auto full=[&]{
  first();const auto fields=cpu->native_raster_geometry_owned(fence);first();
  const BnwSystemFontCacheGeometry actual{fields[0],fields[1],fields[2],fields[3],fields[4],fields[5]};
  need(actual==retained->ring_.geometry(),"Paint native GCP source geometry changed");first();
 };
 // The genuine DTO constructor/move is allocation-free. No borrowed UTF16
 // access follows CPU's own fixed-stack snapshot; caller retains the owner.
 std::optional<detail::BnwSystemFontNativeRunStage> actual_result;
 {
  first();Operation operation(retained->operation_active_,retained->query_active_,retained->thread_);first();
  full();first();auto genuine=cpu->whole_gcp_utf16_owned(units,fence);first();
  cpu->require_native_gcp_current_without_callbacks(genuine);first();
  actual_result.emplace(std::move(genuine),0,0);first();auto& result=*actual_result;
  if(result.gcp.glyph_count()!=0){
  {
   const auto glyphs=result.gcp.glyphs();const auto dx=result.gcp.dx();first();
   need(glyphs.size()==result.gcp.glyph_count()&&dx.size()==glyphs.size(),"Paint native GCP returned arrays unavailable");
   // Avoid the old implicit Ring copy reading source vector bytes after a
   // new_handler callback. Allocate empty same-geometry storage, FIRST, then
   // perform the actual retained descriptor/mapping copies callback-free.
   first();const auto actual_geometry=retained->ring_.geometry_;
   BnwSystemFontSlotRing staged(actual_geometry);first();
   std::copy(retained->ring_.mapping_.begin(),retained->ring_.mapping_.end(),staged.mapping_.begin());first();
   std::copy(retained->ring_.slots_.begin(),retained->ring_.slots_.end(),staged.slots_.begin());first();
   staged.next_=retained->ring_.next_;staged.revision_=retained->ring_.revision_;first();
   std::vector<std::size_t> second_indices;first();second_indices.reserve(glyphs.size());first();
   cpu->require_native_gcp_current_without_callbacks(result.gcp);first();full();first();
   for(int pass=0;pass<2;++pass){
    for(const auto glyph:glyphs){
     first();cpu->require_native_gcp_current_without_callbacks(result.gcp);first();
     const auto actual=retained->ensure_native_ring_glyph_owned(staged,glyph,fence);first();++result.ensure_calls;
     if(pass==1){
      need(second_indices.size()<second_indices.capacity(),"Paint native second-pass capacity unavailable");first();
      second_indices.push_back(actual.slot_index);first();
     }
    }
   }
   // Native SECOND-pass addresses, ordered CVTSI2SS-equivalent float32 dx.
   // Aliased requested glyphs retain physical slot semantics; last write wins.
   for(std::size_t n=0;n<second_indices.size();++n){
    first();need(second_indices[n]<staged.slots_.size(),"Paint native saved actual slot unavailable");first();
    staged.slots_[second_indices[n]].advance_float=static_cast<float>(dx[n]);++result.advance_writes;first();
   }
   cpu->require_native_gcp_current_without_callbacks(result.gcp);first();full();first();
   retained->ring_.mapping_.swap(staged.mapping_);retained->ring_.slots_.swap(staged.slots_);first();
   std::swap(retained->ring_.next_,staged.next_);std::swap(retained->ring_.revision_,staged.revision_);first();
  } // Old Ring storage, second indices and staging views clean up before FULL.
  first();
  }
  first();full();first();cpu->require_native_gcp_current_without_callbacks(result.gcp);first();
 } // Cache operation and moved CPU DTO temporaries clean up BEFORE final FULL.
 first();full();first();cpu->require_native_gcp_current_without_callbacks(actual_result->gcp);first();
 return std::move(*actual_result);
}
static_assert(std::is_nothrow_move_constructible_v<detail::BnwSystemFontNativeRunStage>,
 "Paint native GCP result return must not allocate");

detail::BnwSystemFontNativeGlyphData BnwSystemFontCacheOwner::get_native_glyph_owned(std::uint32_t codepoint){
 const auto retained=shared_from_this();Operation operation(operation_active_,query_active_,thread_);
 need(geometry(cpu_->current())==ring_.geometry(),"Native glyph lookup source geometry changed");
 detail::BnwSystemFontNativeGlyphData result;
 {
  // 550330 truncates the decoded uint32 codepoint to one real WCHAR.
  auto mapped=cpu_->map_wchar_to_glyph_index_owned(static_cast<std::uint16_t>(codepoint));
  cpu_->require_native_glyph_index_current_without_callbacks(mapped);
  result.slot=ensure_native_ring_glyph_owned(ring_,mapped.glyph_index());
  cpu_->require_native_glyph_index_current_without_callbacks(mapped);
  result.words=native_words(result.slot.slot);
 } // Mapped producer DTO cleanup before Source FULL and quote POST.
 need(geometry(cpu_->current())==ring_.geometry(),"Native glyph lookup source changed after cleanup");
 require_native_glyph_current_without_callbacks(result);return result;
}

detail::BnwSystemFontNativeGlyphData BnwSystemFontCacheOwner::get_native_glyph_owned(
 std::uint32_t codepoint,const graphics::detail::BnwOwnedFontPaintEffectFence& fence){
 fence.require_current_without_callbacks();const auto retained=shared_from_this();fence.require_current_without_callbacks();
 const auto cpu=retained->cpu_;fence.require_current_without_callbacks();
 const auto first=[&]{
  fence.require_current_without_callbacks();const auto physical=cpu->impl_;
  BnwSystemFontCpuOwner::require_native_origin_current_without_callbacks_owned(physical,physical);
  need(retained->cpu_.get()==cpu.get()&&!retained->cpu_.owner_before(cpu)&&!cpu.owner_before(retained->cpu_),
   "Paint glyph lookup actual CPU control changed");fence.require_current_without_callbacks();
 };
 const auto full=[&]{
  first();const auto fields=cpu->native_raster_geometry_owned(fence);first();
  const BnwSystemFontCacheGeometry actual{fields[0],fields[1],fields[2],fields[3],fields[4],fields[5]};
  need(actual==retained->ring_.geometry(),"Paint glyph lookup actual source geometry changed");first();
 };
 detail::BnwSystemFontNativeGlyphData result;
 first();
 {
  Operation operation(retained->operation_active_,retained->query_active_,retained->thread_);first();full();first();
  {
   // Native550330 truncates uint32 to a real WCHAR; genuine CPU DTO remains
   // same-origin. Existing fenced ensure performs canonical HIT before widths.
   auto mapped=cpu->map_wchar_to_glyph_index_owned(static_cast<std::uint16_t>(codepoint),fence);first();
   cpu->require_native_glyph_index_current_without_callbacks(mapped);first();
   result.slot=retained->ensure_native_ring_glyph_owned(retained->ring_,mapped.glyph_index(),fence);first();
   cpu->require_native_glyph_index_current_without_callbacks(mapped);first();
   result.words=native_words(result.slot.slot);first();
  } // Mapped producer DTO is gone before actual Source FULL and slot POST.
  first();full();first();retained->require_native_glyph_current_without_callbacks(result);first();
 } // Cache Operation cleanup precedes last FULL and final physical nine-word POST.
 first();full();first();retained->require_native_glyph_current_without_callbacks(result);first();return result;
}

void BnwSystemFontCacheOwner::require_native_slot_current_without_callbacks(const BnwSystemFontSlotQuote&q)const{
 need(std::this_thread::get_id()==thread_,"Native physical slot quote foreign creator");
 need(q.slot_index<ring_.slots_.size(),"Native physical slot quote index unavailable");
 const auto&actual=ring_.slots_[q.slot_index];
 need(actual.generation==q.slot.generation&&actual.glyph_index==q.slot.glyph_index&&
      actual.advance==q.slot.advance&&native_words(actual)==native_words(q.slot),
      "Native physical slot generation/descriptor changed");
 // Requested mapping and raster flag are separate observations. Neither may
 // erase the authentic zero alias or invalidate metrics after a genuine upload.
}
void BnwSystemFontCacheOwner::require_native_glyph_current_without_callbacks(const detail::BnwSystemFontNativeGlyphData&data)const{
 require_native_slot_current_without_callbacks(data.slot);
 need(ring_.mapping_[data.slot.requested_glyph_index]==static_cast<std::int32_t>(data.slot.slot_index)&&
      native_words(ring_.slots_[data.slot.slot_index])==data.words,"Native glyph map/nine DWORD descriptor changed");
}
std::size_t BnwSystemFontCacheOwner::native_slot_count_without_callbacks()const{
 need(std::this_thread::get_id()==thread_,"Native physical slot enumeration foreign creator");return ring_.slots_.size();
}
std::optional<BnwSystemFontSlotQuote>BnwSystemFontCacheOwner::pending_native_slot_quote_without_callbacks(std::size_t index)const{
 need(std::this_thread::get_id()==thread_,"Native pending slot enumeration foreign creator");
 if(index>=ring_.slots_.size()||ring_.slots_[index].raster_uploaded)return std::nullopt;
 const auto&actual=ring_.slots_[index];return BnwSystemFontSlotQuote{actual.glyph_index,index,actual,false};
}
BnwSystemFontRasterStage BnwSystemFontCacheOwner::stage_native_slot_raster_owned(const BnwSystemFontSlotQuote&borrowed){
 const auto retained=shared_from_this();Operation operation(operation_active_,query_active_,thread_);
 const auto quote=borrowed;require_native_slot_current_without_callbacks(quote);
 need(!ring_.slots_[quote.slot_index].raster_uploaded,"Native pending physical slot already uploaded");
 need(geometry(cpu_->current())==ring_.geometry(),"Native pending slot source geometry changed");
 BnwSystemFontRasterStage result;
 {
  const auto raster=cpu_->raster_glyph_index(quote.slot.glyph_index);
  need(raster.glyph_index==quote.slot.glyph_index&&raster.advance==quote.slot.advance,"Native physical slot raster/width disagree");
  result.quote=quote;result.cpu=raster;const auto&g=ring_.geometry();
  result.left=g.cell_width*static_cast<int>(quote.slot_index%g.columns);result.top=g.cell_height*static_cast<int>(quote.slot_index/g.columns);
  result.right=result.left+raster.width;result.bottom=result.top+g.cell_height;
  result.required_producer="SAME counted renderer Context and actual green/shadow L8 uploads for physical slot";
 } // Raster temporary allocation/cleanup precedes final Source and slot POST.
 need(geometry(cpu_->current())==ring_.geometry(),"Native physical slot source changed after raster cleanup");
 require_native_slot_current_without_callbacks(quote);
 need(!ring_.slots_[quote.slot_index].raster_uploaded,"Native physical pending slot changed during raster");return result;
}

BnwSystemFontSlotQuote BnwSystemFontCacheOwner::native_slot_quote_without_callbacks(std::size_t index)const{
 need(std::this_thread::get_id()==thread_,"Native complete physical slot quote foreign creator");
 const auto cpu=cpu_;need(bool(cpu),"Native complete physical slot CPU source retired");
 const auto physical=cpu->impl_;
 BnwSystemFontCpuOwner::require_native_origin_current_without_callbacks_owned(physical,physical);
 need(index<ring_.slots_.size(),"Native complete physical slot index unavailable");
 const auto& actual=ring_.slots_[index];
 const BnwSystemFontSlotQuote result{actual.glyph_index,index,actual,false};
 BnwSystemFontCpuOwner::require_native_origin_current_without_callbacks_owned(physical,physical);
 return result;
}
BnwSystemFontRasterStage BnwSystemFontCacheOwner::stage_native_slot_raster_owned(
 const BnwSystemFontSlotQuote& borrowed,const graphics::detail::BnwOwnedFontPaintEffectFence& fence){
 fence.require_current_without_callbacks();
 const auto quote=borrowed;const auto retained=shared_from_this();fence.require_current_without_callbacks();
 const auto first=[&]{
  fence.require_current_without_callbacks();retained->require_native_slot_current_without_callbacks(quote);
  need(!retained->ring_.slots_[quote.slot_index].raster_uploaded&&
       quote.slot.raster_uploaded==retained->ring_.slots_[quote.slot_index].raster_uploaded,
   "Paint physical pending slot raster flag changed");fence.require_current_without_callbacks();
 };
 const auto full=[&]{
  first();const auto fields=retained->cpu_->native_raster_geometry_owned(fence);first();
  const BnwSystemFontCacheGeometry current{fields[0],fields[1],fields[2],fields[3],fields[4],fields[5]};
  need(current==retained->ring_.geometry(),"Paint physical slot source geometry changed");first();
 };
 BnwSystemFontRasterStage result;
 {
  first();Operation operation(retained->operation_active_,retained->query_active_,retained->thread_);first();
  full();first();
  {
   auto raster=retained->cpu_->raster_glyph_index_owned(quote.slot.glyph_index,fence);first();
   need(raster.glyph_index==quote.slot.glyph_index&&raster.advance==quote.slot.advance,
    "Paint native physical slot raster/width disagree");first();
   result.quote=quote;
   // Move each returned plane owner; no combined vector-copy allocation hides
   // an allocator boundary from the bottom fence.
   result.cpu=std::move(raster);first();
   const auto g=retained->ring_.geometry();first();
   result.left=g.cell_width*static_cast<int>(quote.slot_index%g.columns);
   result.top=g.cell_height*static_cast<int>(quote.slot_index/g.columns);
   result.right=result.left+result.cpu.width;result.bottom=result.top+g.cell_height;first();
   result.required_producer="SAME sealed Paint/Metadata and counted Context green/shadow L8 uploads";first();
  }
  first();full();first();
 } // CPU result and Cache operation cleanup precede final actual Source POST.
 first();full();first();return result;
}
static_assert(std::is_nothrow_move_constructible_v<BnwSystemFontRasterStage>,
 "Paint raster stage return must not allocate/copy planes");

BnwSystemFontSlotQuote BnwSystemFontCacheOwner::publish_uploaded(BnwSystemFontSlotQuote q)noexcept{ring_.slots_[q.slot_index].raster_uploaded=true;q.slot.raster_uploaded=true;return q;}

}
