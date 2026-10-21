#pragma once
#include <cstdint>
namespace civ5::ui {
enum class BnwGoldenAgeGameplayKind { started, ended };
// Genuine standalone Civis gameplay facade payload. Publication/application
// does not attest an original DLL effect, physical renderer or audio delivery.
struct BnwEngineUiGoldenAgeGameplayEvent {
 BnwGoldenAgeGameplayKind kind=BnwGoldenAgeGameplayKind::started;
 std::int32_t actor=-1,turn=-1;
 std::uint64_t generation=0,world_revision=0,occurrence=0;
 bool operator==(const BnwEngineUiGoldenAgeGameplayEvent&)const=default;
};
} // namespace civ5::ui
