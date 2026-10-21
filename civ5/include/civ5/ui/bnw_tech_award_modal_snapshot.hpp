#pragma once
#include <cstdint>
#include <vector>
#include <utility>
namespace civ5::ui {
struct BnwTechAwardShownMessage {
 std::int32_t type=68;
 std::int64_t data1=0,data2=0,data3=0;
 std::int32_t flags=0;
 bool option1=false,option2=false;
 std::uint64_t occurrence=0,generation=0;
 std::int64_t team=-1,player=-1,turn=-1;
};
// Measured owned UI effect publication; data does not mint a native handler,
// original Layout, input loan or renderer/present acknowledgement.
struct BnwTechAwardModalSnapshot {
 std::uint64_t occurrence=0,revision=0;
 std::int32_t timer_semaphore=0;
 bool modal_queued=false,shown_received=false,source_showhide_pending=false,gpu_presented=false;
 std::vector<BnwTechAwardShownMessage> shown;
 std::vector<std::pair<std::int32_t,std::int32_t>> processed;
};
}
