#pragma once
#include "civ5/ui/bnw_system_font_cache_owner.hpp"
namespace civ5::ui::detail {
// Values only; these records do not issue Label/run/source authority.
struct BnwSystemFontNativeGlyphData {
 BnwSystemFontSlotQuote slot;
 std::array<std::uint32_t,9> words{};
};
struct BnwSystemFontNativeRunStage {
 BnwSystemFontCpuOwner::NativeGcpResult gcp;
 std::size_t ensure_calls=0,advance_writes=0;
};
}
