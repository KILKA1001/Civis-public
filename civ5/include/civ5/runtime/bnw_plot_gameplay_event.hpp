#pragma once
#include <array>
#include <cstdint>
namespace civ5::runtime {
enum class BnwPlotGameplayEventKind { yield_might_have_changed };
// Retained factual notification data. Construction of this value grants no
// write, continuation, renderer-completed or refresh capability.
struct BnwPlotGameplayEvent {
 std::uint64_t occurrence=0,generation=0,operation=0,target_revision=0;
 std::array<std::uint8_t,16> map_guid{};
 std::int64_t plot=-1;
 std::uint64_t x=0,y=0;
 std::array<std::int16_t,6> yields{};
 BnwPlotGameplayEventKind kind=BnwPlotGameplayEventKind::yield_might_have_changed;
};
}
