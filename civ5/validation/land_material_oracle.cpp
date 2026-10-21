// Text transport contains signed scalar inputs, never native pointers or ABI.
#include "civ5/materials/land_material.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    using namespace civ5::materials;
    try {
        int count = 0;
        if (!(std::cin >> count) || count < 1 || count > 100000)
            throw std::invalid_argument("Invalid LAND selection corpus");
        for (int i = 0; i < count; ++i) {
            int pass, mode, extra, state, spec;
            if (!(std::cin >> pass >> mode >> extra >> state >> spec) || (pass != 0 && pass != 1))
                throw std::invalid_argument("Invalid LAND selection record");
            const auto result = select_land_technique(pass == 0 ? LandPass::main : LandPass::overlay, mode,
                                                      extra, state, spec);
            if (!result)
                std::cout << "0 -1 -1\n";
            else
                std::cout << "1 " << result->technique.selector << ' ' << result->packet_offset << '\n';
        }
        std::cin >> std::ws;
        if (!std::cin.eof())
            throw std::invalid_argument("Trailing LAND selection data");
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
