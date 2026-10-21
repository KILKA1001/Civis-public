#include "civ5/materials/material_packet_pool.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5::materials;
void require(bool b, const char *s) {
  if (!b)
    throw std::runtime_error(s);
}
template <class F> void rejects(F f) {
  bool rejected = false;
  try {
    f();
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, "Invalid packet operation accepted");
}
int main() {
  try {
    MaterialPacketPool p;
    unsigned h = 0;
    auto &packet = p.allocate(0x49, h);
    packet.geometry[0] = 37;
    p.retain(h);
    p.release(h);
    require(p.references(h) == 1 && p.pending(0).empty(),
            "Still-referenced packet retired");
    p.release(h);
    rejects([&] { p.release(h); });
    rejects([&] { p.retain(h); });
    rejects([&] { p.packet(h); });
    rejects([&] { p.packet(0); });
    rejects([&] { p.references(1234); });
    rejects([&] { p.pending(4); });
    p.collect();
    require(p.available().empty(), "Early collection");
    for (unsigned i = 0; i < 3; ++i) {
      p.advance_frame();
      p.collect();
      require(p.available().empty(), "Early reuse");
    }
    p.advance_frame();
    require(p.available().empty() && p.retired().size() == 1,
            "Retirement phase");
    p.collect();
    unsigned recycled = 0;
    auto &fresh = p.allocate(7, recycled);
    require(recycled == h && fresh.key == 7 && fresh.geometry[0] == 0 &&
                p.references(h) == 1,
            "Packet reset/reuse");
    p.retain(0);
    p.release(0);
    require(p.references(0) == 0, "Null reference");
    MaterialPacketPool full;
    for (unsigned expected = 1;
         expected <= MaterialPacketPool::last_fresh_handle; ++expected) {
      unsigned id = 0;
      full.allocate(0x49, id);
      require(id == expected, "Fresh ID order");
    }
    bool overflow = false;
    unsigned unchanged = 123;
    try {
      full.allocate(0x49, unchanged);
    } catch (const std::overflow_error &) {
      overflow = true;
    }
    require(overflow && unchanged == 123 && full.next_handle() == 0x1ffff,
            "Exhaustion must preserve state/output");
    full.release(123);
    for (unsigned i = 0; i < 4; ++i)
      full.advance_frame();
    full.collect();
    full.allocate(0x49, unchanged);
    require(unchanged == 123, "Exhausted pool did not recycle");
    std::cout << "PASS packet pool lifecycle and diagnostic boundaries\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
