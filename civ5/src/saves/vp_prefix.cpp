#include "civ5/saves/vp_prefix.hpp"
#include <stdexcept>

namespace civ5::saves {
VpPrefixObservation observe_vp_prefix(runtime::BinaryReader &input) {
    VpPrefixObservation result;
    result.save_version = input.u32();
    if (result.save_version != 0)
        throw std::runtime_error("Unverified VP save version");
    for (auto &value : result.game_data_hash)
        value = input.u32();
    result.gamecore_build = input.string32(4096);
    if (result.gamecore_build != "Release-5.4.2-0-g9f0904a01 Clean")
        throw std::runtime_error("Unverified VP gamecore build");
    for (auto &value : result.scalars)
        value = input.i32();
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
