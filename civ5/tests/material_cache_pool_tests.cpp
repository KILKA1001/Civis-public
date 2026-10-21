#include "civ5/materials/material_cache_free_lists.hpp"
#include <iostream>
#include <stdexcept>

using namespace civ5::materials;
namespace {
void check(bool ok) {
    if (!ok)
        throw std::runtime_error("Material pool ownership check failed");
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try {
        f();
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected);
}
} // namespace
int main() {
    try {
        MaterialCacheFreeLists pool;
        pool.add_texture(81, 64, false);
        pool.add_texture(17, 64, true);
        pool.add_texture(29, 256, true);
        for (auto request : {0u, 1u, 32u, 63u, 65u, 127u, 257u, 1024u, 0xffffffffu}) {
            unsigned width = 391;
            check(pool.take(request, width) == 0 && width == 391);
        }
        rejects([&] { pool.add_texture(0, 64, true); });
        rejects([&] { pool.add_texture(17, 128, false); });
        rejects([&] { pool.add_texture(13, 65, true); });
        rejects([&] { pool.give(777); });
        rejects([&] { pool.give(17); });
        check(pool.texture_count() == 3 && pool.texture_width(17) == 64 && pool.available(17));
        unsigned width = 7;
        check(pool.take(512, width) == 29 && width == 256 && !pool.available(29));
        check(pool.take(512, width) == 17 && width == 64 && !pool.available(17));
        width = 83;
        check(pool.take(512, width) == 0 && width == 83);
        pool.give(0);
        pool.give(81);
        pool.give(17);
        check(pool.take(64, width) == 17 && pool.take(64, width) == 81);
        pool.give(29);
        check(pool.take(512, width) == 29 && width == 256);
        check(pool.free_handles(64).empty() && pool.free_handles(256).empty());
        rejects([&] { pool.free_handles(17); });
        std::cout << "PASS material pool ownership guards, exhaustion and retained "
                     "leases\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
