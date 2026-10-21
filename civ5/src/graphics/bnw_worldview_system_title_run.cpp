#include "civ5/graphics/bnw_worldview_system_title_run.hpp"
#include <atomic>
#include <limits>
#include <stdexcept>
#include <thread>
namespace civ5::graphics {
namespace {
void need(bool v,const char*why){if(!v)throw std::runtime_error(why);}
void own_profile(std::uint16_t cw){
 need((cw&0xffu)==0x7fu&&(cw&~0x0f7fu)==0&&((cw>>8)&3u)!=1,
  "System title requires an explicit own masked nonreserved cursor profile");
}
struct Candidate {
 std::shared_ptr<ui::BnwSystemFontCpuOwner>cpu;
 std::shared_ptr<ui::BnwSystemFontCacheOwner>cache;
 ui::BnwSystemFontRunStage run;
 std::vector<std::uint32_t>origins;
 std::int32_t width=0;
};
void measure_cursor(Candidate&c,std::uint16_t cw){
 const auto&g=c.run.gcp;
 need(g.pointers_preserved&&g.explicit_zero_class_input&&!g.original_class_inputs_verified&&
  !g.native_label_initialized&&g.utf16.size()==g.glyphs.size()&&
  g.glyphs.size()==c.run.final_lookup_slots.size()&&!g.glyphs.empty(),
  "System title requires the bounded SAME GCP run/lookup sequence");
 c.origins.clear();c.origins.reserve(g.glyphs.size());std::uint32_t cursor=0,maximum=0;
 for(std::size_t n=0;n<g.glyphs.size();++n){
  need(!(g.utf16[n]>=0xd800&&g.utf16[n]<=0xdfff),"System title surrogate shaping branch unavailable");
  const auto&q=c.run.final_lookup_slots[n];
  need(q.requested_glyph_index==g.glyphs[n]&&q.slot.glyph_index==g.glyphs[n],"System title glyph/slot identity differs");
  c.origins.push_back(cursor);
  const auto step=continue_bnw_extended_glyph_cursor(q,{cursor,maximum,cw,true});
  // Native low32 arithmetic is preserved by the reusable leaf, but a wrapped
  // or backwards title cannot become this bounded positive width issuer.
  need(step.truncated_advance>=0&&step.truncated_advance<=std::numeric_limits<std::int32_t>::max()&&
   std::uint64_t{cursor}+static_cast<std::uint64_t>(step.truncated_advance)<=
    static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())&&
   step.cursor_x>=cursor&&step.maximum_width>=maximum,
   "System title cursor exceeds nonwrapping own layout scope");
  cursor=step.cursor_x;maximum=step.maximum_width;
 }
 c.width=static_cast<std::int32_t>(maximum);
}
bool same_stamp(const ui::BnwClientFrameStamp&a,const ui::BnwClientFrameStamp&b){
 return a.identity==b.identity&&a.selection==b.selection&&a.viewport==b.viewport;
}
}
struct BnwWorldViewSystemTitleRunQuote::Data {
 BnwWorldViewSystemTitleRunSnapshot snapshot;
 std::shared_ptr<ui::BnwSystemFontCpuOwner>cpu;
 std::shared_ptr<ui::BnwSystemFontCacheOwner>cache;
};
struct BnwWorldViewSystemTitleRun::Impl {
 std::thread::id thread=std::this_thread::get_id();
 std::atomic_bool retired=false,poisoned=false;
 mutable bool busy=false;
 std::weak_ptr<BnwWorldViewSystemTitleRun>facade;
 std::shared_ptr<const ui::BnwWorldViewTitleFontIssuer>issuer;
 std::shared_ptr<ui::BnwClientTextPolicyOwner>policy;
 std::array<std::optional<Candidate>,3>candidates;
 std::shared_ptr<const BnwWorldViewSystemTitleRunQuote::Data>published;
 void live(bool require_facade)const{
  need(std::this_thread::get_id()==thread,"System title run used on foreign thread");
  need(!retired.load()&&!poisoned.load(),"System title run retired/quarantined");
  if(require_facade)need(!facade.expired(),"System title run facade retired");
 }
 struct Operation {
  std::shared_ptr<Impl>i;
  Operation(std::shared_ptr<Impl>v,bool require_facade):i(std::move(v)){
   i->live(require_facade);if(i->busy){i->poisoned.store(true);throw std::runtime_error("System title run recursive query");}i->busy=true;
  }
  ~Operation(){i->busy=false;}
 };
 void source(const ui::BnwWorldViewTitleFontFrame&bound,bool borrowed)const{
  live(borrowed);const auto now=issuer->current();live(borrowed);
  need(same_stamp(now.publication,bound.publication)&&now.client.surface_owner==bound.client.surface_owner&&
   now.client.revision==bound.client.revision&&now.client.width==bound.client.width&&now.client.height==bound.client.height&&
   now.locale==bound.locale&&now.language_row==bound.language_row,
   "System title publication/client/locale differs from its private issuer");
  const auto actual=policy->current();live(borrowed);
  need(actual.backend==ui::BnwClientTextBackend::native_system_font&&
   !actual.native_label_initialization_complete&&actual.locale==bound.locale&&actual.source_row==bound.language_row,
   "Current selected policy does not admit the bounded system title");
 }
 void validate(const std::shared_ptr<const BnwWorldViewSystemTitleRunQuote::Data>&data,bool borrowed)const{
  need(data&&data==published,"System title quote is absent/foreign/unpublished");
  source(data->snapshot.source,borrowed);
  for(std::size_t n=0;n<candidates.size();++n)if(candidates[n]){
   const auto&c=*candidates[n];const auto current=c.cpu->current();live(borrowed);
   const auto*d=&issuer->descriptor(n);live(borrowed);issuer->require_descriptor(*d);live(borrowed);
   need(current.measurement_available&&current.cpu_raster_available&&!current.gpu_pair_ready&&
    current.font_key==d->font_key()&&current.font_sha256==d->font_sha256()&&
    current.source_line_height==d->source_line_height()&&current.source_label_ascent==d->adjusted_ascent(),
    "System title SAME CPU descriptor binding differs");
   need(c.cache->source_cpu_owner().get()==c.cpu.get()&&c.cache->measurement_current(c.run),"System title cache/run metrics retired or replaced");live(borrowed);
  }
  source(data->snapshot.source,borrowed);live(borrowed);
 }
};
const BnwWorldViewSystemTitleRunSnapshot&BnwWorldViewSystemTitleRunQuote::snapshot_no_current()const{
 need(static_cast<bool>(data_),"Absent system title run quote");return data_->snapshot;
}
BnwWorldViewSystemTitleRun::BnwWorldViewSystemTitleRun(std::shared_ptr<Impl>i):impl_(std::move(i)){}
BnwWorldViewSystemTitleRun::~BnwWorldViewSystemTitleRun()=default;
void BnwWorldViewSystemTitleRun::retire()noexcept{const auto i=impl_;i->retired.store(true);}
std::shared_ptr<BnwWorldViewSystemTitleRun>BnwWorldViewSystemTitleRun::create(
 std::shared_ptr<const ui::BnwWorldViewTitleFontIssuer>issuer,std::uint16_t cw){
 need(static_cast<bool>(issuer),"Current private system-title issuer absent");own_profile(cw);
 auto i=std::make_shared<Impl>();i->issuer=std::move(issuer);Impl::Operation operation(i,false);
 // Pin the complete immutable title/client/request before policy/GDI callbacks.
 const auto bound=i->issuer->current();i->live(false);
 need(bound.locale.language=="ru_RU"&&bound.title.unit_name_font_selection&&bound.title.font_style&&
  bound.title.font_style->font=="TwCenMT24"&&bound.title.font_style->style=="SoftShadow"&&
  !bound.title.text.empty()&&bound.title.text.size()<=4096,
  "Current plain RU SoftShadow UnitName dependency unavailable");
 const auto request=*bound.title.unit_name_font_selection;
 need(request.text==bound.title.text&&request.context_id=="UnitPanel"&&request.control_id=="UnitName"&&
  request.button_width==bound.title.destination.width&&request.snapshot_frame==bound.publication.identity.frame&&
  request.player_id==bound.publication.identity.player&&bound.publication.selection.unit&&
  request.selected_unit_id==*bound.publication.selection.unit&&request.turn&&*request.turn==bound.publication.identity.turn,
  "System title request differs from the actual private publication");
 i->policy=ui::BnwClientTextPolicyOwner::create_from_worldview_title(i->issuer);i->live(false);i->source(bound,false);
 auto data=std::make_shared<BnwWorldViewSystemTitleRunQuote::Data>();data->snapshot.source=bound;data->snapshot.own_x87_control_word=cw;
 const auto measure=[&](std::size_t n)->Candidate&{
  i->source(bound,false);
  if(!i->candidates[n]){
   const auto*d=&i->policy->declared_worldview_descriptor(n);i->live(false);i->issuer->require_descriptor(*d);i->live(false);
   Candidate c;c.cpu=ui::BnwSystemFontCpuOwner::create_from_worldview_title_descriptor(i->policy,*d);i->live(false);
   c.cache=ui::BnwSystemFontCacheOwner::create(c.cpu);i->live(false);i->candidates[n]=std::move(c);
  }
  auto&c=*i->candidates[n];c.run=c.cache->measure_plain_run(bound.title.text);i->live(false);
  need(c.cache->current(c.run),"System title run changed during measurement");i->live(false);
  measure_cursor(c,cw);++data->snapshot.measure_calls[n];i->source(bound,false);return c;
 };
 std::size_t selected=0;auto&first=measure(0);
 data->snapshot.measured_indices[0]=0;data->snapshot.measured_widths[0]=first.width;
 if(ui::unit_name_own_width_exceeds_button(first.width,request.button_width))selected=1;
 auto&second=measure(selected);data->snapshot.measured_indices[1]=selected;data->snapshot.measured_widths[1]=second.width;
 if(ui::unit_name_own_width_exceeds_button(second.width,request.button_width))selected=2;
 auto&draw=selected==2?measure(2):*i->candidates[selected];
 data->snapshot.selected_candidate=selected;data->snapshot.draw_width=draw.width;
 data->snapshot.cursor_origins=draw.origins;data->snapshot.pending_run=draw.run;
 const auto cpu=draw.cpu->current();i->live(false);
 data->snapshot.source_line_height=cpu.source_line_height;data->snapshot.source_label_ascent=cpu.source_label_ascent;
 data->cpu=draw.cpu;data->cache=draw.cache;i->published=data;i->validate(data,false);
 auto result=std::shared_ptr<BnwWorldViewSystemTitleRun>(new BnwWorldViewSystemTitleRun(i));i->facade=result;i->live(true);return result;
}
BnwWorldViewSystemTitleRunQuote BnwWorldViewSystemTitleRun::current()const{
 const auto i=impl_;Impl::Operation operation(i,true);const auto data=i->published;i->validate(data,true);return BnwWorldViewSystemTitleRunQuote(data);
}
void BnwWorldViewSystemTitleRun::require_current(const BnwWorldViewSystemTitleRunQuote&borrowed)const{
 const auto i=impl_;Impl::Operation operation(i,true);const auto data=borrowed.data_;i->validate(data,true);
}
std::shared_ptr<ui::BnwSystemFontCacheOwner>BnwWorldViewSystemTitleRun::selected_cache(const BnwWorldViewSystemTitleRunQuote&borrowed)const{
 const auto i=impl_;Impl::Operation operation(i,true);const auto data=borrowed.data_;i->validate(data,true);return data->cache;
}
std::shared_ptr<ui::BnwSystemFontCpuOwner>BnwWorldViewSystemTitleRun::selected_cpu(const BnwWorldViewSystemTitleRunQuote&borrowed)const{
 const auto i=impl_;Impl::Operation operation(i,true);const auto data=borrowed.data_;i->validate(data,true);return data->cpu;
}
} // namespace civ5::graphics
