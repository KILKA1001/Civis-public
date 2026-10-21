#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace civ5::terrain {
// Single-threaded equivalent of the original scene command bank selected by
// 785420. Each stored packet begins with {byte size, command ID, two words},
// followed by the payload passed to the command's registered callback.
class SceneCommandQueue {
  public:
    using Handler=std::function<void(std::span<const std::uint8_t>)>;
    static constexpr std::size_t bank_capacity=0x400000;
    static constexpr std::size_t handler_count=0xc0;

    void register_handler(std::uint32_t command_id,Handler handler);
    void enqueue(std::span<const std::uint8_t> packet);
    // Swap banks before walking the old bank. A callback that enqueues a new
    // packet leaves it in the new bank until the following drain.
    std::size_t drain();
    unsigned active_bank() const {return active_;}
    std::size_t bank_bytes(unsigned bank) const {return banks_.at(bank).size();}

  private:
    std::array<std::vector<std::uint8_t>,2> banks_;
    std::array<Handler,handler_count> handlers_{};
    unsigned active_=0;
    bool draining_=false;
};
} // namespace civ5::terrain
