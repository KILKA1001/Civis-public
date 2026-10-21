#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace civ5::ui {
// Pure input preparation only, not a Label/TextUpdate/font/cache/context owner.
struct BnwWholeRawLabelInput {
    std::string raw_utf8;
    std::vector<std::uint16_t> utf16;
    std::uint32_t source_bytes_consumed=0;
    bool whole_raw_preserved=false;
    bool original_class_inputs_verified=false;
    bool native_label_initialized=false;
    bool gcp_executed=false;
};
// Strict complete UTF8 subset of original83B780 through410900: <=4096bytes,
// no embedded NUL. Preserve all markup and controls; no substring filtering.
// Empty is the source no-GCP branch. No class bytes are fabricated here.
// Malformed/oversized input is refused instead of wider native replacement,
// unchecked obsolete sequences, prefix truncation or sourceExhausted semantics.
BnwWholeRawLabelInput prepare_bnw_whole_raw_label_input(std::string_view);
} // namespace civ5::ui
