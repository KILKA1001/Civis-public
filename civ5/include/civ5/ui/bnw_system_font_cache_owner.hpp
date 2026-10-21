#pragma once
#include "civ5/ui/bnw_system_font_cpu_owner.hpp"
#include <array>
#include <thread>
#include <optional>
namespace civ5::graphics {class GlBnwSystemFontAtlas;}
namespace civ5::ui {
class UiScreenGlRenderer;
namespace detail {
class BnwLabelSystemFontBorrower;class BnwLabelSystemFontPreparedRun;
struct BnwSystemFontNativeGlyphData;struct BnwSystemFontNativeRunStage;
}
struct BnwSystemFontCacheGeometry {
 std::int32_t cell_width=0,cell_height=0,columns=0,rows=0,atlas_width=0,atlas_height=0;
 bool operator==(const BnwSystemFontCacheGeometry&)const=default;
};
struct BnwSystemFontGlyphSlot {
 std::uint16_t glyph_index=0;
 std::int32_t advance=0;
 float u0=0,v0=0,u1=0,v1=0,x0=0,y0=0,x1=0,y1=0,advance_float=0;
 // Initial source cache bytes are 1, empty glyphs 0. New measured glyphs
 // require BOTH actual atlas uploads before this bit may become true.
 bool raster_uploaded=true;
 std::uint64_t generation=0;
 bool operator==(const BnwSystemFontGlyphSlot&)const=default;
};
struct BnwSystemFontSlotQuote {
 std::uint16_t requested_glyph_index=0;
 std::size_t slot_index=0;
 BnwSystemFontGlyphSlot slot;
 bool cache_hit=false;
};
// Source-only ring reducer. A standalone instance is algebra, not native font
// initialization. Production uses the retained CPU wrapper below.
class BnwSystemFontSlotRing {
 public:
 explicit BnwSystemFontSlotRing(BnwSystemFontCacheGeometry);
 BnwSystemFontSlotQuote measure(BnwSystemFontGlyphMeasurement);
 bool current(const BnwSystemFontSlotQuote&)const noexcept;
 std::size_t next_slot()const noexcept{return next_;}
 std::size_t capacity()const noexcept{return slots_.size();}
 const BnwSystemFontCacheGeometry&geometry()const noexcept{return geometry_;}
 private:
 friend class BnwSystemFontCacheOwner;
 struct Candidate{BnwSystemFontSlotQuote quote;std::uint16_t evicted=0;std::uint64_t revision=0;};
 Candidate prepare(BnwSystemFontGlyphMeasurement)const;
 void publish(const Candidate&)noexcept;
 BnwSystemFontCacheGeometry geometry_;
 std::array<std::int32_t,65536>mapping_{};
 std::vector<BnwSystemFontGlyphSlot>slots_;
 std::size_t next_=0;std::uint64_t revision_=0;
};
struct BnwSystemFontRasterStage {
 BnwSystemFontSlotQuote quote;
 BnwSystemFontGlyphCpu cpu;
 std::int32_t left=0,top=0,right=0,bottom=0;
 bool atlas_pair_uploaded=false;
 std::string required_producer;
};
struct BnwSystemFontRunStage {
 BnwSystemFontWholeRunGcp gcp;
 std::vector<BnwSystemFontSlotQuote> final_lookup_slots;
 std::size_t ensure_calls=0,advance_writes=0;
 // Only one explicit preparation sequence. Original cross-label global order,
 // batch raster flush, parser/wrap/parent layout and shader draw stay gated.
 bool native_label_initialized=false;
};
class BnwSystemFontCacheOwner : public std::enable_shared_from_this<BnwSystemFontCacheOwner> {
 public:
 static std::shared_ptr<BnwSystemFontCacheOwner>create(std::shared_ptr<BnwSystemFontCpuOwner>);
 BnwSystemFontSlotQuote measure(std::uint32_t bmp_codepoint);
 BnwSystemFontSlotQuote measure_glyph_index(std::uint16_t glyph_index);
 BnwSystemFontRasterStage stage_raster_glyph_index(const BnwSystemFontSlotQuote&);
 BnwSystemFontRunStage measure_plain_run(std::string_view utf8);
 bool current(const BnwSystemFontRunStage&)const;
 // Measurement-only identity: all actual mapping/index/generation/descriptor
 // fields must match. A genuine upload may change raster_uploaded alone.
 // This never proves an atlas upload; current(run/slot) remains strict.
 // Resident slot metrics + returned glyph sequence only. This does not
 // authenticate mutable GCP text/dx/order/classes or a whole-run producer.
 // Private title consumers retain their own immutable issued run data.
 bool measurement_current(const BnwSystemFontRunStage&)const;
 BnwSystemFontRasterStage stage_raster(std::uint32_t bmp_codepoint,const BnwSystemFontSlotQuote&);
 bool current(const BnwSystemFontSlotQuote&)const;
 std::size_t capacity()const;
 // Retained identity/lifetime only; no source validation or GDI/WGL call.
 std::shared_ptr<const BnwSystemFontCpuOwner>source_cpu_owner()const noexcept{return cpu_;}
 private:
 friend class graphics::GlBnwSystemFontAtlas;
 friend class detail::BnwLabelSystemFontBorrower;
 friend class detail::BnwLabelSystemFontPreparedRun;
 friend class UiScreenGlRenderer;
 BnwSystemFontSlotQuote ensure_glyph_index_owned(std::uint16_t);
 detail::BnwSystemFontNativeRunStage prepare_native_gcp_owned(std::u16string_view);
 detail::BnwSystemFontNativeRunStage prepare_native_gcp_owned(std::u16string_view,const graphics::detail::BnwOwnedFontPaintEffectFence&);
 detail::BnwSystemFontNativeGlyphData get_native_glyph_owned(std::uint32_t);
 detail::BnwSystemFontNativeGlyphData get_native_glyph_owned(std::uint32_t,const graphics::detail::BnwOwnedFontPaintEffectFence&);
 void require_native_glyph_current_without_callbacks(const detail::BnwSystemFontNativeGlyphData&)const;
 std::size_t native_slot_count_without_callbacks()const;
 std::optional<BnwSystemFontSlotQuote> pending_native_slot_quote_without_callbacks(std::size_t)const;
 BnwSystemFontRasterStage stage_native_slot_raster_owned(const BnwSystemFontSlotQuote&);
 BnwSystemFontRasterStage stage_native_slot_raster_owned(const BnwSystemFontSlotQuote&,const graphics::detail::BnwOwnedFontPaintEffectFence&);
 BnwSystemFontSlotQuote native_slot_quote_without_callbacks(std::size_t)const;
 BnwSystemFontSlotQuote ensure_native_ring_glyph_owned(BnwSystemFontSlotRing&,std::uint16_t);
 BnwSystemFontSlotQuote ensure_native_ring_glyph_owned(BnwSystemFontSlotRing&,std::uint16_t,const graphics::detail::BnwOwnedFontPaintEffectFence&);
 void require_native_slot_current_without_callbacks(const BnwSystemFontSlotQuote&)const;

 BnwSystemFontSlotQuote publish_uploaded(BnwSystemFontSlotQuote)noexcept;
 std::shared_ptr<BnwSystemFontCpuOwner>cpu_;
 BnwSystemFontSlotRing ring_;
 bool operation_active_=false;
 mutable bool query_active_=false;
 std::thread::id thread_=std::this_thread::get_id();
 explicit BnwSystemFontCacheOwner(std::shared_ptr<BnwSystemFontCpuOwner>,BnwSystemFontCacheGeometry);
};
}
