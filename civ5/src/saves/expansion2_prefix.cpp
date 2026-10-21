#include "civ5/saves/expansion2_prefix.hpp"
#include <stdexcept>

namespace civ5::saves {
Expansion2PrefixObservation observe_expansion2_prefix(runtime::BinaryReader& input) {
    Expansion2PrefixObservation result;
    result.version = input.u32();
    if (result.version != 1) throw std::runtime_error("Unverified Expansion2 CvGame version");
    for (auto& value : result.scalars) value = input.i32();
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
