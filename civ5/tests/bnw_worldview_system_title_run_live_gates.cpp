#include "bnw_worldview_system_title_run_live_gates.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include <cmath>
#include <limits>
#include <thread>
#include <unordered_map>
namespace civ5::tests {
namespace {
void check(bool v,const char*s){if(!v)throw std::runtime_error(s);}
template<class F>void refused(F&&f){bool no=false;try{f();}catch(const std::exception&){no=true;}check(no,"System-title negative fixture unexpectedly accepted");}
}
graphics::BnwWorldViewSystemTitleRunSnapshot exercise_current_worldview_system_title_run(std::shared_ptr<const ui::BnwWorldViewTitleFontIssuer>issuer){
 check(static_cast<bool>(issuer),"Actual camera issuer absent; no synthetic positive");
 const auto source=issuer->current();check(source.locale.language=="ru_RU","Actual RU title gate required");
 auto owner=graphics::BnwWorldViewSystemTitleRun::create(issuer,0x037f);const auto quote=owner->current();
 const auto snapshot=quote.snapshot_no_current();owner->require_current(quote);
 check(snapshot.source.publication.identity==source.publication.identity&&snapshot.source.title.text==source.title.text&&
  snapshot.source.title.unit_name_font_selection&&source.title.unit_name_font_selection,
  "Actual selected title/publication was exchanged");
 // Independent fresh actual GDI oracle. Do not derive expected widths from
 // the producer-under-test's recorded width values or numeric cursor helper.
 const auto oracle_policy=ui::BnwClientTextPolicyOwner::create_from_worldview_title(issuer);
 struct Oracle {std::int32_t width=0;std::vector<std::uint16_t> glyphs;std::vector<float> advances;std::vector<std::uint32_t> origins;};
 const auto oracle_width=[&](std::size_t index){
  const auto&descriptor=issuer->descriptor(index);
  auto actual_cpu=ui::BnwSystemFontCpuOwner::create_from_worldview_title_descriptor(oracle_policy,descriptor);
  const auto cpu_frame=actual_cpu->current();
  check(cpu_frame.font_key==descriptor.font_key()&&cpu_frame.font_sha256==descriptor.font_sha256()&&
   cpu_frame.language_row==source.language_row&&cpu_frame.locale==source.locale&&cpu_frame.measurement_available,
   "Independent actual GDI oracle descriptor/language differs");
  const auto gcp=actual_cpu->measure_plain_run(source.title.text);
  check(gcp.utf8==source.title.text&&gcp.glyphs.size()==gcp.dx.size()&&gcp.pointers_preserved&&
   gcp.explicit_zero_class_input&&!gcp.original_class_inputs_verified&&!gcp.native_label_initialized,
   "Independent actual whole-run input is unavailable");
  std::unordered_map<std::uint16_t,float> last_advance;
  for(std::size_t n=0;n<gcp.glyphs.size();++n)last_advance[gcp.glyphs[n]]=static_cast<float>(gcp.dx[n]);
  Oracle result;result.glyphs=gcp.glyphs;std::int64_t total=0;
  for(const auto glyph:gcp.glyphs){
   result.origins.push_back(static_cast<std::uint32_t>(total));result.advances.push_back(last_advance.at(glyph));
   const double step=std::trunc(static_cast<double>(last_advance.at(glyph)));
   check(std::isfinite(step)&&step>=0&&step<=std::numeric_limits<std::int32_t>::max(),"Independent GDI cursor overflow");
   total+=static_cast<std::int64_t>(step);check(total<=std::numeric_limits<std::int32_t>::max(),"Independent GDI width overflow");
  }
  (void)actual_cpu->current();issuer->require_descriptor(descriptor);result.width=static_cast<std::int32_t>(total);return result;
 };
 const auto threshold=std::int64_t{source.title.unit_name_font_selection->button_width}-50;
 const auto first_width=oracle_width(0);const auto after_first=first_width.width>threshold?std::size_t{1}:std::size_t{0};
 const auto second_width=oracle_width(after_first);const auto expected=second_width.width>threshold?std::size_t{2}:after_first;
 const auto draw_width=expected==2?oracle_width(2):second_width;
 check(snapshot.measured_widths==std::array<std::int32_t,2>{first_width.width,second_width.width}&&snapshot.draw_width==draw_width.width&&
  snapshot.cursor_origins==draw_width.origins&&snapshot.pending_run.gcp.glyphs==draw_width.glyphs&&
  snapshot.measured_indices==std::array<std::size_t,2>{0,after_first}&&snapshot.selected_candidate==expected&&
  snapshot.measure_calls[0]==(after_first==0?2u:1u)&&snapshot.measure_calls[1]==(after_first==1?1u:0u)&&
  snapshot.measure_calls[2]==(expected==2?1u:0u),"Actual independent GDI widths/two read positions/selected draw differ");
 auto cache=owner->selected_cache(quote);auto cpu=owner->selected_cpu(quote);const auto actual=cpu->current();
 check(cache->source_cpu_owner().get()==cpu.get()&&actual.font_key==issuer->descriptor(expected).font_key()&&
  snapshot.source_line_height==actual.source_line_height&&snapshot.source_label_ascent==actual.source_label_ascent,
  "Selected private descriptor/CPU/cache mismatch");
 const auto&run=snapshot.pending_run;check(cache->current(run)&&cache->measurement_current(run),"Genuine pending run is not current");
 check(!snapshot.atlas_uploads_accepted&&!snapshot.shader_draw_accepted&&!snapshot.window_composition_accepted&&
  !actual.gpu_pair_ready,"CPU measurement promoted upload/render readiness");
 std::int64_t cursor=0;
 for(std::size_t n=0;n<run.final_lookup_slots.size();++n){
  const auto&q=run.final_lookup_slots[n];const auto advance=std::trunc(static_cast<double>(q.slot.advance_float));
  check(q.requested_glyph_index==draw_width.glyphs[n]&&q.slot.advance_float==draw_width.advances[n],
   "Selected glyph/advance differs from independent actual GDI sequence");
  check(!q.slot.raster_uploaded,"Fresh measurement minted an uploaded slot without actual Atlas.upload");
  check(std::isfinite(advance)&&advance>=0&&advance<=std::numeric_limits<std::int32_t>::max()&&
   snapshot.cursor_origins[n]==cursor,"Actual own cursor is inconsistent with final SAME GCP descriptor");
  cursor+=static_cast<std::int64_t>(advance);
  for(std::size_t k=0;k<n;++k)if(run.final_lookup_slots[k].requested_glyph_index==q.requested_glyph_index)
   check(run.final_lookup_slots[k].slot_index==q.slot_index&&run.final_lookup_slots[k].slot.generation==q.slot.generation,
    "Repeated actual glyph did not retain the same slot/generation");
 }
 check(cursor==snapshot.draw_width,"Actual nonwrapping own draw width differs from descriptor sum");
 auto changed=run;++changed.final_lookup_slots[0].slot.generation;
 check(!cache->measurement_current(changed)&&!cache->current(changed),"Stale generation accepted as measurement/upload current");
 changed=run;changed.final_lookup_slots[0].slot.x0+=.25f;
 check(!cache->measurement_current(changed),"Modified descriptor accepted as source measurement");
 changed=run;changed.final_lookup_slots[0].slot.u1=std::nextafter(changed.final_lookup_slots[0].slot.u1,
  std::numeric_limits<float>::infinity());check(!cache->measurement_current(changed),"Modified actual atlas coordinate accepted as measurement");
 changed=run;changed.final_lookup_slots[0].slot.advance_float=std::nextafter(changed.final_lookup_slots[0].slot.advance_float,
  std::numeric_limits<float>::infinity());check(!cache->measurement_current(changed),"Modified run advance accepted");
 changed=run;changed.final_lookup_slots[0].slot_index=cache->capacity();
 check(!cache->measurement_current(changed),"Out-of-range mapping/index accepted");
 changed=run;changed.final_lookup_slots[0].requested_glyph_index^=1;
 check(!cache->measurement_current(changed),"Wrong returned glyph identity accepted");
 refused([&]{owner->require_current({});});
 auto foreign=graphics::BnwWorldViewSystemTitleRun::create(issuer,0x037f);const auto foreign_quote=foreign->current();
 refused([&]{owner->require_current(foreign_quote);});owner->require_current(quote);
 bool wrong_thread=false;std::thread worker([&]{try{owner->require_current(quote);}catch(const std::exception&){wrong_thread=true;}});worker.join();
 check(wrong_thread,"Foreign-thread query accepted");owner->require_current(quote);
 refused([&]{graphics::BnwWorldViewSystemTitleRun::create(issuer,0x017f);});
 owner->retire();refused([&]{owner->current();});refused([&]{owner->selected_cache(quote);});
 check(quote.snapshot_no_current().source.title.text==source.title.text,"Retired immutable measurement payload lost safe lifetime");
 std::weak_ptr<graphics::BnwWorldViewSystemTitleRun>weak=owner;owner.reset();check(weak.expired(),"Measurement owner retained a facade cycle");
 refused([&]{foreign->require_current(quote);});foreign->require_current(foreign_quote);foreign->retire();
 // Real SAME issuer resource callbacks exercise producer lifetime/reentry.
 // Observer failures never authorize anything; the callback only invalidates
 // the new own measurement facade, leaving the genuine source issuer intact.
 const auto&resources=issuer->resources();
 auto nested=graphics::BnwWorldViewSystemTitleRun::create(issuer,0x037f);
 bool entered=false,nested_refused=false;
 {
  assets::ScopedResourceResolveObserver observer(resources,[&](const assets::AssetKey&,std::span<const assets::MountedResource>,bool){
   if(entered)return;entered=true;
   try{nested->current();}catch(const std::exception&){nested_refused=true;}
  });
  refused([&]{nested->current();});
  check(entered&&nested_refused&&!observer.failed(),"Actual resource callback did not quarantine recursive title consumption");
 }
 refused([&]{nested->current();});
 auto dying=graphics::BnwWorldViewSystemTitleRun::create(issuer,0x037f);
 const auto retained=dying->current();std::weak_ptr<graphics::BnwWorldViewSystemTitleRun>weak_dying=dying;entered=false;
 {
  assets::ScopedResourceResolveObserver observer(resources,[&](const assets::AssetKey&,std::span<const assets::MountedResource>,bool){
   if(entered)return;entered=true;dying.reset();
  });
  refused([&]{dying->current();});
  check(entered&&weak_dying.expired()&&!observer.failed(),"Dropped facade survived or actual lifetime callback was not exercised");
 }
 check(retained.snapshot_no_current().source.title.text==source.title.text,"Callback retirement destroyed retained immutable data");
 auto throwing=graphics::BnwWorldViewSystemTitleRun::create(issuer,0x037f);entered=false;
 {
  assets::ScopedResourceResolveObserver observer(resources,[&](const assets::AssetKey&,std::span<const assets::MountedResource>,bool){
   if(entered)return;entered=true;throwing->retire();throw std::runtime_error("Explicit negative fixture observer failure");
  });
  refused([&]{throwing->current();});
  check(entered&&observer.failed(),"Explicit callback retirement/throw fixture was not exercised");
 }
 // No generic observer-failed => private-font readiness rule is invented.
 // Refusal above is caused by actual retire/reentry/facade lifetime guards.
 (void)issuer->current();return snapshot;
}
void exercise_worldview_system_title_source_retirement(
 std::shared_ptr<const ui::BnwWorldViewTitleFontIssuer>issuer,const std::function<void()>&borrowed){
 check(static_cast<bool>(issuer),"Actual camera issuer absent");
 // Copy the invalidator before its invocation; caller keeps callable storage
 // alive during this copy. It is a negative fixture, never a source supplier.
 const auto invalidate=borrowed;check(static_cast<bool>(invalidate),"Real source/publication retirement required");
 auto owner=graphics::BnwWorldViewSystemTitleRun::create(issuer,0x037f);const auto quote=owner->current();
 auto cache=owner->selected_cache(quote);auto cpu=owner->selected_cpu(quote);
 owner->require_current(quote);invalidate();refused([&]{owner->require_current(quote);});
 refused([&]{owner->current();});
 refused([&]{cache->measurement_current(quote.snapshot_no_current().pending_run);});
 refused([&]{cpu->current();});
 check(!quote.snapshot_no_current().window_composition_accepted,"Retained negative quote issued render readiness");
}
}
