#pragma once
#include <cstdint>
namespace civ5::ui {
// ICvDLLUtility::GameplayTurnChanged(int iNewTurn) payload. This standalone
// Civis publication/client listener does not attest OriginalDLL/render/Lua.
struct BnwGameTurnChangedEvent {
 std::int32_t turn=-1;
 std::uint64_t generation=0,world_revision=0,occurrence=0;
 bool operator==(const BnwGameTurnChangedEvent&)const=default;
};
}
