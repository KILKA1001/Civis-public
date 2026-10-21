#pragma once
#include "civ5/graphics/bnw_extended_label_glyph_cursor.hpp"
#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/ui/unit_name_font_selection.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
namespace civ5::graphics {
// Immutable own-C++ measurement only. Cursor widths are measured from SAME
// current GDI run/cache descriptors under this explicit own profile, never
// native GetSizeVal, entering native CW, parent layout or applied GPU pixels.
struct BnwWorldViewSystemTitleRunSnapshot {
 ui::BnwWorldViewTitleFontFrame source;
 std::size_t selected_candidate=0;
 std::array<std::size_t,2> measured_indices{};
 std::array<std::int32_t,2> measured_widths{};
 std::array<std::size_t,3> measure_calls{};
 std::int32_t draw_width=0,source_line_height=0,source_label_ascent=0;
 std::uint16_t own_x87_control_word=0;
 std::vector<std::uint32_t> cursor_origins;
 ui::BnwSystemFontRunStage pending_run;
 static constexpr bool native_get_size_val=false,native_label_ready=false,
  entering_native_cw_observed=false,atlas_uploads_accepted=false,
  shader_draw_accepted=false,window_composition_accepted=false;
};
class BnwWorldViewSystemTitleRun;
class BnwWorldViewSystemTitleRunQuote {
 public:
 BnwWorldViewSystemTitleRunQuote()=default; // absent, never authorizing
 // Data only; the actual producer must authenticate before/after consumption.
 const BnwWorldViewSystemTitleRunSnapshot&snapshot_no_current()const;
 private:
 friend class BnwWorldViewSystemTitleRun;
 struct Data;std::shared_ptr<const Data>data_;
 explicit BnwWorldViewSystemTitleRunQuote(std::shared_ptr<const Data>d):data_(std::move(d)){}
};
class BnwWorldViewSystemTitleRun final {
 public:
 static std::shared_ptr<BnwWorldViewSystemTitleRun>create(
  std::shared_ptr<const ui::BnwWorldViewTitleFontIssuer>,std::uint16_t explicit_own_x87_control_word);
 ~BnwWorldViewSystemTitleRun();
 BnwWorldViewSystemTitleRun(const BnwWorldViewSystemTitleRun&)=delete;
 BnwWorldViewSystemTitleRun&operator=(const BnwWorldViewSystemTitleRun&)=delete;
 void retire()noexcept;
 BnwWorldViewSystemTitleRunQuote current()const;
 void require_current(const BnwWorldViewSystemTitleRunQuote&)const;
 // Actual SAME selected owners, not copied state or GL/upload permission.
 // A future atlas upload changes raster_uploaded in the quotes. That consumer
 // must retain this measurement and authenticate a separate uploaded run.
 // require_current authenticates measurement only across that one flag
 // transition; it never attests the original pending quote is uploaded.
 std::shared_ptr<ui::BnwSystemFontCacheOwner>selected_cache(const BnwWorldViewSystemTitleRunQuote&)const;
 std::shared_ptr<ui::BnwSystemFontCpuOwner>selected_cpu(const BnwWorldViewSystemTitleRunQuote&)const;
 private:
 struct Impl;std::shared_ptr<Impl>impl_;
 explicit BnwWorldViewSystemTitleRun(std::shared_ptr<Impl>);
};
} // namespace civ5::graphics
