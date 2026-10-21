#include "civ5/materials/material_packet_pool.hpp"
#include "material_cache_rebuild_fixture.hpp"
#include "material_generation_fixture.hpp"
namespace io = generation_validation;
using namespace civ5::materials;
int main(int argc, char **argv) {
  try {
    io::require(argc == 3,
                "Packet pool primitive input and fresh output required");
    const auto data = io::read(argv[1]);
    io::Reader in{data};
    const auto magic = in.take(8);
    io::require(std::string_view(reinterpret_cast<const char *>(magic.data()),
                                 8) == "C5PKPLI1",
                "Packet pool schema");
    const auto count = in.word();
    io::require(count > 0 && count <= 100, "Packet pool group count");
    io::Bytes out{'C', '5', 'P', 'K', 'P', 'L', 'O', '1'};
    io::word(out, count);
    for (unsigned group = 0; group < count; ++group) {
      MaterialPacketPool pool;
      std::map<unsigned, unsigned> labels;
      std::map<unsigned, MaterialCachePacket *> storage;
      const auto operations = in.word();
      io::require(operations <= 100000, "Packet operation count");
      io::word(out, operations);
      for (unsigned n = 0; n < operations; ++n) {
        const auto op = in.word(), label = in.word(), value = in.word();
        unsigned result = 0;
        switch (op) {
        case 0: {
          auto &p = pool.allocate(value, result);
          storage[result] = &p;
          labels[label] = result;
          break;
        }
        case 1:
          pool.retain(label ? labels.at(label) : 0);
          break;
        case 2:
          pool.release(label ? labels.at(label) : 0);
          break;
        case 3:
          pool.advance_frame();
          break;
        case 4:
          pool.collect();
          break;
        case 5:
          storage.at(labels.at(label))->geometry[0] = value;
          break;
        case 6:
          break;
        default:
          throw std::invalid_argument("Packet operation");
        }
        io::word(out, result);
        if (op != 6)
          continue;
        io::Bytes state;
        io::word(state, pool.next_handle());
        io::word(state, pool.frame_slot());
        io::word(state, static_cast<unsigned>(storage.size()));
        for (const auto &[h, p] : storage) {
          io::word(state, h);
          io::word(state, pool.references(h));
          const auto bytes = cache_rebuild_validation::packet_bytes(*p);
          state.insert(state.end(), bytes.begin(), bytes.end());
        }
        const auto append = [&](auto values) {
          io::word(state, static_cast<unsigned>(values.size()));
          for (auto v : values)
            io::word(state, v);
        };
        for (unsigned slot = 0; slot < 4; ++slot)
          append(pool.pending(slot));
        append(pool.retired());
        append(pool.available());
        io::blob(out, state);
      }
    }
    in.end();
    io::write(argv[2], out);
    std::cout << "PASS native packet pool " << count << " groups\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
